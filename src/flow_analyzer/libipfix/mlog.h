/*
** mpoll.h - export declarations for poll funcs
**
** Copyright Fraunhofer FOKUS
**
** $Id: mlog.h 22 2008-08-12 08:34:40Z tor $
**
** Remarks: This is experimental code!
**
*/
#ifndef _MLOG_H
#define _MLOG_H

#ifndef __GNUC__
#define __attribute__(x)
#endif

#ifdef   __cplusplus
extern "C" {
#endif

extern int mlog_vlevel;

void errorf ( char fmt[], ... ) __attribute__ ((format (printf, 1, 2)));
void debugf ( char fmt[], ... ) __attribute__ ((format (printf, 1, 2)));
void mlogf  ( int verbosity,
              char fmt[], ... ) __attribute__ ((format (printf, 2, 3)));
int  mlog_open  ( char *logfile, char *prefix );
void mlog_close ( void );
void mlog_set_vlevel( int vlevel );

#ifdef   __cplusplus
}
#endif
#endif 
