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
 * Internal queue buffer
 */
struct QueueBuffer;

/**
 * Queue class
 */
class LIBNETXMS_EXPORTABLE Queue
{
   DISABLE_COPY_CTOR(Queue)

private:
#ifdef _WIN32
   CRITICAL_SECTION m_lock;
   CONDITION_VARIABLE m_wakeupCondition;
#else
   pthread_mutex_t m_lock;
   pthread_cond_t m_wakeupCondition;
#endif
   QueueBuffer *m_head;
   QueueBuffer *m_tail;
   size_t m_size;
   size_t m_blockSize;
   size_t m_blockCount;
   int m_readers;
	bool m_shutdownFlag;
	bool m_owner;

	void commonInit();
#ifdef _WIN32
   void lock() { EnterCriticalSection(&m_lock); }
   void unlock() { LeaveCriticalSection(&m_lock); }
#else
   void lock() { pthread_mutex_lock(&m_lock); }
   void unlock() { pthread_mutex_unlock(&m_lock); }
#endif

   void *getInternal();

protected:
   void (*m_destructor)(void*, Queue*);

public:
   Queue();
   Queue(size_t blockSize, Ownership owner);
   virtual ~Queue();

   void put(void *object);
	void insert(void *object);
	void setShutdownMode();
	void setOwner(bool owner) { m_owner = owner; }
   void *get();
   void *getOrBlock(uint32_t timeout = INFINITE);
   size_t size() const { return m_size; }
   size_t allocated() const { return m_blockSize * m_blockCount; }
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
   static void destructor(void *object, Queue *queue) { delete static_cast<T*>(object); }

public:
   ObjectQueue() : Queue() { m_destructor = destructor; }
   ObjectQueue(size_t blockSize, Ownership owner) : Queue(blockSize, owner) { m_destructor = destructor; }
   ObjectQueue(size_t blockSize, Ownership owner, void (*customDestructor)(void *, Queue *)) : Queue(blockSize, owner) { m_destructor = customDestructor; }
   virtual ~ObjectQueue() { }

   T *get() { return (T*)Queue::get(); }
   T *getOrBlock(uint32_t timeout = INFINITE) { return (T*)Queue::getOrBlock(timeout); }
   template<typename K> T *find(const K *key, bool (*comparator)(const K *, const T *), T *(*transform)(T*) = NULL) { return (T*)Queue::find(key, (QueueComparator)comparator, (void *(*)(void*))transform); }
   template<typename K> bool remove(const K *key, bool (*comparator)(const K *, const T *)) { return Queue::remove(key, (QueueComparator)comparator); }
   template<typename C> void forEach(EnumerationCallbackResult (*callback)(const T *, C *), C *context) { Queue::forEach((QueueEnumerationCallback)callback, (void *)context); }
};

/**
 * Shared object queue
 */
template<typename T> class SharedObjectQueue : public Queue
{
   DISABLE_COPY_CTOR(SharedObjectQueue)

private:
   ObjectMemoryPool<shared_ptr<T>> m_pool;

   static void destructor(void *object, Queue *queue) { static_cast<SharedObjectQueue<T>*>(queue)->m_pool.destroy(static_cast<shared_ptr<T>*>(object)); }

public:
   SharedObjectQueue() : Queue(256, Ownership::True) { m_destructor = destructor; }
   SharedObjectQueue(size_t blockSize) : Queue(blockSize, Ownership::True) { m_destructor = destructor; }
   virtual ~SharedObjectQueue() { }

   shared_ptr<T> get()
   {
      auto p = static_cast<shared_ptr<T>*>(Queue::get());
      if ((p == nullptr) || (reinterpret_cast<void*>(p) == INVALID_POINTER_VALUE))
         return shared_ptr<T>();
      auto v = *p;
      m_pool.destroy(p);
      return v;
   }
   shared_ptr<T> getOrBlock(uint32_t timeout = INFINITE)
   {
      auto p = static_cast<shared_ptr<T>*>(Queue::getOrBlock(timeout));
      if ((p == nullptr) || (reinterpret_cast<void*>(p) == INVALID_POINTER_VALUE))
         return shared_ptr<T>();
      auto v = *p;
      m_pool.destroy(p);
      return v;
   }

   void put(shared_ptr<T> object) { Queue::put(new(m_pool.allocate()) shared_ptr<T>(object)); }
   void insert(void *object) { Queue::insert(new(m_pool.allocate()) shared_ptr<T>(object)); }
};

#endif    /* _nxqueue_h_ */
