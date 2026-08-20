#include <nms_util.h>
#include <testtools.h>

/**
 * Convert "YYYY-MM-DD HH:MM:SS" local time to time_t and filled in struct tm
 */
static time_t ParseTestTime(const char *text, struct tm *t)
{
   memset(t, 0, sizeof(struct tm));
   int year, month;
   sscanf(text, "%d-%d-%d %d:%d:%d", &year, &month, &t->tm_mday, &t->tm_hour, &t->tm_min, &t->tm_sec);
   t->tm_year = year - 1900;
   t->tm_mon = month - 1;
   t->tm_isdst = -1;
   time_t now = mktime(t);
#if HAVE_LOCALTIME_R
   localtime_r(&now, t);   // fill in tm_wday
#else
   memcpy(t, localtime(&now), sizeof(struct tm));
#endif
   return now;
}

/**
 * Check schedule match with minute resolution
 */
static void CheckMatch(const TCHAR *schedule, const char *time, bool expected)
{
   struct tm t;
   time_t now = ParseTestTime(time, &t);
   AssertEx(MatchSchedule(schedule, &t, now) == expected, schedule);
}

/**
 * Check schedule match with second resolution
 */
static void CheckMatchWithSeconds(const TCHAR *schedule, const char *time, bool expected, bool expectedWithSeconds)
{
   struct tm t;
   time_t now = ParseTestTime(time, &t);
   bool withSeconds = false;
   AssertEx(MatchScheduleWithSeconds(schedule, &withSeconds, &t, now) == expected, schedule);
   AssertEx(withSeconds == expectedWithSeconds, schedule);
}

/**
 * Test matching of individual schedule fields
 */
void TestScheduleFields()
{
   StartTest(_T("Schedule matching - minute"));
   CheckMatch(_T("30 * * * *"), "2026-08-20 10:30:00", true);
   CheckMatch(_T("30 * * * *"), "2026-08-20 10:31:00", false);
   CheckMatch(_T("*/15 * * * *"), "2026-08-20 10:30:00", true);
   CheckMatch(_T("*/15 * * * *"), "2026-08-20 10:31:00", false);
   CheckMatch(_T("10,30,50 * * * *"), "2026-08-20 10:50:00", true);
   CheckMatch(_T("10,30,50 * * * *"), "2026-08-20 10:40:00", false);
   CheckMatch(_T("10-20 * * * *"), "2026-08-20 10:15:00", true);
   CheckMatch(_T("10-20 * * * *"), "2026-08-20 10:25:00", false);
   EndTest();

   StartTest(_T("Schedule matching - hour"));
   CheckMatch(_T("30 8 * * *"), "2026-08-20 08:30:00", true);
   CheckMatch(_T("30 8 * * *"), "2026-08-20 09:30:00", false);
   CheckMatch(_T("0 9-17 * * *"), "2026-08-20 12:00:00", true);
   CheckMatch(_T("0 9-17 * * *"), "2026-08-20 18:00:00", false);
   EndTest();

   StartTest(_T("Schedule matching - day of month"));
   CheckMatch(_T("0 0 1 * *"), "2026-08-01 00:00:00", true);
   CheckMatch(_T("0 0 1 * *"), "2026-08-02 00:00:00", false);
   CheckMatch(_T("0 0 L * *"), "2026-08-31 00:00:00", true);
   CheckMatch(_T("0 0 L * *"), "2026-08-30 00:00:00", false);
   CheckMatch(_T("0 0 L * *"), "2026-02-28 00:00:00", true);
   EndTest();

   StartTest(_T("Schedule matching - month"));
   CheckMatch(_T("0 0 * 8 *"), "2026-08-20 00:00:00", true);
   CheckMatch(_T("0 0 * 7 *"), "2026-08-20 00:00:00", false);
   CheckMatch(_T("0 0 * 1,7,8 *"), "2026-08-20 00:00:00", true);
   EndTest();
}

/**
 * Test matching of day of week field. Value 7 is an alias for Sunday in addition to 0 (issue #3525).
 * 2026-08-19 is Wednesday, 2026-08-20 is Thursday, 2026-08-23 is Sunday, 2026-08-24 is Monday,
 * 2026-08-30 is last Sunday of the month, 2026-08-31 is last Monday of the month.
 */
