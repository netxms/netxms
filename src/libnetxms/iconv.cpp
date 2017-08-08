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
class IconvDescriptor
{
public:
   char *from;
   char *to;
   iconv_t cd;
   bool busy;

   IconvDescriptor(iconv_t _cd, const char *_from, const char *_to)
   {
      cd = _cd;
      from = strdup(_from);
      to = strdup(_to);
      busy = true;
   }

   ~IconvDescriptor()
   {
      free(from);
      free(to);
      iconv_close(cd);
   }
};

/**
 * iconv descriptor cache
 */
static ObjectArray<IconvDescriptor> s_cache(16, 16, true);

/**
 * Cache access mutex
 */
static Mutex s_cacheLock;

/**
 * Open descriptor
 */
iconv_t IconvOpen(const char *to, const char *from)
{
   iconv_t cd = (iconv_t)(-1);

   s_cacheLock.lock();

   for(int i = 0; i < s_cache.size(); i++)
   {
      IconvDescriptor *d = s_cache.get(i);
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
         IconvDescriptor *d = new IconvDescriptor(cd, from, to);
         s_cache.add(d);
      }
   }

   s_cacheLock.unlock();
   return cd;
}

/**
 * Close descriptor
 */
void IconvClose(iconv_t cd)
{
   s_cacheLock.lock();
   for(int i = 0; i < s_cache.size(); i++)
   {
      IconvDescriptor *d = s_cache.get(i);
      if (d->cd == cd)
      {
#if HAVE_ICONV_STATE_RESET
         iconv(cd, NULL, NULL, NULL, NULL);
#endif
         d->busy = false;
         break;
      }
   }
   s_cacheLock.unlock();
}

#endif /* !defined(__DISABLE_ICONV) && WITH_ICONV_CACHE */
