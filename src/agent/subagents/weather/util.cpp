/*
** NetXMS weather subagent
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: util.cpp
** Provider-neutral helpers shared by adapters and the location registry.
**
**/

#include "weather.h"

/**
 * Format a canonical location key from provider and coordinates. Coordinates are
 * limited to four decimals both for cache dedup and because MET Norway's terms of
 * service cap request precision at that many digits.
 */
void FormatLocationKey(const char *providerName, double latitude, double longitude, char *buffer)
{
   snprintf(buffer, MAX_LOC_KEY, "%s:%.4f,%.4f", providerName, latitude, longitude);
}

/**
 * Parse a "lat,lon" pair. Returns true only when the whole string is two
 * comma-separated numbers within valid ranges, so location names (which never
 * take this shape) fall through to catalog lookup.
 */
bool ParseLatLon(const TCHAR *str, double *latitude, double *longitude)
{
   if ((str == nullptr) || (*str == 0))
      return false;

   TCHAR *end;
   double lat = _tcstod(str, &end);
   if (end == str)
      return false;
   while((*end == ' ') || (*end == '\t'))
      end++;
   if (*end != _T(','))
      return false;
   const TCHAR *p = end + 1;
   double lon = _tcstod(p, &end);
   if (end == p)
      return false;
   while((*end == ' ') || (*end == '\t'))
      end++;
   if (*end != 0)
      return false;

   if ((lat < -90.0) || (lat > 90.0) || (lon < -180.0) || (lon > 180.0))
      return false;

   *latitude = lat;
   *longitude = lon;
   return true;
}

/**
 * Parse an ISO 8601 timestamp with a zone designator: "Z" (MET Norway) or a
 * numeric "+HH:MM"/"-HH:MM" offset (Bright Sky). Anything else yields 0.
 */
time_t ParseIsoTimestamp(const char *str)
{
   if (str == nullptr)
      return 0;

   int year, month, day, hour, minute, second, consumed;
   if (sscanf(str, "%4d-%2d-%2dT%2d:%2d:%2d%n", &year, &month, &day, &hour, &minute, &second, &consumed) != 6)
      return 0;

   const char *zone = str + consumed;
   int offset = 0;   // seconds east of UTC
   if (!strcmp(zone, "Z"))
   {
      offset = 0;
   }
   else if ((*zone == '+') || (*zone == '-'))
   {
      int offsetHours, offsetMinutes;
      if ((sscanf(zone + 1, "%2d:%2d%n", &offsetHours, &offsetMinutes, &consumed) != 2) || (zone[1 + consumed] != 0))
         return 0;
      offset = (offsetHours * 3600 + offsetMinutes * 60) * ((*zone == '-') ? -1 : 1);
   }
   else
   {
      return 0;
   }

   struct tm t;
   memset(&t, 0, sizeof(struct tm));
   t.tm_year = year - 1900;
   t.tm_mon = month - 1;
   t.tm_mday = day;
   t.tm_hour = hour;
   t.tm_min = minute;
   t.tm_sec = second;
   t.tm_isdst = 0;
   return timegm(&t) - offset;
}
