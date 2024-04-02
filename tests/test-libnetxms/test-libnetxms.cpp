#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxproc.h>
#include <testtools.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(test-libnetxms)

void TestGauge64();
void TestMemoryPool();
void TestObjectMemoryPool();
void TestThreadPool();
void TestThreadPoolDelayedExecution();
void TestQueue();
void TestSharedObjectQueue();
void TestMsgWaitQueue();
void TestMessageClass();
void TestMutex();
void TestUniqueLock();
void TestCondition();
void TestRWLock();
void TestThreadCountAndMaxWaitTime();
void TestProcessExecutor(const char *procname);
void TestProcessExecutorWorker();
void TestStringConversion();
void TestSubProcess(const char *procname, bool debug);
void TestGeoLocation();
void TestRSA();
void TestMD4();
NXCPMessage *TestSubProcessRequestHandler(UINT16 command, const void *data, size_t dataSize);

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
   s1->addPreallocated(MemCopyString(_T("text2")));
   s1->add(3.1415);

   AssertEquals(s1->size(), 5);
   AssertEquals(s1->get(0), _T("1"));
   AssertEquals(s1->get(1), _T("12345000000001"));
   AssertEquals(s1->get(2), _T("text1"));
   AssertEquals(s1->get(3), _T("text2"));
   AssertTrue(!_tcsncmp(s1->get(4), _T("3.1415"), 6));
   EndTest();

   StartTest(_T("String list - copy constructor"));
   StringList *s2 = new StringList(s1);
   delete s1;

   AssertEquals(s2->size(), 5);
   AssertEquals(s2->get(0), _T("1"));
   AssertEquals(s2->get(1), _T("12345000000001"));
   AssertEquals(s2->get(2), _T("text1"));
   AssertEquals(s2->get(3), _T("text2"));
   AssertTrue(!_tcsncmp(s2->get(4), _T("3.1415"), 6));
   EndTest();

   StartTest(_T("String list - fill message"));

   NXCPMessage msg;
   s2->fillMessage(&msg, 100, 1);
   delete s2;

   StringList *s3 = new StringList(msg, 100, 1);
   AssertEquals(s3->size(), 5);
   AssertEquals(s3->get(0), _T("1"));
   AssertEquals(s3->get(1), _T("12345000000001"));
   AssertEquals(s3->get(2), _T("text1"));
   AssertEquals(s3->get(3), _T("text2"));
   AssertTrue(!_tcsncmp(s3->get(4), _T("3.1415"), 6));
   delete s3;
   EndTest();
   
   StartTest(_T("String list - sort"));
   s3 = new StringList();
   s3->add(1);
   s3->add(3);
   s3->add(2);
   s3->sort();
   AssertEquals(s3->get(0), _T("1"));
   AssertEquals(s3->get(1), _T("2"));
   AssertEquals(s3->get(2), _T("3"));
   delete s3;
   EndTest();   

#if !WITH_ADDRESS_SANITIZER
   StartTest(_T("String list - performance"));
   int64_t startTime = GetCurrentTimeMs();
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
#endif
}

/**
 * Test string map
 */
