/* $Id: main.h,v 1.2 2005-06-16 13:34:21 alk Exp $ */

#ifndef __MAIN_H__
#define __MAIN_H__

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

#include <nms_common.h>
//#include <nms_threads.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <string>

#include "serial.h"

#endif // __MAIN_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.1  2005/06/16 13:19:38  alk
added sms-driver for generic gsm modem


*/
