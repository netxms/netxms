#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>

static char mbText[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static WCHAR wcText[] = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static char mbTextShort[] = "Lorem ipsum";
static UCS2CHAR ucs2TextShort[] = { 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm', 0 };
static UCS4CHAR ucs4TextShort[] = { 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm', 0 };
#if UNICODE_UCS4
#define wcTextShort ucs4TextShort
#else
#define wcTextShort ucs2TextShort
#endif
static UCS2CHAR ucs2TextSurrogates[] = { 'L', 'o', 'r', 'e', 'm', 0xD801, 0xDFFF, 'i', 'p', 's', 'u', 'm', 0x1CD, '.', 0 };
static UCS4CHAR ucs4TextSurrogates[] = { 'L', 'o', 'r', 'e', 'm', 0x0107FF, 'i', 'p', 's', 'u', 'm', 0x1CD, '.', 0 };
static char utf8TextSurrogates[] = { 'L', 'o', 'r', 'e', 'm', (char)0xF0, (char)0x90, (char)0x9F, (char)0xBF, 'i', 'p', 's', 'u', 'm', (char)0xC7, (char)0x8D, '.', 0 };
static char utf8InvalidSequence[] = { 'A', (char)0xED, (char)0xBD, (char)0xB6, 'x', 0 };
#if defined(_WIN32) || defined(__IBMCPP__) || defined(__SUNPRO_CC)
static WCHAR wcTextISO8859_1[] = L"Lorem ipsum dolor sit amet, \xA3 10, \xA2 20, \xA5 50, b\xF8nne";
#else
static WCHAR wcTextISO8859_1[] = L"Lorem ipsum dolor sit amet, £ 10, ¢ 20, ¥ 50, bønne";
#endif
static char mbTextISO8859_1[] = "Lorem ipsum dolor sit amet, \xA3 10, \xA2 20, \xA5 50, b\xF8nne";
static char mbTextASCII[] = "Lorem ipsum dolor sit amet, ? 10, ? 20, ? 50, b?nne";

/**
 * Test string conversion
 */
void TestStringConversion()
{
   StartTest(_T("mb_to_wchar"));
   WCHAR wcBuffer[1024];
   size_t len = mb_to_wchar(mbTextShort, -1, wcBuffer, 1024);
   AssertEquals(len, 12);
   AssertTrue(!memcmp(wcBuffer, wcTextShort, sizeof(WCHAR) * 12));
   memset(wcBuffer, 0x7F, sizeof(wcBuffer));
   len = mb_to_wchar(mbTextShort, 11, wcBuffer, 1024);
   AssertEquals(len, 11);
   AssertTrue(!memcmp(wcBuffer, wcTextShort, sizeof(WCHAR) * 11));
#ifdef UNICODE_UCS4
   AssertTrue(wcBuffer[11] == 0x7F7F7F7F);
#else
   AssertTrue(wcBuffer[11] == 0x7F7F);
#endif
   EndTest();

   StartTest(_T("wchar_to_mb"));
   char mbBuffer[1024];
   len = wchar_to_mb(wcTextShort, -1, mbBuffer, 1024);
   AssertEquals(len, 12);
   AssertTrue(!strcmp(mbBuffer, mbTextShort));
   memset(mbBuffer, 0x7F, sizeof(mbBuffer));
   len = wchar_to_mb(wcTextShort, 11, mbBuffer, 1024);
   AssertEquals(len, 11);
   AssertTrue(!memcmp(mbBuffer, mbTextShort, 11));
   AssertTrue(mbBuffer[11] == 0x7F);
   EndTest();

   StartTest(_T("mb_to_ucs2"));
   UCS2CHAR ucs2buffer[1024];
   len = mb_to_ucs2(mbTextShort, -1, ucs2buffer, 1024);
   AssertEquals(len, 12);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextShort, sizeof(UCS2CHAR) * 12));
   memset(ucs2buffer, 0x7F, sizeof(ucs2buffer));
   len = mb_to_ucs2(mbTextShort, 11, ucs2buffer, 1024);
   AssertEquals(len, 11);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextShort, sizeof(UCS2CHAR) * 11));
   AssertTrue(ucs2buffer[11] == 0x7F7F);
   EndTest();

   StartTest(_T("ucs2_to_mb"));
   len = ucs2_to_mb(ucs2TextShort, -1, mbBuffer, 1024);
   AssertEquals(len, 12);
   AssertTrue(!strcmp(mbBuffer, mbTextShort));
   memset(mbBuffer, 0x7F, sizeof(mbBuffer));
   len = ucs2_to_mb(ucs2TextShort, 11, mbBuffer, 1024);
   AssertEquals(len, 11);
   AssertTrue(!memcmp(mbBuffer, mbTextShort, 11));
   AssertTrue(mbBuffer[11] == 0x7F);
   EndTest();

   UCS4CHAR ucs4buffer[1024];

