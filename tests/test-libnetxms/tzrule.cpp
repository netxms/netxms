#include <nms_util.h>
#include <testtools.h>

/**
 * Build UTC timestamp from calendar fields
 */
static time_t UTC(int year, int month, int day, int hour = 0, int minute = 0, int second = 0)
{
   struct tm t;
   memset(&t, 0, sizeof(t));
   t.tm_year = year - 1900;
   t.tm_mon = month - 1;
   t.tm_mday = day;
   t.tm_hour = hour;
   t.tm_min = minute;
   t.tm_sec = second;
   return timegm(&t);
}

/**
 * Check local date of given moment
 */
static void AssertLocalDate(const TimeZoneRule& tz, time_t t, int year, int month, int day)
{
   int y, m, d;
   tz.toLocalDate(t, &y, &m, &d);
   AssertEquals(y, year);
   AssertEquals(m, month);
   AssertEquals(d, day);
}

/**
 * Check that clocks switch from offset "before" to offset "after" exactly at given moment
 */
static void AssertTransition(const TimeZoneRule& tz, time_t t, int before, int after)
{
   AssertEquals(tz.offsetAt(t - 1), before);
   AssertEquals(tz.offsetAt(t), after);
}

/**
 * Test fixed offset rules
 */
static void TestFixedOffsets()
{
   StartTest(_T("TimeZoneRule - fixed offsets"));

   TimeZoneRule utc("UTC0");
   AssertTrue(utc.isValid());
   AssertFalse(utc.hasDaylightSaving());
   AssertEquals(utc.getStandardOffset(), 0);
   AssertEquals(utc.getStandardName(), "UTC");
   AssertEquals(utc.offsetAt(UTC(2026, 7, 1)), 0);
   AssertEquals(static_cast<int64_t>(utc.localDayStart(2026, 3, 29)), static_cast<int64_t>(UTC(2026, 3, 29)));
   AssertEquals(utc.toString(), "UTC0");

   TimeZoneRule jst("JST-9");
   AssertTrue(jst.isValid());
   AssertEquals(jst.getStandardOffset(), 32400);
   AssertEquals(jst.offsetAt(0), 32400);
   AssertLocalDate(jst, 0, 1970, 1, 1);
   AssertLocalDate(jst, -1, 1970, 1, 1);   // pre-epoch instant
   AssertLocalDate(jst, UTC(2026, 6, 1, 14, 59, 59), 2026, 6, 1);
   AssertLocalDate(jst, UTC(2026, 6, 1, 15, 0, 0), 2026, 6, 2);
   AssertEquals(static_cast<int64_t>(jst.localDayStart(2026, 6, 2)), static_cast<int64_t>(UTC(2026, 6, 1, 15)));

   TimeZoneRule est("EST5");
   AssertTrue(est.isValid());
   AssertEquals(est.getStandardOffset(), -18000);
   AssertEquals(static_cast<int64_t>(est.localDayStart(2026, 6, 2)), static_cast<int64_t>(UTC(2026, 6, 2, 5)));

   TimeZoneRule tehran("<+0330>-3:30");
   AssertTrue(tehran.isValid());
   AssertEquals(tehran.getStandardName(), "+0330");
   AssertEquals(tehran.getStandardOffset(), 12600);
   AssertLocalDate(tehran, UTC(2026, 6, 1, 20, 29, 59), 2026, 6, 1);
   AssertLocalDate(tehran, UTC(2026, 6, 1, 20, 30, 0), 2026, 6, 2);

   TimeZoneRule minus3("<-03>3");
   AssertTrue(minus3.isValid());
   AssertEquals(minus3.getStandardName(), "-03");
   AssertEquals(minus3.getStandardOffset(), -10800);

   TimeZoneRule withSeconds("LMT-1:39:49");
   AssertTrue(withSeconds.isValid());
   AssertEquals(withSeconds.getStandardOffset(), 5989);

   TimeZoneRule padded("  UTC0  ");
   AssertTrue(padded.isValid());
   AssertEquals(padded.toString(), "UTC0");

   EndTest();
}

/**
 * Test northern hemisphere DST rules
 */
