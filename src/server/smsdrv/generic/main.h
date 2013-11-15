/* $Id$ */

#ifndef __MAIN_H__
#define __MAIN_H__

#define INCLUDE_LIBNXSRV_MESSAGES

#include <nxsrvapi.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else

# define EXPORT

# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <termios.h>
# include <errno.h>

#endif // _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#endif // __MAIN_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.3  2005/06/16 20:54:26  victor
Modem hardware ID written to server log

Revision 1.2  2005/06/16 13:34:21  alk
project files addded

Revision 1.1  2005/06/16 13:19:38  alk
added sms-driver for generic gsm modem


*/
