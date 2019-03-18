/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: mempool.cpp
**/

#include "libnetxms.h"

/**
 * Create new memory pool
 */
MemoryPool::MemoryPool(size_t regionSize)
{
   m_headerSize = sizeof(void*);
   if (m_headerSize % 16 != 0)
      m_headerSize += 16 - m_headerSize % 16;
   m_regionSize = regionSize;
   m_currentRegion = MemAlloc(m_regionSize);
   *((void **)m_currentRegion) = NULL; // pointer to previous region
   m_allocated = m_headerSize;
}

/**
 * Destroy memory pool (object destructors will not be called)
 */
MemoryPool::~MemoryPool()
{
   void *r = m_currentRegion;
   while(r != NULL)
   {
      void *n = *((void **)r);
      MemFree(r);
      r = n;
   }
}

/**
 * Allocate memory block
 */
void *MemoryPool::allocate(size_t size)
{
   size_t allocationSize = ((size % 8) == 0) ? size : (size + 8 - size % 8);
   void *p;
   if (m_allocated + allocationSize <= m_regionSize)
   {
      p = (char*)m_currentRegion + m_allocated;
      m_allocated += allocationSize;
   }
   else
   {
      void *region = MemAlloc(std::max(m_regionSize, allocationSize + m_headerSize));
      *((void **)region) = m_currentRegion;
      m_currentRegion = region;
      p = (char*)m_currentRegion + m_headerSize;
      m_allocated = m_headerSize + allocationSize;
   }
   return p;
}

/**
 * Create copy of given C string within pool
 */
TCHAR *MemoryPool::copyString(const TCHAR *s)
{
   if (s == NULL)
      return NULL;
   size_t l = _tcslen(s) + 1;
   TCHAR *p = static_cast<TCHAR*>(allocate(l * sizeof(TCHAR)));
   memcpy(p, s, l * sizeof(TCHAR));
   return p;
}

/**
 * Drop all allocated memory except one region
 */
void MemoryPool::clear()
{
   void *r = *((void **)m_currentRegion);
   while(r != NULL)
   {
      void *n = *((void **)r);
      MemFree(r);
      r = n;
   }
   *((void **)m_currentRegion) = NULL;
   m_allocated = m_headerSize;
}
