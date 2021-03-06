
#include <assert.h>
#include <stdio.h>
#include <ertran.h>

#include "cfrags.h"

/*
 * sector-io.c -- functions to read sectors from a disk file
 *
 * For now, this supports only ONE file at a time, and that file
 * must be assigned to the run.
 *
 * Exported functions:
 * int sio_open(char *filename)
 *     If the file is not assigned, return 0.
 *     If it is assigned, try to read the first track. We don't
 *     catch a fatal I/O error, so a crash may result.
 *     If we read data, return 1, else 0.
 *
 * int *sio_read(int sector, int num_sectors)
 *     The sectors requested must not span a track boundary (for now).
 *     Return pointer to first_sector, or NULL if we can't do it.
 *     XXX: num_sectors must be ONE for now.
 */

#define BUFSIZE 1792            /* word size of I/O buffer */
#define SPB     (BUFSIZE / 28)  /* sectors per buffer */


static int sio_opened = 0;

static _io_pkt_type io_pkt;

static int buffer[BUFSIZE];

static int first_sector = -2;  /* force first sio_read to do physical read */
static int last_sector  = -1;

#if LOG
static char lmsg[100];
#endif


int
sio_open(char *filename)
{
    int num_buffers = 1;
    int directions[1] = {0};
    int word_counts[1] = {BUFSIZE};
    int function = _FR;
    int buffers[1] =  {(int) buffer};
    int num_words_used;
    int sector_addr = 0;

    assert(!sio_opened);  /* don't open me twice */
    sio_opened = 1;

    if (!file_assigned(filename)) {
#if LOG
        log("File is NOT assigned");
#endif
        return 0;
    }
#if LOG
    log("sio: file is assigned");
#endif

    /* build I/O packet and do first I/O */
    ucsmakeiopk(&io_pkt, filename, &function, &sector_addr, &num_buffers,
                directions, word_counts, buffers, &num_words_used);
    assert(num_words_used > 0);  /* should never happen */
    sio_read(0, 1);  /* force a physical read */
    /* octal_fdata_dump(buffer, 28); */
    return 1;  /* return 'ok' */
}


void
sio_close(void)
{
    assert(sio_opened);
    sio_opened = 0;
}


int *
sio_read(int sector, int num_sectors)
{
    int sector_to_read;
    int ptr;

    assert(sio_opened);
#if LOG
    log1d("sio: read(%d)", sector);
#endif
    assert(num_sectors == 1);
    if (sector < first_sector || sector > last_sector) {
#if LOG
        log("sio: cache miss");
#endif
        sector_to_read = (sector / SPB) * SPB;
        physical_read(sector_to_read);
    } else {
#if LOG
        log("sio: cache hit");
#endif
    }
    ptr = 28 * (sector - first_sector);  /* offset of requested sector */
    /* printf("offset = %d\n", ptr); */
    /* octal_fdata_dump(buffer + ptr, 28); */
    return buffer + ptr;
}


/* Function to read a buffer of data from disk.
   Updates first_sector and last_sector to show what's in the buffer.
*/

void
physical_read(int sector_wanted)
{
    int n;

    io_pkt.trkad = sector_wanted;
#if LOG
    log1d("sio: phys read sector %d", sector_wanted);
#endif
    fiow(&io_pkt);

    first_sector = sector_wanted;
    n = io_pkt.subst / 28;  /* sectors actually read */
    if (n == 0) {
#if LOG
        log("sio: ERROR: I/O read 0 sectors");
#endif
        printf("sio: ERROR: I/O read 0 sectors\n");
        feabt();
    }
    last_sector = first_sector + n - 1;

#if LOG
    sprintf(lmsg, "sio: iostat: %02o read: %d",
                  io_pkt.istat, io_pkt.subst);
    log(lmsg);
#endif
    /* printf("read sectors %d to %d\n", first_sector, last_sector); */
    if (io_pkt.istat > 0)
        printf("WARNING: I/O status: %02o\n", io_pkt.istat);
}


/* Function to check whether a file is assigned.
    Return 1 if it is, else 0.
*/

static int
file_assigned(char *use_name)
{
    char qual[12 + 1], file[12 + 1];
    int  fitem[7];

    ucsfitem(use_name, file, qual, fitem);
    if (fitem[0] >> 30 == 0)
        return 0;
    return 1;
}
