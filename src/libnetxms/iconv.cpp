/*
 ** NetXMS - Network Management System
 ** Copyright (C) 2003-2014 Raden Solutions
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
 ** File: iconv.cpp
 **
 **/

#include "libnetxms.h"

#if HAVE_ICONV_H
#include <iconv.h>
#endif

#if !defined(__DISABLE_ICONV) && WITH_ICONV_CACHE

/**
 * iconv descriptor
 */
struct ICONV_DESCRIPTOR
{
   char *from;
   char *to;
   iconv_t cd;
   bool busy;
};

/**
 * iconv descriptor cache
 */
static ObjectArray<ICONV_DESCRIPTOR> s_cache;

/**
 * Cache access mutex
 */
static MUTEX s_cacheLock = MutexCreate();

/**
 * Open descriptor
 */
iconv_t IconvOpen(const char *to, const char *from)
{
   iconv_t cd = (iconv_t)(-1);

   MutexLock(s_cacheLock);

   for(int i = 0; i < s_cache.size(); i++)
   {
      ICONV_DESCRIPTOR *d = s_cache.get(i);
      if (!d->busy && !strcmp(from, d->from) && !strcmp(to, d->to))
      {
         d->busy = true;
         cd = d->cd;
         break;
      }
   }

   if (cd == (iconv_t)(-1))
   {
      cd = iconv_open(to, from);
      if (cd != (iconv_t)(-1))
      {
         ICONV_DESCRIPTOR *d = new ICONV_DESCRIPTOR;
         d->cd = cd;
         d->busy = true;
         d->from = strdup(from);
         d->to = strdup(to);
         s_cache.add(d);
      }
   }

   MutexUnlock(s_cacheLock);
   return cd;
}

/**
 * Close descriptor
 */
void IconvClose(iconv_t cd)
{
   MutexLock(s_cacheLock);
   for(int i = 0; i < s_cache.size(); i++)
   {
      ICONV_DESCRIPTOR *d = s_cache.get(i);
      if (d->cd == cd)
      {
#if HAVE_ICONV_STATE_RESET
         iconv(cd, NULL, NULL, NULL, NULL);
#endif
         d->busy = false;
         break;
      }
   }
   MutexUnlock(s_cacheLock);
}

#endif /* !defined(__DISABLE_ICONV) && WITH_ICONV_CACHE */
