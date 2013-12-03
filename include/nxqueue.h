/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Comparator for queue search
 */
typedef bool (*QUEUE_COMPARATOR)(void *key, void *object);

/**
 * Queue class
 */
class LIBNETXMS_EXPORTABLE Queue
{
private:
   MUTEX m_mutexQueueAccess;
   CONDITION m_condWakeup;
   void **m_pElements;
   UINT32 m_dwNumElements;
   UINT32 m_dwBufferSize;
   UINT32 m_dwFirst;
   UINT32 m_dwLast;
   UINT32 m_dwBufferIncrement;
	BOOL m_bShutdownFlag;

	void commonInit();
   void lock() { MutexLock(m_mutexQueueAccess); }
   void unlock() { MutexUnlock(m_mutexQueueAccess); }

public:
   Queue();
   Queue(UINT32 dwInitialSize, UINT32 dwBufferIncrement = 32);
   ~Queue();

   void Put(void *pObject);
	void Insert(void *pObject);
	void SetShutdownMode();
   void *Get();
   void *GetOrBlock();
   UINT32 Size() { return m_dwNumElements; }
   void Clear();
	void *find(void *key, QUEUE_COMPARATOR comparator);
	bool remove(void *key, QUEUE_COMPARATOR comparator);
};

#endif    /* _nxqueue_h_ */