void TestScheduleDayOfWeek()
{
   StartTest(_T("Schedule matching - day of week"));
   CheckMatch(_T("* * * * 0"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * 0"), "2026-08-24 10:00:00", false);
   CheckMatch(_T("* * * * 1-5"), "2026-08-20 10:00:00", true);
   CheckMatch(_T("* * * * 1-5"), "2026-08-23 10:00:00", false);
   CheckMatch(_T("* * * * 0,3"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * 0,3"), "2026-08-19 10:00:00", true);
   CheckMatch(_T("* * * * 0,3"), "2026-08-20 10:00:00", false);
   EndTest();

   StartTest(_T("Schedule matching - day of week 7 as alias for Sunday"));
   CheckMatch(_T("* * * * 7"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * 7"), "2026-08-24 10:00:00", false);
   CheckMatch(_T("* * * * 7,3"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * 7,3"), "2026-08-19 10:00:00", true);
   CheckMatch(_T("* * * * 7,3"), "2026-08-20 10:00:00", false);
   CheckMatch(_T("* * * * 1-7"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * 1-7"), "2026-08-24 10:00:00", true);
   EndTest();

   StartTest(_T("Schedule matching - day of week step"));
   CheckMatch(_T("* * * * */7"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * */7"), "2026-08-24 10:00:00", false);
   CheckMatch(_T("* * * * */7"), "2026-08-20 10:00:00", false);
   CheckMatch(_T("* * * * */2"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * */2"), "2026-08-24 10:00:00", false);
   EndTest();

   StartTest(_T("Schedule matching - last and Nth day of week in month"));
   CheckMatch(_T("* * * * 1L"), "2026-08-31 10:00:00", true);
   CheckMatch(_T("* * * * 1L"), "2026-08-24 10:00:00", false);
   CheckMatch(_T("* * * * 0L"), "2026-08-30 10:00:00", true);
   CheckMatch(_T("* * * * 7L"), "2026-08-30 10:00:00", true);
   CheckMatch(_T("* * * * 4#3"), "2026-08-20 10:00:00", true);
   CheckMatch(_T("* * * * 4#2"), "2026-08-20 10:00:00", false);
   CheckMatch(_T("* * * * 7#4"), "2026-08-23 10:00:00", true);
   CheckMatch(_T("* * * * 7#3"), "2026-08-23 10:00:00", false);
   EndTest();
}

/**
 * Test handling of seconds field. It is honored only by MatchScheduleWithSeconds and ignored by
 * MatchSchedule, which is intended for consumers evaluating schedules once per minute (issue #3525).
 */
void TestScheduleSeconds()
{
   StartTest(_T("Schedule matching - seconds field ignored with minute resolution"));
   CheckMatch(_T("0 12 * * * 30"), "2026-08-20 12:00:00", true);
   CheckMatch(_T("0 12 * * * 30"), "2026-08-20 12:00:17", true);
   CheckMatch(_T("0 12 * * * 30"), "2026-08-20 12:00:30", true);
   CheckMatch(_T("0 12 * * * 30"), "2026-08-20 12:01:30", false);
   CheckMatch(_T("0 12 * * * */15"), "2026-08-20 12:00:07", true);
   EndTest();

   StartTest(_T("Schedule matching - seconds field with second resolution"));
   CheckMatchWithSeconds(_T("0 12 * * * 30"), "2026-08-20 12:00:30", true, true);
   CheckMatchWithSeconds(_T("0 12 * * * 30"), "2026-08-20 12:00:00", false, true);
   CheckMatchWithSeconds(_T("0 12 * * * */15"), "2026-08-20 12:00:45", true, true);
   CheckMatchWithSeconds(_T("0 12 * * * */15"), "2026-08-20 12:00:44", false, true);
   CheckMatchWithSeconds(_T("0 12 * * * 0,30"), "2026-08-20 12:00:00", true, true);
   CheckMatchWithSeconds(_T("0 12 * * * 30"), "2026-08-20 12:01:30", false, false);
   CheckMatchWithSeconds(_T("0 12 * * * 30"), "2026-08-20 13:00:30", false, false);
   EndTest();

   StartTest(_T("Schedule matching - schedule without seconds field"));
   CheckMatchWithSeconds(_T("0 12 * * *"), "2026-08-20 12:00:37", true, false);
   CheckMatchWithSeconds(_T("0 12 * * *"), "2026-08-20 13:00:37", false, false);
   EndTest();
}
