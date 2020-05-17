
#include <assert.h>
#include <ertran.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sysutil.h>

#include "cfrags.h"


/* words in MFD extract header */

#define MFLABL 0  /* offset of label ('*MFDB*') */
#define MFFLCT 1  /* offset of file count */
#define MFFLAD 4  /* offset of first MFD sector addr */

typedef enum {
    opt_list,
    opt_min,
    opt_dist
} option_type;

typedef struct {
    option_type option;
    int min_frags;
} parser_type;

typedef struct {
    char qual[12 + 1];
    char file[12 + 1];
    short fcycle;
    short is_tape;
    short num_frags;
} file_pkt_type;

typedef struct {
    int mfd_file_count;
    int mfd_first_record;
    int cur_sector;
    int *cur_buff;
} mfd_info_type;

#define MAX_COUNT 100
typedef struct {
    int frag_counts[MAX_COUNT];
    int really_big_count;
    int max_count_found;
} counts_type;


/************************************/

int
main(int argc, char *argv[])
{
    int i;
    int stat;
    file_pkt_type fpkt;
    parser_type parse_info;
    mfd_info_type mfd_info;
    counts_type counts;

    printf("FRAGS 1R1\n");

    /* clear all the counts */
    counts.really_big_count = 0;
    counts.max_count_found = 0;
    for (i = 0; i < MAX_COUNT; ++i)
        counts.frag_counts[i] = 0;

    parse_arguments(argc, argv, &parse_info);
#if LOG
    log_open("LOG");
    log("frags start");
#endif
    stat = sio_open("$MFDB$");
    if (stat == 0) {
        printf("ERROR: file $MFDB$ is not assigned\n");
        fexit();
    }
    open_mfd_extract(&mfd_info);

    mfd_info.cur_buff = sio_read(mfd_info.mfd_first_record, 1);
    mfd_info.cur_sector = mfd_info.mfd_first_record;

    for (i = 0; i < mfd_info.mfd_file_count; ++i) {
        get_next_file_info(&fpkt, &mfd_info);
        /***
        printf("file: %d, qual: %s, name:%s\n", i, fpkt.qual,fpkt.file);
        printf("fcyc = %d, is_tape = %d\n", fpkt.fcycle, fpkt.is_tape);
        printf("dads = %d\n", fpkt.num_frags);
        ***/
        if (parse_info.option == opt_list
        || (parse_info.option == opt_min
            && fpkt.num_frags >= parse_info.min_frags))
            print_file_info(fpkt);
        else {
            save_dist_info(fpkt, &counts);
        }
    }
    if (parse_info.option == opt_dist)
        print_dist_info(&counts);

    sio_close();
#if LOG
    log_close();
#endif
    return 0;
}

/************************************/


/* process_next_file: process the next file records and all following
 * DAD tables. On entry, the current sector (the one we have a pointer to)
 * must be a file record, and not a DAD table.
 */

void
get_next_file_info(file_pkt_type *p, mfd_info_type *m) {
    assert(H1(m->cur_buff[0]) != 0);  /* must be a file record */

    fdasc(p->qual, m->cur_buff + 0, 12);
    p->qual[12] = '\0';
    rtrim(p->qual);

    fdasc(p->file, m->cur_buff + 2, 12);
    p->file[12] = '\0';
    rtrim(p->file);

    p->fcycle = H2(m->cur_buff[19]);
    p->is_tape = S6(m->cur_buff[12]) & 01;
    p->num_frags = 0;

    while (1) {
        fetch_next_record(m);
        if (H1(m->cur_buff[0]) != 0) {  /* if it's not a DAD table */
            /* cur_buff now contains the next file record */
            break;
        } else {
            p->num_frags += count_dads(m->cur_buff);
        }
    }
}


/* count_dads: Return count of number of fragsments (not holes) in
 * the DAD table passed in.
 */

#define MAX_DADS 8
#define OFFSET_TO_DAD1 4
#define DAD_SIZE 3
#define IS_HOLE 0400000
#define IS_LAST_DAD 04

int
count_dads(int *dadt)
{
    int i;
    int count = 0;
    int *dad = dadt + OFFSET_TO_DAD1;

    for (i = 0; i < MAX_DADS; ++i) {
        /* octal_dump(dad, 3); */
        if ((H2(dad[2]) & IS_HOLE) == 0)
            ++count;
        if (H1(dad[2]) & IS_LAST_DAD)
            break;
        dad += DAD_SIZE;
    }
    return count;
}


/* open_mfd_extract: Open the MFD file and verify the header record.
 * Get the file count and address of first file record.
 */

void
open_mfd_extract(mfd_info_type *m)
{
    int *sec;

    /* sio_open("$MFDB$"); */
    sec = sio_read(0, 1);
    /* Word 1 should be '*MFDB*', and file count non-zero */
    if ((sec[MFLABL] != 0502213110750) || (sec[MFFLCT] == 0)) {
        printf("ERROR: Invalid MFD file header\n");
        feabt();
    }
    m->mfd_file_count = sec[MFFLCT];
    m->mfd_first_record = sec[MFFLAD];
#if LOG
    log("MFD file header okay");
    log1d("file count = %d", m->mfd_file_count);
    log1d("first record at sector %d", m->mfd_first_record);
#endif
}


/* fetch_next_record: Read the next sector from the MFD file, and
 * update the 'mfd' struct.
 */

void
fetch_next_record(mfd_info_type *m)
{
    m->cur_buff = sio_read(m->cur_sector + 1, 1);
    ++(m->cur_sector);;
}


/* parse_arguments: parse program arguments. Return only if they're okay. */

void
parse_arguments(int argc, char *argv[], parser_type *parse_info)
{
    char *spec;
    char *token;
    int n;

    if (argc != 2)
        usage();

    spec = argv[1];
    str_tolower(spec);
    if (startswith(spec, "list")) {
        parse_info->option = opt_list;
    } else if (startswith(spec, "dist")) {
        parse_info->option = opt_dist;
    } else if (startswith(spec, "min")) {
        parse_info->option = opt_min;
        token = strtok(spec, "/");
        token = strtok(NULL, "/");  /* token is stuff after the slash */
        if (token == NULL)
            usage();
        n = atoi(token);
        if (n > 0)
            parse_info->min_frags = n;
        else
            usage();
    } else
        usage();
}


/* print_file_info: print filename and DAD count */

void
print_file_info(file_pkt_type p)
{
    if (!p.is_tape) {
        printf("%3d  %s*%s(%d)\n", p.num_frags, p.qual, p.file, p.fcycle);
    }
}


/* usage: print help message, and terminate. */


char msg_usage[] =
"usage:\n\
  @FRAGS list\n\
    show all cataloged files and DAD counts\n\
  @FRAGS min/<n>\n\
    show only files with at least <n> DADs\n\
  #FRAGS dist\n\
    show distribution DAD counts\n";

void
usage(void)
{
    printf(msg_usage);
    fexit();
}


/* save_dist_info: Update frag counts */

void
save_dist_info(file_pkt_type p, counts_type *c)
{
    int num;
    num = p.num_frags;
    if (num < MAX_COUNT) {
        ++(c->frag_counts[num]);
        if (num > c->max_count_found)
            c->max_count_found = num;
    } else
        ++(c->really_big_count);
}


/* print_dist_info: Print file counts by number of fragments */

void
print_dist_info(counts_type *c)
{
    int i;
    printf("Frags Count\n");
    printf("----- -----\n");
    for (i = 0; i <= c->max_count_found; ++i)
        printf("%5d %5d\n", i, c->frag_counts[i]);
    printf("%MORE  %5d\n", c->really_big_count);
}
