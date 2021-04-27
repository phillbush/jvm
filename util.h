#define LEN(x) (sizeof (x) / sizeof *(x))

void setprogname(char *s);
void *ecalloc(size_t nmemb, size_t size);
void *emalloc(size_t size);
int getopt(int argc, char * const *argv, const char *options);
int32_t getint(uint32_t bytes);
float getfloat(uint32_t bytes);
int64_t getlong(uint32_t high_bytes, uint32_t low_bytes);
double getdouble(uint32_t high_bytes, uint32_t low_bytes);
void err(int eval, const char *fmt, ...);
void errx(int eval, const char *fmt, ...);
void warn(const char *fmt, ...);
void warnx(const char *fmt, ...);

extern int optind, optopt;
