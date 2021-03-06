
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sysutil.h>

#include "cfrags.h"

#define WPL 4    /* words per line */


/* octal_dump: print a buffer in octal, 'WPL' words per line*/

void
octal_dump(int *words, int count)
{
    int i, j;

    for (i = 0; i < count; i += WPL) {
        int z = (i + WPL < count ? WPL : count - i);
        for (j = 0; j < z; ++j)
             printf("%012o ", words[i + j]);
        printf("\n");
    }
}


/* octal_fdata_dump: octal_dump + Fieldata */

void
octal_fdata_dump(int *words, int count)
{
    int i, j;
    char buff1[8 + 1];
    char buff2[8 + 1];

    for (i = 0; i < count; i += WPL) {
        int z = (i + WPL < count ? WPL : count - i);
        for (j = 0; j < z; ++j)
             printf("%012o ", words[i + j]);
        for (j = 0; j < z; ++j) {
            fdasc(buff1, &words[i + j], 6);
            buff1[6] = '\0';
            make_printable(buff1, buff2, 6);
            buff2[6] = '\0';
            printf("%s ", buff2);
        }
        printf("\n");
    }
}


/* octal_fdata_dump: octal_dump + ASCII */

void
octal_ascii_dump(int *words, int count)
{
    int i, j;
    char buff[4 + 1];

    for (i = 0; i < count; i += WPL) {
        int z = (i + WPL < count ? WPL : count - i);
        for (j = 0; j < z; ++j)
             printf("%012o ", words[i + j]);
        for (j = 0; j < z; ++j) {
            make_printable((char *)(words + i + j), buff, 4);
            buff[4] = '\0';
            printf("%s ", buff);
        }
        printf("\n");
    }
}


/* convert a string to all printable characters */

static void
make_printable(char *inbuf, char *outbuf, int len)
{
    int i;
    char c;
    for (i = 0; i < len; ++i) {
        c = inbuf[i];
        if ((c < 0x20) || (c >= 0x7f))
            c = '.';
        outbuf[i] = c;
    }
}


/* startswith: return 1 if 'src' is a leading substring of 'dest', else 0. */

int startswith(char *dest, char *src)
{
    int len = strlen(src);
    if (strncmp(dest, src, len) == 0)
        return 1;
    else
        return 0;
}


/* rtrim: remove trailing blanks from an LJSF string.
 * WARNING: This mutates the string! */

void
rtrim(char *s)
{
    int i;
    int len = strlen(s);
    for (i = 0; i < len; ++i)
        if (s[i] == ' '){
            s[i] = '\0';
            break;
        }
}


/* str_tolower: Destructively change all upper case characters in
 * a string to lower case.
 */

void str_tolower(char *s)
{
    int i;
    int len = strlen(s);
    for (i = 0; i < len; ++i)
        s[i] = tolower(s[i]);
}
