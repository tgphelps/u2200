#include <stdio.h>
#include <ertran.h>


int main()
{
    int i;
    char file[13], qual[13];
    int ar[7];  /* words 6 thru 12 of fitem$ packet */
                /* 6,,s1 == 0 if not assigned */


    ucsfitem("$MFDB$", file, qual, ar);
    /* returns file and qual set to all '@' chars if not assigned */
    printf("file = %s, qual = %s\n", file, qual);
    for (i = 0; i < 7; ++i)
        printf("%d  %012o\n", i, ar[i]);
    printf("equip code = %02o\n", ar[0] >> 30);

    ucsfitem("XXXXXX", file, qual, ar);
    /* returns file and qual set to all '@' chars if not assigned */
    printf("file = %s, qual = %s\n", file, qual);
    for (i = 0; i < 7; ++i)
        printf("%d  %012o\n", i, ar[i]);
    printf("equip code = %02o\n", ar[0] >> 30);
    return 0;
}
