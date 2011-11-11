/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: nxqueue.h
**
**/

#ifndef _nxqueue_h_
#define _nxqueue_h_

#include <nms_util.h>


//
// Queue class
//

typedef bool (*QUEUE_COMPARATOR)(void *key, void *object);

class LIBNETXMS_EXPORTABLE Queue
{
private:
   MUTEX m_mutexQueueAccess;
   CONDITION m_condWakeup;
   void **m_pElements;
   DWORD m_dwNumElements;
   DWORD m_dwBufferSize;
   DWORD m_dwFirst;
   DWORD m_dwLast;
   DWORD m_dwBufferIncrement;
	BOOL m_bShutdownFlag;

	void CommonInit();
   void Lock() { MutexLock(m_mutexQueueAccess); }
   void Unlock() { MutexUnlock(m_mutexQueueAccess); }

public:
   Queue();
   Queue(DWORD dwInitialSize, DWORD dwBufferIncrement = 32);
   ~Queue();

   void Put(void *pObject);
	void Insert(void *pObject);
	void SetShutdownMode();
   void *Get();
   void *GetOrBlock();
   DWORD Size() { return m_dwNumElements; }
   void Clear();
	void *find(void *key, QUEUE_COMPARATOR comparator);
};

#endif    /* _nxqueue_h_ */
