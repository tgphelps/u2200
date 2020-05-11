
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

enum {
    opt_list,
    opt_min,
    opt_dist
} option;

int min_dads;

typedef struct {
    char qual[12 + 1];
    char file[12 + 1];
    short fcycle;
    short is_tape;
    short num_frags;
} file_pkt_type;

static int mfd_file_count;
static int mfd_first_record;

static struct mfd_status {
    int cur_sector;
    int *cur_buff;
} mfd;

#define MAX_COUNT 100

int frag_counts[MAX_COUNT];  /* initially zero */
int really_big_count = 0;
int max_count_found = 0;

/************************************/

int
main(int argc, char *argv[])
{
    int i;
    file_pkt_type fpkt;

    parse_arguments(argc, argv);
    log_open("LOG", 3);
    log(3, "cfrags start");
    sio_open("$MFDB$");
    open_mfd_extract();

    mfd.cur_buff = sio_read(mfd_first_record, 1);
    mfd.cur_sector = mfd_first_record;

    for (i = 0; i < mfd_file_count; ++i) {
        get_next_file_info(&fpkt);
        /***
        printf("file: %d, qual: %s, name:%s\n", i, fpkt.qual,fpkt.file);
        printf("fcyc = %d, is_tape = %d\n", fpkt.fcycle, fpkt.is_tape);
        printf("dads = %d\n", fpkt.num_frags);
        ***/
        if (option == opt_list
        || (option == opt_min && fpkt.num_frags >= min_dads))
            print_file_info(fpkt);
        else {
            save_dist_info(fpkt);
        }
    }
    if (option == opt_dist)
        print_dist_info();

    log_close();
    return 0;
}

/************************************/


/* process_next_file: process the next file records and all following
 * DAD tables. On entry, the current sector (the one we have a pointer to)
 * must be a file record, and not a DAD table.
 */

void
get_next_file_info(file_pkt_type *p) {
    assert(H1(mfd.cur_buff[0]) != 0);
    fdasc(p->qual, mfd.cur_buff + 0, 12);
    p->qual[12] = '\0';
    rtrim(p->qual);
    fdasc(p->file, mfd.cur_buff + 2, 12);
    p->file[12] = '\0';
    rtrim(p->file);
    p->fcycle = H2(mfd.cur_buff[19]);
    p->is_tape = S6(mfd.cur_buff[12]) & 01;
    p->num_frags = 0;
    while (1) {
        fetch_next_record();
        if (H1(mfd.cur_buff[0]) != 0) {
            /* mfd.cur_buff now contains the next file record */
            break;
        } else {
            p->num_frags = count_dads(mfd.cur_buff);
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
        if ((H2(dad[0]) & IS_HOLE) == 0)
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
open_mfd_extract(void)
{
    int *sec;

    sio_open("$MFDB$");
    sec = sio_read(0, 1);
    if ((sec[MFLABL] != 0502213110750) || (sec[MFFLCT] == 0)) {
        printf("ERROR: Invalid MFD file header\n");
        feabt();
    }
    log(3, "MFD file header okay");
    mfd_file_count = sec[MFFLCT];
    mfd_first_record = sec[MFFLAD];
    log1d(3, "file count = %d", mfd_file_count);
    log1d(3, "first record at sector %d", mfd_first_record);
}


/* fetch_next_record: Read the next sector from the MFD file, and
 * update the 'mfd' struct.
 */

void
fetch_next_record()
{
    mfd.cur_buff = sio_read(mfd.cur_sector + 1, 1);
    ++mfd.cur_sector;;
}


/* parse_arguments: parse program arguments. Return only if they're okay. */

void
parse_arguments(int argc, char *argv[])
{
    char *spec;
    char *token;
    int n;

    if (argc != 2)
        usage();

    spec = argv[1];
    if (startswith(spec, "list")) {
        option = opt_list;
    } else if (startswith(spec, "dist")) {
        option = opt_dist;
    } else if (startswith(spec, "min")) {
        option = opt_min;
        token = strtok(spec, "/");
        token = strtok(NULL, "/");
        if (token == NULL)
            usage();
        n = atoi(token);
        if (n > 0)
            min_dads = n;
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
save_dist_info(file_pkt_type p)
{
    int num;
    num = p.num_frags;
    if (num < MAX_COUNT) {
        ++frag_counts[num];
        if (num > max_count_found)
            max_count_found = num;
    } else
        ++really_big_count;
}


/* print_dist_info: Print file counts by number of fragments */

void
print_dist_info(void)
{
    int i;
    printf("Frags Count\n");
    printf("----- -----\n");
    for (i = 0; i <= max_count_found; ++i)
        printf("%5d %5d\n", i, frag_counts[i]);
    printf("%MORE  %5d\n", really_big_count);
}
