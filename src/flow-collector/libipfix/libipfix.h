#ifndef _libipfix_h_
#define _libipfix_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcrypto.h>
#include <nxlog.h>
#include <nxsocket.h>

#define LIBIPFIX_DEBUG_TAG _T("ipfix")

#ifdef _WIN32
#define hstrerror strerror
#endif

#ifdef _WITH_ENCRYPTION
#define SSLSUPPORT   1
#endif

#include "ipfix.h"
#include "mpoll.h"

//
// gettimeofday() for Windows
//

#ifdef _WIN32

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

#endif	/* _WIN32 */


#endif
