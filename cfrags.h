
/* Definitions that all C source files might need. */

extern int sio_open(char *use_name);
extern int *sio_read(int sector, int count);

extern void octal_dump(int *words, int count);
