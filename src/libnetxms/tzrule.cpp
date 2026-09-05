/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2026 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: tzrule.cpp
**
**/

#include "libnetxms.h"

/**
 * Check if given year is a leap year in the proleptic Gregorian calendar
 */
static inline bool IsLeapYear(int64_t year)
{
   return ((year % 4) == 0) && (((year % 100) != 0) || ((year % 400) == 0));
}

/**
 * Day of week (0 = Sunday) for given number of days since the epoch (1970-01-01 was a Thursday)
 */
static inline int DayOfWeek(int64_t days)
{
   return static_cast<int>(((days + 4) % 7 + 7) % 7);
}

/**
 * Parse unsigned decimal number of at most maxDigits digits. Advances *p past the digits.
 * Returns false if there is no digit at *p.
 */
static bool ParseNumber(const char **p, int *value, int maxDigits)
{
   const char *s = *p;
   if (!isdigit(static_cast<unsigned char>(*s)))
      return false;

   int v = 0;
   int n = 0;
   while (isdigit(static_cast<unsigned char>(*s)) && (n < maxDigits))
   {
      v = v * 10 + (*s - '0');
      s++;
      n++;
   }
   *p = s;
   *value = v;
   return true;
}

/**
 * Days since the epoch of the transition day in given year
 */
int64_t TimeZoneRule::Transition::dayOfYear(int64_t year) const
{
   int64_t jan1 = DaysFromCivil(year, 1, 1);
   switch(form)
   {
      case 'J':   // 1..365, February 29 is never counted
         return jan1 + day - 1 + ((IsLeapYear(year) && (day >= 60)) ? 1 : 0);
      case 'N':   // 0..365, February 29 is counted in leap years
         return jan1 + day;
      case 'M':
      {
         int64_t first = DaysFromCivil(year, month, 1);
         int64_t d = first + (day - DayOfWeek(first) + 7) % 7 + (week - 1) * 7;
         if (week == 5)
         {
            // "Last" weekday of the month: step back if the fifth occurrence falls into the next month
            int64_t nextMonth = (month == 12) ? DaysFromCivil(year + 1, 1, 1) : DaysFromCivil(year, month + 1, 1);
            if (d >= nextMonth)
               d -= 7;
         }
         return d;
      }
      default:
         return jan1;
   }
}

/**
 * Reset to invalid state
 */
void TimeZoneRule::reset()
{
   m_text[0] = 0;
   m_stdName[0] = 0;
   m_dstName[0] = 0;
   m_stdOffset = 0;
   m_dstOffset = 0;
   m_hasDst = false;
   memset(&m_dstStart, 0, sizeof(Transition));
   memset(&m_dstEnd, 0, sizeof(Transition));
   m_valid = false;
}

/**
 * Parse timezone name: either three or more letters, or a string of letters, digits, '+' and '-' enclosed in angle brackets.
 * The name is stored without brackets.
 */
bool TimeZoneRule::parseName(const char **p, char *name, size_t size)
{
   const char *s = *p;
   size_t len = 0;
   if (*s == '<')
   {
      s++;
      while ((*s != '>') && (*s != 0))
      {
         if (!isalnum(static_cast<unsigned char>(*s)) && (*s != '+') && (*s != '-'))
            return false;
         if (len >= size - 1)
            return false;
         name[len++] = *s++;
      }
      if (*s != '>')
         return false;
      s++;
   }
   else
   {
      while (isalpha(static_cast<unsigned char>(*s)))
      {
         if (len >= size - 1)
            return false;
         name[len++] = *s++;
      }
   }
   if (len < 3)
      return false;
   name[len] = 0;
   *p = s;
   return true;
}

/**
 * Parse offset or time in form [+-]h[h[h]][:mm[:ss]]. Result is in seconds and carries the sign as written.
 * Absolute value of the hours part must not exceed maxHours.
 */
