
#include <assert.h>
#include <ertran.h>
#include <stdio.h>
#include <string.h>
#include <sysutil.h>

#include "cfrags.h"

/* words in MFD extract header */

#define MFLABL 0  /* offset of label ('*MFDB*') */
#define MFFLCT 1  /* offset of file count */
#define MFFLAD 4  /* offset of first MFD sector addr */

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


int main(void)
{
    int i;
    file_pkt_type fpkt;

    log_open("LOG", 3);
    log(3, "cfrags start");
    sio_open("$MFDB$");
    open_mfd_extract();

    mfd.cur_buff = sio_read(mfd_first_record, 1);
    mfd.cur_sector = mfd_first_record;

    for (i = 0; i < mfd_file_count; ++i) {
        get_next_file_info(&fpkt);
        printf("file: %d, qual: %s, name:%s\n", i, fpkt.qual,fpkt.file);
        printf("fcyc = %d, is_tape = %d\n", fpkt.fcycle, fpkt.is_tape);
        printf("dads = %d\n", fpkt.num_frags);
    }
    log_close();
    return 0;
}


/* process_next_file: process the next file records and all following
 * DAD tables. On entry, the current sector (the one we have a pointer to)
 * must be a file record, and not a DAD table.
 */

void
get_next_file_info(file_pkt_type *p) {
    assert(H1(mfd.cur_buff[0]) != 0);
    fdasc(p->qual, mfd.cur_buff + 0, 12);
    p->qual[12] = '\0';
    fdasc(p->file, mfd.cur_buff + 2, 12);
    p->file[12] = '\0';
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
