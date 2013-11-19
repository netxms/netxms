#ifndef _nxstat_h_
#define _nxstat_h_

#ifndef S_ISDIR
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif

#if defined(_WIN32)
#define NX_STAT _tstati64
#define NX_STAT_STRUCT struct _stati64
#elif HAVE_LSTAT64 && HAVE_STRUCT_STAT64
#define NX_STAT lstat64
#define NX_STAT_STRUCT struct stat64
#else
#define NX_STAT lstat
#define NX_STAT_STRUCT struct stat
#endif

#if defined(UNICODE) && !defined(_WIN32)
inline int __call_stat(const WCHAR *f, NX_STAT_STRUCT *s)
{
	char *mbf = MBStringFromWideString(f);
	int rc = NX_STAT(mbf, s);
	free(mbf);
	return rc;
}
#define CALL_STAT(f, s) __call_stat(f, s)
#else
#define CALL_STAT(f, s) NX_STAT(f, s)
#endif

#endif
