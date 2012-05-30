/*
 * dirw_unix.c
 */

#include "libnetxms.h"
#include <sys/stat.h>

#ifdef UNICODE

/**
 * opendir() wrapper
 */
DIRW *wopendir(const WCHAR *name)
{
	char *utf8name = UTF8StringFromWideString(name);
	DIR *dir = opendir(utf8name);
	free(utf8name);
	if (dir == NULL)
		return NULL;
	DIRW *d = (DIRW *)malloc(sizeof(DIRW));
	d->dir = dir;
	return d;
}

/**
 * readdir() wrapper
 */
struct dirent_w *wreaddir(DIRW *dirp)
{
	struct dirent *d = readdir(dirp->dir);
	if (d == NULL)
		return NULL;
	MultiByteToWideChar(CP_UTF8, 0, d->d_name, -1, dirp->dirstr.d_name, 257);
	dirp->dirstr.d_name[256] = 0;
	dirp->dirstr.d_ino = d->d_ino;
	return &dirp->dirstr;
}

/*
 * closedir() wrapper
 */
int wclosedir(DIRW *dirp)
{
	closedir(dirp->dir);
	free(dirp);
	return 0;
}

#endif
