#ifndef _libipfix_h_
#define _libipfix_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcrypto.h>
#include <nxlog.h>

#define LIBIPFIX_DEBUG_TAG _T("ipfix")

#ifdef _WIN32
#define hstrerror strerror
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

#ifdef __cplusplus
extern "C" {
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz);

#ifdef __cplusplus
}
#endif

#endif	/* _WIN32 */


#endif
