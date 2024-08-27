/* 
** NetXMS - Network Management System
** NetXMS Message Bus Library
** Copyright (C) 2009-2016 Victor Kirhenshtein
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
** File: dispatcher.cpp
**
**/

#include "libnxmb.h"

/**
 * Worker thread starter
 */
THREAD_RESULT THREAD_CALL NXMBDispatcher::workerThreadStarter(void *arg)
{
	((NXMBDispatcher *)arg)->workerThread();
   ((NXMBDispatcher *)arg)->m_stopCondition.set();
	return THREAD_OK;
}

/**
 * Constructor
 */
NXMBDispatcher::NXMBDispatcher() : m_subscriberListAccess(MutexType::FAST), m_callHandlerAccess(MutexType::FAST), m_startCondition(true), m_stopCondition(true)
{
	m_queue = new Queue;
	m_numSubscribers = 0;
	m_subscribers = nullptr;
	m_filters = nullptr;
	m_workerThreadHandle = INVALID_THREAD_HANDLE;
   m_callHandlers = new CallHandlerMap();
}

/**
 * Destructor
 */
NXMBDispatcher::~NXMBDispatcher()
{
	NXMBMessage *msg;
	while((msg = (NXMBMessage *)m_queue->get()) != nullptr)
		delete msg;

   if (m_workerThreadHandle != INVALID_THREAD_HANDLE)
   {
      // ThreadJoin cannot be used here because at least on
      // Windows waiting on thread from DLL unload handler
      // will cause deadlock
	   ThreadDetach(m_workerThreadHandle);
	   m_queue->put(INVALID_POINTER_VALUE);
      m_stopCondition.wait(30000);
   }
	delete m_queue;

	for(int i = 0; i < m_numSubscribers; i++)
	{
		if ((m_subscribers[i] != NULL) && m_subscribers[i]->isOwnedByDispatcher())
			delete m_subscribers[i];
		if ((m_filters[i] != NULL) && m_filters[i]->isOwnedByDispatcher())
			delete m_filters[i];
	}
	MemFree(m_subscribers);
	MemFree(m_filters);

   delete m_callHandlers;
}

/**
 * Worker thread
 */
void NXMBDispatcher::workerThread()
{
   nxlog_debug_tag(NXMB_DEBUG_TAG, 3, _T("NXMB dispatcher thread started"));
   m_startCondition.set();
	while(true)
	{
		NXMBMessage *msg = (NXMBMessage *)m_queue->getOrBlock();
		if (msg == INVALID_POINTER_VALUE)
			break;

      nxlog_debug(7, _T("NXMB: processing message %s from %s"), msg->getType(), msg->getSenderId());

		m_subscriberListAccess.lock();
		for(int i = 0; i < m_numSubscribers; i++)
		{
			if (m_filters[i]->isAllowed(*msg))
			{
				m_subscribers[i]->messageHandler(*msg);
			}
		}
		m_subscriberListAccess.unlock();
		delete msg;
	}
   nxlog_debug_tag(NXMB_DEBUG_TAG, 3, _T("NXMB dispatcher thread stopped"));
}

/**
 * Post message
 */
void NXMBDispatcher::postMessage(NXMBMessage *msg)
{
	m_queue->put(msg);
}

/**
 * Add subscriber
 */
void NXMBDispatcher::addSubscriber(NXMBSubscriber *subscriber, NXMBFilter *filter)
{
	int i;

   m_subscriberListAccess.lock();

	for(i = 0; i < m_numSubscribers; i++)
	{
		if ((m_subscribers[i] != NULL) && (!_tcscmp(m_subscribers[i]->getId(), subscriber->getId())))
		{
			// Subscriber already registered, replace it
			if (m_subscribers[i] != subscriber)
			{
				// Different object with same ID
				if (m_subscribers[i]->isOwnedByDispatcher())
					delete m_subscribers[i];
				m_subscribers[i] = subscriber;
			}

			// Replace filter
			if (m_filters[i] != filter)
			{
				if (m_filters[i]->isOwnedByDispatcher())
					delete m_filters[i];
				m_filters[i] = filter;
			}
			break;
		}
	}

	if (i == m_numSubscribers)		// New subscriber
	{
		m_numSubscribers++;
		m_subscribers = (NXMBSubscriber **)realloc(m_subscribers, sizeof(NXMBSubscriber *) * m_numSubscribers);
		m_filters = (NXMBFilter **)realloc(m_filters, sizeof(NXMBFilter *) * m_numSubscribers);
		m_subscribers[i] = subscriber;
		m_filters[i] = filter;
	}

   m_subscriberListAccess.unlock();
}

/**
 * Remove subscriber
 */
void NXMBDispatcher::removeSubscriber(const TCHAR *id)
{
   m_subscriberListAccess.lock();

	for(int i = 0; i < m_numSubscribers; i++)
	{
		if ((m_subscribers[i] != NULL) && (!_tcscmp(m_subscribers[i]->getId(), id)))
		{
			if (m_subscribers[i]->isOwnedByDispatcher())
				delete m_subscribers[i];
			if ((m_filters[i] != NULL) && m_filters[i]->isOwnedByDispatcher())
				delete m_filters[i];
			m_numSubscribers--;
			memmove(&m_subscribers[i], &m_subscribers[i + 1], sizeof(NXMBSubscriber *) * (m_numSubscribers - i));
			memmove(&m_filters[i], &m_filters[i + 1], sizeof(NXMBFilter *) * (m_numSubscribers - i));
			break;
		}
	}

   m_subscriberListAccess.unlock();
}

/**
 * Add call handler
 */
void NXMBDispatcher::addCallHandler(const TCHAR *callName, NXMBCallHandler handler)
{
   m_callHandlerAccess.lock();
   m_callHandlers->set(callName, handler);
   m_callHandlerAccess.unlock();
}

/**
 * Remove call handler
 */
void NXMBDispatcher::removeCallHandler(const TCHAR *callName)
{
   m_callHandlerAccess.lock();
   m_callHandlers->remove(callName);
   m_callHandlerAccess.unlock();
}

/**
 * Make a call
 */
bool NXMBDispatcher::call(const TCHAR *callName, const void *input, void *output)
{
   m_callHandlerAccess.lock();
   NXMBCallHandler handler = m_callHandlers->get(callName);
   m_callHandlerAccess.unlock();
   if (handler == nullptr)
   {
      nxlog_debug_tag(NXMB_DEBUG_TAG, 7, _T("NXMB call handler %s not registered"), callName);
      return false;
   }
   bool success = handler(callName, input, output);
   nxlog_debug_tag(NXMB_DEBUG_TAG, 7, _T("NXMB call to %s %s"), callName, success ? _T("successful") : _T("failed"));
   return success;
}

/**
 * Global dispatcher instance
 */
static NXMBDispatcher s_instance;

/**
 * Instance access lock
 */
static Mutex s_instanceLock;

/**
 * Get global dispatcher instance
 */
NXMBDispatcher *NXMBDispatcher::getInstance()
{
   s_instanceLock.lock();
   if (s_instance.m_workerThreadHandle == INVALID_THREAD_HANDLE)
   {
      s_instance.m_workerThreadHandle = ThreadCreateEx(NXMBDispatcher::workerThreadStarter, 0, &s_instance);
      s_instance.m_startCondition.wait(INFINITE);
   }
   s_instanceLock.unlock();
	return &s_instance;
}
