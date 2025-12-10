/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: aitools.h
**
**/

#ifndef _aitools_h_
#define _aitools_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_core.h>
#include <nxmodule.h>
#include <iris.h>

#define DEBUG_TAG _T("ai.tools")

/**
 * Parse timestamp from string
 */
static inline time_t ParseTimestamp(const char *ts)
{
   char *eptr;
   if (ts[0] == '-')
   {
      // Offset from now
      int64_t offset = strtoll(ts, &eptr, 10);
      if (*eptr != 0)
      {
         // check for suffix
         if (stricmp(eptr, "m") == 0)
            offset *= 60;
         else if (stricmp(eptr, "h") == 0)
            offset *= 3600;
         else if (stricmp(eptr, "d") == 0)
            offset *= 86400;
         else if (stricmp(eptr, "s") != 0)
            return 0;  // invalid format
      }
      else
      {
         // no suffix, assume minutes
         offset *= 60;
      }
      return time(nullptr) - static_cast<time_t>(-offset);
   }

   int64_t n = strtoll(ts, &eptr, 10);
   if (*eptr == 0)
      return static_cast<time_t>(n);   // Assume UNIX timestamp

   struct tm t;
   if (strptime(ts, "%Y-%m-%dT%H:%M:%SZ", &t) == nullptr)
      return 0;

   return timegm(&t);
}

#endif
