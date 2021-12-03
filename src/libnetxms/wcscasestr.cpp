/*
** NetXMS - Network Management System
** Copyright (C) 2021 Raden Solutions
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

#if !HAVE_WCSCASESTR

/**
 * Find substring in a string ignoring the case of both arguments (wide character version)
 */
WCHAR LIBNETXMS_EXPORTABLE *wcscasestr(const WCHAR *s, const WCHAR *ss)
{
   WCHAR c;
   if ((c = *ss++) == 0)
      return const_cast<WCHAR*>(s);

   c = towlower(c);
   size_t sslen = wcslen(ss);
   do
   {
      WCHAR sc;
      do
      {
         if ((sc = *s++) == 0)
            return nullptr;
      } while(static_cast<WCHAR>(towlower(sc)) != c);
   } while (wcsnicmp(s, ss, sslen) != 0);
   s--;
   return const_cast<WCHAR*>(s);
}

#endif