#if !WITH_ADDRESS_SANITIZER
   StartTest(_T("Multibyte to UCS-2 conversion performance"));
   int64_t start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS2CHAR buffer[1024];
      mb_to_ucs2(mbText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UCS-2 to multibyte conversion performance"));
   mb_to_ucs2(mbText, -1, ucs2buffer, 1024);
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      char buffer[1024];
      ucs2_to_mb(ucs2buffer, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Multibyte to UCS-4 conversion performance"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS4CHAR buffer[1024];
      mb_to_ucs4(mbText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UCS-4 to multibyte conversion performance"));
   mb_to_ucs4(mbText, -1, ucs4buffer, 1024);
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      char buffer[1024];
      ucs4_to_mb(ucs4buffer, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);
#endif   /* !WITH_ADDRESS_SANITIZER */

   StartTest(_T("ucs2_to_ucs4"));
   len = ucs2_to_ucs4(ucs2TextSurrogates, -1, ucs4buffer, 128);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, len * sizeof(UCS4CHAR)));
   len = ucs2_to_ucs4(ucs2TextSurrogates, -1, ucs4buffer, 14);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, len * sizeof(UCS4CHAR)));
   len = ucs2_to_ucs4(ucs2TextSurrogates, -1, ucs4buffer, 10);
   AssertEquals(len, 10);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, 9 * sizeof(UCS4CHAR)));
   AssertEquals(static_cast<uint32_t>(ucs4buffer[9]), 0u);
   memset(ucs4buffer, 0x7F, sizeof(ucs4buffer));
   len = ucs2_to_ucs4(ucs2TextSurrogates, 14, ucs4buffer, 128);
   AssertEquals(len, 13);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, len * sizeof(UCS4CHAR)));
   AssertTrue(ucs4buffer[13] == 0x7F7F7F7F);
   EndTest();

   StartTest(_T("ucs4_to_ucs2"));
   len = ucs4_to_ucs2(ucs4TextSurrogates, -1, ucs2buffer, 1024);
   AssertEquals(len, 15);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, len * sizeof(UCS2CHAR)));
   len = ucs4_to_ucs2(ucs4TextSurrogates, -1, ucs2buffer, 15);
   AssertEquals(len, 15);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, len * sizeof(UCS2CHAR)));
   len = ucs4_to_ucs2(ucs4TextSurrogates, -1, ucs2buffer, 10);
   AssertEquals(len, 10);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, 9 * sizeof(UCS2CHAR)));
   AssertEquals(ucs2buffer[9], 0);
   memset(ucs2buffer, 0x7F, sizeof(ucs2buffer));
   len = ucs4_to_ucs2(ucs4TextSurrogates, 13, ucs2buffer, 1024);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, len * sizeof(UCS2CHAR)));
   EndTest();

#if !WITH_ADDRESS_SANITIZER
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
#endif

   StartTest(_T("ucs2_utf8len"));
   AssertEquals(ucs2_utf8len(ucs2TextSurrogates, -1), 18);  // with trailing 0
   AssertEquals(ucs2_utf8len(ucs2TextSurrogates, ucs2_strlen(ucs2TextSurrogates)), 17);  // without trailing 0
   EndTest();

   StartTest(_T("utf8_ucs2len"));
   AssertEquals(utf8_ucs2len(utf8TextSurrogates, -1), 15);  // with trailing 0
   AssertEquals(utf8_ucs2len(utf8TextSurrogates, strlen(utf8TextSurrogates)), 14);  // without trailing 0
   EndTest();

   StartTest(_T("ucs2_to_utf8"));
   char utf8buffer[128];
   len = ucs2_to_utf8(ucs2TextSurrogates, -1, utf8buffer, 128);
   AssertEquals(len, 18);
   AssertTrue(!strcmp(utf8buffer, utf8TextSurrogates));
   memset(utf8buffer, 0x7F, sizeof(utf8buffer));
   len = ucs2_to_utf8(ucs2TextSurrogates, 14, utf8buffer, 128);
   AssertEquals(len, 17);
   AssertTrue(!memcmp(utf8buffer, utf8TextSurrogates, len));
   AssertTrue(utf8buffer[len] == 0x7F);
   EndTest();

   StartTest(_T("utf8_to_ucs2"));
   len = utf8_to_ucs2(utf8TextSurrogates, -1, ucs2buffer, 1024);
   AssertEquals(len, 15);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, len * sizeof(UCS2CHAR)));
   len = utf8_to_ucs2(utf8TextSurrogates, -1, ucs2buffer, 15);
   AssertEquals(len, 15);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, len * sizeof(UCS2CHAR)));
   len = utf8_to_ucs2(utf8TextSurrogates, -1, ucs2buffer, 10);
   AssertEquals(len, 10);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, 9 * sizeof(UCS2CHAR)));
   AssertEquals(ucs2buffer[9], 0);
   memset(ucs2buffer, 0x7F, sizeof(ucs2buffer));
   len = utf8_to_ucs2(utf8TextSurrogates, strlen(utf8TextSurrogates), ucs2buffer, 1024);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextSurrogates, len * sizeof(UCS2CHAR)));
   AssertTrue(ucs2buffer[len] == 0x7F7F);
   len = utf8_to_ucs2(utf8InvalidSequence, strlen(utf8InvalidSequence), ucs2buffer, 1024);
   AssertEquals(len, 2);
   AssertTrue(ucs2buffer[0] == 'A');
   AssertTrue(ucs2buffer[1] == 'x');
   EndTest();

