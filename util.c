
#include <stdio.h>


void
octal_dump(int *words, int count)
{
#define WPL 4    /* words per line */
    int i, j;
    for (i = 0; i < count; i += WPL) {
        int z = (i + WPL < count ? WPL : count - i);
            for (j = 0; j < z; ++j)
        printf("%.12o ",words[i + j]);
        printf("\n");
    }
}
