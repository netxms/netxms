/* 
** NetXMS - Network Management System
** NetXMS Message Bus API
** Copyright (C) 2009-2015 Victor Kirhenshtein
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
** File: nxmbapi.h
**
**/

#ifndef _nxmbapi_h_
#define _nxmbapi_h_

#ifdef _WIN32
#ifdef LIBNXMB_EXPORTS
#define LIBNXMB_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXMB_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXMB_EXPORTABLE
#endif

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxqueue.h>

/**
 * Message class
 */
class LIBNXMB_EXPORTABLE NXMBMessage
{
protected:
	TCHAR *m_type;
	TCHAR *m_senderId;

public:
	NXMBMessage();
	NXMBMessage(const TCHAR *type, const TCHAR *senderId);
	virtual ~NXMBMessage();

	const TCHAR *getType() const { return m_type; }
	const TCHAR *getSenderId() const { return m_senderId; }
};

/**
 * Subscriber class
 */
class LIBNXMB_EXPORTABLE NXMBSubscriber
{
protected:
	TCHAR *m_id;

public:
	NXMBSubscriber(const TCHAR *id);
	virtual ~NXMBSubscriber();

	const TCHAR *getId() { return CHECK_NULL(m_id); }

	virtual void messageHandler(NXMBMessage &msg);
	virtual bool isOwnedByDispatcher();
};

/**
 * Abstract message filter class
 */
class LIBNXMB_EXPORTABLE NXMBFilter
{
public:
	NXMBFilter();
	virtual ~NXMBFilter();

	virtual bool isAllowed(NXMBMessage &msg);
	virtual bool isOwnedByDispatcher();
};

/**
 * Message filter which accept messages of specific type(s)
 */
class LIBNXMB_EXPORTABLE NXMBTypeFilter : public NXMBFilter
{
protected:
	StringMap m_types;

public:
	NXMBTypeFilter();
	virtual ~NXMBTypeFilter();

	virtual bool isAllowed(NXMBMessage &msg);

	void addMessageType(const TCHAR *type);
	void removeMessageType(const TCHAR *type);
};

/**
 * Call handler
 */
typedef bool (* NXMBCallHandler)(const TCHAR *, const void *, void *);

/**
 * String map template for holding objects as values
 */
class CallHandlerMap : public StringMapBase
{
public:
	CallHandlerMap() : StringMapBase(false) { }

	void set(const TCHAR *name, NXMBCallHandler handler) { setObject((TCHAR *)name, (void *)handler, false); }
	NXMBCallHandler get(const TCHAR *name) { return (NXMBCallHandler)getObject(name); }
};

/**
 * Message dispatcher class
 */
class LIBNXMB_EXPORTABLE NXMBDispatcher
{
private:
	Queue *m_queue;
	int m_numSubscribers;
	NXMBSubscriber **m_subscribers;
	NXMBFilter **m_filters;
	MUTEX m_subscriberListAccess;
	THREAD m_workerThreadHandle;
   CallHandlerMap *m_callHandlers;
   MUTEX m_callHandlerAccess;
   CONDITION m_stopCondition;

	void workerThread();
	static THREAD_RESULT THREAD_CALL workerThreadStarter(void *);

public:
	NXMBDispatcher();
	~NXMBDispatcher();

	void postMessage(NXMBMessage *msg);
   bool call(const TCHAR *callName, const void *input, void *output);
	
	void addSubscriber(NXMBSubscriber *subscriber, NXMBFilter *filter);
	void removeSubscriber(const TCHAR *id);

   void addCallHandler(const TCHAR *callName, NXMBCallHandler handler);
   void removeCallHandler(const TCHAR *callName);

   static NXMBDispatcher *getInstance();
};

#endif   /* _nxmbapi_h_ */
