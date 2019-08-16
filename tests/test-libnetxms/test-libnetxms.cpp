#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <nxqueue.h>
#include <nxcpapi.h>
#include <nxproc.h>
#include <testtools.h>

NETXMS_EXECUTABLE_HEADER(test-libnetxms)

void TestMemoryPool();
void TestObjectMemoryPool();
void TestThreadPool();
void TestMsgWaitQueue();
void TestMessageClass();
void TestMutex();
void TestMutexWrapper();
void TestRWLockWrapper();
void TestConditionWrapper();
void TestThreadCountAndMaxWaitTime();
void TestProcessExecutor(const char *procname);
void TestProcessExecutorWorker();
void TestSubProcess(const char *procname);
NXCPMessage *TestSubProcessRequestHandler(UINT16 command, const void *data, size_t dataSize);

static char mbText[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static WCHAR wcText[] = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static char mbTextShort[] = "Lorem ipsum";
static UCS2CHAR ucs2TextShort[] = { 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm', 0 };
static UCS2CHAR ucs2TextSurrogates[] = { 'L', 'o', 'r', 'e', 'm', 0xD801, 0xDFFF, 'i', 'p', 's', 'u', 'm', 0x1CD, '.', 0 };
static UCS4CHAR ucs4TextSurrogatesTest[] = { 'L', 'o', 'r', 'e', 'm', 0x0107FF, 'i', 'p', 's', 'u', 'm', 0x1CD, '.', 0 };
static char utf8TextSurrogatesTest[] = { 'L', 'o', 'r', 'e', 'm', (char)0xF0, (char)0x90, (char)0x9F, (char)0xBF, 'i', 'p', 's', 'u', 'm', (char)0xC7, (char)0x8D, '.', 0 };
#if defined(_WIN32) || defined(__HP_aCC)
static WCHAR wcTextISO8859_1[] = L"Lorem ipsum dolor sit amet, \xA3 10, \xA2 20, \xA5 50, b\xF8nne";
#else
static WCHAR wcTextISO8859_1[] = L"Lorem ipsum dolor sit amet, £ 10, ¢ 20, ¥ 50, bønne";
#endif
static char mbTextISO8859_1[] = "Lorem ipsum dolor sit amet, \xA3 10, \xA2 20, \xA5 50, b\xF8nne";
static char mbTextASCII[] = "Lorem ipsum dolor sit amet, ? 10, ? 20, ? 50, b?nne";

/**
 * Test string conversion
 */
static void TestStringConversion()
{
   StartTest(_T("ANSI to UCS-2 conversion"));
   UCS2CHAR ucs2buffer[1024];
   mb_to_ucs2(mbTextShort, -1, ucs2buffer, 1024);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextShort, sizeof(UCS2CHAR) * 12));
   EndTest();

   StartTest(_T("UCS-2 to ANSI conversion"));
   char mbBuffer[1024];
   ucs2_to_mb(ucs2TextShort, -1, mbBuffer, 1024);
   AssertTrue(!strcmp(mbBuffer, mbTextShort));
   EndTest();

   StartTest(_T("ANSI to UCS-2 conversion performance"));
   INT64 start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS2CHAR buffer[1024];
      mb_to_ucs2(mbText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UCS-2 to ANSI conversion performance"));
   mb_to_ucs2(mbText, -1, ucs2buffer, 1024);
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      char buffer[1024];
      ucs2_to_mb(ucs2buffer, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

#ifdef UNICODE_UCS4
   StartTest(_T("ANSI to UCS-4 conversion performance"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      WCHAR buffer[1024];
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UCS-4 to ANSI conversion performance"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      char buffer[1024];
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, wcText, -1, buffer, 1024, NULL, NULL);
   }
   EndTest(GetCurrentTimeMs() - start);
#endif

   StartTest(_T("UCS-2 to UCS-4 conversion"));
   UCS4CHAR ucs4buffer[1024];
   size_t len = ucs2_to_ucs4(ucs2TextSurrogates, -1, ucs4buffer, 128);
   AssertEquals(len, 13);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogatesTest, 13 * sizeof(UCS4CHAR)));
   EndTest();

   StartTest(_T("UCS-4 to UCS-2 conversion"));
   len = ucs4_to_ucs2(ucs4TextSurrogatesTest, -1, ucs2buffer, 1024);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, 14 * sizeof(UCS2CHAR)));
   EndTest();

   StartTest(_T("UCS-2 to UCS-4 conversion performance"));
#ifdef UNICODE_UCS4
   mb_to_ucs2(mbText, -1, ucs2buffer, 1024);
#endif
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS4CHAR buffer[1024];
#ifdef UNICODE_UCS4
      ucs2_to_ucs4(ucs2buffer, -1, buffer, 1024);
#else
      ucs2_to_ucs4(wcText, -1, buffer, 1024);
#endif
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UCS-4 to UCS-2 conversion performance"));
#ifdef UNICODE_UCS2
   ucs2_to_ucs4(wcText, -1, ucs4buffer, 1024);
#endif
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS2CHAR buffer[1024];
#ifdef UNICODE_UCS4
      ucs4_to_ucs2(wcText, -1, buffer, 1024);
#else
      ucs4_to_ucs2(ucs4buffer, -1, buffer, 1024);
#endif
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("ucs2_utf8len"));
   AssertEquals(ucs2_utf8len(ucs2TextSurrogates, -1), 18);  // ucs2_utf8len includes trailing 0 into length
   EndTest();

   StartTest(_T("utf8_ucs2len"));
   AssertEquals(utf8_ucs2len(utf8TextSurrogatesTest, -1), 15);  // utf8_ucs2len includes trailing 0 into length
   EndTest();

   StartTest(_T("UCS-2 to UTF-8 conversion"));
   char utf8buffer[128];
   len = ucs2_to_utf8(ucs2TextSurrogates, -1, utf8buffer, 128);
   AssertEquals(len, 17);
   AssertTrue(!strcmp(utf8buffer, utf8TextSurrogatesTest));
   EndTest();

   StartTest(_T("UTF-8 to UCS-2 conversion"));
   len = utf8_to_ucs2(utf8TextSurrogatesTest, -1, ucs2buffer, 1024);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, 14 * sizeof(UCS2CHAR)));
   EndTest();

   StartTest(_T("UCS-2 to UTF-8 conversion performance"));
   mb_to_ucs2(mbText, -1, ucs2buffer, 1024);
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      char buffer[1024];
      ucs2_to_utf8(ucs2buffer, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UTF-8 to UCS-2 conversion performance"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS2CHAR buffer[1024];
      utf8_to_ucs2(mbText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("ucs4_utf8len"));
   AssertEquals(ucs4_utf8len(ucs4TextSurrogatesTest, -1), 18);  // ucs4_utf8len includes trailing 0 into length
   EndTest();

   StartTest(_T("utf8_ucs4len"));
   AssertEquals(utf8_ucs4len(utf8TextSurrogatesTest, -1), 14);  // utf8_ucs2len includes trailing 0 into length
   EndTest();

   StartTest(_T("UCS-4 to UTF-8 conversion"));
   len = ucs4_to_utf8(ucs4TextSurrogatesTest, -1, utf8buffer, 128);
   AssertEquals(len, 17);
   AssertTrue(!strcmp(utf8buffer, utf8TextSurrogatesTest));
   EndTest();

   StartTest(_T("UTF-8 to UCS-4 conversion"));
   len = utf8_to_ucs4(utf8TextSurrogatesTest, -1, ucs4buffer, 1024);
   AssertEquals(len, 13);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogatesTest, 13 * sizeof(UCS2CHAR)));
   EndTest();

   StartTest(_T("UCS-4 to UTF-8 conversion performance"));
   mb_to_ucs4(mbText, -1, ucs4buffer, 1024);
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      char buffer[1024];
      ucs4_to_utf8(ucs4buffer, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UTF-8 to UCS-4 conversion performance"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS4CHAR buffer[1024];
      utf8_to_ucs4(mbText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

#if UNICODE_UCS4
#define ucs4TextISO8859_1 wcTextISO8859_1
   UCS2CHAR ucs2TextISO8859_1[256];
   ucs4_to_ucs2(wcTextISO8859_1, -1, ucs2TextISO8859_1, 256);
#else
#define ucs2TextISO8859_1 wcTextISO8859_1
   UCS4CHAR ucs4TextISO8859_1[256];
   ucs2_to_ucs4(wcTextISO8859_1, -1, ucs4TextISO8859_1, 256);
#endif

   StartTest(_T("ISO8859-1 to UCS-4 conversion"));
   len = ISO8859_1_to_ucs4(mbTextISO8859_1, -1, ucs4buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextISO8859_1, 51 * sizeof(UCS4CHAR)));
   EndTest();

   StartTest(_T("UCS-4 to ISO8859-1 conversion"));
   len = ucs4_to_ISO8859_1(ucs4TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextISO8859_1, 51));
   EndTest();

   StartTest(_T("ISO8859-1 to UCS-2 conversion"));
   len = ISO8859_1_to_ucs2(mbTextISO8859_1, -1, ucs2buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextISO8859_1, 51 * sizeof(UCS2CHAR)));
   EndTest();

   StartTest(_T("UCS-2 to ISO8859-1 conversion"));
   len = ucs2_to_ISO8859_1(ucs2TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextISO8859_1, 51));
   EndTest();

   StartTest(_T("UCS-4 to ASCII conversion"));
   len = ucs4_to_ASCII(ucs4TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextASCII, 51));
   EndTest();

   StartTest(_T("UCS-2 to ASCII conversion"));
   len = ucs2_to_ASCII(ucs2TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextASCII, 51));
   EndTest();
}