static void TestNorthernDST()
{
   StartTest(_T("TimeZoneRule - northern hemisphere DST"));

   TimeZoneRule riga("EET-2EEST,M3.5.0/3,M10.5.0/4");
   AssertTrue(riga.isValid());
   AssertTrue(riga.hasDaylightSaving());
   AssertEquals(riga.getStandardName(), "EET");
   AssertEquals(riga.getDaylightName(), "EEST");
   AssertEquals(riga.getStandardOffset(), 7200);
   AssertEquals(riga.getDaylightOffset(), 10800);
   AssertEquals(riga.offsetAt(UTC(2026, 1, 15, 12)), 7200);
   AssertEquals(riga.offsetAt(UTC(2026, 7, 15, 12)), 10800);
   AssertFalse(riga.isDaylightSavingAt(UTC(2026, 1, 15, 12)));
   AssertTrue(riga.isDaylightSavingAt(UTC(2026, 7, 15, 12)));
   AssertTransition(riga, UTC(2026, 3, 29, 1), 7200, 10800);    // 03:00 EET -> 04:00 EEST
   AssertTransition(riga, UTC(2026, 10, 25, 1), 10800, 7200);   // 04:00 EEST -> 03:00 EET
   AssertTransition(riga, UTC(2027, 3, 28, 1), 7200, 10800);
   AssertTransition(riga, UTC(2027, 10, 31, 1), 10800, 7200);
   AssertEquals(static_cast<int64_t>(riga.localDayStart(2026, 3, 29)), static_cast<int64_t>(UTC(2026, 3, 28, 22)));    // spring transition day starts in EET
   AssertEquals(static_cast<int64_t>(riga.localDayStart(2026, 3, 30)), static_cast<int64_t>(UTC(2026, 3, 29, 21)));    // next day starts in EEST
   AssertEquals(static_cast<int64_t>(riga.localDayStart(2026, 10, 25)), static_cast<int64_t>(UTC(2026, 10, 24, 21)));  // autumn transition day starts in EEST
   AssertEquals(static_cast<int64_t>(riga.localDayStart(2026, 10, 26)), static_cast<int64_t>(UTC(2026, 10, 25, 22)));  // next day starts in EET
   AssertEquals(static_cast<int64_t>(riga.localDayStart(2026, 1, 1)), static_cast<int64_t>(UTC(2025, 12, 31, 22)));
   AssertEquals(static_cast<int64_t>(riga.localDayStart(2026, 12, 31)), static_cast<int64_t>(UTC(2026, 12, 30, 22)));
   AssertLocalDate(riga, UTC(2026, 3, 28, 21, 59, 59), 2026, 3, 28);
   AssertLocalDate(riga, UTC(2026, 3, 28, 22), 2026, 3, 29);
   AssertLocalDate(riga, UTC(2026, 10, 25, 21, 59, 59), 2026, 10, 25);
   AssertLocalDate(riga, UTC(2026, 10, 25, 22), 2026, 10, 26);

   // Default transition time (02:00) and week 2/week 1 rules
   TimeZoneRule us("EST5EDT,M3.2.0,M11.1.0");
   AssertTrue(us.isValid());
   AssertEquals(us.getStandardOffset(), -18000);
   AssertEquals(us.getDaylightOffset(), -14400);
   AssertTransition(us, UTC(2026, 3, 8, 7), -18000, -14400);   // 02:00 EST -> 03:00 EDT
   AssertTransition(us, UTC(2026, 11, 1, 6), -14400, -18000);  // 02:00 EDT -> 01:00 EST

   // Week 5 falling back into the month (last Sunday of February 2026 is the 22nd, of November 2026 the 29th)
   TimeZoneRule lastWeek("EST5EDT,M2.5.0,M11.5.0");
   AssertTrue(lastWeek.isValid());
   AssertTransition(lastWeek, UTC(2026, 2, 22, 7), -18000, -14400);
   AssertTransition(lastWeek, UTC(2026, 11, 29, 6), -14400, -18000);

   EndTest();
}

/**
 * Test southern hemisphere DST rules (DST spans the year boundary)
 */
