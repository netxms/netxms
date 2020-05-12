#ifndef _testtools_h_
#define _testtools_h_

/**
 * Exit test process
 */
#if HAVE_SYS_PTRACE_H

#include <sys/ptrace.h>

inline void ExitTestProcess()
{
#ifdef _AIX
#ifdef __64BIT__
   int rc = ptrace64(PT_TRACE_ME, 0, 0, 0, nullptr);
#else
   int rc = ptrace(PT_TRACE_ME, 0, nullptr, 0, nullptr);
#endif
#else
   int rc = ptrace(PT_TRACE_ME, 0, nullptr, 0);
#endif
   if (rc == -1)
      abort();
   else
      exit(1);
}

#else

inline void ExitTestProcess()
{
   exit(1);
}

#endif

/**
 * Assert failure
 */
#define Assert(c) do { if (!(c)) { _tprintf(_T("FAIL\n   Assert failed at %hs:%d\n"), __FILE__, __LINE__); ExitTestProcess(); } } while(0)

/**
 * Assert failure with additional message
 */
#define AssertEx(c, m) do { if (!(c)) { _tprintf(_T("FAIL\n   %s\n   Assert failed at %hs:%d\n"), (m), __FILE__, __LINE__); ExitTestProcess(); } } while(0)

/**
 * Assert that two values are equal
 */
#define AssertEquals(x, y) Assert((x) == (y))

/**
 * Assert that two values are not equal
 */
#define AssertNotEquals(x, y) Assert((x) != (y))

/**
 * Assert that value is TRUE
 */
#define AssertTrue(x) Assert((x))

/**
 * Assert that value is TRUE with message
 */
#define AssertTrueEx(x, m) AssertEx((x), (m))

/**
 * Assert that value is FALSE
 */
#define AssertFalse(x) Assert(!(x))

/**
 * Assert that value is FALSE with message
 */
#define AssertFalseEx(x, m) AssertEx(!(x), (m))

/**
 * Assert that value is NULL
 */
#define AssertNull(x) Assert((x) == NULL)

/**
 * Assert that value is not NULL
 */
#define AssertNotNull(x) Assert((x) != NULL)

/**
 * Assert that value is not NULL with message
 */
#define AssertNotNullEx(x, m) AssertEx((x) != NULL, (m))

/**
 * Show test start mark
 */
static inline void StartTest(const TCHAR *name)
{
   TCHAR filler[80];
   int l = 60 - (int)_tcslen(name);
   if (l > 0)
   {
      for(int i = 0; i < l; i++)
         filler[i] = _T('.');
      filler[l] = 0;
   }
   else
   {
      filler[0] = 0;
   }
   _tprintf(_T("%s %s "), name, filler);
   fflush(stdout);
}

/**
 * Show test start mark
 */
static inline void StartTest(const TCHAR *prefix, const TCHAR *name)
{
   TCHAR fullName[256];
   _sntprintf(fullName, 256, _T("%s: %s"), prefix, name);
   StartTest(fullName);
}

/**
 * Show test end
 */
static inline void EndTest()
{
   _tprintf(_T("OK\n"));
}

/**
 * Show test end with timimg
 */
static inline void EndTest(INT64 ms)
{
   _tprintf(_T("%d ms\n"), (int)ms);
}

#endif
