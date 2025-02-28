/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
   m_currentRegion = nullptr;
   m_allocated = 0;
}

/**
 * Move constructor
 */
MemoryPool::MemoryPool(MemoryPool&& src)
{
   m_headerSize = src.m_headerSize;
   m_regionSize = src.m_regionSize;
   m_currentRegion = src.m_currentRegion;
   m_allocated = src.m_allocated;
   src.m_currentRegion = nullptr;
   src.m_allocated = 0;
}

/**
 * Move assignment
 */
MemoryPool& MemoryPool::operator=(MemoryPool&& src)
{
   void *r = m_currentRegion;
   while(r != nullptr)
   {
      void *n = *((void **)r);
      MemFree(r);
      r = n;
   }

   m_headerSize = src.m_headerSize;
   m_regionSize = src.m_regionSize;
   m_currentRegion = src.m_currentRegion;
   m_allocated = src.m_allocated;
   src.m_currentRegion = nullptr;
   src.m_allocated = 0;
   return *this;
}

/**
 * Destroy memory pool (object destructors will not be called)
 */
MemoryPool::~MemoryPool()
{
   void *r = m_currentRegion;
   while(r != nullptr)
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
   if ((m_currentRegion != nullptr) && (m_allocated + allocationSize <= m_regionSize))
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
   if (s == nullptr)
      return nullptr;
   size_t l = _tcslen(s) + 1;
   TCHAR *p = allocateString(l);
   memcpy(p, s, l * sizeof(TCHAR));
   return p;
}

/**
 * Drop all allocated memory except one region
 */
void MemoryPool::clear()
{
   if (m_currentRegion == nullptr)
      return;

   void *r = *((void **)m_currentRegion);
   while(r != nullptr)
   {
      void *n = *((void **)r);
      MemFree(r);
      r = n;
   }
   *((void **)m_currentRegion) = nullptr;
   m_allocated = m_headerSize;
}

/**
 * Get number of allocated regions
 */
size_t MemoryPool::getRegionCount() const
{
   size_t count = 0;
   void *r = m_currentRegion;
   while(r != nullptr)
   {
      count++;
      r = *((void **)r);
   }
   return count;
}
