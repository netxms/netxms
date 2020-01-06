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
 * Callback for queue element enumeration
 */
typedef EnumerationCallbackResult (*QueueEnumerationCallback)(const void *object, void *context);

/**
 * Queue class
 */
class LIBNETXMS_EXPORTABLE Queue
{
   DISABLE_COPY_CTOR(Queue)

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
	bool m_owner;

	void commonInit();
   void lock() { MutexLock(m_mutexQueueAccess); }
   void unlock() { MutexUnlock(m_mutexQueueAccess); }
   void shrink();

protected:
   void (*m_destructor)(void*);

public:
   Queue(bool owner = false);
   Queue(size_t initialSize, size_t bufferIncrement, bool owner = false);
   virtual ~Queue();

   void put(void *object);
	void insert(void *object);
	void setShutdownMode();
	void setOwner(bool owner) { m_owner = owner; }
   void *get();
   void *getOrBlock(UINT32 timeout = INFINITE);
   size_t size() const { return m_numElements; }
   size_t allocated() const { return m_bufferSize; }
   void clear();
	void *find(const void *key, QueueComparator comparator, void *(*transform)(void*) = NULL);
	bool remove(const void *key, QueueComparator comparator);
	void forEach(QueueEnumerationCallback callback, void *context);
};

/**
 * Object queue
 */
template<typename T> class ObjectQueue : public Queue
{
   DISABLE_COPY_CTOR(ObjectQueue)

private:
   static void destructor(void *object) { delete static_cast<T*>(object); }

public:
   ObjectQueue(bool owner = false) : Queue(owner) { m_destructor = destructor; }
   ObjectQueue(size_t initialSize, size_t bufferIncrement, bool owner = false) : Queue(initialSize, bufferIncrement, owner) { m_destructor = destructor; }
   virtual ~ObjectQueue() { }

   T *get() { return (T*)Queue::get(); }
   T *getOrBlock(UINT32 timeout = INFINITE) { return (T*)Queue::getOrBlock(timeout); }
   template<typename K> T *find(const K *key, bool (*comparator)(const K *, const T *), T *(*transform)(T*) = NULL) { return (T*)Queue::find(key, (QueueComparator)comparator, (void *(*)(void*))transform); }
   template<typename K> bool remove(const K *key, bool (*comparator)(const K *, const T *)) { return Queue::remove(key, (QueueComparator)comparator); }
   template<typename C> void forEach(EnumerationCallbackResult (*callback)(const T *, C *), C *context) { Queue::forEach((QueueEnumerationCallback)callback, (void *)context); }
};

#endif    /* _nxqueue_h_ */
