/* musl_shims.c — thin wrappers for glibc _FORTIFY_SOURCE symbols
 * and LFS64 aliases, so pre-built .a libs link against musl.  */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

int __sprintf_chk(char *s, int flag, size_t slen, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsprintf(s, fmt, ap);
    va_end(ap); return r;
}
int __snprintf_chk(char *s, size_t n, int flag, size_t slen, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(s, n, fmt, ap);
    va_end(ap); return r;
}
int __printf_chk(int flag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vprintf(fmt, ap);
    va_end(ap); return r;
}
int __fprintf_chk(FILE *fp, int flag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vfprintf(fp, fmt, ap);
    va_end(ap); return r;
}
char *__strcpy_chk(char *d, const char *s, size_t dlen) { return strcpy(d, s); }
void *__memcpy_chk(void *d, const void *s, size_t n, size_t dlen) { return memcpy(d, s, n); }
void *__memmove_chk(void *d, const void *s, size_t n, size_t dlen) { return memmove(d, s, n); }

/* LFS64 aliases — musl's fopen is already 64-bit */
FILE *fopen64(const char *p, const char *m) { return fopen(p, m); }
FILE *freopen64(const char *p, const char *m, FILE *f) { return freopen(p, m, f); }
FILE *tmpfile64(void) { return tmpfile(); }