bool TimeZoneRule::parseOffset(const char **p, int *offset, int maxHours, bool requireSign)
{
   const char *s = *p;
   bool negative = false;
   if ((*s == '+') || (*s == '-'))
   {
      negative = (*s == '-');
      s++;
   }
   else if (requireSign)
   {
      return false;
   }

   int hours, minutes = 0, seconds = 0;
   if (!ParseNumber(&s, &hours, 3) || (hours > maxHours))
      return false;
   if (*s == ':')
   {
      s++;
      if (!ParseNumber(&s, &minutes, 2) || (minutes > 59))
         return false;
      if (*s == ':')
      {
         s++;
         if (!ParseNumber(&s, &seconds, 2) || (seconds > 59))
            return false;
      }
   }

   int value = hours * 3600 + minutes * 60 + seconds;
   *offset = negative ? -value : value;
   *p = s;
   return true;
}

/**
 * Parse DST transition rule: Mm.w.d[/time], Jn[/time] or n[/time]
 */
bool TimeZoneRule::parseTransition(const char **p, Transition *t)
{
   const char *s = *p;
   if (*s == 'M')
   {
      s++;
      t->form = 'M';
      if (!ParseNumber(&s, &t->month, 2) || (t->month < 1) || (t->month > 12) || (*s != '.'))
         return false;
      s++;
      if (!ParseNumber(&s, &t->week, 1) || (t->week < 1) || (t->week > 5) || (*s != '.'))
         return false;
      s++;
      if (!ParseNumber(&s, &t->day, 1) || (t->day > 6))
         return false;
   }
   else if (*s == 'J')
   {
      s++;
      t->form = 'J';
      if (!ParseNumber(&s, &t->day, 3) || (t->day < 1) || (t->day > 365))
         return false;
   }
   else
   {
      t->form = 'N';
      if (!ParseNumber(&s, &t->day, 3) || (t->day > 365))
         return false;
   }

   if (*s == '/')
   {
      s++;
      if (!parseOffset(&s, &t->time, 167, false))
         return false;
   }
   else
   {
      t->time = 7200;   // POSIX default: 02:00:00 local time
   }

   *p = s;
   return true;
}

/**
 * Parse POSIX TZ rule string. Leading and trailing whitespace is ignored. On failure the object becomes invalid
 * (toString() returns an empty string and the rule evaluates as UTC).
 */
bool TimeZoneRule::parse(const char *rule)
{
   reset();
   if (rule == nullptr)
      return false;

   while (isspace(static_cast<unsigned char>(*rule)))
      rule++;
   size_t len = strlen(rule);
   while ((len > 0) && isspace(static_cast<unsigned char>(rule[len - 1])))
      len--;
   if ((len == 0) || (len >= sizeof(m_text)))
      return false;
   memcpy(m_text, rule, len);
   m_text[len] = 0;

   const char *p = m_text;
   bool success = false;
   do
   {
      // Standard time name and offset (mandatory). POSIX offsets are positive west of UTC, class stores seconds east of UTC.
      if (!parseName(&p, m_stdName, sizeof(m_stdName)))
         break;
      if (!parseOffset(&p, &m_stdOffset, 24, false))
         break;
      m_stdOffset = -m_stdOffset;
      m_dstOffset = m_stdOffset;
      if (*p == 0)
      {
         success = true;
         break;
      }

      // Daylight saving time name with optional offset (default is one hour ahead of standard time)
      if (!parseName(&p, m_dstName, sizeof(m_dstName)))
         break;
      if ((*p != ',') && (*p != 0))
      {
         if (!parseOffset(&p, &m_dstOffset, 24, false))
            break;
         m_dstOffset = -m_dstOffset;
      }
      else
      {
         m_dstOffset = m_stdOffset + 3600;
      }

      // Both transition rules are mandatory when DST name is present
      if (*p != ',')
         break;
      p++;
      if (!parseTransition(&p, &m_dstStart))
         break;
      if (*p != ',')
         break;
      p++;
      if (!parseTransition(&p, &m_dstEnd))
         break;
      if (*p != 0)
         break;

      m_hasDst = true;
      success = true;
   } while(false);

   if (!success)
   {
      reset();
      return false;
   }

   m_valid = true;
   return true;
}