static void TestSouthernDST()
{
   StartTest(_T("TimeZoneRule - southern hemisphere DST"));

   TimeZoneRule sydney("AEST-10AEDT,M10.1.0,M4.1.0/3");
   AssertTrue(sydney.isValid());
   AssertEquals(sydney.offsetAt(UTC(2026, 1, 15)), 39600);
   AssertEquals(sydney.offsetAt(UTC(2026, 7, 15)), 36000);
   AssertEquals(sydney.offsetAt(UTC(2025, 12, 31, 23)), 39600);
   AssertEquals(sydney.offsetAt(UTC(2026, 1, 1, 1)), 39600);
   AssertTransition(sydney, UTC(2026, 4, 4, 16), 39600, 36000);    // Apr 5 03:00 AEDT -> 02:00 AEST
   AssertTransition(sydney, UTC(2026, 10, 3, 16), 36000, 39600);   // Oct 4 02:00 AEST -> 03:00 AEDT
   AssertEquals(static_cast<int64_t>(sydney.localDayStart(2026, 1, 1)), static_cast<int64_t>(UTC(2025, 12, 31, 13)));
   AssertEquals(static_cast<int64_t>(sydney.localDayStart(2026, 4, 5)), static_cast<int64_t>(UTC(2026, 4, 4, 13)));
   AssertEquals(static_cast<int64_t>(sydney.localDayStart(2026, 10, 4)), static_cast<int64_t>(UTC(2026, 10, 3, 14)));
   AssertLocalDate(sydney, UTC(2025, 12, 31, 13), 2026, 1, 1);
   AssertLocalDate(sydney, UTC(2025, 12, 31, 12, 59, 59), 2025, 12, 31);

   TimeZoneRule auckland("NZST-12NZDT,M9.5.0,M4.1.0/3");
   AssertTrue(auckland.isValid());
   AssertTransition(auckland, UTC(2026, 4, 4, 14), 46800, 43200);
   AssertTransition(auckland, UTC(2026, 9, 26, 14), 43200, 46800);

   EndTest();
}

/**
 * Test unusual but valid rules
 */
