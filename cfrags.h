
/* Definitions that all C source files might need. */

#pragma function_prototypes_required

/* sector-io.c */
extern int sio_open(char *use_name);
extern int *sio_read(int sector, int count);

/* util.c */
extern void octal_dump(int *words, int count);
extern void octal_fdata_dump(int *words, int count);
extern void octal_ascii_dump(int *words, int count);

/* log.c */
void log_open(char *fname, int level);
void log_close(void);
void log(int level, char *s);
void log1d(int level, char *s, int n);
void log_set_level(int level);


#define H1(w) (w >> 18)
#define H2(w) (w & 0777777)

#define S6(w) (w & 077)
