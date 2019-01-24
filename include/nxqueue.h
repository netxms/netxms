/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
typedef bool (*QueueComparator)(const void *key, const void *object);

/**
 * Queue class
 */
class LIBNETXMS_EXPORTABLE Queue
{
private:
   MUTEX m_mutexQueueAccess;
   CONDITION m_condWakeup;
   void **m_elements;
   size_t m_numElements;
   size_t m_bufferSize;
   size_t m_initialSize;
   size_t m_first;
   size_t m_last;
   size_t m_bufferIncrement;
	bool m_shutdownFlag;

	void commonInit();
   void lock() { MutexLock(m_mutexQueueAccess); }
   void unlock() { MutexUnlock(m_mutexQueueAccess); }
   void shrink();

public:
   Queue();
   Queue(size_t initialSize, size_t bufferIncrement = 32);
   virtual ~Queue();

   void put(void *object);
	void insert(void *object);
	void setShutdownMode();
   void *get();
   void *getOrBlock(UINT32 timeout = INFINITE);
   size_t size() const { return m_numElements; }
   size_t allocated() const { return m_bufferSize; }
   void clear();
	void *find(const void *key, QueueComparator comparator);
	bool remove(const void *key, QueueComparator comparator);
};

template<typename T> class ObjectQueue : public Queue
{
public:
   ObjectQueue() : Queue() { }
   ObjectQueue(size_t initialSize, size_t bufferIncrement = 32) : Queue(initialSize, bufferIncrement) { }
   virtual ~ObjectQueue() { }

   T *get() { return (T*)Queue::get(); }
   T *getOrBlock(UINT32 timeout = INFINITE) { return (T*)Queue::getOrBlock(timeout); }
   void *find(const void *key, bool (*comparator)(const void *key, const T *object)) { return Queue::find(key, (QueueComparator)comparator); }
   bool remove(const void *key, bool (*comparator)(const void *key, const T *object)) { return Queue::remove(key, (QueueComparator)comparator); }
};

#endif    /* _nxqueue_h_ */
