/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2024 Raden Solutions
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
**/

#include "linux_subagent.h"

/**
 * Count items in a list of ranges, like e.g. /sys/devices/system/cpu/online
 *
 * @param buffer contents of the file verbatim. The buffer is destructively modified!
 * @return count of items (e.g. CPU) as described in the buffer
 */
uint32_t CountRanges(char *buffer)
{
   char *endline = strchrnul(buffer, '\n');
   *endline = 0;

   uint32_t count = 0;
   char *rangeStartPtr = buffer;
   while (true)
   {
      char *rangeEndPtr = strchrnul(rangeStartPtr, ',');
      bool last = *rangeEndPtr == '\0'; // true if this is the last or the only range
      *rangeEndPtr = 0;
      // Now rangeStartPtr points at null-terminated string with a single range of non-negative numbers
      char *afterStartIndex;
      long startIndex = strtol(rangeStartPtr, &afterStartIndex, 10);
      if (afterStartIndex == rangeStartPtr)
      {
         ; // empty range
      }
      else if (*afterStartIndex == '\0')
      {
         assert(startIndex >= 0);
         count++;
      }
      else if (*afterStartIndex == '-')
      {
         long endIndex = strtol(afterStartIndex + 1 /* skip minus sign */, &afterStartIndex, 10);
         assert(endIndex >= 0);
         assert(*afterStartIndex == '\0');
         count += endIndex - startIndex + 1;
      }
      else
      {
         assert(false);
      }

      if (last)
         break;
      rangeStartPtr = rangeEndPtr + 1;
   }
   return count;
}

/**
 * Count items in a ranges-list file, like e.g. /sys/devices/system/cpu/online
 */
LONG CountRangesFile(const char *filepath, uint32_t *value)
{
   FILE *f = fopen(filepath, "r");
   if (f == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("CountRangesFile: Cannot open file %hs"), filepath);
      return SYSINFO_RC_ERROR;
   }

   char buffer[256];
   if (fgets(buffer, sizeof(buffer), f) == nullptr)
   {
      fclose(f);
      return SYSINFO_RC_ERROR;
   }
   fclose(f);

   *value = CountRanges(buffer);
   return SYSINFO_RC_SUCCESS;
}
