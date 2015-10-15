/* 
** NetXMS - Network Management System
** NetXMS Message Bus Library
** Copyright (C) 2009 Victor Kirhenshtein
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
	return THREAD_OK;
}

/**
 * Constructor
 */
NXMBDispatcher::NXMBDispatcher()
{
	m_queue = new Queue;
	m_numSubscribers = 0;
	m_subscribers = NULL;
	m_filters = NULL;
	m_subscriberListAccess = MutexCreate();
	m_workerThreadHandle = ThreadCreateEx(NXMBDispatcher::workerThreadStarter, 0, this);
   m_callHandlers = new CallHandlerMap();
   m_callHandlerAccess = MutexCreate();
}

/**
 * Destructor
 */
NXMBDispatcher::~NXMBDispatcher()
{
	NXMBMessage *msg;
	int i;

	while((msg = (NXMBMessage *)m_queue->get()) != NULL)
		delete msg;
	m_queue->put(INVALID_POINTER_VALUE);
	ThreadJoin(m_workerThreadHandle);

	delete m_queue;

	MutexDestroy(m_subscriberListAccess);

	for(i = 0; i < m_numSubscribers; i++)
	{
		if ((m_subscribers[i] != NULL) && m_subscribers[i]->isOwnedByDispatcher())
			delete m_subscribers[i];
		if ((m_filters[i] != NULL) && m_filters[i]->isOwnedByDispatcher())
			delete m_filters[i];
	}
	safe_free(m_subscribers);
	safe_free(m_filters);

   MutexDestroy(m_callHandlerAccess);
   delete m_callHandlers;
}

/**
 * Worker thread
 */
void NXMBDispatcher::workerThread()
{
	NXMBMessage *msg;
	int i;

	while(true)
	{
		msg = (NXMBMessage *)m_queue->getOrBlock();
		if (msg == INVALID_POINTER_VALUE)
			break;

		MutexLock(m_subscriberListAccess);
		for(i = 0; i < m_numSubscribers; i++)
		{
			if (m_filters[i]->isAllowed(*msg))
			{
				m_subscribers[i]->messageHandler(*msg);
			}
		}
		MutexUnlock(m_subscriberListAccess);
		delete msg;
	}
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

	MutexLock(m_subscriberListAccess);

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

	MutexUnlock(m_subscriberListAccess);
}

/**
 * Remove subscriber
 */
void NXMBDispatcher::removeSubscriber(const TCHAR *id)
{
	int i;

	MutexLock(m_subscriberListAccess);

	for(i = 0; i < m_numSubscribers; i++)
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

	MutexUnlock(m_subscriberListAccess);
}

/**
 * Add call handler
 */
void NXMBDispatcher::addCallHandler(const TCHAR *callName, NXMBCallHandler handler)
{
   MutexLock(m_callHandlerAccess);
   m_callHandlers->set(callName, handler);
   MutexUnlock(m_callHandlerAccess);
}

/**
 * Remove call handler
 */
void NXMBDispatcher::removeCallHandler(const TCHAR *callName)
{
   MutexLock(m_callHandlerAccess);
   m_callHandlers->remove(callName);
   MutexUnlock(m_callHandlerAccess);
}

/**
 * Make a call
 */
bool NXMBDispatcher::call(const TCHAR *callName, const void *input, void *output)
{
   MutexLock(m_callHandlerAccess);
   NXMBCallHandler handler = m_callHandlers->get(callName);
   MutexUnlock(m_callHandlerAccess);
   return (handler != NULL) ? handler(callName, input, output) : false;
}

/**
 * Synchronization counter
 */
MUTEX NXMBDispatcher::m_instanceAccess = MutexCreate();

/**
 * Global dispatcher instance
 */
NXMBDispatcher *NXMBDispatcher::m_instance = NULL;

/**
 * Get global dispatcher instance
 */
NXMBDispatcher *NXMBDispatcher::getInstance()
{
   MutexLock(m_instanceAccess);
	if (m_instance == NULL)
		m_instance = new NXMBDispatcher();
   MutexUnlock(m_instanceAccess);
	return m_instance;
}
