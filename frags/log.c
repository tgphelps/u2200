
#include <stdio.h>
#include <ertran.h>
#include "cfrags.h"

#if LOG

static FILE *log_file;


/* log_open: open log file */

void
log_open(char *fname)
{
    log_file = fopen(fname, "wt");
    if (!log_file) {
        printf("failed to open log file\n");
        feabt();
    }
    log("log opened");
}


/* log_close: close the log file, if it's open */
void

log_close(void)
{
    if (log_file)
        log("log closed");
        fclose(log_file);
        log_file = NULL;
}


/* log: write msg to file, then '\n' */

void
log(char *s)
{
    if (log_file) {
        fputs(s, log_file);
        fputc('\n', log_file);
    }
}


/* log1d: call log with one decimal int parameter */

void
log1d(char *s, int n)
{
    char lmsg[100];

    sprintf(lmsg, s, n);
    log(lmsg);
}



/*****
int
main(void)
{
    log_open("LOG");
    log("test message 2");
    log_close();
}
*****/
#else
char ident[] = "No logging code here.";
#endif
