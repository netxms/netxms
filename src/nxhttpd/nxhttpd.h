/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006 Alex Kirhenshtein
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
** File: nxhttpd.h
**
**/

#ifndef __NXHTTPD__H__
#define __NXHTTPD__H__

#include <nms_common.h>
#include <nms_util.h>
#include <nxclapi.h>

#ifdef _WIN32
#define NXHTTPD_SERVICE_NAME     "nxhttpd"
#endif


//
// Generic HTTP daemon instance
//

class NxHttpd
{
public:
	NxHttpd();
	virtual ~NxHttpd();

	//////////////////////////////////////////////////////////////////////////

	bool Start(void);
	bool Stop(void);

	//////////////////////////////////////////////////////////////////////////

	void SetPort(int);
	void SetDocumentRoot(string);

	//////////////////////////////////////////////////////////////////////////
	
	String FilterEnt(String);

	//////////////////////////////////////////////////////////////////////////

	virtual BOOL HandleRequest(HttpRequest &, HttpResponse &);

private:
	BOOL DefaultHandleRequest(HttpRequest &, HttpResponse &);
	static THREAD_RESULT THREAD_CALL ClientHandler(void *);
	
	BOOL m_goDown;

	unsigned short m_port;

	SOCKET m_socket;
	SOCKET m_clientSocket;

	String m_documentRoot;
};

//////////////////////////////////////////////////////////////////////////

class NxClientThreadStorage
{
public:
	SOCKET clientSocket;
	NxHttpd *instance;
};


#endif // __NXHTTPD__H__