/**
 * Calculate UTC instants of DST start and DST end transitions for given year. Start rule time is interpreted
 * in local standard time, end rule time in local daylight saving time, as required by POSIX.
 */
void TimeZoneRule::transitionsForYear(int64_t year, time_t *dstStart, time_t *dstEnd) const
{
   *dstStart = static_cast<time_t>(m_dstStart.dayOfYear(year) * 86400 + m_dstStart.time - m_stdOffset);
   *dstEnd = static_cast<time_t>(m_dstEnd.dayOfYear(year) * 86400 + m_dstEnd.time - m_dstOffset);
}

/**
 * Check if daylight saving time is in effect at given moment
 */
bool TimeZoneRule::isDaylightSavingAt(time_t t) const
{
   if (!m_hasDst)
      return false;

   // Transitions are resolved for the year of t in local standard time, so a transition that falls on
   // the year boundary is always evaluated against the year it actually belongs to.
   int64_t year;
   int month, day;
   CivilFromDays(FloorDiv(static_cast<int64_t>(t) + m_stdOffset, 86400), &year, &month, &day);

   time_t start, end;
   transitionsForYear(year, &start, &end);
   if (start < end)
      return (t >= start) && (t < end);   // Northern hemisphere: DST within the year
   return (t >= start) || (t < end);      // Southern hemisphere: DST spans the year boundary
}

/**
 * Get UTC offset in effect at given moment (seconds east of UTC)
 */
int TimeZoneRule::offsetAt(time_t t) const
{
   return isDaylightSavingAt(t) ? m_dstOffset : m_stdOffset;
}

/**
 * Get local calendar date for given moment
 */
void TimeZoneRule::toLocalDate(time_t t, int *year, int *month, int *day) const
{
   int64_t y;
   CivilFromDays(FloorDiv(static_cast<int64_t>(t) + offsetAt(t), 86400), &y, month, day);
   *year = static_cast<int>(y);
}

/**
 * Get first UTC instant of given local calendar day. Normally this is local midnight. If a DST transition skips
 * local midnight, the result is the transition instant (the first moment that belongs to the requested date);
 * if local midnight occurs twice, the result is the earlier occurrence.
 */
time_t TimeZoneRule::localDayStart(int year, int month, int day) const
{
   int64_t midnight = DaysFromCivil(year, month, day) * 86400;   // local midnight expressed as if it was UTC
   if (!m_hasDst)
      return static_cast<time_t>(midnight - m_stdOffset);

   // Local midnight in standard time, in daylight saving time, plus any transition between the two: the day
   // can only begin at one of these instants. Pick the earliest one that actually belongs to the requested date.
   time_t candidates[8];
   int count = 0;
   candidates[count++] = static_cast<time_t>(midnight - m_stdOffset);
   candidates[count++] = static_cast<time_t>(midnight - m_dstOffset);
   time_t lo = std::min(candidates[0], candidates[1]);
   time_t hi = std::max(candidates[0], candidates[1]);
   for(int y = year - 1; y <= year + 1; y++)
   {
      time_t start, end;
      transitionsForYear(y, &start, &end);
      if ((start >= lo) && (start <= hi))
         candidates[count++] = start;
      if ((end >= lo) && (end <= hi))
         candidates[count++] = end;
   }
   std::sort(candidates, candidates + count);

   for(int i = 0; i < count; i++)
   {
      int y, m, d;
      toLocalDate(candidates[i], &y, &m, &d);
      if ((y == year) && (m == month) && (d == day))
         return candidates[i];
   }
   return lo;   // not reachable with a consistent rule, kept as a safe fallback
}