/**
 * Test string list
 */
static void TestStringList()
{
   StartTest(_T("String list - add"));
   StringList *s1 = new StringList();
   s1->add(1);
#ifdef __HP_aCC
   s1->add(static_cast<INT64>(_LL(12345000000001)));
#else
   s1->add(_LL(12345000000001));
#endif
   s1->add(_T("text1"));
   s1->addPreallocated(_tcsdup(_T("text2")));
   s1->add(3.1415);

   AssertEquals(s1->size(), 5);
   AssertTrue(!_tcscmp(s1->get(0), _T("1")));
   AssertTrue(!_tcscmp(s1->get(1), _T("12345000000001")));
   AssertTrue(!_tcscmp(s1->get(2), _T("text1")));
   AssertTrue(!_tcscmp(s1->get(3), _T("text2")));
   AssertTrue(!_tcsncmp(s1->get(4), _T("3.1415"), 6));
   EndTest();

   StartTest(_T("String list - copy constructor"));
   StringList *s2 = new StringList(s1);
   delete s1;

   AssertEquals(s2->size(), 5);
   AssertTrue(!_tcscmp(s2->get(0), _T("1")));
   AssertTrue(!_tcscmp(s2->get(1), _T("12345000000001")));
   AssertTrue(!_tcscmp(s2->get(2), _T("text1")));
   AssertTrue(!_tcscmp(s2->get(3), _T("text2")));
   AssertTrue(!_tcsncmp(s2->get(4), _T("3.1415"), 6));
   EndTest();

   StartTest(_T("String list - fill message"));

   NXCPMessage msg;
   s2->fillMessage(&msg, 100, 1);
   delete s2;

   StringList *s3 = new StringList(&msg, 100, 1);
   AssertEquals(s3->size(), 5);
   AssertTrue(!_tcscmp(s3->get(0), _T("1")));
   AssertTrue(!_tcscmp(s3->get(1), _T("12345000000001")));
   AssertTrue(!_tcscmp(s3->get(2), _T("text1")));
   AssertTrue(!_tcscmp(s3->get(3), _T("text2")));
   AssertTrue(!_tcsncmp(s3->get(4), _T("3.1415"), 6));
   delete s3;
   EndTest();

   StartTest(_T("String list - performance"));
   INT64 startTime = GetCurrentTimeMs();
   StringList *s4 = new StringList();
   for(int i = 0; i < 100000; i++)
   {
      s4->add(42);
   }
   AssertEquals(s4->size(), 100000);
   for(int i = 0; i < 100000; i++)
   {
      s4->replace(i, _T("value for replacement"));
   }
   AssertEquals(s4->size(), 100000);
   delete s4;
   EndTest(GetCurrentTimeMs() - startTime);
}

/**
 * Test string map
 */
static void TestStringMap()
{
   StringMap *m = new StringMap();

   StartTest(_T("String map - insert"));
   INT64 start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d"), i);
      m->set(key, _T("Lorem ipsum dolor sit amet"));
   }
   AssertEquals(m->size(), 10000);
   const TCHAR *v = m->get(_T("key-42"));
   AssertNotNull(v);
   AssertTrue(!_tcscmp(v, _T("Lorem ipsum dolor sit amet")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - replace"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d"), i);
      m->set(key, _T("consectetur adipiscing elit"));
   }
   AssertEquals(m->size(), 10000);
   v = m->get(_T("key-42"));
   AssertNotNull(v);
   AssertTrue(!_tcscmp(v, _T("consectetur adipiscing elit")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - get"));
   start = GetCurrentTimeMs();
   v = m->get(_T("key-888"));
   AssertNotNull(v);
   AssertTrue(!_tcscmp(v, _T("consectetur adipiscing elit")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - keys"));
   StringList *keys = m->keys();
   AssertNotNull(keys);
   AssertEquals(keys->size(), 10000);
   AssertTrue(!_tcsncmp(keys->get(500), _T("key-"), 4));
   delete keys;
   EndTest();

   StartTest(_T("String map - clear"));
   start = GetCurrentTimeMs();
   m->clear();
   AssertEquals(m->size(), 0);
   EndTest(GetCurrentTimeMs() - start);

   delete m;
}

/**
 * Test string set
 */
static void TestStringSet()
{
   StringSet *s = new StringSet();

   StartTest(_T("String set - insert"));
   INT64 start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d lorem ipsum"), i);
      s->add(key);
   }
   AssertEquals(s->size(), 10000);
   AssertTrue(s->contains(_T("key-42 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - replace"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d lorem ipsum"), i);
      s->add(key);
   }
   AssertEquals(s->size(), 10000);
   AssertTrue(s->contains(_T("key-42 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - contains"));
   start = GetCurrentTimeMs();
   AssertTrue(s->contains(_T("key-888 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - iterator"));
   Iterator<const TCHAR> *it = s->iterator();
   AssertTrue(it->hasNext());
   bool found = false;
   while(it->hasNext())
   {
      const TCHAR *v = it->next();
      AssertNotNull(v);
      if (!_tcscmp(v, _T("key-42 lorem ipsum")))
      {
         found = true;
         break;
      }
   }
   AssertTrue(found);
   it->remove();
   AssertEquals(s->size(), 9999);
   AssertFalse(s->contains(_T("key-42 lorem ipsum")));
   delete it;
   EndTest();

   StartTest(_T("String set - clear"));
   start = GetCurrentTimeMs();
   s->clear();
   AssertEquals(s->size(), 0);
   EndTest(GetCurrentTimeMs() - start);

   delete s;
}

/**
 * Test string functions
 */
static void TestStringFunctionsA()
{
   char buffer[36];
   buffer[32] = '$';

   StartTest(_T("strlcpy"));

   strlcpy(buffer, "short text", 32);
   AssertEquals(buffer[32], '$');
   AssertTrue(!strcmp(buffer, "short text"));

   strlcpy(buffer, "long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], '$');
   AssertTrue(!strcmp(buffer, "long text: 1234567890 123456789"));

   EndTest();

   StartTest(_T("strlcat"));

   memset(buffer, 0, sizeof(buffer));
   buffer[32] = '$';
   strlcat(buffer, "part1", 32);
   AssertEquals(buffer[32], '$');
   AssertTrue(!strcmp(buffer, "part1"));

   strlcat(buffer, "part2", 32);
   AssertEquals(buffer[32], '$');
   AssertTrue(!strcmp(buffer, "part1part2"));

   strlcat(buffer, "long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], '$');
   AssertTrue(!strcmp(buffer, "part1part2long text: 1234567890"));

   EndTest();
}

/**
 * Test string functions (UNICODE)
 */
static void TestStringFunctionsW()
{
   WCHAR buffer[36];
   buffer[32] = L'$';

   StartTest(_T("wcslcpy"));

   wcslcpy(buffer, L"short text", 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer,L"short text"));

   wcslcpy(buffer, L"long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, L"long text: 1234567890 123456789"));

   EndTest();

   StartTest(_T("wcslcat"));

   memset(buffer, 0, sizeof(buffer));
   buffer[32] = L'$';
   wcslcat(buffer, L"part1", 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, L"part1"));

   wcslcat(buffer, L"part2", 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, L"part1part2"));

   wcslcat(buffer, L"long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, L"part1part2long text: 1234567890"));

   EndTest();

   StartTest(_T("wcsupr"));
   WCHAR textToUpper[64] = L"TeXt 123 abCD";
   wcsupr(textToUpper);
   AssertTrue(!wcscmp(textToUpper, L"TEXT 123 ABCD"));
   EndTest();

   StartTest(_T("wcslwr"));
   WCHAR textToLower[64] = L"TeXt 123 abCD";
   wcslwr(textToLower);
   AssertTrue(!wcscmp(textToLower, L"text 123 abcd"));
   EndTest();
}

/**
 * Test pattern matching functions
 */
static void TestPatternMatching()
{
   StartTest(_T("MatchString"));
   AssertTrue(MatchString(_T("*"), _T("whatever"), true));
   AssertTrue(MatchString(_T("??*"), _T("whatever"), true));
   AssertTrue(MatchString(_T("??*"), _T("zz"), true));
   AssertFalse(MatchString(_T("??*"), _T("X"), true));
   AssertFalse(MatchString(_T("A*B*"), _T("Alphabet"), true));
   AssertTrue(MatchString(_T("A*B*"), _T("Alphabet"), false));
   AssertTrue(MatchString(_T("*t"), _T("Alphabet"), true));
   AssertTrue(MatchString(_T("*?*"), _T("some text"), true));
   AssertTrue(MatchString(_T("*?*t"), _T("some text"), true));
   EndTest();
}

/**
 * Test string class
 */
static void TestString()
{
   String s;

   StartTest(_T("String - append"));
   for(int i = 0; i < 256; i++)
      s.append(_T("ABC "));
   AssertEquals(s.length(), 1024);
   AssertTrue(!_tcsncmp(s.getBuffer(), _T("ABC ABC ABC ABC "), 16));
   EndTest();

   StartTest(_T("String - assign #1"));
   s = _T("alpha");
   AssertEquals(s.length(), 5);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("alpha")));
   EndTest();

   StartTest(_T("String - assign #2"));
   String t(_T("init string"));
   s = t;
   AssertEquals(s.length(), 11);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("init string")));
   EndTest();

   StartTest(_T("String - shrink"));
   s.shrink();
   AssertEquals(s.length(), 10);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("init strin")));
   EndTest();

   StartTest(_T("String - escape"));
   s.escapeCharacter('i', '+');
   AssertEquals(s.length(), 13);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("+in+it str+in")));
   EndTest();

   StartTest(_T("String - replace #1"));
   s = _T("alpha beta gamma");
   s.replace(_T("beta"), _T("epsilon"));
   AssertEquals(s.length(), 19);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("alpha epsilon gamma")));
   EndTest();

   StartTest(_T("String - replace #2"));
   s = _T("alpha beta gamma");
   s.replace(_T("beta"), _T("xxxx"));
   AssertEquals(s.length(), 16);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("alpha xxxx gamma")));
   EndTest();

   StartTest(_T("String - replace #3"));
   s = _T("alpha beta gamma alpha omega");
   s.replace(_T("alpha"), _T("Z"));
   AssertEquals(s.length(), 20);
   AssertTrue(!_tcscmp(s.getBuffer(), _T("Z beta gamma Z omega")));
   EndTest();

   StartTest(_T("String - substring #1"));
   s = _T("alpha beta gamma");
   TCHAR *str = s.substring(0, 5, NULL);
   AssertTrue(!_tcscmp(str, _T("alpha")));
   free(str);
   EndTest();

   StartTest(_T("String - substring #2"));
   s = _T("alpha beta gamma");
   str = s.substring(5, -1, NULL);
   AssertTrue(!_tcscmp(str, _T(" beta gamma")));
   free(str);
   EndTest();

   StartTest(_T("String - substring #3"));
   s = _T("alpha beta gamma");
   str = s.substring(14, 4, NULL);
   AssertTrue(!_tcscmp(str, _T("ma")));
   free(str);
   EndTest();

   StartTest(_T("String - substring #4"));
   s = _T("alpha beta gamma");
   String ss = s.substring(0, 5);
   AssertTrue(!_tcscmp(ss, _T("alpha")));
   EndTest();

   StartTest(_T("String - substring #5"));
   s = _T("alpha beta gamma");
   ss = s.substring(5, -1);
   AssertTrue(!_tcscmp(ss, _T(" beta gamma")));
   EndTest();

   StartTest(_T("String - substring #6"));
   s = _T("alpha beta gamma");
   ss = s.substring(14, 4);
   AssertTrue(!_tcscmp(ss, _T("ma")));
   EndTest();

   StartTest(_T("String - left #1"));
   s = _T("alpha beta gamma");
   String ls = s.left(5);
   AssertTrue(ls.equals(_T("alpha")));
   EndTest();

   StartTest(_T("String - left #2"));
   s = _T("alpha");
   ls = s.left(15);
   AssertTrue(ls.equals(_T("alpha")));
   EndTest();

   StartTest(_T("String - right #1"));
   s = _T("alpha beta gamma");
   String rs = s.right(5);
   AssertTrue(rs.equals(_T("gamma")));
   EndTest();

   StartTest(_T("String - right #2"));
   s = _T("alpha");
   rs = s.right(15);
   AssertTrue(rs.equals(_T("alpha")));
   EndTest();

   StartTest(_T("String - split"));
   s = _T("alpha;;beta;gamma;;delta");
   StringList *list = s.split(_T(";;"));
   AssertNotNull(list);
   AssertEquals(list->size(), 3);
   AssertTrue(!_tcscmp(list->get(0), _T("alpha")));
   AssertTrue(!_tcscmp(list->get(1), _T("beta;gamma")));
   AssertTrue(!_tcscmp(list->get(2), _T("delta")));
   delete list;
   EndTest();
}

