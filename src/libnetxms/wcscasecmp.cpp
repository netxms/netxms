/* 
** NetXMS - Network Management System
** Copyright (C) 2013 Raden Solutions
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
**/

#include "libnetxms.h"

#if !(HAVE_WCSCASECMP)

#if !HAVE_WINT_T && !defined(_WIN32)
typedef int wint_t;
#endif

int wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
   if (s1 == s2)
   {
      return 0;
   }

   wint_t c1, c2;
   do
   {
      c1 = towlower(*(s1++));
      c2 = towlower(*(s2++));

      if (c1 == L'\0')
      {
         break;
      }
   } while (c1 == c2);

   if (c1 == c2)
   {
      return 0;
   }
   if (c1 < c2)
   {
      return -1;
   }
   return 1;
}

#endif   /* !(HAVE_WCSCASECMP) */
