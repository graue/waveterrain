/*
 * Timer routines for busy-wait loops.
 *
 * TODO: Where possible, delay a little bit (yielding the
 * CPU) and still keep time. On some operating systems this
 * can't be done, but on others, it can and should.
 */

#include <stddef.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/time.h>
#endif
#include "ctimer.h"

#if defined(_WIN32) || defined(__BEOS__)
typedef unsigned long long u_int64_t;
#endif

#ifdef _WIN32

/*
 * Stuff from winbase.h, but to avoid including tons of unneeded
 * Windows garbage too, it is included here.
 */
typedef struct _FILETIME
{
	unsigned long dwLowDateTime;
	unsigned long dwHighDateTime;
} FILETIME;
void __stdcall GetSystemTimeAsFileTime(FILETIME *);

/* Our own struct timeval, hooray. */
struct timeval
{
	long tv_sec;
	long tv_usec;
};

typedef union
{
	long long ns100; /* time since Jan. 1, 1601 in 100ns units */
	FILETIME ft;
} w32_ftv;

#define _W32_FT_OFFSET 116444736000000000LL

/* Fake gettimeofday for Windows. */
static void gettimeofday(struct timeval *tv, void *ignored)
{
	w32_ftv _now;
	GetSystemTimeAsFileTime(&(_now.ft));
	tv->tv_usec = (long)((_now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec  = (long)((_now.ns100 - _W32_FT_OFFSET)/10000000LL);
	(void)ignored;
}
#endif

u_int64_t get_usecs(void)
{
	struct timeval tv;
	(void)gettimeofday(&tv, NULL);
	return ((u_int64_t)tv.tv_sec * 1000000UL) + tv.tv_usec;
}
