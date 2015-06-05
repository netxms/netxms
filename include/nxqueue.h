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
   void **m_elements;
   UINT32 m_numElements;
   UINT32 m_bufferSize;
   UINT32 m_initialSize;
   UINT32 m_first;
   UINT32 m_last;
   UINT32 m_bufferIncrement;
	bool m_shutdownFlag;

	void commonInit();
   void lock() { MutexLock(m_mutexQueueAccess); }
   void unlock() { MutexUnlock(m_mutexQueueAccess); }
   void shrink();

public:
   Queue();
   Queue(UINT32 initialSize, UINT32 bufferIncrement = 32);
   ~Queue();

   void put(void *object);
	void insert(void *object);
	void setShutdownMode();
   void *get();
   void *getOrBlock();
   int size() { return (int)m_numElements; }
   int allocated() { return (int)m_bufferSize; }
   void clear();
	void *find(void *key, QUEUE_COMPARATOR comparator);
	bool remove(void *key, QUEUE_COMPARATOR comparator);
};

#endif    /* _nxqueue_h_ */