/**
 * Test MacAddress class
 */
static void TestMacAddress()
{
   MacAddress a, b, c, d, e, f;

   StartTest(_T("MacAddress - parse()"));
   a = MacAddress::parse("0180C2300100");
   AssertTrue(_tcscmp(_T("0180C2300100"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("01:80:C2:30:01:00"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("01-80-C2-30-01-00"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("018.0C2.300.100"), (const TCHAR*)a.toString(MacAddressNotation::DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("0180.C230.0100"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("0180:C230:0100"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("1.128.194.48.1.0"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("01:80:C2:00:00:00");
   AssertTrue(_tcscmp(_T("0180C2000000"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("01:80:C2:00:00:00"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("01-80-C2-00-00-00"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("018.0C2.000.000"), (const TCHAR*)a.toString(MacAddressNotation::DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("0180.C200.0000"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("0180:C200:0000"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("1.128.194.0.0.0"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("09-80-C2-FF-FF-FF");
   AssertTrue(_tcscmp(_T("0980C2FFFFFF"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("09:80:C2:FF:FF:FF"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("09-80-C2-FF-FF-FF"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("098.0C2.FFF.FFF"), (const TCHAR*)a.toString(MacAddressNotation::DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("0980.C2FF.FFFF"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("0980:C2FF:FFFF"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("9.128.194.255.255.255"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("482.C6A.1E5.93D");
   AssertTrue(_tcscmp(_T("482C6A1E593D"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("48:2C:6A:1E:59:3D"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("48-2C-6A-1E-59-3D"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("482.C6A.1E5.93D"), (const TCHAR*)a.toString(MacAddressNotation::DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("482C.6A1E.593D"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("482C:6A1E:593D"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("72.44.106.30.89.61"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("48AC.6B1A.F6FD");
   AssertTrue(_tcscmp(_T("48AC6B1AF6FD"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("48:AC:6B:1A:F6:FD"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("48-AC-6B-1A-F6-FD"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("48A.C6B.1AF.6FD"), (const TCHAR*)a.toString(MacAddressNotation::DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("48AC.6B1A.F6FD"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("48AC:6B1A:F6FD"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("72.172.107.26.246.253"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("482C:6A1E:593D");
   AssertTrue(_tcscmp(_T("482C6A1E593D"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("48:2C:6A:1E:59:3D"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("48-2C-6A-1E-59-3D"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("482.C6A.1E5.93D"), (const TCHAR*)a.toString(MacAddressNotation::DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("482C.6A1E.593D"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("482C:6A1E:593D"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("72.44.106.30.89.61"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("70B3D5B020035F11");
   AssertTrue(_tcscmp(_T("70B3D5B020035F11"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("70:B3:D5:B0:20:03:5F:11"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70-B3-D5-B0-20-03-5F-11"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3.D5B0.2003.5F11"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3:D5B0:2003:5F11"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("112.179.213.176.32.3.95.17"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("70:B3:D5:B0:20:03:5F:16");
   AssertTrue(_tcscmp(_T("70B3D5B020035F16"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("70:B3:D5:B0:20:03:5F:16"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70-B3-D5-B0-20-03-5F-16"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3.D5B0.2003.5F16"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3:D5B0:2003:5F16"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("112.179.213.176.32.3.95.22"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("70-B3-D5-B0-20-00-00-39");
   AssertTrue(_tcscmp(_T("70B3D5B020000039"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("70:B3:D5:B0:20:00:00:39"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70-B3-D5-B0-20-00-00-39"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3.D5B0.2000.0039"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3:D5B0:2000:0039"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("112.179.213.176.32.0.0.57"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);

   a = MacAddress::parse("70B3.D5B0.2003.5D5E");
   AssertTrue(_tcscmp(_T("70B3D5B020035D5E"), (const TCHAR*)a.toString(MacAddressNotation::FLAT_STRING)) == 0);
   AssertTrue(_tcscmp(_T("70:B3:D5:B0:20:03:5D:5E"), (const TCHAR*)a.toString(MacAddressNotation::COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70-B3-D5-B0-20-03-5D-5E"), (const TCHAR*)a.toString(MacAddressNotation::HYPHEN_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3.D5B0.2003.5D5E"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_DOT_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("70B3:D5B0:2003:5D5E"), (const TCHAR*)a.toString(MacAddressNotation::BYTEPAIR_COLON_SEPARATED)) == 0);
   AssertTrue(_tcscmp(_T("112.179.213.176.32.3.93.94"), (const TCHAR*)a.toString(MacAddressNotation::DECIMAL_DOT_SEPARATED)) == 0);
   EndTest();

   StartTest(_T("MacAddress - isMulticast()"));
   a = MacAddress::parse("01:80:C2:00:00:00");
   AssertTrue(a.isMulticast());
   b = MacAddress::parse("09-80-C2-FF-FF-FF");
   AssertTrue(b.isMulticast());
   c = MacAddress::parse("482.C6A.1E5.93D");
   AssertFalse(c.isMulticast());
   EndTest();

   StartTest(_T("MacAddress - isBroadcast()"));
   a = MacAddress::parse("FF:FF:FF:FF:FF:FF");
   AssertTrue(a.isBroadcast());
   b = MacAddress::parse("FF-2C-6A-1E-59-3D");
   AssertFalse(b.isBroadcast());
   c = MacAddress::parse("FFF.FC2.FFF.FFF");
   AssertFalse(c.isBroadcast());
   EndTest();

   StartTest(_T("MacAddress - equals()"));
   a = MacAddress::parse("09-80-C2-FF-FF-FF");
   b = MacAddress::parse("48:2C:6A:1E:59:3D");
   c = MacAddress::parse("098.0C2.FFF.FFF");
   d = MacAddress::parse("70B3D5B020035F11");
   e = MacAddress::parse("70B3:D5B0:2003:5F11");
   f = MacAddress::parse("70B3.D5B0.2003.5D5E");
   AssertFalse(a.equals(b));
   AssertFalse(b.equals(c));
   AssertFalse(e.equals(f));
   AssertFalse(d.equals(b));
   AssertTrue(c.equals(a));
   AssertTrue(d.equals(e));
   EndTest();
}

/**
 * Test InetAddress class
 */
static void TestInetAddress()
{
   InetAddress a, b, c;

   StartTest(_T("InetAddress - isLoopback() - IPv4"));
   a = InetAddress::parse("127.0.0.1");
   AssertTrue(a.isLoopback());
   a = InetAddress::parse("192.168.1.1");
   AssertFalse(a.isLoopback());
   EndTest();

   StartTest(_T("InetAddress - isLoopback() - IPv6"));
   a = InetAddress::parse("::1");
   AssertTrue(a.isLoopback());
   a = InetAddress::parse("2000:1234::1");
   AssertFalse(a.isLoopback());
   EndTest();

   StartTest(_T("InetAddress - isSubnetBroadcast() - IPv4"));
   a = InetAddress::parse("192.168.0.255");
   AssertTrue(a.isSubnetBroadcast(24));
   AssertFalse(a.isSubnetBroadcast(23));
   EndTest();

   StartTest(_T("InetAddress - isSubnetBroadcast() - IPv6"));
   a = InetAddress::parse("fe80::ffff:ffff:ffff:ffff");
   AssertFalse(a.isSubnetBroadcast(64));
   AssertFalse(a.isSubnetBroadcast(63));
   EndTest();

   StartTest(_T("InetAddress - isLinkLocal() - IPv4"));
   a = InetAddress::parse("169.254.17.198");
   AssertTrue(a.isLinkLocal());
   a = InetAddress::parse("192.168.1.1");
   AssertFalse(a.isLinkLocal());
   EndTest();

   StartTest(_T("InetAddress - isLinkLocal() - IPv6"));
   a = InetAddress::parse("fe80::1");
   AssertTrue(a.isLinkLocal());
   a = InetAddress::parse("2000:1234::1");
   AssertFalse(a.isLinkLocal());
   EndTest();

   StartTest(_T("InetAddress - sameSubnet() - IPv4"));
   a = InetAddress::parse("192.168.1.43");
   a.setMaskBits(23);
   b = InetAddress::parse("192.168.0.180");
   b.setMaskBits(23);
   c = InetAddress::parse("192.168.2.22");
   c.setMaskBits(23);
   AssertTrue(a.sameSubnet(b));
   AssertFalse(a.sameSubnet(c));
   EndTest();

   StartTest(_T("InetAddress - sameSubnet() - IPv6"));
   a = InetAddress::parse("2000:1234:1000:1000::1");
   a.setMaskBits(62);
   b = InetAddress::parse("2000:1234:1000:1001::cdef:1");
   b.setMaskBits(62);
   c = InetAddress::parse("2000:1234:1000:1007::1");
   c.setMaskBits(62);
   AssertTrue(a.sameSubnet(b));
   AssertFalse(a.sameSubnet(c));
   EndTest();
   
   StartTest(_T("InetAddress - buildHashKey() - IPv4"));
   a = InetAddress::parse("10.3.1.91");
   BYTE key[18];
   a.buildHashKey(key);
#if WORDS_BIGENDIAN
   static BYTE keyIPv4[] = { 0x06, AF_INET, 0x0A, 0x03, 0x01, 0x5B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
   AssertTrue(memcmp(key, keyIPv4, 18) == 0);
#else
   static BYTE keyIPv4[] = { 0x06, AF_INET, 0x5B, 0x01, 0x03, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
   AssertTrue(memcmp(key, keyIPv4, 18) == 0);
#endif
   EndTest();
   
   StartTest(_T("InetAddress - buildHashKey() - IPv6"));
   a = InetAddress::parse("fe80:1234::6e88:14ff:fec4:b8f8");
   a.buildHashKey(key);
   static BYTE keyIPv6[] = { 0x12, AF_INET6, 0xFE, 0x80, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x88, 0x14, 0xFF, 0xFE, 0xC4, 0xB8, 0xF8 };
   AssertTrue(memcmp(key, keyIPv6, 18) == 0);
   EndTest();

   TCHAR text[64];

   StartTest(_T("InetAddress - toString() - IPv4"));
   a = InetAddress::parse("10.217.11.43");
   a.toString(text);
   AssertTrue(!_tcscmp(text, _T("10.217.11.43")));
   EndTest();

   StartTest(_T("InetAddress - toString() - IPv6"));
   a = InetAddress::parse("2000:1234::1");
   a.toString(text);
   AssertTrue(!_tcscmp(text, _T("2000:1234::1")));
   a = InetAddress::parse("2001:47c::");
   a.toString(text);
   AssertTrue(!_tcscmp(text, _T("2001:47c::")));
   a = InetAddress::parse("2001:47c::df33:16a");
   a.toString(text);
   AssertTrue(!_tcscmp(text, _T("2001:47c::df33:16a")));
   EndTest();
}

/**
 * Test itoa/itow
 */
static void TestItoa()
{
   char buffer[64];
   WCHAR wbuffer[64];

   StartTest(_T("itoa"));
   AssertTrue(!strcmp(_itoa(127, buffer, 10), "127"));
   AssertTrue(!strcmp(_itoa(0, buffer, 10), "0"));
   AssertTrue(!strcmp(_itoa(-3, buffer, 10), "-3"));
   AssertTrue(!strcmp(_itoa(0555, buffer, 8), "555"));
   AssertTrue(!strcmp(_itoa(0xFA48, buffer, 16), "fa48"));
   EndTest();

   StartTest(_T("itow"));
   AssertTrue(!wcscmp(_itow(127, wbuffer, 10), L"127"));
   AssertTrue(!wcscmp(_itow(0, wbuffer, 10), L"0"));
   AssertTrue(!wcscmp(_itow(-3, wbuffer, 10), L"-3"));
   AssertTrue(!wcscmp(_itow(0555, wbuffer, 8), L"555"));
   AssertTrue(!wcscmp(_itow(0xFA48, wbuffer, 16), L"fa48"));
   EndTest();
}

/**
 * Test queue
 */
static void TestQueue()
{
   Queue *q = new Queue(16, 16);

   StartTest(_T("Queue: put/get"));
   for(int i = 0; i < 40; i++)
      q->put(CAST_TO_POINTER(i + 1, void *));
   AssertEquals(q->size(), 40);
   AssertEquals(q->allocated(), 48);
   for(int i = 0; i < 40; i++)
   {
      void *p = q->get();
      AssertNotNull(p);
      AssertEquals(CAST_FROM_POINTER(p, int), i + 1);
   }
   EndTest();

   StartTest(_T("Queue: shrink"));
   for(int i = 0; i < 60; i++)
      q->put(CAST_TO_POINTER(i + 1, void *));
   AssertEquals(q->size(), 60);
   AssertEquals(q->allocated(), 64);
   for(int i = 0; i < 55; i++)
   {
      void *p = q->get();
      AssertNotNull(p);
      AssertEquals(CAST_FROM_POINTER(p, int), i + 1);
   }
   AssertEquals(q->size(), 5);
   AssertEquals(q->allocated(), 16);
   EndTest();

   StartTest(_T("Queue: performance"));
   delete q;
   q = new Queue();
   INT64 startTime = GetCurrentTimeMs();
   for(int i = 0; i < 100; i++)
   {
      for(int j = 0; j < 10000; j++)
         q->put(CAST_TO_POINTER(j + 1, void *));
      AssertEquals(q->size(), 10000);

      void *p;
      while((p = q->get()) != NULL)
         ;
      AssertEquals(q->size(), 0);
   }
   EndTest(GetCurrentTimeMs() - startTime);

   delete q;
}

/**
 * Key for hash map
 */
typedef char HASH_KEY[6];

/**
 * Long hash key
 */
typedef char LONG_HASH_KEY[32];

/**
 * Test hash map
 */
static void TestHashMap()
{
   StartTest(_T("HashMap - create"));
   HashMap<HASH_KEY, String> *hashMap = new HashMap<HASH_KEY, String>(true);
   AssertEquals(hashMap->size(), 0);
   EndTest();

   HASH_KEY k1 = { '1', '2', '3', '4', '5', '6' };
   HASH_KEY k2 = { '0', '0', 'a', 'b', 'c', 'd' };
   HASH_KEY k3 = { '0', '0', '3', 'X', '1', '1' };

   StartTest(_T("HashMap - set/get"));

   hashMap->set(k1, new String(_T("String 1")));
   hashMap->set(k2, new String(_T("String 2")));
   hashMap->set(k3, new String(_T("String 3")));

   String *s = hashMap->get(k1);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 1")));

   s = hashMap->get(k2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 2")));

   s = hashMap->get(k3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 3")));

   EndTest();

   StartTest(_T("HashMap - replace"));
   hashMap->set(k2, new String(_T("REPLACE")));
   s = hashMap->get(k2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("REPLACE")));
   EndTest();

   StartTest(_T("HashMap - iterator"));
   Iterator<String> *it = hashMap->iterator();
   AssertTrue(it->hasNext());
   s = it->next();
   AssertNotNull(s);
   AssertNotNull(it->next());
   AssertNotNull(it->next());
   AssertFalse(it->hasNext());
   AssertNull(it->next());
   AssertFalse(it->hasNext());
   delete it;
   EndTest();

   StartTest(_T("HashMap - iterator remove"));
   it = hashMap->iterator();
   AssertTrue(it->hasNext());
   AssertNotNull(it->next());
   s = it->next();
   AssertNotNull(s);
   it->remove();
   AssertTrue(it->hasNext());
   AssertNotNull(it->next());
   AssertFalse(it->hasNext());
   AssertNull(it->next());
   delete it;
   AssertNotNull(hashMap->get(k1));
   AssertNull(hashMap->get(k2));
   AssertNotNull(hashMap->get(k3));
   EndTest();

   StartTest(_T("HashMap - remove"));
   hashMap->remove(k3);
   AssertNull(hashMap->get(k3));
   EndTest();

   StartTest(_T("HashMap - clear"));
   hashMap->clear();
   AssertEquals(hashMap->size(), 0);
   it = hashMap->iterator();
   AssertFalse(it->hasNext());
   AssertNull(it->next());
   delete it;
   EndTest();

   delete hashMap;

   StartTest(_T("HashMap - InetAddress as key"));

   InetAddress addr1 = InetAddress::parse("127.0.0.1");
   InetAddress addr2 = InetAddress::parse("10.0.0.76");
   InetAddress addr3 = InetAddress::parse("172.17.11.2");
   InetAddress addr4 = InetAddress::parse("fe80::250:56ff:fec0:8");

   HashMap<InetAddress, String> *ipAddrMap = new HashMap<InetAddress, String>(true);
   ipAddrMap->set(addr1, new String(_T("addr1")));
   ipAddrMap->set(addr2, new String(_T("addr2")));
   ipAddrMap->set(addr3, new String(_T("addr3")));
   ipAddrMap->set(addr4, new String(_T("addr4")));

   s = ipAddrMap->get(addr2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("addr2")));

   s = ipAddrMap->get(addr3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("addr3")));

   s = ipAddrMap->get(addr4);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("addr4")));

   ipAddrMap->set(addr3, new String(_T("addr3_replaced")));
   s = ipAddrMap->get(addr3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("addr3_replaced")));

   AssertEquals(ipAddrMap->size(), 4);

   delete ipAddrMap;
   EndTest();

   StartTest(_T("HashMap - long key"));

   LONG_HASH_KEY lk1 = "alpha";
   LONG_HASH_KEY lk2 = "omega";
   LONG_HASH_KEY lk3 = "some long key";
   LONG_HASH_KEY lk4 = "other key";

   HashMap<LONG_HASH_KEY, String> *longKeyMap = new HashMap<LONG_HASH_KEY, String>(true);
   longKeyMap->set(lk1, new String(_T("key1")));
   longKeyMap->set(lk2, new String(_T("key2")));
   longKeyMap->set(lk3, new String(_T("key3")));
   longKeyMap->set(lk4, new String(_T("key4")));

   s = longKeyMap->get(lk2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("key2")));

   s = longKeyMap->get(lk3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("key3")));

   s = longKeyMap->get(lk4);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("key4")));

   longKeyMap->set(lk3, new String(_T("key3_replaced")));
   s = longKeyMap->get(lk3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("key3_replaced")));

   AssertEquals(longKeyMap->size(), 4);

   delete longKeyMap;
   EndTest();
}

/**
 * Test shared hash map
 */
static void TestSharedHashMap()
{
   StartTest(_T("SharedHashMap: create"));
   SharedHashMap<HASH_KEY, String> *sharedHashMap = new SharedHashMap<HASH_KEY, String>(true);
   AssertEquals(sharedHashMap->size(), 0);
   EndTest();

   HASH_KEY k1 = { '1', '2', '3', '4', '5', '6' };
   HASH_KEY k2 = { '0', '0', 'a', 'b', 'c', 'd' };
   HASH_KEY k3 = { '0', '0', '3', 'X', '1', '1' };
   HASH_KEY k4 = { '1', '0', '3', 'X', '1', '1' };

   StartTest(_T("SharedHashMap: set/get"));

   sharedHashMap->set(k1, new String(_T("String 1")));
   sharedHashMap->set(k2, new String(_T("String 2")));
   sharedHashMap->set(k3, new String(_T("String 3")));
   AssertEquals(sharedHashMap->size(), 3);

   String *s = sharedHashMap->get(k1);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 1")));

   s = sharedHashMap->get(k2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 2")));

   s = sharedHashMap->get(k3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 3")));

   s = sharedHashMap->get(k4);
   AssertNull(s);

   EndTest();

   StartTest(_T("SharedHashMap: remove"));
   sharedHashMap->remove(k1);
   AssertEquals(sharedHashMap->size(), 2);
   s = sharedHashMap->get(k1);
   AssertNull(s);
   EndTest();

   StartTest(_T("SharedHashMap: get shared"));
   shared_ptr<String> shared = sharedHashMap->getShared(k2);
   AssertEquals(shared.use_count(), 2);
   delete sharedHashMap;
   AssertEquals(shared.use_count(), 1);
   AssertTrue(!_tcscmp(shared->getBuffer(), _T("String 2")));
   EndTest();
}

/**
 * Test shared hash map
 */
static void TestSynchronizedSharedHashMap()
{
   StartTest(_T("SynchronizedSharedHashMap: create"));
   SynchronizedSharedHashMap<HASH_KEY, String> *hashMap = new SynchronizedSharedHashMap<HASH_KEY, String>(true);
   AssertEquals(hashMap->size(), 0);
   EndTest();

   HASH_KEY k1 = { '1', '2', '3', '4', '5', '6' };
   HASH_KEY k2 = { '0', '0', 'a', 'b', 'c', 'd' };
   HASH_KEY k3 = { '0', '0', '3', 'X', '1', '1' };
   HASH_KEY k4 = { '1', '0', '3', 'X', '1', '1' };

   StartTest(_T("SynchronizedSharedHashMap: set/get"));

   hashMap->set(k1, new String(_T("String 1")));
   hashMap->set(k2, new String(_T("String 2")));
   hashMap->set(k3, new String(_T("String 3")));
   AssertEquals(hashMap->size(), 3);

   shared_ptr<String> s = hashMap->getShared(k1);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 1")));

   s = hashMap->getShared(k2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 2")));

   s = hashMap->getShared(k3);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("String 3")));

   s = hashMap->getShared(k4);
   AssertNull(s);

   EndTest();

   StartTest(_T("SynchronizedSharedHashMap: remove"));
   hashMap->remove(k1);
   AssertEquals(hashMap->size(), 2);
   s = hashMap->getShared(k1);
   AssertNull(s);
   EndTest();

   StartTest(_T("SynchronizedSharedHashMap: get shared"));
   shared_ptr<String> shared = hashMap->getShared(k2);
   AssertEquals(shared.use_count(), 2);
   delete hashMap;
   AssertEquals(shared.use_count(), 1);
   AssertTrue(!_tcscmp(shared->getBuffer(), _T("String 2")));
   EndTest();
}

/**
 * Test hash set
 */
static void TestHashSet()
{
   StartTest(_T("HashSet - create"));
   HashSet<INT32> *s1 = new HashSet<INT32>();
   AssertEquals(s1->size(), 0);
   EndTest();

   StartTest(_T("HashSet - put/contains"));
   s1->put(1);
   s1->put(10);
   s1->put(33);
   s1->put(10);
   AssertEquals(s1->size(), 3);
   AssertTrue(s1->contains(1));
   AssertTrue(s1->contains(10));
   AssertTrue(s1->contains(33));
   AssertFalse(s1->contains(12));
   EndTest();

   StartTest(_T("HashSet - remove"));
   s1->remove(25);
   s1->remove(33);
   AssertEquals(s1->size(), 2);
   AssertTrue(s1->contains(10));
   AssertFalse(s1->contains(33));
   EndTest();

   StartTest(_T("HashSet - clear"));
   s1->clear();
   AssertEquals(s1->size(), 0);
   AssertFalse(s1->contains(1));
   EndTest();

   delete s1;

   LONG_HASH_KEY k1 = "alpha";
   LONG_HASH_KEY k2 = "omega";
   LONG_HASH_KEY k3 = "some long key";
   LONG_HASH_KEY k4 = "unused key";

   StartTest(_T("HashSet (long key) - create"));
   HashSet<LONG_HASH_KEY> *s2 = new HashSet<LONG_HASH_KEY>();
   AssertEquals(s2->size(), 0);
   EndTest();

   StartTest(_T("HashSet (long key) - put/contains"));
   s2->put(k1);
   s2->put(k2);
   s2->put(k3);
   s2->put(k2);
   AssertEquals(s2->size(), 3);
   AssertTrue(s2->contains(k1));
   AssertTrue(s2->contains(k2));
   AssertTrue(s2->contains(k3));
   AssertFalse(s2->contains(k4));
   EndTest();

   StartTest(_T("HashSet (long key) - remove"));
   s2->remove(k2);
   s2->remove(k4);
   AssertEquals(s2->size(), 2);
   AssertTrue(s2->contains(k1));
   AssertFalse(s2->contains(k2));
   EndTest();

   StartTest(_T("HashSet (long key) - clear"));
   s2->clear();
   AssertEquals(s2->size(), 0);
   AssertFalse(s2->contains(k1));
   EndTest();

   delete s2;
}

/**
 * Test object array
 */
static void TestObjectArray()
{
   StartTest(_T("ObjectArray: create"));
   ObjectArray<String> *array = new ObjectArray<String>(16, 16, true);
   AssertEquals(array->size(), 0);
   EndTest();

   StartTest(_T("ObjectArray: add/get"));
   array->add(new String(_T("value 1")));
   array->add(new String(_T("value 2")));
   array->add(new String(_T("value 3")));
   array->add(new String(_T("value 4")));
   array->add(new String(_T("value 5")));
   AssertEquals(array->size(), 5);
   AssertNull(array->get(5));
   AssertNotNull(array->get(1));
   AssertTrue(!_tcscmp(array->get(1)->getBuffer(), _T("value 2")));
   EndTest();

   StartTest(_T("ObjectArray: replace"));
   array->replace(0, new String(_T("replace")));
   AssertEquals(array->size(), 5);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("replace")));
   EndTest();

   StartTest(_T("ObjectArray: remove"));
   array->remove(0);
   AssertEquals(array->size(), 4);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("value 2")));
   array->remove(3);
   AssertEquals(array->size(), 3);
   AssertTrue(!_tcscmp(array->get(2)->getBuffer(), _T("value 4")));
   EndTest();

   StartTest(_T("ObjectArray: iterator"));
   Iterator<String> *it = array->iterator();
   AssertTrue(it->hasNext());
   String *s = it->next();
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 2")));
   s = it->next();
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 3")));
   s = it->next();
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 4")));
   s = it->next();
   AssertNull(s);
   delete it;
   EndTest();

   StartTest(_T("ObjectArray: remove with iterator"));
   it = array->iterator();
   AssertTrue(it->hasNext());
   while(it->hasNext())
   {
      String *s = it->next();
      if (!_tcscmp(s->getBuffer(), _T("value 4")))
      {
         it->remove();
      }
   }
   delete it;
   AssertEquals(array->size(), 2);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("value 2")));
   AssertTrue(!_tcscmp(array->get(1)->getBuffer(), _T("value 3")));
   EndTest();

   delete array;
}

/**
 * Test shared object array
 */
static void TestSharedObjectArray()
{
   StartTest(_T("SharedObjectArray: create"));
   SharedObjectArray<String> *array = new SharedObjectArray<String>(16, 16);
   AssertEquals(array->size(), 0);
   EndTest();

   StartTest(_T("SharedObjectArray: add/get"));
   array->add(new String(_T("value 1")));
   array->add(new String(_T("value 2")));
   array->add(new String(_T("value 3")));
   array->add(new String(_T("value 4")));
   array->add(new String(_T("value 5")));
   AssertEquals(array->size(), 5);
   AssertNull(array->get(5));
   AssertNotNull(array->get(1));
   AssertTrue(!_tcscmp(array->get(1)->getBuffer(), _T("value 2")));
   EndTest();

   StartTest(_T("SharedObjectArray: replace"));
   array->replace(0, new String(_T("replace")));
   AssertEquals(array->size(), 5);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("replace")));
   EndTest();

   StartTest(_T("SharedObjectArray: remove"));
   array->remove(0);
   AssertEquals(array->size(), 4);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("value 2")));
   array->remove(3);
   AssertEquals(array->size(), 3);
   AssertTrue(!_tcscmp(array->get(2)->getBuffer(), _T("value 4")));
   EndTest();

   /*
   StartTest(_T("SharedObjectArray: iterator"));
   Iterator<String> *it = array->iterator();
   AssertTrue(it->hasNext());
   String *s = it->next();
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 2")));
   s = it->next();
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 3")));
   s = it->next();
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 4")));
   s = it->next();
   AssertNull(s);
   delete it;
   EndTest();

   StartTest(_T("SharedObjectArray: remove with iterator"));
   it = array->iterator();
   AssertTrue(it->hasNext());
   while(it->hasNext())
   {
      String *s = it->next();
      if (!_tcscmp(s->getBuffer(), _T("value 4")))
      {
         it->remove();
      }
   }
   delete it;
   AssertEquals(array->size(), 2);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("value 2")));
   AssertTrue(!_tcscmp(array->get(1)->getBuffer(), _T("value 3")));
   EndTest();
   */

   StartTest(_T("SharedObjectArray: get shared"));
   shared_ptr<String> s = array->getShared(0);
   AssertEquals(s.use_count(), 2);
   delete array;
   AssertEquals(s.use_count(), 1);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("value 2")));
   EndTest();
}

/**
 * Table tests
 */
static void TestTable()
{
   StartTest(_T("Table: create"));
   Table *table = new Table();
   AssertEquals(table->getNumRows(), 0);
   EndTest();

   StartTest(_T("Table: set on empty table"));
   table->set(0, 1.0);
   table->set(1, _T("test"));
   table->setPreallocated(1, _tcsdup(_T("test")));
   AssertEquals(table->getNumRows(), 0);
   EndTest();

   StartTest(_T("Table: add row"));
   table->addRow();
   AssertEquals(table->getNumRows(), 1);
   AssertEquals(table->getNumColumns(), 0);
   EndTest();

   StartTest(_T("Table: set on empty row"));
   table->set(0, _T("test"));
   table->setPreallocated(1, _tcsdup(_T("test")));
   AssertEquals(table->getNumRows(), 1);
   AssertEquals(table->getNumColumns(), 0);
   EndTest();

   table->addColumn(_T("NAME"));
   table->addColumn(_T("VALUE"));
   table->addColumn(_T("DATA1"));
   table->addColumn(_T("DATA2"));
   table->addColumn(_T("DATA3"));
   table->addColumn(_T("DATA4"));
   for(int i = 0; i < 50; i++)
   {
      table->addRow();
      TCHAR b[64];
      _sntprintf(b, 64, _T("Process #%d"), i);
      table->set(0, b);
      table->set(1, i);
      table->set(2, i * 100);
      table->set(3, i * 100001);
      table->set(4, _T("/some/long/path/on/file/system"));
      table->set(5, _T("constant"));
   }

   StartTest(_T("Table: pack"));
   INT64 start = GetCurrentTimeMs();
   char *packedTable = table->createPackedXML();
   AssertNotNull(packedTable);
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Table: unpack"));
   start = GetCurrentTimeMs();
   Table *table2 = Table::createFromPackedXML(packedTable);
   free(packedTable);
   AssertNotNull(table2);
   AssertEquals(table2->getNumColumns(), table->getNumColumns());
   AssertEquals(table2->getNumRows(), table->getNumRows());
   AssertEquals(table2->getAsInt(10, 1), table->getAsInt(10, 1));
   AssertTrue(!_tcscmp(table2->getAsString(15, 0), table->getAsString(15, 0)));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Table: merge"));
   Table *table3 = new Table();
   table3->addColumn(_T("NAME"));
   table3->addColumn(_T("VALUE"));
   table3->addColumn(_T("DATA5"));

   table3->addRow();
   table3->set(0, _T("T3R1"));
   table3->set(1, 101);
   table3->set(2, _T("Data5-1"));

   table3->addRow();
   table3->set(0, _T("T3R2"));
   table3->set(1, 102);
   table3->set(2, _T("Data5-2"));

   table->merge(table3);
   AssertEquals(table->getNumRows(), 53);
   AssertEquals(table->getNumColumns(), 7);
   AssertNotEquals(table->getColumnIndex(_T("DATA5")), -1);
   AssertTrue(!_tcscmp(table->getAsString(52, table->getColumnIndex(_T("DATA5")), _T("")), _T("Data5-2")));
   EndTest();

   StartTest(_T("Table: merge row"));
   Table *table4 = new Table();
   table4->addColumn(_T("NAME"));
   table4->addColumn(_T("DATA6"));

   table4->addRow();
   table4->set(0, _T("T4R1"));
   table4->set(1, _T("Data6-1"));

   table->mergeRow(table4, 0);
   table->mergeRow(table4, 1);
   AssertEquals(table->getNumRows(), 54);
   AssertEquals(table->getNumColumns(), 8);
   AssertNotEquals(table->getColumnIndex(_T("DATA6")), -1);
   AssertTrue(!_tcscmp(table->getAsString(53, table->getColumnIndex(_T("DATA6")), _T("")), _T("Data6-1")));
   EndTest();

   delete table;
   delete table2;
   delete table3;
   delete table4;
}

/**
 * Test byte swap
 */
static void TestByteSwap()
{
   StartTest(_T("bswap_16"));
   AssertEquals(bswap_16(0xABCD), 0xCDAB);
   EndTest();

   StartTest(_T("bswap_32"));
   AssertEquals(bswap_32(0x0102ABCD), 0xCDAB0201);
   EndTest();

   StartTest(_T("bswap_64"));
   AssertEquals(bswap_64(_ULL(0x01020304A1A2A3A4)), _ULL(0xA4A3A2A104030201));
   EndTest();

   StartTest(_T("bswap_array_16"));
   UINT16 s16[] = { 0xABCD, 0x1020, 0x2233, 0x0102 };
   UINT16 d16[] = { 0xCDAB, 0x2010, 0x3322, 0x0201 };
   bswap_array_16(s16, 4);
   AssertTrue(!memcmp(s16, d16, 8));
   EndTest();

   StartTest(_T("bswap_array_16 (string)"));
   UINT16 ss16[] = { 0xABCD, 0x1020, 0x2233, 0x0102, 0 };
   UINT16 sd16[] = { 0xCDAB, 0x2010, 0x3322, 0x0201, 0 };
   bswap_array_16(ss16, -1);
   AssertTrue(!memcmp(ss16, sd16, 10));
   EndTest();

   StartTest(_T("bswap_array_32"));
   UINT32 s32[] = { 0xABCDEF01, 0x10203040, 0x22334455, 0x01020304 };
   UINT32 d32[] = { 0x01EFCDAB, 0x40302010, 0x55443322, 0x04030201 };
   bswap_array_32(s32, 4);
   AssertTrue(!memcmp(s32, d32, 16));
   EndTest();

   StartTest(_T("bswap_array_32 (string)"));
   UINT32 ss32[] = { 0xABCDEF01, 0x10203040, 0x22334455, 0x01020304, 0 };
   UINT32 sd32[] = { 0x01EFCDAB, 0x40302010, 0x55443322, 0x04030201, 0 };
   bswap_array_32(ss32, -1);
   AssertTrue(!memcmp(ss32, sd32, 20));
   EndTest();
}

/**
 * Test diff
 */
static void TestDiff()
{
   static const TCHAR *diffLeft = _T("line 1\nline 2\nline3\nFIXED TEXT\nalpha\n");
   static const TCHAR *diffRight = _T("line 1\nline 3\nline 4\nFIXED TEXT\nbeta\n");
   static const TCHAR *expectedDiff = _T("-line 2\n-line3\n+line 3\n+line 4\n-alpha\n+beta\n");

   StartTest(_T("GenerateLineDiff (multiple lines)"));
   String diff = GenerateLineDiff(diffLeft, diffRight);
   AssertTrue(diff.equals(expectedDiff));
   EndTest();

   StartTest(_T("GenerateLineDiff (single line)"));
   diff = GenerateLineDiff(_T("prefix-alpha"), _T("prefix-beta"));
   AssertTrue(diff.equals(_T("-prefix-alpha\n+prefix-beta\n")));
   EndTest();
}

/**
 * Test ring buffer
 */
static void TestRingBuffer()
{
   RingBuffer rb(32, 32);
   BYTE buffer[256];

   StartTest(_T("RingBuffer: write #1"));
   rb.write((const BYTE *)"short data", 10);
   AssertEquals(rb.size(), 10);
   EndTest();

   StartTest(_T("RingBuffer: read #1"));
   size_t bytes = rb.read(buffer, 256);
   AssertEquals(bytes, 10);
   AssertTrue(!memcmp(buffer, "short data", 10));
   AssertEquals(rb.size(), 0);
   EndTest();

   StartTest(_T("RingBuffer: write #2"));
   rb.write((const BYTE *)"short data", 10);
   AssertEquals(rb.size(), 10);
   EndTest();

   StartTest(_T("RingBuffer: read #2"));
   memset(buffer, 0, 256);
   bytes = rb.read(buffer, 4);
   AssertEquals(bytes, 4);
   AssertTrue(!memcmp(buffer, "shor", 4));
   AssertEquals(rb.size(), 6);
   EndTest();

   StartTest(_T("RingBuffer: write #3"));
   rb.write((const BYTE *)"long data: 123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.", 111);
   AssertEquals(rb.size(), 117);
   EndTest();

   StartTest(_T("RingBuffer: read #3"));
   memset(buffer, 0, 256);
   bytes = rb.read(buffer, 17);
   AssertEquals(bytes, 17);
   AssertTrue(!memcmp(buffer, "t datalong data: ", 17));
   AssertEquals(rb.size(), 100);
   EndTest();

   StartTest(_T("RingBuffer: write #4"));
   rb.write((const BYTE *)"short data", 10);
   AssertEquals(rb.size(), 110);
   EndTest();

   StartTest(_T("RingBuffer: read #4"));
   memset(buffer, 0, 256);
   bytes = rb.read(buffer, 108);
   AssertEquals(bytes, 108);
   AssertTrue(!memcmp(buffer, "123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.123456789.short da", 108));
   AssertEquals(rb.size(), 2);
   EndTest();

   StartTest(_T("RingBuffer: write #5"));
   rb.write((const BYTE *)"test", 4);
   AssertEquals(rb.size(), 6);
   EndTest();

   StartTest(_T("RingBuffer: read #5"));
   memset(buffer, 0, 256);
   bytes = rb.read(buffer, 256);
   AssertEquals(bytes, 6);
   AssertTrue(!memcmp(buffer, "tatest", 6));
   AssertEquals(rb.size(), 0);
   EndTest();

   StartTest(_T("RingBuffer: random read/write"));
   AssertTrue(rb.isEmpty());
   for(int i = 0; i < 10000; i++)
   {
      int writes = rand() % 10;
      int reads = rand() % 10;
      for(int j = 0; j < writes; j++)
      {
         rb.write((const BYTE *)"---------/---------/---------/---------/---------/---------/---------/---------/---------/---------/", 100);
      }
      for(int j = 0; j < reads; j++)
      {
         memset(buffer, 0, 256);
         bytes = rb.read(buffer, 100);
         AssertTrue(((bytes == 100) && !memcmp(buffer, "---------/---------/---------/---------/---------/---------/---------/---------/---------/---------/", 100)) || (bytes == 0));
      }
   }
   while(!rb.isEmpty())
   {
      memset(buffer, 0, 256);
      bytes = rb.read(buffer, 100);
      AssertTrue((bytes == 100) && !memcmp(buffer, "---------/---------/---------/---------/---------/---------/---------/---------/---------/---------/", 100));
   }
   EndTest();
}

/**
 * Test get/set debug level
 */
static void TestDebugLevel()
{
   StartTest(_T("Default debug level"));
   AssertEquals(nxlog_get_debug_level(), 0);
   EndTest();

   StartTest(_T("Set debug level"));
   nxlog_set_debug_level(7);
   AssertEquals(nxlog_get_debug_level(), 7);
   nxlog_set_debug_level(0);
   AssertEquals(nxlog_get_debug_level(), 0);
   EndTest();

   StartTest(_T("nxlog_get_debug_level() performance"));
   UINT64 startTime = GetCurrentTimeMs();
   for(int i = 0; i < 1000000; i++)
      nxlog_get_debug_level();
   EndTest(GetCurrentTimeMs() - startTime);
}

/**
 * Test debug tags
 */
static void TestDebugTags()
{
   StartTest(_T("Default debug level for tags"));
   AssertEquals(nxlog_get_debug_level_tag(_T("server.db")), 0);
   EndTest();

   StartTest(_T("Debug tags: set debug level"));
   nxlog_set_debug_level_tag(_T("*"), 1);
   nxlog_set_debug_level_tag(_T("db"), 2);
   nxlog_set_debug_level_tag(_T("db.*"), 3);
   nxlog_set_debug_level_tag(_T("db.local"), 4);
   nxlog_set_debug_level_tag(_T("db.local.*"), 5);
   nxlog_set_debug_level_tag(_T("db.local.sql"), 6);
   nxlog_set_debug_level_tag(_T("db.local.sql.*"), 7);
   nxlog_set_debug_level_tag(_T("db.local.sql.testing"), 8);
   nxlog_set_debug_level_tag(_T("db.local.sql.testing.*"), 9);
   nxlog_set_debug_level_tag(_T("server.objects.lock"), 9);

   AssertEquals(nxlog_get_debug_level_tag(_T("*")), nxlog_get_debug_level());

   AssertEquals(nxlog_get_debug_level_tag(_T("server.objects.db")), 1);

   AssertEquals(nxlog_get_debug_level_tag(_T("node")), 1);
   AssertEquals(nxlog_get_debug_level_tag(_T("node.status")), 1);
   AssertEquals(nxlog_get_debug_level_tag(_T("node.status.test")), 1);

   AssertEquals(nxlog_get_debug_level_tag(_T("db")), 2);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.status")), 3);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.status.test")), 3);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local")), 4);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.server")), 5);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.server.test")), 5);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql")), 6);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.server")), 7);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.server.status")), 7);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing")), 8);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing.server")), 9);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing.server.status")), 9);
   EndTest();

   StartTest(_T("Debug tags: get all tags"));
   ObjectArray<DebugTagInfo> *tags = nxlog_get_all_debug_tags();
   AssertNotNull(tags);
   AssertEquals(tags->size(), 9);
   AssertTrue(!_tcscmp(tags->get(0)->tag, _T("db")));
   AssertTrue(!_tcscmp(tags->get(1)->tag, _T("db.*")));
   delete tags;
   EndTest();

   StartTest(_T("Debug tags: change debug level"));
   nxlog_set_debug_level_tag(_T("*"), 9);
   nxlog_set_debug_level_tag(_T("db"), 8);
   nxlog_set_debug_level_tag(_T("db.*"), 7);
   nxlog_set_debug_level_tag(_T("db.local"), 6);
   nxlog_set_debug_level_tag(_T("db.local.*"), 5);
   nxlog_set_debug_level_tag(_T("db.local.sql"), 4);
   nxlog_set_debug_level_tag(_T("db.local.sql.*"), 3);
   nxlog_set_debug_level_tag(_T("db.local.sql.testing"), 2);
   nxlog_set_debug_level_tag(_T("db.local.sql.testing.*"), 1);

   AssertEquals(nxlog_get_debug_level(), 9);
   AssertEquals(nxlog_get_debug_level_tag(_T("*")), nxlog_get_debug_level());

   AssertEquals(nxlog_get_debug_level_tag(_T("node")), 9);
   AssertEquals(nxlog_get_debug_level_tag(_T("node.status")), 9);
   AssertEquals(nxlog_get_debug_level_tag(_T("node.status.test")), 9);

   AssertEquals(nxlog_get_debug_level_tag(_T("db")), 8);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.status")), 7);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.status.test")), 7);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local")), 6);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.server")), 5);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.server.test")), 5);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql")), 4);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.server")), 3);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.server.status")), 3);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing")), 2);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing.server")), 1);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing.server.status")), 1);
   EndTest();

   StartTest(_T("Debug tags: remove"));
   nxlog_set_debug_level_tag(_T("db.test.remove.*"), 4);
   nxlog_set_debug_level_tag(_T("db.test.remove.child.tag"), 3);
   nxlog_set_debug_level_tag(_T("db.local.child"), 2);
   nxlog_set_debug_level_tag(_T("db.local.child.*"), 5);

   nxlog_set_debug_level_tag(_T("db"), -1);
   nxlog_set_debug_level_tag(_T("db.*"), -1);
   nxlog_set_debug_level_tag(_T("db.local"), -1);
   nxlog_set_debug_level_tag(_T("db.local.*"), -1);
   nxlog_set_debug_level_tag(_T("db.local.sql"), -1);
   nxlog_set_debug_level_tag(_T("db.local.sql.*"), -1);;

   nxlog_set_debug_level_tag(_T("server"), 8);
   nxlog_set_debug_level_tag(_T("server.*"), 7);
   nxlog_set_debug_level_tag(_T("server.node"), 6);
   nxlog_set_debug_level_tag(_T("server.node.*"), 5);
   nxlog_set_debug_level_tag(_T("server.node.status"), 4);
   nxlog_set_debug_level_tag(_T("server.node.status.*"), 3);
   nxlog_set_debug_level_tag(_T("server.node.status.poll"), 2);
   nxlog_set_debug_level_tag(_T("server.node.status.poll.*"), 1);

   AssertEquals(nxlog_get_debug_level_tag(_T("node")), 9);
   AssertEquals(nxlog_get_debug_level_tag(_T("node.status")), 9);
   AssertEquals(nxlog_get_debug_level_tag(_T("node.status.test")), 9);

   AssertEquals(nxlog_get_debug_level_tag(_T("server")), 8);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.object")), 7);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.object.add")), 7);

   AssertEquals(nxlog_get_debug_level_tag(_T("server.node")), 6);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.add")), 5);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.add.new")), 5);

   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.status")), 4);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.status.interface")), 3);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.status.interface.state")), 3);

   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.status.poll")), 2);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.status.poll.time")), 1);
   AssertEquals(nxlog_get_debug_level_tag(_T("server.node.status.poll.time.ms")), 1);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing")), 2);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing.server")), 1);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.testing.server.status")), 1);

   AssertEquals(nxlog_get_debug_level_tag(_T("db.test.remove.something")), 4);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.test.remove.child.tag")), 3);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.child")), 2);
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.child.remove.something.else")), 5);

   AssertEquals(nxlog_get_debug_level_tag(_T("db")), nxlog_get_debug_level());
   AssertEquals(nxlog_get_debug_level_tag(_T("db.status")), nxlog_get_debug_level());
   AssertEquals(nxlog_get_debug_level_tag(_T("db.status.test")), nxlog_get_debug_level());

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local")), nxlog_get_debug_level());
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.server")), nxlog_get_debug_level());
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.server.test")), nxlog_get_debug_level());

   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql")), nxlog_get_debug_level());
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.server")), nxlog_get_debug_level());
   AssertEquals(nxlog_get_debug_level_tag(_T("db.local.sql.server.status")), nxlog_get_debug_level());
   EndTest();

   StartTest(_T("nxlog_get_debug_level performance with multiple tags"));
   TCHAR tag[64];
   for(int i = 0; i < 1000; i++)
   {
      _sntprintf(tag, 64, _T("test.tag%d.subtag%d"), i % 10, i);
      nxlog_set_debug_level_tag(tag, 7);
   }
   UINT64 startTime = GetCurrentTimeMs();
   for(int i = 0; i < 1000000; i++)
      nxlog_get_debug_level();
   EndTest(GetCurrentTimeMs() - startTime);

   StartTest(_T("Debug tags: reset"));
   nxlog_reset_debug_level_tags();
   AssertEquals(nxlog_get_debug_level_tag(_T("test.tag1.subtag1")), nxlog_get_debug_level());
   EndTest();
}

/**
 * Debug writer for logger
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != NULL)
      _tprintf(_T("[DEBUG/%-20s] "), tag);
   else
      _tprintf(_T("[DEBUG%-21s] "), _T(""));
   _vtprintf(format, args);
   _fputtc(_T('\n'), stdout);
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);
   if (argc > 1)
   {
      if (!strcmp(argv[1], "@proc"))
      {
         TestProcessExecutorWorker();
         return 0;
      }
      else if (!strcmp(argv[1], "@subproc"))
      {
         SubProcessMain(argc, argv, TestSubProcessRequestHandler);
         return 0;
      }
      else if (!strcmp(argv[1], "-debug"))
      {
         nxlog_set_debug_writer(DebugWriter);
      }
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   TestMemoryPool();
   TestObjectMemoryPool();
   TestString();
   TestStringConversion();
   TestStringList();
   TestStringMap();
   TestStringSet();
   TestStringFunctionsA();
   TestStringFunctionsW();
   TestPatternMatching();
   TestMessageClass();
   TestMsgWaitQueue();
   TestMacAddress();
   TestInetAddress();
   TestItoa();
   TestQueue();
   TestHashMap();
   TestSharedHashMap();
   TestSynchronizedSharedHashMap();
   TestHashSet();
   TestObjectArray();
   TestSharedObjectArray();
   TestTable();
   TestMutex();
   TestMutexWrapper();
   TestRWLockWrapper();
   TestConditionWrapper();
   TestByteSwap();
   TestDiff();
   TestRingBuffer();
   TestDebugLevel();
   TestDebugTags();
   TestProcessExecutor(argv[0]);
   TestSubProcess(argv[0]);
   TestThreadPool();
   TestThreadCountAndMaxWaitTime();
   return 0;
}
