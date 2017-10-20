#include <nms_common.h>
#include <nms_util.h>
#include <nxqueue.h>
#include <nxcpapi.h>
#include <testtools.h>

void TestMsgWaitQueue();
void TestMessageClass();
void TestMutexWrapper();
void TestRWLockWrapper();
void TestConditionWrapper();

static char mbText[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static WCHAR wcText[] = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
static char mbTextShort[] = "Lorem ipsum";
static UCS2CHAR ucs2TextShort[] = { 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm', 0 };

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

   StartTest(_T("UCS-2 to UCS-4 conversion performance"));
   mb_to_ucs2(mbText, -1, ucs2buffer, 1024);
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      WCHAR buffer[1024];
      ucs2_to_ucs4(ucs2buffer, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("UCS-4 to UCS-2 conversion performance"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < 10000; i++)
   {
      UCS2CHAR buffer[1024];
      ucs4_to_ucs2(wcText, -1, buffer, 1024);
   }
   EndTest(GetCurrentTimeMs() - start);
#endif
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
   _tprintf(_T("size: %d"), m->size());
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

   wcslcpy(buffer, _T("short text"), 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, _T("short text")));

   wcslcpy(buffer, _T("long text: 1234567890 1234567890 1234567890 1234567890"), 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, _T("long text: 1234567890 123456789")));

   EndTest();

   StartTest(_T("wcslcat"));

   memset(buffer, 0, sizeof(buffer));
   buffer[32] = L'$';
   wcslcat(buffer, _T("part1"), 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, _T("part1")));

   wcslcat(buffer, _T("part2"), 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, _T("part1part2")));

   wcslcat(buffer, _T("long text: 1234567890 1234567890 1234567890 1234567890"), 32);
   AssertEquals(buffer[32], L'$');
   AssertTrue(!wcscmp(buffer, _T("part1part2long text: 1234567890")));

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

   delete q;
}

/**
 * Key for hash map
 */
typedef char HASH_KEY[6];

/**
 * Test hash map
 */
static void TestHashMap()
{
   StartTest(_T("HashMap: create"));
   HashMap<HASH_KEY, String> *hashMap = new HashMap<HASH_KEY, String>(true);
   AssertEquals(hashMap->size(), 0);
   EndTest();

   HASH_KEY k1 = { '1', '2', '3', '4', '5', '6' };
   HASH_KEY k2 = { '0', '0', 'a', 'b', 'c', 'd' };
   HASH_KEY k3 = { '0', '0', '3', 'X', '1', '1' };

   StartTest(_T("HashMap: set/get"));

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

   StartTest(_T("HashMap: replace"));
   hashMap->set(k2, new String(_T("REPLACE")));
   s = hashMap->get(k2);
   AssertNotNull(s);
   AssertTrue(!_tcscmp(s->getBuffer(), _T("REPLACE")));
   EndTest();

   StartTest(_T("HashMap: iterator"));
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

   StartTest(_T("HashMap: iterator remove"));
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

   StartTest(_T("HashMap: remove"));
   hashMap->remove(k3);
   AssertNull(hashMap->get(k3));
   EndTest();

   StartTest(_T("HashMap: clear"));
   hashMap->clear();
   AssertEquals(hashMap->size(), 0);
   it = hashMap->iterator();
   AssertFalse(it->hasNext());
   AssertNull(it->next());
   delete it;
   EndTest();

   delete hashMap;
}

/**
 * Test array
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
   AssertEquals(array->size(), 4);
   AssertNull(array->get(4));
   AssertNotNull(array->get(1));
   AssertTrue(!_tcscmp(array->get(1)->getBuffer(), _T("value 2")));
   EndTest();

   StartTest(_T("ObjectArray: replace"));
   array->replace(0, new String(_T("replace")));
   AssertEquals(array->size(), 4);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("replace")));
   EndTest();

   StartTest(_T("ObjectArray: remove"));
   array->remove(0);
   AssertEquals(array->size(), 3);
   AssertTrue(!_tcscmp(array->get(0)->getBuffer(), _T("value 2")));
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

   delete array;
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
   EndTest(GetCurrentTimeMs() - start);

   delete table;
   delete table2;
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
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
#ifdef _WIN32
   WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   TestString();
   TestStringConversion();
   TestStringMap();
   TestStringSet();
   TestStringFunctionsA();
   TestStringFunctionsW();
   TestMessageClass();
   TestMsgWaitQueue();
   TestInetAddress();
   TestItoa();
   TestQueue();
   TestHashMap();
   TestObjectArray();
   TestTable();
   TestMutexWrapper();
   TestRWLockWrapper();
   TestConditionWrapper();
   TestByteSwap();
   TestDiff();
   TestRingBuffer();
   TestDebugLevel();
   TestDebugTags();

   MsgWaitQueue::shutdown();
   return 0;
}