static void TestStringMap()
{
   const int mapSize = 100000;

   StringMap *m = new StringMap();

   StartTest(_T("String map - iterator on empty map"));
   {
      auto it = m->begin();
      AssertFalse(it.hasNext());
      AssertNull(it.next());
   }
   EndTest();

   StartTest(_T("String map - insert"));
   int64_t start = GetCurrentTimeMs();
   for(int i = 0; i < mapSize; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d"), i);
      m->set(key, _T("Lorem ipsum dolor sit amet"));
   }
   AssertEquals(m->size(), mapSize);
   const TCHAR *v = m->get(_T("key-42"));
   AssertNotNull(v);
   AssertEquals(v, _T("Lorem ipsum dolor sit amet"));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - replace"));
   start = GetCurrentTimeMs();
   for(int i = 0; i < mapSize; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d"), i);
      m->set(key, _T("consectetur adipiscing elit"));
   }
   AssertEquals(m->size(), mapSize);
   v = m->get(_T("key-42"));
   AssertNotNull(v);
   AssertEquals(v, _T("consectetur adipiscing elit"));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - get"));
   start = GetCurrentTimeMs();
   v = m->get(_T("key-888"));
   AssertNotNull(v);
   AssertEquals(v, _T("consectetur adipiscing elit"));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - iterator"));
   start = GetCurrentTimeMs();
   {
      auto it = m->begin();
      int i = 0;
      while(it.hasNext())
      {
         TCHAR key[64];
         _sntprintf(key, 64, _T("key-%d"), i++);
         auto pair = it.next();
         AssertNotNull(pair->key);
         AssertNotNull(pair->value);
         AssertEquals(pair->key, key);
         AssertEquals(pair->value, _T("consectetur adipiscing elit"));
      }
      AssertEquals(i, m->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - iterator for loop"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it = m->begin(); it != m->end(); it++)
      {
         TCHAR key[64];
         _sntprintf(key, 64, _T("key-%d"), i++);
         AssertNotNull(it.value()->key);
         AssertNotNull(it.value()->value);
         AssertEquals(it.value()->key, key);
         AssertEquals((TCHAR*)it.value()->value, _T("consectetur adipiscing elit"));
      }
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - range based for"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto p : *m)
      {
         TCHAR key[64];
         _sntprintf(key, 64, _T("key-%d"), i++);
         AssertNotNull(p->key);
         AssertNotNull(p->value);
         AssertEquals(p->key, key);
         AssertEquals(p->value, _T("consectetur adipiscing elit"));
      }
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String map - keys"));
   StringList *keys = m->keys();
   AssertNotNull(keys);
   AssertEquals(keys->size(), mapSize);
   AssertTrue(!_tcsncmp(keys->get(mapSize / 20), _T("key-"), 4));
   delete keys;
   EndTest();

   StartTest(_T("String map - iterator remove"));
   auto it = m->begin();
   AssertTrue(it.hasNext());
   AssertNotNull(it.next());
   auto pair2 = it.next();
   AssertNotNull(pair2);
   it.remove();
   AssertTrue(it.hasNext());
   AssertNotNull(it.next());

   AssertNotNull(m->get(_T("key-0")));
   AssertNull(m->get(_T("key-1")));
   AssertNotNull(m->get(_T("key-2")));

   m->set(_T("key-1"), _T("consectetur adipiscing elit"));
   EndTest();

   StartTest(_T("String map - deleting every odd entry"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it = m->begin(); it != m->end(); it++, i++)
      {
         if(i % 2)
            it.remove();
      }
   }
   AssertEquals(m->size(), mapSize / 2);
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
   const size_t setSize = 100000;

   StringSet *s = new StringSet();

   StartTest(_T("String set - insert"));
   int64_t start = GetCurrentTimeMs();
   for(size_t i = 0; i < setSize; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d lorem ipsum"), static_cast<int>(i));
      s->add(key);
   }
   AssertEquals(s->size(), setSize);
   AssertTrue(s->contains(_T("key-42 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - replace"));
   start = GetCurrentTimeMs();
   for(size_t i = 0; i < setSize; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d lorem ipsum"), static_cast<int>(i));
      s->add(key);
   }
   AssertEquals(s->size(), setSize);
   AssertTrue(s->contains(_T("key-42 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - contains"));
   start = GetCurrentTimeMs();
   AssertTrue(s->contains(_T("key-888 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - iterator for loop"));
   start = GetCurrentTimeMs();
   {
      size_t i = 0;
      for(auto it = s->begin(); it != s->end(); it++)
      {
         TCHAR value[64];
         _sntprintf(value, 64, _T("key-%d lorem ipsum"), static_cast<int>(i++));
         AssertEquals(it.value(), value);
      }
      AssertEquals(i, setSize);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - range for"));
   start = GetCurrentTimeMs();
   {
      size_t i = 0;
      for(const TCHAR *v : *s)
      {
         TCHAR value[64];
         _sntprintf(value, 64, _T("key-%d lorem ipsum"), static_cast<int>(i++));
         AssertEquals(v, value);
      }
      AssertEquals(i, setSize);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - iterator"));
   start = GetCurrentTimeMs();
   {
      size_t i = 0;
      auto it = s->begin();
      while(it.hasNext())
      {
         TCHAR value[64];
         _sntprintf(value, 64, _T("key-%d lorem ipsum"), static_cast<int>(i++));
         AssertEquals(it.next(), value);
      }
      AssertEquals(i, setSize);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("String set - remove"));
   s->remove(_T("key-17 lorem ipsum"));
   AssertFalse(s->contains(_T("key-17 lorem ipsum")));
   AssertEquals(s->size(), setSize - 1);
   s->remove(_T("key-17 lorem ipsum"));   // remove non-existent key
   AssertFalse(s->contains(_T("key-17 lorem ipsum")));
   AssertEquals(s->size(), setSize - 1);
   EndTest();

   StartTest(_T("String set - iterator remove"));
   auto it = s->begin();
   AssertTrue(it.hasNext());
   bool found = false;
   while(it.hasNext())
   {
      const TCHAR *v = it.next();
      AssertNotNull(v);
      if (!_tcscmp(v, _T("key-42 lorem ipsum")))
      {
         found = true;
         break;
      }
   }
   AssertTrue(found);
   it.remove();
   AssertEquals(s->size(), setSize - 2);
   AssertFalse(s->contains(_T("key-42 lorem ipsum")));
   EndTest();

   StartTest(_T("String set - clear"));
   start = GetCurrentTimeMs();
   s->clear();
   AssertEquals(s->size(), 0);
   EndTest(GetCurrentTimeMs() - start);

   delete s;
}

/**
 * Test counting string set
 */
static void TestCountingStringSet()
{
   const size_t setSize = 100000;

   StringSet *cs = new StringSet(true);

   StartTest(_T("Counting string set - insert"));
   int64_t start = GetCurrentTimeMs();
   for(size_t i = 0; i < setSize; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d lorem ipsum"), static_cast<int>(i));
      cs->add(key);
   }
   AssertEquals(cs->size(), setSize);
   AssertTrue(cs->contains(_T("key-42 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Counting string set - increment"));
   start = GetCurrentTimeMs();
   for(size_t i = 0; i < setSize; i++)
   {
      TCHAR key[64];
      _sntprintf(key, 64, _T("key-%d lorem ipsum"), static_cast<int>(i));
      AssertEquals(cs->add(key), 2);
   }
   AssertEquals(cs->size(), setSize);
   AssertTrue(cs->contains(_T("key-42 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Counting string set - contains"));
   start = GetCurrentTimeMs();
   AssertTrue(cs->contains(_T("key-888 lorem ipsum")));
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Counting string set - count"));
   start = GetCurrentTimeMs();
   AssertEquals(cs->count(_T("key-888 lorem ipsum")), 2);
   AssertEquals(cs->count(_T("wrong key")), 0);
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Counting string set - decrement"));
   AssertEquals(cs->remove(_T("key-17 lorem ipsum")), 1);
   AssertTrue(cs->contains(_T("key-17 lorem ipsum")));
   AssertEquals(cs->count(_T("key-17 lorem ipsum")), 1);
   AssertEquals(cs->size(), setSize);
   AssertEquals(cs->remove(_T("key-17 lorem ipsum")), 0);
   AssertFalse(cs->contains(_T("key-17 lorem ipsum")));
   AssertEquals(cs->size(), setSize - 1);
   AssertEquals(cs->remove(_T("key-17 lorem ipsum")), 0);   // remove non-existent key
   AssertFalse(cs->contains(_T("key-17 lorem ipsum")));
   AssertEquals(cs->size(), setSize - 1);
   EndTest();

   StartTest(_T("Counting string set - clear"));
   start = GetCurrentTimeMs();
   cs->clear();
   AssertEquals(cs->size(), 0);
   EndTest(GetCurrentTimeMs() - start);

   delete cs;
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
   AssertEquals(buffer, "short text");

   strlcpy(buffer, "long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], '$');
   AssertEquals(buffer, "long text: 1234567890 123456789");

   EndTest();

   StartTest(_T("strlcat"));

   memset(buffer, 0, sizeof(buffer));
   buffer[32] = '$';
   strlcat(buffer, "part1", 32);
   AssertEquals(buffer[32], '$');
   AssertEquals(buffer, "part1");

   strlcat(buffer, "part2", 32);
   AssertEquals(buffer[32], '$');
   AssertEquals(buffer, "part1part2");

   strlcat(buffer, "long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], '$');
   AssertEquals(buffer, "part1part2long text: 1234567890");

   EndTest();

   StartTest(_T("stristr"));
   static const char *s = "One Two Three";
   AssertTrue(s == stristr(s, ""));
   AssertTrue(&s[4] == stristr(s, "two"));
   AssertNull(stristr(s, "TwoThree"));
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
   AssertEquals(buffer,L"short text");

   wcslcpy(buffer, L"long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], L'$');
   AssertEquals(buffer, L"long text: 1234567890 123456789");

   EndTest();

   StartTest(_T("wcslcat"));

   memset(buffer, 0, sizeof(buffer));
   buffer[32] = L'$';
   wcslcat(buffer, L"part1", 32);
   AssertEquals(buffer[32], L'$');
   AssertEquals(buffer, L"part1");

   wcslcat(buffer, L"part2", 32);
   AssertEquals(buffer[32], L'$');
   AssertEquals(buffer, L"part1part2");

   wcslcat(buffer, L"long text: 1234567890 1234567890 1234567890 1234567890", 32);
   AssertEquals(buffer[32], L'$');
   AssertEquals(buffer, L"part1part2long text: 1234567890");

   EndTest();

   StartTest(_T("wcsupr"));
   WCHAR textToUpper[64] = L"TeXt 123 abCD";
   wcsupr(textToUpper);
   AssertEquals(textToUpper, L"TEXT 123 ABCD");
   EndTest();

   StartTest(_T("wcslwr"));
   WCHAR textToLower[64] = L"TeXt 123 abCD";
   wcslwr(textToLower);
   AssertEquals(textToLower, L"text 123 abcd");
   EndTest();

   StartTest(_T("wcsistr"));
   static const WCHAR *s = L"One Two Three";
   AssertTrue(s == wcsistr(s, L""));
   AssertTrue(&s[4] == wcsistr(s, L"two"));
   AssertNull(wcsistr(s, L"TwoThree"));
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
   StringBuffer s;

   StartTest(_T("String - equals"));
   String s1 = _T("alpha beta gamma");
   String s2 = _T("alpha beta gamma");
   String s3 = _T("alpha beta");
   String s4 = _T("Alpha Beta Gamma");
   AssertTrue(s1.equals(s2));
   AssertTrue(s1.equals(_T("alpha beta gamma")));
   AssertFalse(s1.equals(s3));
   AssertFalse(s1.equals(_T("alpha beta")));
   AssertFalse(s1.equals(s4));
   EndTest();

   StartTest(_T("String - equalsIgnoreCase"));
   AssertTrue(s1.equalsIgnoreCase(s2));
   AssertTrue(s1.equalsIgnoreCase(_T("alpha beta gamma")));
   AssertFalse(s1.equalsIgnoreCase(s3));
   AssertFalse(s1.equalsIgnoreCase(_T("alpha beta")));
   AssertTrue(s1.equalsIgnoreCase(s4));
   AssertTrue(s1.equalsIgnoreCase(_T("Alpha Beta Gamma")));
   EndTest();

   StartTest(_T("String - append"));
   for(size_t i = 0; i < 256; i++)
      s.append(_T("ABC "));
   AssertEquals(s.length(), 1024);
   AssertTrue(!_tcsncmp(s.getBuffer(), _T("ABC ABC ABC ABC "), 16));
   EndTest();

   StartTest(_T("String - insert"));
   s = _T("one");
   s.insert(0, _T("two"));
   AssertEquals(s, _T("twoone"));
   s.insert(3, _T(" "));
   AssertEquals(s, _T("two one"));
   EndTest();

   StartTest(_T("String - assign #1"));
   s = _T("alpha");
   AssertEquals(s.length(), 5);
   AssertEquals(s.getBuffer(), _T("alpha"));
   EndTest();

   StartTest(_T("String - assign #2"));
   String t(_T("init string"));
   s = t;
   AssertEquals(s.length(), 11);
   AssertEquals(s.getBuffer(), _T("init string"));
   EndTest();

   StartTest(_T("String - shrink"));
   s.shrink();
   AssertEquals(s.length(), 10);
   AssertEquals(s.getBuffer(), _T("init strin"));
   EndTest();

   StartTest(_T("String - escape"));
   s.escapeCharacter('i', '+');
   AssertEquals(s.length(), 13);
   AssertEquals(s.getBuffer(), _T("+in+it str+in"));
   EndTest();

   StartTest(_T("String - replace #1"));
   s = _T("alpha beta gamma");
   s.replace(_T("beta"), _T("epsilon"));
   AssertEquals(s.length(), 19);
   AssertEquals(s.getBuffer(), _T("alpha epsilon gamma"));
   EndTest();

   StartTest(_T("String - replace #2"));
   s = _T("alpha beta gamma");
   s.replace(_T("beta"), _T("xxxx"));
   AssertEquals(s.length(), 16);
   AssertEquals(s.getBuffer(), _T("alpha xxxx gamma"));
   EndTest();

   StartTest(_T("String - replace #3"));
   s = _T("alpha beta gamma alpha omega");
   s.replace(_T("alpha"), _T("Z"));
   AssertEquals(s.length(), 20);
   AssertEquals(s.getBuffer(), _T("Z beta gamma Z omega"));
   EndTest();

   StartTest(_T("String - substring #1"));
   s = _T("alpha beta gamma");
   TCHAR *str = s.substring(0, 5, nullptr);
   AssertEquals(str, _T("alpha"));
   free(str);
   EndTest();

   StartTest(_T("String - substring #2"));
   s = _T("alpha beta gamma");
   str = s.substring(5, -1, nullptr);
   AssertEquals(str, _T(" beta gamma"));
   free(str);
   EndTest();

   StartTest(_T("String - substring #3"));
   s = _T("alpha beta gamma");
   str = s.substring(14, 4, nullptr);
   AssertEquals(str, _T("ma"));
   free(str);
   EndTest();

   StartTest(_T("String - substring #4"));
   s = _T("alpha beta gamma");
   AssertEquals(s.substring(0, 5), _T("alpha"));
   EndTest();

   StartTest(_T("String - substring #5"));
   s = _T("alpha beta gamma");
   AssertEquals(s.substring(5, -1), _T(" beta gamma"));
   EndTest();

   StartTest(_T("String - substring #6"));
   s = _T("alpha beta gamma");
   AssertEquals(s.substring(14, 4), _T("ma"));
   EndTest();

   StartTest(_T("String - left #1"));
   s = _T("alpha beta gamma");
   AssertTrue(s.left(5).equals(_T("alpha")));
   EndTest();

   StartTest(_T("String - left #2"));
   s = _T("alpha");
   AssertTrue(s.left(15).equals(_T("alpha")));
   EndTest();

   StartTest(_T("String - right #1"));
   s = _T("alpha beta gamma");
   AssertTrue(s.right(5).equals(_T("gamma")));
   EndTest();

   StartTest(_T("String - right #2"));
   s = _T("alpha");
   AssertTrue(s.right(15).equals(_T("alpha")));
   EndTest();

   StartTest(_T("String - find"));
   s = _T("alpha beta gamma");
   AssertEquals(s.find(_T("beta")), 6);
   AssertEquals(s.find(_T("Beta")), String::npos);
   EndTest();

   StartTest(_T("String - findIgnoreCase"));
   s = _T("alpha beta gamma");
   AssertEquals(s.findIgnoreCase(_T("beta")), 6);
   AssertEquals(s.findIgnoreCase(_T("Beta")), 6);
   AssertEquals(s.findIgnoreCase(_T("BetaGamma")), String::npos);
   EndTest();

   StartTest(_T("String - split"));
   s = _T("alpha;;beta;gamma;;delta");
   StringList *list = s.split(_T(";;"));
   AssertNotNull(list);
   AssertEquals(list->size(), 3);
   AssertEquals(list->get(0), _T("alpha"));
   AssertEquals(list->get(1), _T("beta;gamma"));
   AssertEquals(list->get(2), _T("delta"));
   delete list;
   EndTest();

   StartTest(_T("String - split with callback"));
   s = _T("alpha;; beta;gamma ;; delta");
   int count = 0;
   s.split(_T(";;"), false, [&count] (const String& s) {
      if (count == 0)
         AssertEquals(s, _T("alpha"));
      else if (count == 1)
         AssertEquals(s, _T(" beta;gamma "));
      else if (count == 2)
         AssertEquals(s, _T(" delta"));
      count++;
   });
   AssertEquals(count, 3);
   EndTest();

   StartTest(_T("String - split with callback and trim"));
   s = _T("alpha;; beta;gamma ;; delta");
   count = 0;
   s.split(_T(";;"), true, [&count] (const String& s) {
      if (count == 0)
         AssertEquals(s, _T("alpha"));
      else if (count == 1)
         AssertEquals(s, _T("beta;gamma"));
      else if (count == 2)
         AssertEquals(s, _T("delta"));
      count++;
   });
   AssertEquals(count, 3);
   EndTest();

   StartTest(_T("String - appendAsHexString"));
   s = _T("");
   s.appendAsHexString(reinterpret_cast<const BYTE*>("\x12\x24\x36\x48"), 5);
   AssertTrue(s.equals(_T("1224364800")));
   EndTest();

   StartTest(_T("String - insertAsHexString"));
   s = _T("onetwo");
   s.insertAsHexString(3, reinterpret_cast<const BYTE*>("\xFA\xCD\xEF\xA8"), 5);
   AssertTrue(s.equals(_T("oneFACDEFA800two")));
   s = _T("onetwo");
   s.insertAsHexString(3, reinterpret_cast<const BYTE*>("\xFA\xCD\xEF\xA8"), 5, ':');
   AssertTrue(s.equals(_T("oneFA:CD:EF:A8:00two")));
   EndTest();

   StartTest(_T("String - take ownership"));
   String os(MemCopyString(_T("preallocated string")), -1, Ownership::True);
   AssertEquals(os, _T("preallocated string"));
   String os2(MemCopyString(_T("long preallocated string - 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890")), -1, Ownership::True);
   AssertEquals(os2, _T("long preallocated string - 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"));
   EndTest();

   StartTest(_T("String - move constructor"));
   String src1(_T("string to move"));
   AssertEquals(src1, _T("string to move"));
   String dst1(std::move(src1));
   AssertEquals(dst1, _T("string to move"));
   AssertTrue(src1.isEmpty());
   String src2(_T("long string to move - 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"));
   AssertEquals(src2, _T("long string to move - 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"));
   String dst2(std::move(src2));
   AssertEquals(dst2, _T("long string to move - 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"));
   AssertTrue(src2.isEmpty());
   EndTest();
}

/**
 * Generic ID with sizeof expecetd to be 8
 */
class GenericId_8 : public GenericId<6>
{
public:
   GenericId_8(size_t length = 0) : GenericId<6>(length) { }
   GenericId_8(const BYTE *value, size_t length) : GenericId<6>(value, length) { }
   GenericId_8(const GenericId_8& src) : GenericId<6>(src) { }
};

/**
 * Generic ID with sizeof expecetd to be 16
 */
class GenericId_16 : public GenericId<12>
{
public:
   GenericId_16(size_t length = 0) : GenericId<12>(length) { }
   GenericId_16(const BYTE *value, size_t length) : GenericId<12>(value, length) { }
   GenericId_16(const GenericId_16& src) : GenericId<12>(src) { }
};

/**
 * Test GenericId class
 */
static void TestGenericId()
{
   StartTest(_T("GenericId - sizeof"));
   AssertEquals(sizeof(GenericId_8), 8);
   AssertEquals(sizeof(GenericId_16), 16);
   EndTest();

   BYTE zeroValue[32];
   memset(zeroValue, 0, sizeof(zeroValue));

   BYTE testValue[32];
   RAND_bytes(testValue, sizeof(testValue));

   StartTest(_T("GenericId - no padding"));

   GenericId_8 id8a(testValue, 6);
   AssertEquals(id8a.length(), 6);
   GenericId_8 id8b(id8a);
   AssertTrue(id8a.equals(id8b));
   AssertTrue(!memcmp(id8b.value(), testValue, 6));

   GenericId_8 id8z(0);
   AssertTrue(id8z.isNull());
   AssertEquals(id8z.length(), 0);
   AssertTrue(!memcmp(&id8z, zeroValue, sizeof(GenericId_8)));

   EndTest();

   StartTest(_T("GenericId - padding"));

   GenericId_16 id16a(testValue, 12);
   AssertEquals(id16a.length(), 12);
   GenericId_16 id16b(id16a);
   AssertTrue(id16a.equals(id16b));
   AssertTrue(!memcmp(id16b.value(), testValue, 12));

   GenericId_16 id16z(0);
   AssertTrue(id16z.isNull());
   AssertEquals(id16z.length(), 0);
   AssertTrue(!memcmp(&id16z, zeroValue, sizeof(GenericId_16)));

   EndTest();
}

/**
 * Test string to binary conversion
 */
static void TestStringToBinaryConversions()
{
   static const char inputA[] = "0102fadeA1003dFF";
   static const WCHAR inputW[] = L"0102FADEa1003dff";
   static BYTE binary[8] = { 0x01, 0x02, 0xfa, 0xde, 0xa1, 0x00, 0x3d, 0xff };

   BYTE output[8];

   StartTest(_T("StrToBinA"));
   memset(output, 0, sizeof(output));
   StrToBinA(inputA, output, 8);
   AssertTrue(!memcmp(output, binary, 8));
   EndTest();

   StartTest(_T("StrToBinW"));
   memset(output, 0, sizeof(output));
   StrToBinW(inputW, output, 8);
   AssertTrue(!memcmp(output, binary, 8));
   EndTest();
}

/**
 * Test string to binary conversion
 */
static void TestBinaryToStringConversions()
{
   static BYTE input[8] = { 0x01, 0x02, 0xfa, 0xde, 0xa1, 0x00, 0x3d, 0xff };

   char outputA[64];
   WCHAR outputW[64];

   StartTest(_T("BinToStrA"));
   BinToStrA(input, 8, outputA);
   AssertEquals(outputA, "0102FADEA1003DFF");
   BinToStrAL(input, 8, outputA);
   AssertEquals(outputA, "0102fadea1003dff");
   EndTest();

   StartTest(_T("BinToStrW"));
   BinToStrW(input, 8, outputW);
   AssertEquals(outputW, L"0102FADEA1003DFF");
   BinToStrWL(input, 8, outputW);
   AssertEquals(outputW, L"0102fadea1003dff");
   EndTest();

   StartTest(_T("BinToStrExA"));
   BinToStrExA(input, 8, outputA, ':', 2);
   AssertEquals(outputA, "01:02:FA:DE:A1:00:3D:FF:  :  ");
   BinToStrExAL(input, 8, outputA, '-', 0);
   AssertEquals(outputA, "01-02-fa-de-a1-00-3d-ff");
   EndTest();

   StartTest(_T("BinToStrExW"));
   BinToStrExW(input, 8, outputW, L':', 2);
   AssertEquals(outputW, L"01:02:FA:DE:A1:00:3D:FF:  :  ");
   BinToStrExWL(input, 8, outputW, L'-', 0);
   AssertEquals(outputW, L"01-02-fa-de-a1-00-3d-ff");
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

   StartTest(_T("MacAddress - padding"));
   AssertTrue(sizeof(MacAddress) % 8 == 0);
   BYTE zeroes[sizeof(MacAddress)];
   memset(zeroes, 0, sizeof(zeroes));
   MacAddress z(0);
   AssertTrue(!memcmp(&z, zeroes, sizeof(MacAddress)));
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
   AssertEquals(text, _T("10.217.11.43"));
   EndTest();

   StartTest(_T("InetAddress - toString() - IPv6"));
   a = InetAddress::parse("2000:1234::1");
   a.toString(text);
   AssertEquals(text, _T("2000:1234::1"));
   a = InetAddress::parse("2001:47c::");
   a.toString(text);
   AssertEquals(text, _T("2001:47c::"));
   a = InetAddress::parse("2001:47c::df33:16a");
   a.toString(text);
   AssertEquals(text, _T("2001:47c::df33:16a"));
   EndTest();
}

/**
 * Test integer to string conversion
 */
static void TestIntegerToString()
{
   char buffer[64];
   WCHAR wbuffer[64];

   StartTest(_T("IntegerToString (ASCII)"));
   AssertEquals(IntegerToString(127, buffer, 10), "127");
   AssertEquals(IntegerToString(0, buffer, 10), "0");
   AssertEquals(IntegerToString(-3, buffer, 10), "-3");
   AssertEquals(IntegerToString(0555, buffer, 8), "555");
   AssertEquals(IntegerToString(0xFA48, buffer, 16), "fa48");
   AssertEquals(IntegerToString(_ULL(0xF0F1F20102030405), buffer, 16), "f0f1f20102030405");
   EndTest();

   StartTest(_T("IntegerToString (Unicode)"));
   AssertEquals(IntegerToString(127, wbuffer, 10), L"127");
   AssertEquals(IntegerToString(0, wbuffer, 10), L"0");
   AssertEquals(IntegerToString(-3, wbuffer, 10), L"-3");
   AssertEquals(IntegerToString(0555, wbuffer, 8), L"555");
   AssertEquals(IntegerToString(0xFA48, wbuffer, 16), L"fa48");
   AssertEquals(IntegerToString(_ULL(0xF0F1F20102030405), wbuffer, 16), L"f0f1f20102030405");
   EndTest();
}

/**
 * Keys for hash map
 */
typedef char HASH_KEY[6];

/**
 * Long hash key
 */
typedef unsigned char LONG_HASH_KEY[32];

/**
 * Test hash map
 */
static void TestHashMap()
{
   StartTest(_T("HashMap - create"));
   HashMap<HASH_KEY, String> *hashMap = new HashMap<HASH_KEY, String>(Ownership::True);
   AssertEquals(hashMap->size(), 0);
   EndTest();

   StartTest(_T("HashMap - iterator on empty map"));
   auto it = hashMap->begin();
   AssertFalse(it.hasNext());
   AssertNull(it.next());
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
   AssertEquals(s->cstr(), _T("String 1"));

   s = hashMap->get(k2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 2"));

   s = hashMap->get(k3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 3"));

   EndTest();

   StartTest(_T("HashMap - replace"));
   hashMap->set(k2, new String(_T("REPLACE")));
   s = hashMap->get(k2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("REPLACE"));
   EndTest();

   StartTest(_T("HashMap - iterator"));
   it = hashMap->begin();
   AssertTrue(it.hasNext());
   s = it.next();
   AssertNotNull(s);
   AssertNotNull(it.next());
   AssertNotNull(it.next());
   AssertFalse(it.hasNext());
   AssertNull(it.next());
   AssertFalse(it.hasNext());
   EndTest();

   StartTest(_T("HashMap - iterator for loop"));
   int64_t start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it = hashMap->begin(); it != hashMap->end(); it++, i++)
      {
         AssertNotNull(it.value());
         if(i == 0)
         {
            AssertTrue(it.value()->equals(_T("String 1")));
         }
         if(i == 1)
         {
            AssertTrue(it.value()->equals(_T("REPLACE")));
         }
         if(i == 2)
         {
            AssertTrue(it.value()->equals(_T("String 3")));   
         }
      }
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("HashMap - range based for"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(String *str : *hashMap)
      {
         if (i == 0)
         {
            AssertTrue(str->equals(_T("String 1")));
         }
         else if (i == 1)
         {
            AssertTrue(str->equals(_T("REPLACE")));
         }
         else if (i == 2)
         {
            AssertTrue(str->equals(_T("String 3")));
         }
         i++;
      }
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("HashMap - iterator remove"));
   it = hashMap->begin();
   AssertTrue(it.hasNext());
   AssertNotNull(it.next());
   s = it.next();
   AssertNotNull(s);
   it.remove();
   AssertTrue(it.hasNext());
   AssertNotNull(it.next());
   AssertFalse(it.hasNext());
   AssertNull(it.next());
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
   it = hashMap->begin();
   AssertFalse(it.hasNext());
   AssertNull(it.next());
   EndTest();

   delete hashMap;

   StartTest(_T("HashMap - InetAddress as key"));

   InetAddress addr1 = InetAddress::parse("127.0.0.1");
   InetAddress addr2 = InetAddress::parse("10.0.0.76");
   InetAddress addr3 = InetAddress::parse("172.17.11.2");
   InetAddress addr4 = InetAddress::parse("fe80::250:56ff:fec0:8");

   HashMap<InetAddress, String> *ipAddrMap = new HashMap<InetAddress, String>(Ownership::True);
   ipAddrMap->set(addr1, new String(_T("addr1")));
   ipAddrMap->set(addr2, new String(_T("addr2")));
   ipAddrMap->set(addr3, new String(_T("addr3")));
   ipAddrMap->set(addr4, new String(_T("addr4")));

   s = ipAddrMap->get(addr2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("addr2"));

   s = ipAddrMap->get(addr3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("addr3"));

   s = ipAddrMap->get(addr4);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("addr4"));

   ipAddrMap->set(addr3, new String(_T("addr3_replaced")));
   s = ipAddrMap->get(addr3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("addr3_replaced"));

   AssertEquals(ipAddrMap->size(), 4);

   delete ipAddrMap;
   EndTest();

   StartTest(_T("HashMap - long key"));

   LONG_HASH_KEY lk1, lk2, lk3, lk4;
   RAND_bytes(lk1, sizeof(LONG_HASH_KEY));
   RAND_bytes(lk2, sizeof(LONG_HASH_KEY));
   RAND_bytes(lk3, sizeof(LONG_HASH_KEY));
   RAND_bytes(lk4, sizeof(LONG_HASH_KEY));

   HashMap<LONG_HASH_KEY, String> *longKeyMap = new HashMap<LONG_HASH_KEY, String>(Ownership::True);
   longKeyMap->set(lk1, new String(_T("key1")));
   longKeyMap->set(lk2, new String(_T("key2")));
   longKeyMap->set(lk3, new String(_T("key3")));
   longKeyMap->set(lk4, new String(_T("key4")));

   s = longKeyMap->get(lk2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key2"));

   s = longKeyMap->get(lk3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key3"));

   s = longKeyMap->get(lk4);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key4"));

   longKeyMap->set(lk3, new String(_T("key3_replaced")));
   s = longKeyMap->get(lk3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key3_replaced"));

   AssertEquals(longKeyMap->size(), 4);

   delete longKeyMap;
   EndTest();
}

/**
 * Test shared hash map
 */
static void TestSharedHashMap()
{
   StartTest(_T("SharedHashMap - create"));
   auto sharedHashMap = new SharedHashMap<HASH_KEY, String>();
   AssertEquals(sharedHashMap->size(), 0);
   EndTest();

   HASH_KEY k1 = { '1', '2', '3', '4', '5', '6' };
   HASH_KEY k2 = { '0', '0', 'a', 'b', 'c', 'd' };
   HASH_KEY k3 = { '0', '0', '3', 'X', '1', '1' };
   HASH_KEY k4 = { '1', '0', '3', 'X', '1', '1' };

   StartTest(_T("SharedHashMap - set/get"));

   sharedHashMap->set(k1, new String(_T("String 1")));
   sharedHashMap->set(k2, new String(_T("String 2")));
   sharedHashMap->set(k3, new String(_T("String 3")));
   AssertEquals(sharedHashMap->size(), 3);

   String *s = sharedHashMap->get(k1);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 1"));

   s = sharedHashMap->get(k2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 2"));

   s = sharedHashMap->get(k3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 3"));

   s = sharedHashMap->get(k4);
   AssertNull(s);

   EndTest();

   StartTest(_T("SharedHashMap - remove"));
   sharedHashMap->remove(k1);
   AssertEquals(sharedHashMap->size(), 2);
   s = sharedHashMap->get(k1);
   AssertNull(s);
   EndTest();

   StartTest(_T("SharedHashMap - get shared"));
   shared_ptr<String> shared = sharedHashMap->getShared(k2);
   AssertEquals((int)shared.use_count(), 2);
   delete sharedHashMap;
   AssertEquals((int)shared.use_count(), 1);
   AssertEquals(shared->cstr(), _T("String 2"));
   EndTest();
}

/**
 * Test shared hash map
 */
static void TestSynchronizedSharedHashMap()
{
   StartTest(_T("SynchronizedSharedHashMap - create"));
   SynchronizedSharedHashMap<HASH_KEY, String> *hashMap = new SynchronizedSharedHashMap<HASH_KEY, String>();
   AssertEquals(hashMap->size(), 0);
   EndTest();

   HASH_KEY k1 = { '1', '2', '3', '4', '5', '6' };
   HASH_KEY k2 = { '0', '0', 'a', 'b', 'c', 'd' };
   HASH_KEY k3 = { '0', '0', '3', 'X', '1', '1' };
   HASH_KEY k4 = { '1', '0', '3', 'X', '1', '1' };

   StartTest(_T("SynchronizedSharedHashMap - set/get"));

   hashMap->set(k1, new String(_T("String 1")));
   hashMap->set(k2, new String(_T("String 2")));
   hashMap->set(k3, new String(_T("String 3")));
   AssertEquals(hashMap->size(), 3);

   shared_ptr<String> s = hashMap->getShared(k1);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 1"));

   s = hashMap->getShared(k2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 2"));

   s = hashMap->getShared(k3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("String 3"));

   s = hashMap->getShared(k4);
   AssertNull(s);

   EndTest();

   StartTest(_T("SynchronizedSharedHashMap - remove"));
   hashMap->remove(k1);
   AssertEquals(hashMap->size(), 2);
   s = hashMap->getShared(k1);
   AssertNull(s);
   EndTest();

   StartTest(_T("SynchronizedSharedHashMap - get shared"));
   shared_ptr<String> shared = hashMap->getShared(k2);
   AssertEquals((int)shared.use_count(), 2);
   delete hashMap;
   AssertEquals((int)shared.use_count(), 1);
   AssertEquals(shared->cstr(), _T("String 2"));
   EndTest();

   StartTest(_T("SynchronizedSharedHashMap - long key"));

   LONG_HASH_KEY lk1, lk2, lk3, lk4;
   RAND_bytes(lk1, sizeof(LONG_HASH_KEY));
   RAND_bytes(lk2, sizeof(LONG_HASH_KEY));
   RAND_bytes(lk3, sizeof(LONG_HASH_KEY));
   RAND_bytes(lk4, sizeof(LONG_HASH_KEY));

   SynchronizedSharedHashMap<LONG_HASH_KEY, String> *longKeyMap = new SynchronizedSharedHashMap<LONG_HASH_KEY, String>();
   longKeyMap->set(lk1, make_shared<String>(_T("key1")));
   longKeyMap->set(lk2, make_shared<String>(_T("key2")));
   longKeyMap->set(lk3, make_shared<String>(_T("key3")));
   longKeyMap->set(lk4, make_shared<String>(_T("key4")));

   s = longKeyMap->getShared(lk2);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key2"));

   s = longKeyMap->getShared(lk3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key3"));

   s = longKeyMap->getShared(lk4);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key4"));

   longKeyMap->set(lk3, make_shared<String>(_T("key3_replaced")));
   s = longKeyMap->getShared(lk3);
   AssertNotNull(s);
   AssertEquals(s->cstr(), _T("key3_replaced"));

   AssertEquals(longKeyMap->size(), 4);

   delete longKeyMap;
   EndTest();
}

/**
 * Test hash set
 */
static void TestHashSet()
{
   StartTest(_T("HashSet - create"));
   HashSet<int32_t> *s1 = new HashSet<int32_t>();
   AssertEquals(s1->size(), 0);
   EndTest();

   StartTest(_T("HashSet - iterator on empty set"));
   auto it = s1->begin();
   AssertFalse(it.hasNext());
   AssertNull(it.next());
   EndTest();

   StartTest(_T("HashSet - put/contains"));
   s1->put(1);
   s1->put(10);
   s1->put(33);
   s1->put(10);
   std::vector<int32_t> addVector1 = {10, 100};
   s1->putAll(addVector1);
   AssertEquals(s1->size(), 4);
   AssertTrue(s1->contains(1));
   AssertTrue(s1->contains(10));
   AssertTrue(s1->contains(33));
   AssertFalse(s1->contains(12));
   AssertTrue(s1->contains(100));
   EndTest();

   StartTest(_T("HashSet - copy constructor"));
   s1->put(55);
   HashSet<int32_t> *s1Copy = new HashSet<int32_t>(*s1);
   s1->put(66);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));  
   AssertEquals(s1Copy->size(), 5);
   AssertTrue(s1Copy->contains(100));
   AssertTrue(s1Copy->contains(55)); 
   s1Copy->remove(55);
   AssertTrue(s1->contains(55)); 
   AssertFalse(s1Copy->contains(55)); 
   delete s1Copy;
   s1->remove(55);
   s1->remove(66);
   EndTest();

   StartTest(_T("HashSet - move constructor"));
   s1->put(55);
   HashSet<int32_t> s1MoveConstructor = std::move(HashSet<int32_t>(*s1));
   s1->put(66);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));   
   s1->remove(55);
   s1->remove(66);
   
   AssertEquals(s1MoveConstructor.size(), 5);
   AssertTrue(s1MoveConstructor.contains(55));
   s1MoveConstructor.remove(55);
   EndTest();
   
   StartTest(_T("HashSet - copy operator"));
   s1->put(55);
   HashSet<int32_t> s1CopyOp = *s1;
   s1->put(66);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));   
   s1->remove(55);
   s1->remove(66);
   
   AssertEquals(s1CopyOp.size(), 5);
   AssertTrue(s1CopyOp.contains(55));
   s1CopyOp.remove(55);
   EndTest();
   
   StartTest(_T("HashSet - move operator"));
   s1->put(55);   
   HashSet<int32_t> s1MoveOp;
   HashSet<int32_t> x(*s1);
   s1MoveOp.put(77);
   s1MoveOp = std::move(x);
   s1->put(66);
   AssertEquals(x.size(), 0);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));   
   s1->remove(55);
   s1->remove(66);
   
   AssertEquals(s1MoveOp.size(), 5);
   AssertTrue(s1MoveOp.contains(55));
   s1MoveOp.remove(55);
   EndTest();

   StartTest(_T("HashSet - iterator"));
   int64_t start = GetCurrentTimeMs();
   {
      int i = 0;
      auto it = s1->begin();
      AssertTrue(it.hasNext());
      while(it.hasNext())
      {
         AssertTrue(*it.next() != 0);
         i++;
      }
      AssertEquals(i, s1->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("HashSet - iterator for loop"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it = s1->begin(); it != s1->end(); it++)
      {
         AssertTrue(*it.value() != 0);
         i++;
      }
      AssertEquals(i, s1->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("HashSet - range for"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it : *s1)
      {
         AssertTrue(it != 0);
         i++;
      }
      AssertEquals(i, s1->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("HashSet - remove"));
   std::vector<int32_t> removeVector1 = {200, 100, 300};
   s1->removeAll(removeVector1);
   s1->remove(25);
   s1->remove(33);
   AssertEquals(s1->size(), 2);
   AssertTrue(s1->contains(10));
   AssertFalse(s1->contains(33));
   AssertFalse(s1->contains(200));
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
   LONG_HASH_KEY k5 = "Additional key";
   LONG_HASH_KEY k6 = "Additional key2";

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
   
   StartTest(_T("HashSet (long key) - copy constructor"));
   s2->put(k5);
   HashSet<LONG_HASH_KEY> *s2Copy = new HashSet<LONG_HASH_KEY>(*s2);
   s2->put(k6);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));  
   AssertEquals(s2Copy->size(), 4);
   AssertTrue(s2Copy->contains(k1));
   AssertTrue(s2Copy->contains(k5)); 
   s2Copy->remove(k5);
   AssertTrue(s2->contains(k5)); 
   AssertFalse(s2Copy->contains(k5)); 
   delete s2Copy;
   s2->remove(k5);
   s2->remove(k6);
   EndTest();

   StartTest(_T("HashSet (long key) - move constructor"));
   s2->put(k5);
   HashSet<LONG_HASH_KEY> s2MoveConstructor = std::move(HashSet<LONG_HASH_KEY>(*s2));
   s2->put(k6);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));   
   s2->remove(k5);
   s2->remove(k6);
   
   AssertEquals(s2MoveConstructor.size(), 4);
   AssertTrue(s2MoveConstructor.contains(k5));
   s2MoveConstructor.remove(k5);
   EndTest();
   
   StartTest(_T("HashSet (long key) - copy operator"));
   s2->put(k5);
   HashSet<LONG_HASH_KEY> s2CopyOp = *s2;
   s2->put(k6);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));   
   s2->remove(k5);
   s2->remove(k6);
   
   AssertEquals(s2CopyOp.size(), 4);
   AssertTrue(s2CopyOp.contains(k5));
   s2CopyOp.remove(k5);
   EndTest();
   
   StartTest(_T("HashSet (long key) - move operator"));
   s2->put(k5);   
   HashSet<LONG_HASH_KEY> s2MoveOp;
   HashSet<LONG_HASH_KEY> x2(*s2);
   s2MoveOp.put(k4);
   s2MoveOp = std::move(x2);
   s2->put(k6);
   AssertEquals(x.size(), 0);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));   
   s2->remove(k5);
   s2->remove(k6);
   
   AssertEquals(s2MoveOp.size(), 4);
   AssertTrue(s2MoveOp.contains(k5));
   s2MoveOp.remove(k5);
   EndTest();   

   StartTest(_T("HashSet (long key) - remove"));
   s2->remove(k2);
   s2->remove(k4);
   AssertEquals(s2->size(), 2);
   AssertTrue(s2->contains(k1));
   AssertFalse(s2->contains(k2));
   AssertFalse(s2->contains(k5));
   AssertFalse(s2->contains(k6));
   EndTest();

   StartTest(_T("HashSet (long key) - clear"));
   s2->clear();
   AssertEquals(s2->size(), 0);
   AssertFalse(s2->contains(k1));
   EndTest();

   delete s2;
}

/**
 * Test hash set
 */
static void TestCountingHashSet()
{
   StartTest(_T("CountingHashSet - create"));
   CountingHashSet<int32_t> *s1 = new CountingHashSet<int32_t>();
   AssertEquals(s1->size(), 0);
   EndTest();

   StartTest(_T("CountingHashSet - iterator on empty set"));
   auto it = s1->begin();
   AssertFalse(it.hasNext());
   AssertNull(it.next());
   EndTest();

   StartTest(_T("CountingHashSet - put/contains"));
   s1->put(1);
   s1->put(10);
   s1->put(33);
   s1->put(10);
   std::vector<int32_t> addVector1 = {10, 100};
   s1->putAll(addVector1);
   AssertEquals(s1->size(), 4);
   AssertTrue(s1->contains(1));
   AssertTrue(s1->contains(10));
   AssertTrue(s1->contains(33));
   AssertFalse(s1->contains(12));
   AssertTrue(s1->contains(100));
   EndTest();

   StartTest(_T("CountingHashSet - copy constructor"));
   s1->put(55);
   CountingHashSet<int32_t> *s1Copy = new CountingHashSet<int32_t>(*s1);
   s1->put(66);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));  
   AssertEquals(s1Copy->size(), 5);
   AssertTrue(s1Copy->contains(100));
   AssertTrue(s1Copy->contains(55)); 
   s1Copy->remove(55);
   AssertTrue(s1->contains(55)); 
   AssertFalse(s1Copy->contains(55)); 
   delete s1Copy;
   s1->remove(55);
   s1->remove(66);
   EndTest();

   StartTest(_T("CountingHashSet - move constructor"));
   s1->put(55);
   CountingHashSet<int32_t> s1MoveConstructor = std::move(CountingHashSet<int32_t>(*s1));
   s1->put(66);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));   
   s1->remove(55);
   s1->remove(66);
   
   AssertEquals(s1MoveConstructor.size(), 5);
   AssertTrue(s1MoveConstructor.contains(55));
   s1MoveConstructor.remove(55);
   EndTest();
   
   StartTest(_T("CountingHashSet - copy operator"));
   s1->put(55);
   CountingHashSet<int32_t> s1CopyOp = *s1;
   s1->put(66);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));   
   s1->remove(55);
   s1->remove(66);
   
   AssertEquals(s1CopyOp.size(), 5);
   AssertTrue(s1CopyOp.contains(55));
   AssertTrue(s1CopyOp.contains(10));
   s1CopyOp.remove(10);
   AssertTrue(s1CopyOp.contains(10));
   s1CopyOp.remove(10);
   s1CopyOp.remove(10);
   AssertFalse(s1CopyOp.contains(10));
   s1CopyOp.remove(55);
   EndTest();
   
   StartTest(_T("CountingHashSet - move operator"));
   s1->put(55);
   CountingHashSet<int32_t> s1MoveOp;
   CountingHashSet<int32_t> x(*s1);
   s1MoveOp.put(77);
   s1MoveOp = std::move(x);
   s1->put(66);
   AssertEquals(x.size(), 0);
   AssertEquals(s1->size(), 6);
   AssertTrue(s1->contains(55));
   AssertTrue(s1->contains(66));   
   s1->remove(55);
   s1->remove(66);
   
   AssertEquals(s1MoveOp.size(), 5);
   AssertTrue(s1MoveOp.contains(55));
   AssertFalse(s1MoveOp.contains(77));
   s1MoveOp.remove(55);
   EndTest();

   StartTest(_T("CountingHashSet - iterator"));
   int64_t start = GetCurrentTimeMs();
   {
      int i = 0;
      auto it = s1->begin();
      AssertTrue(it.hasNext());
      while(it.hasNext())
      {
         AssertTrue(*it.next() != 0);
         i++;
      }
      AssertEquals(i, s1->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("CountingHashSet - iterator for loop"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it = s1->begin(); it != s1->end(); it++)
      {
         AssertTrue(*it.value() != 0);
         i++;
      }
      AssertEquals(i, s1->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("CountingHashSet - range for"));
   start = GetCurrentTimeMs();
   {
      int i = 0;
      for(auto it : *s1)
      {
         AssertTrue(it != 0);
         i++;
      }
      AssertEquals(i, s1->size());
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("CountingHashSet - remove"));
   std::vector<int32_t> removeVector1 = {200, 100, 300};
   s1->removeAll(removeVector1);
   s1->remove(25);
   s1->remove(33);
   s1->remove(10);
   s1->remove(10);
   AssertEquals(s1->size(), 2);
   AssertTrue(s1->contains(10));
   AssertFalse(s1->contains(33));
   AssertFalse(s1->contains(200));   
   s1->remove(10);
   AssertFalse(s1->contains(10));
   EndTest();

   StartTest(_T("CountingHashSet - clear"));
   s1->clear();
   AssertEquals(s1->size(), 0);
   AssertFalse(s1->contains(1));
   EndTest();

   delete s1;

   LONG_HASH_KEY k1 = "alpha";
   LONG_HASH_KEY k2 = "omega";
   LONG_HASH_KEY k3 = "some long key";
   LONG_HASH_KEY k4 = "unused key";
   LONG_HASH_KEY k5 = "Additional key";
   LONG_HASH_KEY k6 = "Additional key2";

   StartTest(_T("CountingHashSet (long key) - create"));
   CountingHashSet<LONG_HASH_KEY> *s2 = new CountingHashSet<LONG_HASH_KEY>();
   AssertEquals(s2->size(), 0);
   EndTest();

   StartTest(_T("CountingHashSet (long key) - put/contains"));
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
   
   StartTest(_T("CountingHashSet (long key) - copy constructor"));
   s2->put(k5);
   CountingHashSet<LONG_HASH_KEY> *s2Copy = new CountingHashSet<LONG_HASH_KEY>(*s2);
   s2->put(k6);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));  
   AssertEquals(s2Copy->size(), 4);
   AssertTrue(s2Copy->contains(k1));
   AssertTrue(s2Copy->contains(k5)); 
   s2Copy->remove(k5);
   AssertTrue(s2->contains(k5)); 
   AssertFalse(s2Copy->contains(k5)); 
   delete s2Copy;
   s2->remove(k5);
   s2->remove(k6);
   EndTest();

   StartTest(_T("CountingHashSet (long key) - move constructor"));
   s2->put(k5);
   CountingHashSet<LONG_HASH_KEY> s2MoveConstructor = std::move(CountingHashSet<LONG_HASH_KEY>(*s2));
   s2->put(k6);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));   
   s2->remove(k5);
   s2->remove(k6);
   
   AssertEquals(s2MoveConstructor.size(), 4);
   AssertTrue(s2MoveConstructor.contains(k5));
   s2MoveConstructor.remove(k5);
   EndTest();
   
   StartTest(_T("CountingHashSet (long key) - copy operator"));
   s2->put(k5);
   CountingHashSet<LONG_HASH_KEY> s2CopyOp = *s2;
   s2->put(k6);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));   
   s2->remove(k5);
   s2->remove(k6);
   
   AssertEquals(s2CopyOp.size(), 4);
   AssertTrue(s2CopyOp.contains(k5));
   s2CopyOp.remove(k5);
   EndTest();
   
   StartTest(_T("CountingHashSet (long key) - move operator"));
   s2->put(k5);
   CountingHashSet<LONG_HASH_KEY> s2MoveOp;
   CountingHashSet<LONG_HASH_KEY> x2(*s2);
   s2MoveOp.put(k4);
   s2MoveOp = std::move(x2);
   s2->put(k6);
   AssertEquals(x.size(), 0);
   AssertEquals(s2->size(), 5);
   AssertTrue(s2->contains(k5));
   AssertTrue(s2->contains(k6));   
   s2->remove(k5);
   s2->remove(k6);
   
   AssertEquals(s2MoveOp.size(), 4);
   AssertTrue(s2MoveOp.contains(k5));
   s2MoveOp.remove(k5);
   EndTest();   

   StartTest(_T("CountingHashSet (long key) - remove"));
   s2->remove(k2);
   s2->remove(k4);
   AssertEquals(s2->size(), 3);
   AssertTrue(s2->contains(k2));
   s2->remove(k2);
   AssertEquals(s2->size(), 2);
   AssertTrue(s2->contains(k1));
   AssertFalse(s2->contains(k2));
   AssertFalse(s2->contains(k5));
   AssertFalse(s2->contains(k6));
   EndTest();

   StartTest(_T("CountingHashSet (long key) - clear"));
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
   ObjectArray<String> *array = new ObjectArray<String>(16, 16, Ownership::True);
   AssertEquals(array->size(), 0);
   EndTest();

   StartTest(_T("ObjectArray: iterator on empty array"));
   auto it = array->begin();
   AssertFalse(it.hasNext());
   AssertNull(it.next());
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
   AssertTrue(!_tcscmp(array->get(1)->cstr(), _T("value 2")));
   EndTest();

   StartTest(_T("ObjectArray: replace"));
   array->replace(0, new String(_T("replace")));
   AssertEquals(array->size(), 5);
   AssertTrue(!_tcscmp(array->get(0)->cstr(), _T("replace")));
   EndTest();

   StartTest(_T("ObjectArray: remove"));
   array->remove(0);
   AssertEquals(array->size(), 4);
   AssertTrue(!_tcscmp(array->get(0)->cstr(), _T("value 2")));
   array->remove(3);
   AssertEquals(array->size(), 3);
   AssertTrue(!_tcscmp(array->get(2)->cstr(), _T("value 4")));
   EndTest();

   StartTest(_T("ObjectArray: iterator"));
   it = array->begin();
   AssertTrue(it.hasNext());
   String *s = it.next();
   AssertTrue(!_tcscmp(s->cstr(), _T("value 2")));
   s = it.next();
   AssertTrue(!_tcscmp(s->cstr(), _T("value 3")));
   s = it.next();
   AssertTrue(!_tcscmp(s->cstr(), _T("value 4")));
   s = it.next();
   AssertNull(s);
   EndTest();

   StartTest(_T("ObjectArray: iterator for loop"));
   int64_t start = GetCurrentTimeMs();
   {
      int i = 2;
      for(auto it = array->begin(); it != array->end(); it++)
      {
         AssertNotNull(it.value());
         TCHAR value[64];
         _sntprintf(value, 64, _T("value %d"), i++);
         AssertTrue(!_tcscmp(it.value()->cstr(), value));
      }
      AssertEquals(i, 5);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("ObjectArray: range for")); //C++ 11 only
   start = GetCurrentTimeMs();
   {
      int i = 2;
      for(auto s : *array)
      {
         TCHAR value[64];
         _sntprintf(value, 64, _T("value %d"), i++);
         AssertTrue(!_tcscmp(s->cstr(), value));
      }
      AssertEquals(i, 5);
   }
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("ObjectArray: remove with iterator"));
   it = array->begin();
   AssertTrue(it.hasNext());
   while(it.hasNext())
   {
      String *s = it.next();
      if (!_tcscmp(s->cstr(), _T("value 4")))
      {
         it.remove();
      }
   }
   AssertEquals(array->size(), 2);
   AssertTrue(!_tcscmp(array->get(0)->cstr(), _T("value 2")));
   AssertTrue(!_tcscmp(array->get(1)->cstr(), _T("value 3")));
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
   AssertTrue(!_tcscmp(array->get(1)->cstr(), _T("value 2")));
   EndTest();

   StartTest(_T("SharedObjectArray: replace"));
   array->replace(0, new String(_T("replace")));
   AssertEquals(array->size(), 5);
   AssertTrue(!_tcscmp(array->get(0)->cstr(), _T("replace")));
   EndTest();

   StartTest(_T("SharedObjectArray: remove"));
   array->remove(0);
   AssertEquals(array->size(), 4);
   AssertTrue(!_tcscmp(array->get(0)->cstr(), _T("value 2")));
   array->remove(3);
   AssertEquals(array->size(), 3);
   AssertTrue(!_tcscmp(array->get(2)->cstr(), _T("value 4")));
   EndTest();

   StartTest(_T("SharedObjectArray: iterator"));
   SharedPtrIterator<String> it = array->begin();
   AssertTrue(it.hasNext());
   shared_ptr<String> s = it.next();
   AssertTrue(!_tcscmp(s->cstr(), _T("value 2")));
   s = it.next();
   AssertTrue(!_tcscmp(s->cstr(), _T("value 3")));
   s = it.next();
   AssertTrue(!_tcscmp(s->cstr(), _T("value 4")));
   s = it.next();
   AssertNull(s);
   EndTest();

   StartTest(_T("SharedObjectArray: range for"));
   int index = 2;
   for(shared_ptr<String> sp : *array)
   {
      TCHAR testValue[16];
      _sntprintf(testValue, 16, _T("value %d"), index++);
      AssertTrue(!_tcscmp(sp->cstr(), testValue));
   }
   AssertEquals(index, 5);
   EndTest();

   StartTest(_T("SharedObjectArray: remove with iterator"));
   it = array->begin();
   AssertTrue(it.hasNext());
   while(it.hasNext())
   {
      shared_ptr<String> s = it.next();
      if (!_tcscmp(s->cstr(), _T("value 4")))
      {
         it.remove();
      }
   }
   AssertEquals(array->size(), 2);
   AssertEquals(array->get(0)->cstr(), _T("value 2"));
   AssertEquals(array->get(1)->cstr(), _T("value 3"));
   EndTest();

   StartTest(_T("SharedObjectArray: get shared"));
   s = array->getShared(0);
   AssertEquals((int)s.use_count(), 2);
   SharedObjectArray<String> *array2 = new SharedObjectArray<String>(16, 16);
   array2->add(array->getShared(0));
   AssertEquals((int)s.use_count(), 3);
   delete array;
   AssertEquals((int)s.use_count(), 2);
   delete array2;
   AssertEquals((int)s.use_count(), 1);
   AssertEquals(s->cstr(), _T("value 2"));
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
   table->setPreallocated(1, MemCopyString(_T("test")));
   AssertEquals(table->getNumRows(), 0);
   EndTest();

   StartTest(_T("Table: add row"));
   table->addRow();
   AssertEquals(table->getNumRows(), 1);
   AssertEquals(table->getNumColumns(), 0);
   EndTest();

   StartTest(_T("Table: set on empty row"));
   table->set(0, _T("test"));
   table->setPreallocated(1, MemCopyString(_T("test")));
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
   int64_t start = GetCurrentTimeMs();
   char *packedTable = table->toPackedXML();
   AssertNotNull(packedTable);
   EndTest(GetCurrentTimeMs() - start);

   StartTest(_T("Table: unpack"));
   start = GetCurrentTimeMs();
   Table *table2 = Table::createFromPackedXML(packedTable);
   MemFree(packedTable);
   AssertNotNull(table2);
   AssertEquals(table2->getNumColumns(), table->getNumColumns());
   AssertEquals(table2->getNumRows(), table->getNumRows());
   AssertEquals(table2->getAsInt(10, 1), table->getAsInt(10, 1));
   AssertEquals(table2->getAsString(15, 0), table->getAsString(15, 0));
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
   AssertEquals(table->getAsString(52, table->getColumnIndex(_T("DATA5")), _T("")), _T("Data5-2"));
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
   AssertEquals(table->getAsString(53, table->getColumnIndex(_T("DATA6")), _T("")), _T("Data6-1"));
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
   AssertEquals(bswap_16(0xABCD), static_cast<uint16_t>(0xCDAB));
   EndTest();

   StartTest(_T("bswap_32"));
   AssertEquals(bswap_32(0x0102ABCD), 0xCDAB0201u);
   EndTest();

   StartTest(_T("bswap_64"));
   AssertEquals(bswap_64(_ULL(0x01020304A1A2A3A4)), _ULL(0xA4A3A2A104030201));
   EndTest();

   StartTest(_T("bswap_array_16"));
   uint16_t s16[] = { 0xABCD, 0x1020, 0x2233, 0x0102 };
   uint16_t d16[] = { 0xCDAB, 0x2010, 0x3322, 0x0201 };
   bswap_array_16(s16, 4);
   AssertTrue(!memcmp(s16, d16, 8));
   EndTest();

   StartTest(_T("bswap_array_16 (string)"));
   uint16_t ss16[] = { 0xABCD, 0x1020, 0x2233, 0x0102, 0 };
   uint16_t sd16[] = { 0xCDAB, 0x2010, 0x3322, 0x0201, 0 };
   bswap_array_16(ss16, -1);
   AssertTrue(!memcmp(ss16, sd16, 10));
   EndTest();

   StartTest(_T("bswap_array_32"));
   uint32_t s32[] = { 0xABCDEF01, 0x10203040, 0x22334455, 0x01020304 };
   uint32_t d32[] = { 0x01EFCDAB, 0x40302010, 0x55443322, 0x04030201 };
   bswap_array_32(s32, 4);
   AssertTrue(!memcmp(s32, d32, 16));
   EndTest();

   StartTest(_T("bswap_array_32 (string)"));
   uint32_t ss32[] = { 0xABCDEF01, 0x10203040, 0x22334455, 0x01020304, 0 };
   uint32_t sd32[] = { 0x01EFCDAB, 0x40302010, 0x55443322, 0x04030201, 0 };
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
   StringBuffer diff = GenerateLineDiff(diffLeft, diffRight);
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
      int writes = GenerateRandomNumber(1, 10);
      int reads = GenerateRandomNumber(1, 10);
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

