#ifndef _testtools_h_
#define _testtools_h_

/**
 * Exit test process
 */
#if HAVE_SYS_PTRACE_H && !defined(WITH_ADDRESS_SANITIZER)

#include <sys/ptrace.h>

static inline void ExitTestProcess()
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

static inline void ExitTestProcess()
{
   exit(1);
}

#endif

/**
 * Assert failure
 */
#define Assert(c) do { if (!(c)) { WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m\n"), __FILE__, __LINE__); ExitTestProcess(); } } while(0)

/**
 * Assert failure with additional message
 */
#define AssertEx(c, m) do { if (!(c)) { WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   %s\n   Assert failed at \x1b[1m%hs:%d\x1b[0m\n"), (m), __FILE__, __LINE__); ExitTestProcess(); } } while(0)

/**
 * Assert that two int32 values are equal
 */
static inline void __AssertEquals(int32_t value, int32_t expected, const char *file, int line)
{
   if (value == expected)
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = %d, actual value = %d)\n"), file, line, expected, value);
   ExitTestProcess();
}

/**
 * Assert that two uint32 values are equal
 */
static inline void __AssertEquals(uint32_t value, uint32_t expected, const char *file, int line)
{
   if (value == expected)
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = %u, actual value = %u)\n"), file, line, expected, value);
   ExitTestProcess();
}

/**
 * Assert that two int64 values are equal
 */
static inline void __AssertEquals(int64_t value, int64_t expected, const char *file, int line)
{
   if (value == expected)
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = ") INT64_FMT _T(", actual value = ") INT64_FMT _T(")\n"), file, line, expected, value);
   ExitTestProcess();
}

/**
 * Assert that two uint64 values are equal
 */
static inline void __AssertEquals(uint64_t value, uint64_t expected, const char *file, int line)
{
   if (value == expected)
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = ") UINT64_FMT _T(", actual value = ") UINT64_FMT _T(")\n"), file, line, expected, value);
   ExitTestProcess();
}

/**
 * Assert that size_t value are equal to int value
 */
static inline void __AssertEquals(size_t value, int expected, const char *file, int line)
{
   if ((int)value == expected)
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = %d, actual value = %d)\n"), file, line, (int)expected, value);
   ExitTestProcess();
}

#if CAN_OVERLOAD_SSIZE_T

/**
 * Assert that ssize_t value are equal to int value
 */
static inline void __AssertEquals(ssize_t value, int expected, const char *file, int line)
{
   if ((int)value == expected)
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = %d, actual value = %d)\n"), file, line, (int)expected, value);
   ExitTestProcess();
}

#endif

/**
 * Assert that two string values are equal
 */
static inline void __AssertEquals(const char *value, const char *expected, const char *file, int line)
{
   if (!strcmp(value, expected))
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = '%hs', actual value = '%hs')\n"), file, line, expected, value);
   ExitTestProcess();
}

/**
 * Assert that two string values are equal
 */
static inline void __AssertEquals(const wchar_t *value, const wchar_t *expected, const char *file, int line)
{
   if (!wcscmp(value, expected))
      return;
   WriteToTerminalEx(_T("\x1b[31;1mFAIL\x1b[0m\n   Assert failed at \x1b[1m%hs:%d\x1b[0m (expected value = '%ls', actual value = '%ls')\n"), file, line, expected, value);
   ExitTestProcess();
}

/**
 * Assert that two values are equal
 */
#define AssertEquals(x, y) __AssertEquals(x, y, __FILE__, __LINE__)

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
#define AssertNull(x) Assert((x) == nullptr)

/**
 * Assert that value is not NULL
 */
#define AssertNotNull(x) Assert((x) != nullptr)

/**
 * Assert that value is not NULL with message
 */
#define AssertNotNullEx(x, m) AssertEx((x) != nullptr, (m))

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
   WriteToTerminalEx(_T("%s %s "), name, filler);
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
   WriteToTerminal(_T("\x1b[32;1mOK\x1b[0m\n"));
}

/**
 * Show test end with timimg
 */
static inline void EndTest(int64_t ms)
{
   WriteToTerminalEx(_T("\x1b[36m") INT64_FMT _T(" ms\x1b[0m\n"), ms);
}

#endif
