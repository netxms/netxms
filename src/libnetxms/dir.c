/*
 * dir.c
 */
/***********************************************************
        Copyright 1992 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define WIN32IO_IS_STDIO
#define PATHLEN	1024

#include "libnetxms.h"
#include <stdlib.h>

#ifndef UNDER_CE
#include <sys/stat.h>
#endif


/*
 * The idea here is to read all the directory names into a string table
 * * (separated by nulls) and when one of the other dir functions is called
 * * return the pointer to the current file name.
 */
DIR *opendir(const char *filename)
{
    DIR            *p;
    long            len;
    long            idx;
	 char            tail;
    char            scannamespc[PATHLEN];
    char           *scanname = scannamespc;
    struct stat     sbuf;
    WIN32_FIND_DATAA FindData;
    HANDLE          fh;

    /*
     * check to see if filename is a directory 
     */
    strcpy(scanname, filename);
	 tail = scanname[strlen(scanname) - 1];
	 if ((tail == '/') || (tail == '\\'))
		scanname[strlen(scanname) - 1] = 0;
    if ((stat(filename, &sbuf) < 0) || ((sbuf.st_mode & S_IFDIR) == 0))
    {
        return NULL;
    }

    /*
     * Create the search pattern 
     */
	strcat(scanname, "\\*");

    /*
     * do the FindFirstFile call 
     */
    fh = FindFirstFileA(scanname, &FindData);
    if (fh == INVALID_HANDLE_VALUE)
	 {
        return NULL;
    }

    /*
     * Get us a DIR structure 
     */
    p = (DIR *) malloc(sizeof(DIR));
    if (p == NULL)
        return NULL;

    /*
     * now allocate the first part of the string table for
     * * the filenames that we find.
     */
    idx = (int)strlen(FindData.cFileName) + 1;
    p->start = (char *) malloc(idx);
    /*
     * New(1304, p->start, idx, char);
     */
    if (p->start == NULL) {
        free(p);
        return NULL;
    }
    strcpy(p->start, FindData.cFileName);
    /*
     * if(downcase)
     * *    strlwr(p->start);
     */
    p->nfiles = 0;

    /*
     * loop finding all the files that match the wildcard
     * * (which should be all of them in this directory!).
     * * the variable idx should point one past the null terminator
     * * of the previous string found.
     */
    while (FindNextFileA(fh, &FindData))
    {
        len = (int)strlen(FindData.cFileName);
        /*
         * bump the string table size by enough for the
         * * new name and it's null terminator
         */
        p->start = (char *) realloc((void *) p->start, idx + len + 1);
        /*
         * Renew(p->start, idx+len+1, char);
         */
        if (p->start == NULL) {
            free(p);
            return NULL;
        }
        strcpy(&p->start[idx], FindData.cFileName);
        p->nfiles++;
        idx += len + 1;
    }
    FindClose(fh);
    p->size = idx;
    p->curr = p->start;
    return p;
}


/*
 * Readdir just returns the current string pointer and bumps the
 * * string pointer to the nDllExport entry.
 */
struct dirent *readdir(DIR * dirp)
{
    int             len;
    static int      dummy = 0;

    if (dirp->curr)
    {
        /*
         * first set up the structure to return 
         */
        len = (int)strlen(dirp->curr);
        strcpy(dirp->dirstr.d_name, dirp->curr);
        dirp->dirstr.d_namlen = len;

        /*
         * Fake an inode 
         */
        dirp->dirstr.d_ino = dummy++;

        /*
         * Now set up for the nDllExport call to readdir 
         */
        dirp->curr += len + 1;
        if (dirp->curr >= (dirp->start + dirp->size)) {
            dirp->curr = NULL;
        }

        return &(dirp->dirstr);
    } else
        return NULL;
}

/*
 * free the memory allocated by opendir 
 */
int closedir(DIR * dirp)
{
    free(dirp->start);
    free(dirp);
    return 1;
}

#endif   /* _WIN32 */