static void TestSpecialRules()
{
   StartTest(_T("TimeZoneRule - special rules"));

   // Europe/Dublin as defined by IANA: "daylight saving" period is winter with an offset below the standard one
   TimeZoneRule dublin("IST-1GMT0,M10.5.0,M3.5.0/1");
   AssertTrue(dublin.isValid());
   AssertEquals(dublin.getStandardOffset(), 3600);
   AssertEquals(dublin.getDaylightOffset(), 0);
   AssertEquals(dublin.offsetAt(UTC(2026, 1, 15)), 0);
   AssertEquals(dublin.offsetAt(UTC(2026, 7, 15)), 3600);
   AssertTransition(dublin, UTC(2026, 3, 29, 1), 0, 3600);
   AssertTransition(dublin, UTC(2026, 10, 25, 1), 3600, 0);
   AssertEquals(static_cast<int64_t>(dublin.localDayStart(2026, 3, 29)), static_cast<int64_t>(UTC(2026, 3, 29, 0)));
   AssertEquals(static_cast<int64_t>(dublin.localDayStart(2026, 10, 25)), static_cast<int64_t>(UTC(2026, 10, 24, 23)));

   // America/Nuuk as defined by IANA: negative transition hour
   TimeZoneRule nuuk("<-02>2<-01>,M3.5.0/-1,M10.5.0/0");
   AssertTrue(nuuk.isValid());
   AssertTransition(nuuk, UTC(2026, 3, 29, 1), -7200, -3600);
   AssertTransition(nuuk, UTC(2026, 10, 25, 1), -3600, -7200);

   // America/Santiago as defined by IANA: transitions at 24:00, so local midnight is skipped in September
   // and the first Saturday of April lasts 25 hours
   TimeZoneRule santiago("<-04>4<-03>,M9.1.6/24,M4.1.6/24");
   AssertTrue(santiago.isValid());
   AssertTransition(santiago, UTC(2026, 9, 6, 4), -14400, -10800);   // Sep 5 24:00 -04 -> Sep 6 01:00 -03
   AssertTransition(santiago, UTC(2026, 4, 5, 3), -10800, -14400);   // Apr 4 24:00 -03 -> Apr 4 23:00 -04
   AssertLocalDate(santiago, UTC(2026, 9, 6, 3, 59, 59), 2026, 9, 5);
   AssertLocalDate(santiago, UTC(2026, 9, 6, 4), 2026, 9, 6);
   AssertLocalDate(santiago, UTC(2026, 4, 5, 3, 59, 59), 2026, 4, 4);
   AssertLocalDate(santiago, UTC(2026, 4, 5, 4), 2026, 4, 5);
   AssertEquals(static_cast<int64_t>(santiago.localDayStart(2026, 9, 6)), static_cast<int64_t>(UTC(2026, 9, 6, 4)));   // midnight skipped: day starts at 01:00 local
   AssertEquals(static_cast<int64_t>(santiago.localDayStart(2026, 9, 7)), static_cast<int64_t>(UTC(2026, 9, 7, 3)));
   AssertEquals(static_cast<int64_t>(santiago.localDayStart(2026, 4, 4)), static_cast<int64_t>(UTC(2026, 4, 4, 3)));
   AssertEquals(static_cast<int64_t>(santiago.localDayStart(2026, 4, 5)), static_cast<int64_t>(UTC(2026, 4, 5, 4)));
   AssertEquals(static_cast<int64_t>(santiago.localDayStart(2026, 4, 6)), static_cast<int64_t>(UTC(2026, 4, 6, 4)));

   // Synthetic rule with transitions at 00:00 and 01:00: spring skips midnight, autumn repeats it
   TimeZoneRule midnight("XST-2XDT,M3.5.0/0,M10.5.0/1");
   AssertTrue(midnight.isValid());
   AssertTransition(midnight, UTC(2026, 3, 28, 22), 7200, 10800);   // 00:00 XST -> 01:00 XDT
   AssertTransition(midnight, UTC(2026, 10, 24, 22), 10800, 7200);  // 01:00 XDT -> 00:00 XST
   AssertEquals(static_cast<int64_t>(midnight.localDayStart(2026, 3, 29)), static_cast<int64_t>(UTC(2026, 3, 28, 22)));    // transition instant
   AssertEquals(static_cast<int64_t>(midnight.localDayStart(2026, 10, 25)), static_cast<int64_t>(UTC(2026, 10, 24, 21)));  // earlier of the two midnights
   AssertLocalDate(midnight, UTC(2026, 10, 24, 20, 59, 59), 2026, 10, 24);
   AssertLocalDate(midnight, UTC(2026, 10, 24, 21), 2026, 10, 25);

   // Pacific/Apia as defined by IANA: offsets beyond +12 push the local date a full day ahead of UTC
   TimeZoneRule apia("<+13>-13<+14>,M9.5.0/3,M4.1.0/4");
   AssertTrue(apia.isValid());
   AssertEquals(apia.offsetAt(UTC(2026, 1, 1, 10)), 50400);
   AssertEquals(apia.offsetAt(UTC(2026, 7, 1, 10)), 46800);
   AssertLocalDate(apia, UTC(2026, 1, 1, 10), 2026, 1, 2);
   AssertLocalDate(apia, UTC(2026, 7, 1, 10), 2026, 7, 1);
   AssertLocalDate(apia, UTC(2026, 7, 1, 11), 2026, 7, 2);
   AssertEquals(static_cast<int64_t>(apia.localDayStart(2026, 1, 2)), static_cast<int64_t>(UTC(2026, 1, 1, 10)));
   AssertEquals(static_cast<int64_t>(apia.localDayStart(2026, 7, 2)), static_cast<int64_t>(UTC(2026, 7, 1, 11)));

   // Julian day rules ignore February 29, zero-based day-of-year rules count it
   TimeZoneRule julian("EST5EDT,J60,J300");
   AssertTrue(julian.isValid());
   AssertEquals(julian.offsetAt(UTC(2024, 2, 29, 12)), -18000);
   AssertEquals(julian.offsetAt(UTC(2024, 3, 1, 12)), -14400);
   AssertEquals(julian.offsetAt(UTC(2024, 10, 26, 12)), -14400);
   AssertEquals(julian.offsetAt(UTC(2024, 10, 27, 12)), -18000);
   AssertEquals(julian.offsetAt(UTC(2025, 2, 28, 12)), -18000);
   AssertEquals(julian.offsetAt(UTC(2025, 3, 1, 12)), -14400);

   TimeZoneRule dayOfYear("EST5EDT,59,299");
   AssertTrue(dayOfYear.isValid());
   AssertEquals(dayOfYear.offsetAt(UTC(2024, 2, 29, 12)), -14400);
   AssertEquals(dayOfYear.offsetAt(UTC(2024, 3, 1, 12)), -14400);
   AssertEquals(dayOfYear.offsetAt(UTC(2024, 10, 25, 12)), -14400);
   AssertEquals(dayOfYear.offsetAt(UTC(2024, 10, 26, 12)), -18000);
   AssertEquals(dayOfYear.offsetAt(UTC(2025, 2, 28, 12)), -18000);
   AssertEquals(dayOfYear.offsetAt(UTC(2025, 3, 1, 12)), -14400);

   // Explicit DST offset
   TimeZoneRule lordHowe("<+1030>-10:30<+11>-11,M10.1.0,M4.1.0");
   AssertTrue(lordHowe.isValid());
   AssertEquals(lordHowe.getStandardOffset(), 37800);
   AssertEquals(lordHowe.getDaylightOffset(), 39600);

   EndTest();
}

/**
 * Test rejection of malformed rules
 */