#if !WITH_ADDRESS_SANITIZER
   StartTest(_T("nxlog_get_debug_level() performance"));
   UINT64 startTime = GetCurrentTimeMs();
   for(int i = 0; i < 1000000; i++)
      nxlog_get_debug_level();
   EndTest(GetCurrentTimeMs() - startTime);
#endif
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

#if !WITH_ADDRESS_SANITIZER
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
#endif

   StartTest(_T("Debug tags: reset"));
   nxlog_reset_debug_level_tags();
   AssertEquals(nxlog_get_debug_level_tag(_T("test.tag1.subtag1")), nxlog_get_debug_level());
   EndTest();
}

/**
 * Test ShortenFilePathForDisplay
 */
void TestShortenFilePathForDisplay()
{
   static const TCHAR *path = FS_PATH_SEPARATOR _T("usr") FS_PATH_SEPARATOR _T("lib") FS_PATH_SEPARATOR _T("netxms") FS_PATH_SEPARATOR _T("module.so");

   StartTest(_T("ShortenFilePathForDisplay"));
   AssertEquals(ShortenFilePathForDisplay(path, 3), _T("..."));
   AssertEquals(ShortenFilePathForDisplay(path, 10), _T("...dule.so"));
   AssertEquals(ShortenFilePathForDisplay(path, 20), FS_PATH_SEPARATOR _T("usr") FS_PATH_SEPARATOR _T("li...") FS_PATH_SEPARATOR _T("module.so"));
   AssertEquals(ShortenFilePathForDisplay(path, 50), path);
   AssertEquals(ShortenFilePathForDisplay(_T("very_long_file_name"), 10), _T("...le_name"));
   AssertEquals(ShortenFilePathForDisplay(_T("/very_long_file_name"), 10), _T("...le_name"));
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
   bool debug = false;

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
         if ((argc > 2) && !strcmp(argv[2], "-debug"))
         {
            nxlog_open(_T("subprocess.log"), 0);
            nxlog_set_debug_level(9);
         }
         SubProcessMain(argc, argv, TestSubProcessRequestHandler);
         return 0;
      }
      else if (!strcmp(argv[1], "-debug"))
      {
         nxlog_set_debug_writer(DebugWriter);
         debug = true;
      }
   }

