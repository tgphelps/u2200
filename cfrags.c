
#include <assert.h>
#include <stdio.h>
#include <ertran.h>

#include "cfrags.h"

/* words in MFD extract header */

#define MFLABL 0  /* offset of label ('*MFDB*') */
#define MFFLCT 1  /* offset of file count */
#define MFFLAD 4  /* offset of first MFD sector addr */

static int mfd_file_count;
static int mfd_first_record;

static struct mfd_status {
    int cur_sector;
    int *cur_buff;
} mfd;


int main(void)
{
    int i;
    log_open("LOG", 3);
    log(3, "cfrags start");
    sio_open("$MFDB$");
    open_mfd_extract();

    mfd.cur_buff = sio_read(mfd_first_record, 1);
    mfd.cur_sector = mfd_first_record;

    for (i = 0; i <1; ++i) {
        process_next_file();
    }
    log_close();
    return 0;
}


/* process_next_file: process the next file records and all following
 * DAD tables. On entry, the current sector (the one we have a pointer to)
 * must be a file record, and not a DAD table.
 */

void
process_next_file(void) {
    assert(H1(mfd.cur_buff[0]) != 0);
    printf("doing file\n");
}


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
