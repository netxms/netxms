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

#pragma warning(disable: 4530 4786)

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#endif

#include <nms_common.h>
#include <nms_util.h>

#include <stdio.h>
#include <time.h>

#include <string>
#include <iostream>
#include <map>
#include <vector>

#include <sys/stat.h>
#include "httprequest.h"
#include "httpresponse.h"
#include "misc.h"


#ifndef _WIN32
# include <signal.h>
# include <sys/types.h>
# include <sys/wait.h>
#endif


#ifdef _WIN32
#define NXHTTPD_SERVICE_NAME     "nxhttpd"
#endif


//////////////////////////////////////////////////////////////////////////

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
	
	string FilterEnt(string);

	//////////////////////////////////////////////////////////////////////////

	virtual bool HandleRequest(HttpRequest &, HttpResponse &);

private:
	bool DefaultHandleRequest(HttpRequest &, HttpResponse &);
	static THREAD_RESULT THREAD_CALL ClientHandler(void *);
	
	bool m_goDown;

	unsigned short m_port;

	SOCKET m_socket;
	SOCKET m_clientSocket;

	string m_documentRoot;
};

//////////////////////////////////////////////////////////////////////////

class NxClientThreadStorage
{
public:
	SOCKET clientSocket;
	NxHttpd *instance;
};

//////////////////////////////////////////////////////////////////////////

#endif // __NXHTTPD__H__
