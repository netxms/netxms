/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: msgwq.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>

/**
 * Delete message object
 */
static inline void DeleteMessage(WaitQueueUnclaimedMessage *m)
{
   if (m->isBinary)
      MemFree(m->msg);
   else
      delete static_cast<NXCPMessage*>(m->msg);
}

/**
 * Constructor
 */
MsgWaitQueue::MsgWaitQueue() : m_unclaimedMessagesPool(64), m_waitersPool(64), m_mutex(MutexType::FAST)
{
   m_holdTime = 30;      // Default message TTL is 30 seconds
   m_messagesHead = m_unclaimedMessagesPool.allocate();
   memset(m_messagesHead, 0, sizeof(WaitQueueUnclaimedMessage));
   m_messagesTail = m_messagesHead;
   m_waiters = m_waitersPool.allocate();
   memset(m_waiters, 0, sizeof(WaitQueueWaiter));
   m_lastExpirationCheck = time(nullptr);
}

/**
 * Destructor
 */
MsgWaitQueue::~MsgWaitQueue()
{
   for(WaitQueueUnclaimedMessage *m = m_messagesHead->next; m != nullptr; m = m->next)
      DeleteMessage(m);

   for(WaitQueueWaiter *w = m_waiters->next; w != nullptr; w = w->next)
   {
      w->wakeupCondition.set();
      ThreadSleepMs(10);
      w->~WaitQueueWaiter();
   }
}

/**
 * Clear queue
 */
void MsgWaitQueue::clear()
{
   m_mutex.lock();

   for(WaitQueueUnclaimedMessage *m = m_messagesHead->next; m != nullptr; m = m->next)
   {
      DeleteMessage(m);
   }
   m_unclaimedMessagesPool.reset();
   m_messagesHead = m_unclaimedMessagesPool.allocate();
   memset(m_messagesHead, 0, sizeof(WaitQueueUnclaimedMessage));
   m_messagesTail = m_messagesHead;

   for(WaitQueueWaiter *w = m_waiters->next; w != nullptr; w = w->next)
   {
      w->wakeupCondition.set();
   }
   m_waitersPool.reset();
   m_waiters = m_waitersPool.allocate();
   memset(m_waiters, 0, sizeof(WaitQueueWaiter));

   m_mutex.unlock();
}

/**
 * Put message into queue
 */
void MsgWaitQueue::put(bool isBinary, uint16_t code, uint32_t id, void *msg)
{
   time_t now = time(nullptr);

   LockGuard lockGuard(m_mutex);

   // Delete expired messages
   if (m_lastExpirationCheck < now - 60)
   {
      for(WaitQueueUnclaimedMessage *m = m_messagesHead->next, *p = m_messagesHead; m != nullptr; p = m, m = m->next)
      {
         if (m->timestamp < now - m_holdTime)
         {
            DeleteMessage(m);

            p->next = m->next;
            if (m_messagesTail == m)
               m_messagesTail = p;

            m_unclaimedMessagesPool.free(m);
            m = p;
         }
      }
      m_lastExpirationCheck = now;
   }

   for(WaitQueueWaiter *w = m_waiters->next, *p = m_waiters; w != nullptr; p = w, w = w->next)
   {
      if ((w->isBinary == isBinary) && (w->code == code) && (w->id == id))
      {
         w->msg = msg;
         w->wakeupCondition.set();
         p->next = w->next;   // Remove waiter from list now to avoid duplicate matches (waiter object will be destroyed in waitForMessage)
         return;
      }
   }

   WaitQueueUnclaimedMessage *m = m_unclaimedMessagesPool.allocate();
   m->msg = msg;
   m->timestamp = now;
   m->code = code;
   m->id = id;
   m->isBinary = isBinary;
   m->next = nullptr;
   m_messagesTail->next = m;
   m_messagesTail = m;
}

/**
 * Wait for message with specific code and ID
 * Function return pointer to the message on success or
 * NULL on timeout or error
 */
void *MsgWaitQueue::waitForMessage(bool isBinary, uint16_t code, uint32_t id, uint32_t timeout)
{
   m_mutex.lock();

   for(WaitQueueUnclaimedMessage *m = m_messagesHead->next, *p = m_messagesHead; m != nullptr; p = m, m = m->next)
   {
      if ((m->isBinary == isBinary) && (m->code == code) && (m->id == id))
      {
         p->next = m->next;
         if (m_messagesTail == m)
         {
            m_messagesTail = p;
         }
         void *msg = m->msg;
         m_unclaimedMessagesPool.free(m);
         m_mutex.unlock();
         return msg;
      }
   }

   WaitQueueWaiter *waiter = new(m_waitersPool.allocate()) WaitQueueWaiter(isBinary, code, id);
   waiter->next = m_waiters->next;
   m_waiters->next = waiter;

   m_mutex.unlock();

   waiter->wakeupCondition.wait(timeout);
   void *msg = waiter->msg;

   m_mutex.lock();
   for(WaitQueueWaiter *w = m_waiters->next, *p = m_waiters; w != nullptr; p = w, w = w->next)
   {
      if (w == waiter)
      {
         p->next = w->next;
         break;
      }
   }
   m_waitersPool.destroy(waiter);
   m_mutex.unlock();

   return msg;
}
