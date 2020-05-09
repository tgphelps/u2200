#include <stdio.h>
#include <ertran.h>

int main(void)
{
    print_sector("$MFDB$", 0);
    return 0;
}

void print_sector(char *c, int sector_number) {
    _io_pkt_type pkt;

    int num_buffers;
    int directions[1];
    int word_counts[1];
    int function;
    int buffer[28];
    int buffers[1];
    int i,j,num_words_used;
    buffers[0] = (int) buffer;
    num_buffers = 1;
    directions[0] = 0;
    word_counts[0] = 28;
    function = _FR;

    ucsmakeiopk(&pkt,c,&function,&sector_number,&num_buffers,
                directions,word_counts,buffers,&num_words_used);
    if (num_words_used < 1) {
        printf("ucsmakeiopk failed\n");
        feabt();
    }
    fiow(&pkt);
    printf("i/o status %o\n", pkt.istat);
    if (pkt.istat != 0)
        feabt();
    for (i = 0; i < 28; i += 6) {
        int z = (i + 6 < 28 ? 6 : 28 - i);
            for (j = 0; j < z; ++j)
        printf("%.12o ",buffer[i + j]);
        printf("\n");
    }
}
