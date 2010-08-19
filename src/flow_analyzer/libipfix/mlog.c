/*
$$LIC$$
 */
/*
** mlog.c - loggin funcs
**
** Copyright Fraunhofer FOKUS
**
** $Date: 2008-10-14 12:40:12 +0200 (Di, 14. Okt 2008) $
**
** $Revision: 1.3 $
**
** Remarks: 
*/
#include "libipfix.h"

/*------ defines ---------------------------------------------------------*/

/*------ stuctures -------------------------------------------------------*/

/*------ globals ---------------------------------------------------------*/

int         mlog_vlevel = 9;
FILE        *mlog_fp    = NULL;

static char tmpbuf[4100];      /* !! */

/*------ revision id -----------------------------------------------------*/

static char const cvsid[]="$Id: mlog.c 956 2008-10-14 10:40:12Z hir $";

/*------ export funcs ----------------------------------------------------*/

/*
 * Name         : debugf
 * Parameter    : > fmt, ...   varargs
 * Purpose      : print debug message
 * Return values: none
 */
void debugf ( char fmt[], ... )
{
#ifdef NODEBUG
    return;
#else
    va_list args;

    va_start(args, fmt);
    (void) vsnprintf( tmpbuf, sizeof(tmpbuf), fmt, args );
    va_end(args);

    fprintf ( stderr, "DEBUG <" );
    fprintf ( stderr, "%s", tmpbuf );
    fprintf ( stderr, ">\n" );
    fflush  ( stderr );
#endif
}

/* Name         : errorf
 * Parameter    : > fmt, ...   varargs
 * Purpose      : print error message
 * Return values: none
 */
void errorf ( char fmt[], ... )
{
    va_list args;

    va_start(args, fmt);
    (void) vsnprintf( tmpbuf, sizeof(tmpbuf), fmt, args );
    va_end(args);

    fprintf ( stderr, "%s", tmpbuf );
}

/* Name         : mlogf
 * Parameter    : > int        verbosity
 *                > fmt, ...   varargs
 * Purpose      : write log message
 * Return values: none
 */
void mlogf ( int vlevel, char fmt[], ... )
{
    va_list args;

    if ( ! mlog_fp )
        mlog_fp = stderr;
    if ( vlevel > mlog_vlevel )
        return;

    va_start(args, fmt);
    (void) vsnprintf( tmpbuf, sizeof(tmpbuf), fmt, args );
    va_end(args);

    fprintf( mlog_fp, "%s", tmpbuf );
    if ( mlog_vlevel > 1 )
        fflush( mlog_fp );
}

int mlog_open( char *logfile, char *prefix )
{
    if ( logfile && ((mlog_fp=fopen( logfile, "w+" )) ==NULL))
    {
        errorf( "[mlog_open] cannot open log file <%s>, <%s>!\n",
                logfile, strerror(errno) );
        mlog_fp = stderr;
        return -1;
    }

    return 0;
}

void mlog_close( void )
{
    if ( mlog_fp && (mlog_fp != stderr) )
        fclose( mlog_fp );
}

void mlog_set_vlevel( int vlevel )
{
    mlog_vlevel = vlevel;
}
