/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxqueue.h
**
**/

#ifndef _nxqueue_h_
#define _nxqueue_h_


//
// Queue class
//

class Queue
{
private:
   MUTEX m_hQueueAccess;
   CONDITION m_hConditionNotEmpty;
   void **m_pElements;
   DWORD m_dwNumElements;
   DWORD m_dwBufferSize;
   DWORD m_dwFirst;
   DWORD m_dwLast;
   DWORD m_dwBufferIncrement;

   void Lock(void) { MutexLock(m_hQueueAccess, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hQueueAccess); }

public:
   Queue(DWORD dwInitialSize = 256, DWORD dwBufferIncrement = 32);
   ~Queue();

   void Put(void *pObject);
   void *Get(void);
   void *GetOrBlock(void);
   DWORD Size(void) { return m_dwNumElements; }
};

#endif    /* _nxqueue_h_ */