#if !WITH_ADDRESS_SANITIZER
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
#endif

   StartTest(_T("ucs4_utf8len"));
   AssertEquals(ucs4_utf8len(ucs4TextSurrogates, -1), 18);  // with trailing 0
   AssertEquals(ucs4_utf8len(ucs4TextSurrogates, ucs4_strlen(ucs4TextSurrogates)), 17);  // without trailing 0
   EndTest();

   StartTest(_T("utf8_ucs4len"));
   AssertEquals(utf8_ucs4len(utf8TextSurrogates, -1), 14);  // with trailing 0
   AssertEquals(utf8_ucs4len(utf8TextSurrogates, strlen(utf8TextSurrogates)), 13);  // without trailing 0
   EndTest();

   StartTest(_T("ucs4_to_utf8"));
   len = ucs4_to_utf8(ucs4TextSurrogates, -1, utf8buffer, 128);
   AssertEquals(len, 18);
   AssertTrue(!strcmp(utf8buffer, utf8TextSurrogates));
   memset(utf8buffer, 0x7F, sizeof(utf8buffer));
   len = ucs4_to_utf8(ucs4TextSurrogates, ucs4_strlen(ucs4TextSurrogates), utf8buffer, 128);
   AssertEquals(len, 17);
   AssertTrue(!memcmp(utf8buffer, utf8TextSurrogates, len));
   AssertTrue(utf8buffer[len] == 0x7F);
   EndTest();

   StartTest(_T("utf8_to_ucs4"));
   len = utf8_to_ucs4(utf8TextSurrogates, -1, ucs4buffer, 1024);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, len * sizeof(UCS2CHAR)));
   len = utf8_to_ucs4(utf8TextSurrogates, -1, ucs4buffer, 14);
   AssertEquals(len, 14);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, len * sizeof(UCS2CHAR)));
   len = utf8_to_ucs4(utf8TextSurrogates, -1, ucs4buffer, 10);
   AssertEquals(len, 10);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, 9 * sizeof(UCS2CHAR)));
   AssertEquals(static_cast<uint32_t>(ucs4buffer[9]), 0u);
   memset(ucs4buffer, 0x7F, sizeof(ucs4buffer));
   len = utf8_to_ucs4(utf8TextSurrogates, strlen(utf8TextSurrogates), ucs4buffer, 1024);
   AssertEquals(len, 13);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextSurrogates, len * sizeof(UCS2CHAR)));
   AssertTrue(ucs4buffer[len] == 0x7F7F7F7F);
   len = utf8_to_ucs4(utf8InvalidSequence, strlen(utf8InvalidSequence), ucs4buffer, 1024);
   AssertEquals(len, 2);
   AssertTrue(ucs4buffer[0] == 'A');
   AssertTrue(ucs4buffer[1] == 'x');
   EndTest();

#if !WITH_ADDRESS_SANITIZER
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
#endif

#if UNICODE_UCS4
#define ucs4TextISO8859_1 wcTextISO8859_1
   UCS2CHAR ucs2TextISO8859_1[256];
   ucs4_to_ucs2(wcTextISO8859_1, -1, ucs2TextISO8859_1, 256);
