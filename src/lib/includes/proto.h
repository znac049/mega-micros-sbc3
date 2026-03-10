/* atol.c */
long int atol(const char *str);
/* fclose.c */
int fclose(void *stream);
/* fflush.c */
int fflush(void *stream);
/* fgets.c */
char *fgets(char *str, int n, void *stream);
/* fputc.c */
int fputc(int c, void *stream);
/* fputs.c */
int fputs(const char *str, void *stream);
/* fread.c */
int fread(void *ptr, int size, int nobj, void *stream);
/* fwrite.c */
int fwrite(const void *ptr, int size, int nobj, void *stream);
/* getenv.c */
char *getenv(const char *name);
/* memset.c */
void *memset(void *mem, int fill, unsigned int count);
/* printf.c */
int printf(const char *format, ...);
/* putchar.c */
int putchar(int c);
/* puts.c */
int puts(const char *str);
/* rand.c */
int rand(void);
/* snprintf.c */
int snprintf(char *dst, int n, const char *format, ...);
/* srand.c */
void srand(unsigned int seed);
/* strcmp.c */
int strcmp(const char *str1, const char *str2);
/* strcpy.c */
char *strcpy(char *dst, const char *src);
/* strlen.c */
int strlen(const char *str);
/* strrchr.c */
char *strrchr(const char *str, int c);
/* tolower.c */
int tolower(int c);
