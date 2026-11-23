#include <nms_util.h>
#include <netxms-regex.h>
#include <testtools.h>

/**
 * Size of ovector used by tests (room for 9 capture groups)
 */
#define OVECTOR_SIZE 30

/**
 * Fill ovector with marker value so untouched entries are detectable
 */
static void FillOvector(int *ovector)
{
   for(int i = 0; i < OVECTOR_SIZE; i++)
      ovector[i] = 0x5555;
}

/**
 * Check offsets of given capture group
 */
static void AssertGroup(const int *ovector, int group, int start, int end)
{
   AssertEquals(ovector[group * 2], start);
   AssertEquals(ovector[group * 2 + 1], end);
}

/**
 * Test capture group reporting through TCHAR regex wrappers
 */
static void TestCaptureGroupsTChar()
{
   StartTest(_T("Regex - capture groups (TCHAR)"));

   const char *eptr;
   int eoffset;
   PCRE *preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^(\\S+)\\s*(\\S+)?$")), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   AssertNotNull(preg);

   int captureCount = -1;
   _pcre_fullinfo_t(preg, nullptr, PCRE_INFO_CAPTURECOUNT, &captureCount);
   AssertEquals(captureCount, 2);

   int ovector[OVECTOR_SIZE];
   FillOvector(ovector);
   const TCHAR *subject = _T("abc def");
   AssertEquals(_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(subject), static_cast<int>(_tcslen(subject)), 0, 0, ovector, OVECTOR_SIZE), 3);
   AssertGroup(ovector, 0, 0, 7);
   AssertGroup(ovector, 1, 0, 3);
   AssertGroup(ovector, 2, 4, 7);

   // Trailing optional group does not participate; offsets from previous match must not survive
   subject = _T("abc");
   AssertEquals(_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(subject), static_cast<int>(_tcslen(subject)), 0, 0, ovector, OVECTOR_SIZE), 2);
   AssertGroup(ovector, 0, 0, 3);
   AssertGroup(ovector, 1, 0, 3);
   AssertGroup(ovector, 2, -1, -1);

   // Trailing optional group on first use of fresh ovector
   FillOvector(ovector);
   AssertEquals(_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(subject), static_cast<int>(_tcslen(subject)), 0, 0, ovector, OVECTOR_SIZE), 2);
   AssertGroup(ovector, 2, -1, -1);

   subject = _T("");
   Assert(_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(subject), 0, 0, 0, ovector, OVECTOR_SIZE) < 0);

   _pcre_free_t(preg);

   // Unset group in the middle of the pattern
   preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^(a)?(b)$")), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   AssertNotNull(preg);
   FillOvector(ovector);
   subject = _T("b");
   AssertEquals(_pcre_exec_t(preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(subject), static_cast<int>(_tcslen(subject)), 0, 0, ovector, OVECTOR_SIZE), 3);
   AssertGroup(ovector, 1, -1, -1);
   AssertGroup(ovector, 2, 0, 1);
   _pcre_free_t(preg);

   EndTest();
}

/**
 * Test capture group reporting through 8-bit regex wrappers
 */
static void TestCaptureGroupsUTF8()
{
   StartTest(_T("Regex - capture groups (8-bit)"));

   const char *eptr;
   int eoffset;
   pcre *preg = pcre_compile("^(\\S+)\\s*(\\S+)?$", PCRE_COMMON_FLAGS_A, &eptr, &eoffset, nullptr);
   AssertNotNull(preg);

   int captureCount = -1;
   pcre_fullinfo(preg, nullptr, PCRE_INFO_CAPTURECOUNT, &captureCount);
   AssertEquals(captureCount, 2);

   int ovector[OVECTOR_SIZE];
   FillOvector(ovector);
   AssertEquals(pcre_exec(preg, nullptr, "abc def", 7, 0, 0, ovector, OVECTOR_SIZE), 3);
   AssertGroup(ovector, 2, 4, 7);

   AssertEquals(pcre_exec(preg, nullptr, "abc", 3, 0, 0, ovector, OVECTOR_SIZE), 2);
   AssertGroup(ovector, 1, 0, 3);
   AssertGroup(ovector, 2, -1, -1);

   pcre_free(preg);

   EndTest();
}

/**
 * Regex wrapper tests
 */
void TestRegexCaptureGroups()
{
   TestCaptureGroupsTChar();
   TestCaptureGroupsUTF8();
}