#else
#define ucs2TextISO8859_1 wcTextISO8859_1
   UCS4CHAR ucs4TextISO8859_1[256];
   ucs2_to_ucs4(wcTextISO8859_1, -1, ucs4TextISO8859_1, 256);
#endif

   StartTest(_T("ISO8859_1_to_ucs4"));
   len = ISO8859_1_to_ucs4(mbTextISO8859_1, -1, ucs4buffer, 1024);
   AssertEquals(len, 52);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextISO8859_1, len * sizeof(UCS4CHAR)));
   memset(ucs4buffer, 0x7F, sizeof(ucs4buffer));
   len = ISO8859_1_to_ucs4(mbTextISO8859_1, strlen(mbTextISO8859_1), ucs4buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(ucs4buffer, ucs4TextISO8859_1, len * sizeof(UCS4CHAR)));
   AssertTrue(ucs4buffer[len] == 0x7F7F7F7F);
   EndTest();

   StartTest(_T("ucs4_to_ISO8859_1"));
   len = ucs4_to_ISO8859_1(ucs4TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 52);
   AssertTrue(!memcmp(utf8buffer, mbTextISO8859_1, len));
   memset(utf8buffer, 0x7F, sizeof(utf8buffer));
   len = ucs4_to_ISO8859_1(ucs4TextISO8859_1, ucs4_strlen(ucs4TextISO8859_1), utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextISO8859_1, len));
   AssertTrue(utf8buffer[len] == 0x7F);
   EndTest();

   StartTest(_T("ISO8859_1_to_ucs2"));
   len = ISO8859_1_to_ucs2(mbTextISO8859_1, -1, ucs2buffer, 1024);
   AssertEquals(len, 52);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextISO8859_1, len * sizeof(UCS2CHAR)));
   memset(ucs2buffer, 0x7F, sizeof(ucs2buffer));
   len = ISO8859_1_to_ucs2(mbTextISO8859_1, strlen(mbTextISO8859_1), ucs2buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(ucs2buffer, ucs2TextISO8859_1, len * sizeof(UCS2CHAR)));
   AssertTrue(ucs2buffer[len] == 0x7F7F);
   EndTest();

   StartTest(_T("ucs2_to_ISO8859_1"));
   len = ucs2_to_ISO8859_1(ucs2TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 52);
   AssertTrue(!memcmp(utf8buffer, mbTextISO8859_1, len));
   memset(utf8buffer, 0x7F, sizeof(utf8buffer));
   len = ucs2_to_ISO8859_1(ucs2TextISO8859_1, ucs2_strlen(ucs2TextISO8859_1), utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextISO8859_1, len));
   AssertTrue(utf8buffer[len] == 0x7F);
   EndTest();

   StartTest(_T("ucs4_to_ASCII"));
   len = ucs4_to_ASCII(ucs4TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 52);
   AssertTrue(!memcmp(utf8buffer, mbTextASCII, len));
   memset(utf8buffer, 0x7F, sizeof(utf8buffer));
   len = ucs4_to_ASCII(ucs4TextISO8859_1, ucs4_strlen(ucs4TextISO8859_1), utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextASCII, len));
   AssertTrue(utf8buffer[len] == 0x7F);
   EndTest();

   StartTest(_T("ucs2_to_ASCII"));
   len = ucs2_to_ASCII(ucs2TextISO8859_1, -1, utf8buffer, 1024);
   AssertEquals(len, 52);
   AssertTrue(!memcmp(utf8buffer, mbTextASCII, len));
   memset(utf8buffer, 0x7F, sizeof(utf8buffer));
   len = ucs2_to_ASCII(ucs2TextISO8859_1, ucs2_strlen(ucs2TextISO8859_1), utf8buffer, 1024);
   AssertEquals(len, 51);
   AssertTrue(!memcmp(utf8buffer, mbTextASCII, len));
   AssertTrue(utf8buffer[len] == 0x7F);
   EndTest();

   StartTest(_T("WideStringFromMBString"));
   WCHAR *wcTextTest = WideStringFromMBString(mbText);
   AssertNotNull(wcTextTest);
   AssertTrue(!wcscmp(wcText, wcTextTest));
   MemFree(wcTextTest);
   EndTest();

   StartTest(_T("WideStringFromMBStringSysLocale"));
   wcTextTest = WideStringFromMBStringSysLocale(mbText);
   AssertNotNull(wcTextTest);
   AssertTrue(!wcscmp(wcText, wcTextTest));
   MemFree(wcTextTest);
   EndTest();

   StartTest(_T("WideStringFromUTF8String"));
   wcTextTest = WideStringFromUTF8String(utf8TextSurrogates);
   AssertNotNull(wcTextTest);