static void TestMalformedRules()
{
   StartTest(_T("TimeZoneRule - malformed rules"));

   static const char *malformed[] =
   {
      "",
      "   ",
      "EE-2",                                 // name too short
      "EET",                                  // offset missing
      "EET-25",                               // hour out of range
      "EET-2:60",                             // minutes out of range
      "EET-2EEST",                            // DST name without rules
      "EET-2EEST,M3.5.0/3",                   // one rule only
      "EET-2EEST,M3.5.0/3,",                  // second rule empty
      "EET-2EEST,M13.5.0/3,M10.5.0/4",        // month out of range
      "EET-2EEST,M3.6.0/3,M10.5.0/4",         // week out of range
      "EET-2EEST,M3.0.0/3,M10.5.0/4",         // week out of range
      "EET-2EEST,M3.5.7/3,M10.5.0/4",         // weekday out of range
      "EET-2EEST,M3.5/3,M10.5.0/4",           // incomplete M rule
      "EET-2EEST,J0,J300",                    // Julian day out of range
      "EET-2EEST,J60,J366",                   // Julian day out of range
      "EET-2EEST,59,366",                     // day of year out of range
      "EET-2EEST,M3.5.0/168,M10.5.0/4",       // transition hour out of range
      "EET-2EEST,M3.5.0/3,M10.5.0/4xyz",      // trailing garbage
      "EET-2EEST,M3.5.0/3,M10.5.0/4 ,",       // trailing garbage
      "EET -2EEST,M3.5.0/3,M10.5.0/4",        // internal whitespace
      "<+03",                                 // unterminated quoted name
      "<+03>",                                // offset missing
      "<+0 3>-3",                             // invalid character in quoted name
      ":EET-2",                               // implementation-defined form
      "EET-2/3",                              // stray transition time
      "E3T-2",                                // digit in unquoted name
      nullptr
   };

   for(int i = 0; malformed[i] != nullptr; i++)
   {
      TimeZoneRule tz(malformed[i]);
      AssertFalseEx(tz.isValid(), _T("Malformed rule accepted"));
      AssertEquals(tz.toString(), "");
      AssertEquals(tz.offsetAt(UTC(2026, 7, 1)), 0);   // invalid rule evaluates as UTC
   }

   TimeZoneRule empty;
   AssertFalse(empty.isValid());
   AssertEquals(empty.toString(), "");

   TimeZoneRule fromNull(nullptr);
   AssertFalse(fromNull.isValid());

   // Rule longer than the internal buffer
   char longRule[256];
   memset(longRule, 'A', sizeof(longRule) - 1);
   longRule[sizeof(longRule) - 1] = 0;
   AssertFalse(TimeZoneRule(longRule).isValid());

   EndTest();
}

/**
 * Test reparse, copy and round trip of rule text
 */
static void TestReparseAndCopy()
{
   StartTest(_T("TimeZoneRule - reparse and copy"));

   TimeZoneRule tz("EET-2EEST,M3.5.0/3,M10.5.0/4");
   AssertTrue(tz.isValid());
   AssertEquals(tz.toString(), "EET-2EEST,M3.5.0/3,M10.5.0/4");

   TimeZoneRule copy(tz);
   AssertTrue(copy.isValid());
   AssertEquals(copy.toString(), "EET-2EEST,M3.5.0/3,M10.5.0/4");
   AssertEquals(copy.offsetAt(UTC(2026, 7, 15)), 10800);

   AssertTrue(tz.parse("JST-9"));
   AssertFalse(tz.hasDaylightSaving());
   AssertEquals(tz.offsetAt(UTC(2026, 7, 15)), 32400);
   AssertEquals(copy.offsetAt(UTC(2026, 7, 15)), 10800);   // copy unaffected

   AssertFalse(tz.parse("garbage"));   // failed parse invalidates the object
   AssertFalse(tz.isValid());
   AssertEquals(tz.offsetAt(UTC(2026, 7, 15)), 0);

   TimeZoneRule reparsed(TimeZoneRule("<-04>4<-03>,M9.1.6/24,M4.1.6/24").toString());
   AssertTrue(reparsed.isValid());
   AssertEquals(reparsed.offsetAt(UTC(2026, 1, 15)), -10800);

   EndTest();
}

/**
 * Test POSIX timezone rule evaluation
 */
void TestTimeZoneRule()
{
   TestFixedOffsets();
   TestNorthernDST();
   TestSouthernDST();
   TestSpecialRules();
   TestMalformedRules();
   TestReparseAndCopy();
}