#ifdef _WIN32
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

   TestMutex();
   TestUniqueLock();
   TestCondition();
   TestRWLock();
   TestGauge64();
   TestMemoryPool();
   TestObjectMemoryPool();
   TestString();
   TestStringConversion();
   TestStringList();
   TestStringMap();
   TestStringSet();
   TestCountingStringSet();
   TestStringFunctionsA();
   TestStringFunctionsW();
   TestStringToBinaryConversions();
   TestBinaryToStringConversions();
   TestPatternMatching();
   TestShortenFilePathForDisplay();
   TestMessageClass();
   TestMsgWaitQueue();
   TestGenericId();
   TestMacAddress();
   TestInetAddress();
   TestIntegerToString();
   TestQueue();
   TestSharedObjectQueue();
   TestHashMap();
   TestSharedHashMap();
   TestSynchronizedSharedHashMap();
   TestHashSet();
   TestCountingHashSet();
   TestObjectArray();
   TestSharedObjectArray();
   TestTable();
   TestByteSwap();
   TestDiff();
   TestRingBuffer();
   TestDebugLevel();
   TestDebugTags();
   TestGeoLocation();
   TestRSA();
   TestMD4();

   if (debug)
      nxlog_set_debug_level(9);

   TestProcessExecutor(argv[0]);
   TestSubProcess(argv[0], debug);
   TestThreadPool();
   TestThreadPoolDelayedExecution();
   TestThreadCountAndMaxWaitTime();

   InitiateProcessShutdown();

   return 0;
}
