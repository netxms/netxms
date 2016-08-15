#include <nms_common.h>
#include <nms_util.h>
#include <nxqueue.h>
#include <testtools.h>

void TestMsgWaitQueue();
void TestMessageClass();

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
   TCHAR *str = s.substring(0, 5);
   AssertTrue(!_tcscmp(str, _T("alpha")));
   free(str);
   EndTest();

   StartTest(_T("String - substring #2"));
   s = _T("alpha beta gamma");
   str = s.substring(5, -1);
   AssertTrue(!_tcscmp(str, _T(" beta gamma")));
   free(str);
   EndTest();

   StartTest(_T("String - substring #3"));
   s = _T("alpha beta gamma");
   str = s.substring(14, 4);
   AssertTrue(!_tcscmp(str, _T("ma")));
   free(str);
   EndTest();
}

/**
 * Test InetAddress class
 */
static void TestInetAddress()
{
   InetAddress a, b, c;

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
   AssertTrue(memcmp(key, "\x06\x02\x0A\x03\x01\x5B\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 18) == 0);
#else
   AssertTrue(memcmp(key, "\x06\x02\x5B\x01\x03\x0A\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 18) == 0);
#endif
   EndTest();
   
   StartTest(_T("InetAddress - buildHashKey() - IPv6"));
   a = InetAddress::parse("fe80:1234::6e88:14ff:fec4:b8f8");
   a.buildHashKey(key);
   AssertTrue(memcmp(key, "\x12\x0A\xFE\x80\x12\x34\x00\x00\x00\x00\x6E\x88\x14\xFF\xFE\xC4\xB8\xF8", 18) == 0);
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
   TestMessageClass();
   TestMsgWaitQueue();
   TestInetAddress();
   TestItoa();
   TestQueue();
   TestHashMap();
   TestObjectArray();
   TestTable();
   return 0;
}
