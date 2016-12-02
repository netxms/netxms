#ifndef _libipfix_h_
#define _libipfix_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxcrypto.h>

#ifdef _WIN32
#define hstrerror strerror
#endif

#include "ipfix.h"
#include "mlog.h"
#include "mpoll.h"


//
// Mutex wrappers
//

typedef void *LIBIPFIX_MUTEX;

#ifdef __cplusplus
extern "C" {
#endif

LIBIPFIX_MUTEX mutex_create();
void mutex_destroy(LIBIPFIX_MUTEX mutex);
BOOL mutex_lock(LIBIPFIX_MUTEX mutex);
void mutex_unlock(LIBIPFIX_MUTEX mutex);

#ifdef __cplusplus
}
#endif


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
