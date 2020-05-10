
#include <stdio.h>
#include <ertran.h>

#include <cfrags.h>

/*
 * sector-io.c -- functions to read sectors from a disk file
 *
 * For now, this supports only ONE file at a time, and that file
 * must be assigned to the run.
 *
 * Functions:
 * int sio_open(char *filename)
 *     If the file is not assigned, print a message and return 0.
 *     If it is assigned, try to read the first track. We don't
 *     catch a fatal I/O error. If we read data, return 1, else 0.
 * int *sio_read(int sector, int count)
 *     For now, count must be ONE. Return a point to a 28-word
 *     array of the data. If the data isn't available, return NULL.
 */


static _io_pkt_type io_pkt;

static char the_file[12 + 1];
static int num_buffers = 1;
static int directions[1] = {0};
static int word_counts[1] = {64 * 28};
static int function = _FR;
static int buffer[64 * 28];
static int buffers[1] =  {(int) buffer};
static int num_words_used;
static int sector_addr = 0;

int
sio_open(char *filename)
{
    if (!file_assigned(filename)) {
        printf("File %s is not assigned.\n", filename);
        return 0;
    }
    printf("File is assigned\n");
    /* build I/O packet and do first I/O */
    ucsmakeiopk(&io_pkt, filename, &function, &sector_addr, &num_buffers,
                directions, word_counts, buffers, &num_words_used);
    if (num_words_used < 1) {
        printf("ERROR: ucsmakeiopk failed\n");
        feabt();
    }
    fiow(&io_pkt);
    printf("I/O status: %02o\n", io_pkt.istat);
    octal_fdata_dump(buffer, 28);
    octal_ascii_dump(buffer, 28);
    return 0;
}


int *
sio_read(int sector, int count)
{
    return NULL;
}


/* Function to check whether a file is assigned.
    Return 1 if it is,
   else 0.
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