#if UNICODE_UCS4
   AssertEquals(wcTextTest, ucs4TextSurrogates);
#else
   AssertEquals(wcTextTest, ucs2TextSurrogates);
#endif
   MemFree(wcTextTest);
   EndTest();

   StartTest(_T("MBStringFromWideString"));
   char *mbTextTest = MBStringFromWideString(wcText);
   AssertNotNull(mbTextTest);
   AssertEquals(mbTextTest, mbText);
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("MBStringFromWideStringSysLocale"));
   mbTextTest = MBStringFromWideStringSysLocale(wcText);
   AssertNotNull(mbTextTest);
   AssertEquals(mbTextTest, mbText);
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("MBStringFromUCS2String"));
   mbTextTest = MBStringFromUCS2String(ucs2TextShort);
   AssertNotNull(mbTextTest);
   AssertEquals(mbTextTest, mbTextShort);
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("MBStringFromUCS4String"));
   mbTextTest = MBStringFromUCS4String(ucs4TextShort);
   AssertNotNull(mbTextTest);
   AssertEquals(mbTextTest, mbTextShort);
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("MBStringFromUTF8String"));
   mbTextTest = MBStringFromUTF8String(mbText);
   AssertNotNull(mbTextTest);
   AssertEquals(mbTextTest, mbText);
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("UTF8StringFromWideString"));
#if UNICODE_UCS4
   mbTextTest = UTF8StringFromWideString(ucs4TextSurrogates);
#else
   mbTextTest = UTF8StringFromWideString(ucs2TextSurrogates);
#endif
   AssertNotNull(mbTextTest);
   AssertTrue(!strcmp(mbTextTest, utf8TextSurrogates));
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("UTF8StringFromMBString"));
   mbTextTest = UTF8StringFromMBString(mbText);
   AssertNotNull(mbTextTest);
   AssertTrue(!strcmp(mbText, mbTextTest));
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("UTF8StringFromUCS2String"));
   mbTextTest = UTF8StringFromUCS2String(ucs2TextSurrogates);
   AssertNotNull(mbTextTest);
   AssertTrue(!strcmp(mbTextTest, utf8TextSurrogates));
   MemFree(mbTextTest);
   EndTest();

   StartTest(_T("UCS2StringFromUCS4String"));
   UCS2CHAR *ucs2TextTest = UCS2StringFromUCS4String(ucs4TextSurrogates);
   AssertNotNull(ucs2TextTest);
   AssertTrue(!memcmp(ucs2TextTest, ucs2TextSurrogates, (ucs2_strlen(ucs2TextSurrogates) + 1) * sizeof(UCS2CHAR)));
   MemFree(ucs2TextTest);
   EndTest();

   StartTest(_T("UCS2StringFromMBString"));
   ucs2TextTest = UCS2StringFromMBString(mbTextShort);
   AssertNotNull(ucs2TextTest);
   AssertTrue(!memcmp(ucs2TextTest, ucs2TextShort, (ucs2_strlen(ucs2TextShort) + 1) * sizeof(UCS2CHAR)));
   MemFree(ucs2TextTest);
   EndTest();

   StartTest(_T("UCS2StringFromUTF8String"));
   ucs2TextTest = UCS2StringFromUTF8String(utf8TextSurrogates);
   AssertNotNull(ucs2TextTest);
   AssertTrue(!memcmp(ucs2TextTest, ucs2TextSurrogates, (ucs2_strlen(ucs2TextSurrogates) + 1) * sizeof(UCS2CHAR)));
   MemFree(ucs2TextTest);
   EndTest();

   StartTest(_T("UCS4StringFromUCS2String"));
   UCS4CHAR *ucs4TextTest = UCS4StringFromUCS2String(ucs2TextSurrogates);
   AssertNotNull(ucs4TextTest);
   AssertTrue(!memcmp(ucs4TextTest, ucs4TextSurrogates, (ucs4_strlen(ucs4TextSurrogates) + 1) * sizeof(UCS4CHAR)));
   MemFree(ucs4TextTest);
   EndTest();

   StartTest(_T("UCS4StringFromMBString"));
   ucs4TextTest = UCS4StringFromMBString(mbTextShort);
   AssertNotNull(ucs4TextTest);
   AssertTrue(!memcmp(ucs4TextTest, ucs4TextShort, (ucs4_strlen(ucs4TextShort) + 1) * sizeof(UCS4CHAR)));
   MemFree(ucs4TextTest);
   EndTest();
}
