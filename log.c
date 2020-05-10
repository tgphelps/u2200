
#include <stdio.h>


static FILE *log_file;
static int log_level = 3;  /* 0,1,2,3 => off,low,medium,high */


/* log_open: open log file, set initial level */

void
log_open(char *fname, int level)
{
    log_file = fopen(fname, "wt");
    if (!log_file) {
        printf("failed to open log file\n");
        feabt();
    }
    log(1, "log opened");
    log_level = level;
}


/* log_close: close the log file, if it's open */
void

log_close(void)
{
    if (log_file)
        log(1, "log closed");
        fclose(log_file);
        log_file = NULL;
}


/* log: write msg to file, then '\n', if level is suitable */

void
log(int level, char *s)
{
    if (log_file)
        if (level <= log_level) {
            fputs(s, log_file);
            fputc('\n', log_file);
        }
}


/* log1d: call log with one decimal int parameter */

void
log1d(int level, char *s, int n)
{
    char lmsg[100];

    sprintf(lmsg, s, n);
    log(level, lmsg);
}


/* log_set_level: adjust the log level */

void
log_set_level(int level)
{
    log_level = level;
}


/*****
int
main(void)
{
    log_open("LOG", 3);
    log(1, "test message 2");
    log_close();
}
*****/
