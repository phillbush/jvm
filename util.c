#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *progname;

int optind = 1;
int optopt;

/* set program name */
void
setprogname(char *s)
{
	progname = s;
}

/* get int32_t from uint32_t */
int32_t
getint(uint32_t bytes)
{
	int32_t i;

	memcpy(&i, &bytes, sizeof i);
	return i;
}

/* get float from uint32_t */
float
getfloat(uint32_t bytes)
{
	float f;

	memcpy(&f, &bytes, sizeof f);
	return f;
}

/* get int64_t from uint32_t */
int64_t
getlong(uint32_t high_bytes, uint32_t low_bytes)
{
	uint64_t u;
	int64_t l;

	u = ((uint64_t)high_bytes << 32) | (uint64_t)low_bytes;
	memcpy(&l, &u, sizeof l);
	return l;
}

/* get double from uint32_t */
double
getdouble(uint32_t high_bytes, uint32_t low_bytes)
{
	uint64_t u;
	double d;

	u = ((uint64_t)high_bytes << 32) | (uint64_t)low_bytes;
	memcpy(&d, &u, sizeof d);
	return d;
}

/* print format error message with error string and exit */
void
err(int eval, const char *fmt, ...)
{
	va_list ap;
	int saverrno;

	saverrno = errno;
	if (progname)
		(void)fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	if (fmt) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	va_end(ap);
	(void)fprintf(stderr, "%s\n", strerror(saverrno));
	exit(eval);
}

/* print format error message and exit */
void
errx(int eval, const char *fmt, ...)
{
	va_list ap;

	if (progname)
		(void)fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	if (fmt)
		(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
	exit(eval);
}

/* print format error message with error string */
void
warn(const char *fmt, ...)
{
	va_list ap;
	int saverrno;

	saverrno = errno;
	if (progname)
		(void)fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	if (fmt) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	va_end(ap);
	(void)fprintf(stderr, "%s\n", strerror(saverrno));
}

/* print format error message */
void
warnx(const char *fmt, ...)
{
	va_list ap;

	if (progname)
		(void)fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	if (fmt)
		(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, "\n");
}

/* call calloc checking for errors */
void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if ((p = calloc(nmemb, size)) == NULL)
		err(EXIT_FAILURE, "calloc");
	return p;
}

/* call malloc checking for errors */
void *
emalloc(size_t size)
{
	void *p;

	if ((p = malloc(size)) == NULL)
		err(EXIT_FAILURE, "calloc");
	return p;
}

/* get options, we do not support ':' on options */
int
getopt(int argc, char * const *argv, const char *options)
{
	static int optpos = 1;

	if (!optind) {                  /* reset */
		optind = 1;
		optpos = 1;
	}
	if (optind >= argc || !argv[optind] || argv[optind][0] != '-') {
		return -1;
	}
	if (strcmp(argv[optind], "--") == 0) {
		optind++;
		return -1;
	}
	optopt = argv[optind][optpos];
	if (strchr(options, argv[optind][optpos]) == NULL) {
		warnx("unknown option -- %c", argv[optind][optpos]);
		return '?';
	}
	if (!argv[optind][++optpos]) {
		optind++;
		optpos = 1;
	}
	return optopt;
}
