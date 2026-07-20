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
** File: timegm.cpp
**
**/

#include "libnetxms.h"

#if !HAVE_TIMEGM

/**
 * Floor division (quotient rounded toward negative infinity)
 */
static inline int64_t FloorDiv(int64_t a, int64_t b)
{
   int64_t q = a / b;
   return ((a % b != 0) && ((a < 0) != (b < 0))) ? q - 1 : q;
}

/**
 * Days from civil date (proleptic Gregorian calendar) to days since the epoch
 * (1970-01-01). Month is 1-based, day is 1-based. Based on Howard Hinnant's
 * public domain calendar algorithms (https://howardhinnant.github.io/date_algorithms.html).
 */
static int64_t DaysFromCivil(int64_t y, int m, int d)
{
   y -= (m <= 2);
   int64_t era = FloorDiv(y, 400);
   int yoe = static_cast<int>(y - era * 400);                        // [0, 399]
   int doy = (153 * (m + ((m > 2) ? -3 : 9)) + 2) / 5 + d - 1;      // [0, 365]
   int64_t doe = static_cast<int64_t>(yoe) * 365 + yoe / 4 - yoe / 100 + doy;   // [0, 146096]
   return era * 146097 + doe - 719468;
}

/**
 * Civil date from days since the epoch (inverse of DaysFromCivil)
 */
static void CivilFromDays(int64_t z, int64_t *year, int *month, int *day)
{
   z += 719468;
   int64_t era = FloorDiv(z, 146097);
   int doe = static_cast<int>(z - era * 146097);                    // [0, 146096]
   int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
   int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);               // [0, 365]
   int mp = (5 * doy + 2) / 153;                                    // [0, 11]
   *day = doy - (153 * mp + 2) / 5 + 1;                             // [1, 31]
   *month = mp + ((mp < 10) ? 3 : -9);                              // [1, 12]
   *year = yoe + era * 400 + ((*month <= 2) ? 1 : 0);
}

/**
 * UTC version of mktime(). Conformant with POSIX.1-2024 / glibc semantics:
 * out-of-range fields are folded into the higher-order ones, and on success
 * the structure is updated to the normalized broken-down time (tm_wday and
 * tm_yday are recomputed, tm_isdst is set to 0). Returns -1 if the result is
 * not representable in time_t or the normalized year does not fit tm_year.
 *
 * Assumes POSIX time_t (86400 seconds per day, no leap seconds), which holds
 * on all supported platforms. All intermediate values fit int64_t for any
 * combination of int inputs, so the arithmetic itself cannot overflow.
 */
time_t LIBNETXMS_EXPORTABLE timegm(struct tm *_tm)
{
   // Fold seconds/minutes/hours into whole days plus time of day
   int64_t t = static_cast<int64_t>(_tm->tm_sec) + static_cast<int64_t>(_tm->tm_min) * 60 + static_cast<int64_t>(_tm->tm_hour) * 3600;
   int64_t days = FloorDiv(t, 86400);
   int tod = static_cast<int>(t - days * 86400);   // [0, 86399]

   // Fold month into year
   int64_t ym = FloorDiv(_tm->tm_mon, 12);
   int64_t year = static_cast<int64_t>(_tm->tm_year) + 1900 + ym;
   int month = static_cast<int>(_tm->tm_mon - ym * 12);   // [0, 11]

   days += DaysFromCivil(year, month + 1, 1) + static_cast<int64_t>(_tm->tm_mday) - 1;

   int64_t timestamp = days * 86400 + tod;
   time_t result = static_cast<time_t>(timestamp);
   if (static_cast<int64_t>(result) != timestamp)
      return (time_t)-1;   // does not fit time_t on this platform

   int64_t ny;
   int nm, nd;
   CivilFromDays(days, &ny, &nm, &nd);
   if ((ny < static_cast<int64_t>(INT_MIN) + 1900) || (ny > static_cast<int64_t>(INT_MAX) + 1900))
      return (time_t)-1;   // normalized year not representable in tm_year

   _tm->tm_year = static_cast<int>(ny - 1900);
   _tm->tm_mon = nm - 1;
   _tm->tm_mday = nd;
   _tm->tm_hour = tod / 3600;
   _tm->tm_min = (tod / 60) % 60;
   _tm->tm_sec = tod % 60;
   _tm->tm_wday = static_cast<int>(((days + 4) % 7 + 7) % 7);   // day 0 (1970-01-01) was a Thursday
   _tm->tm_yday = static_cast<int>(days - DaysFromCivil(ny, 1, 1));
   _tm->tm_isdst = 0;
   return result;
}

#endif
