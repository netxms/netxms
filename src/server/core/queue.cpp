/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: queue.cpp
**
**/

#include "nms_core.h"


//
// Queue constructor
//

Queue::Queue()
{
   m_hQueueAccess = MutexCreate();
   m_dwNumElements = 0;
   m_dwFirst = 0;
   m_dwLast = 0;
   m_dwBufferSize = 256;    // Initial buffer size of 256 elements
   m_pElements = (void **)malloc(sizeof(void *) * m_dwBufferSize);
}


//
// Destructor
//

Queue::~Queue()
{
   MutexDestroy(m_hQueueAccess);
   if (m_pElements != NULL)
      free(m_pElements);
}


//
// Put new element into queue
//

void Queue::Put(void *pElement)
{
   Lock();
   if (m_dwNumElements == m_dwBufferSize)
   {
      // Extend buffer
      m_dwBufferSize += 32;
      m_pElements = (void **)realloc(m_pElements, sizeof(void *) * m_dwBufferSize);
   }
   m_pElements[m_dwLast++] = pElement;
   if (m_dwLast == m_dwBufferSize)
      m_dwLast = 0;
   m_dwNumElements++;
   Unlock();
}


//
// Get object from queue. Return NULL if queue is empty
//

void *Queue::Get(void)
{
   void *pElement = NULL;

   Lock();
   if (m_dwNumElements != 0)
   {
      pElement = m_pElements[m_dwFirst++];
      if (m_dwFirst == m_dwBufferSize)
         m_dwFirst = 0;
      m_dwNumElements--;
   }
   Unlock();
   return pElement;
}
