/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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
#include "messages.h"

#ifdef _WIN32
#define NXHTTPD_SERVICE_NAME     "nxhttpd"
#define NXHTTPD_EVENT_SOURCE     "NetXMS Web Server"
#endif

#define MAX_MODULE_NAME				128
#define MAX_SID_LEN					(SHA1_DIGEST_SIZE * 2 + 1)
#define MAX_SESSIONS					256


//
// Global application flags
//

#define AF_DAEMON				0x00000001
#define AF_USE_SYSLOG		0x00000002
#define AF_DEBUG				0x00000004
#define AF_SHUTDOWN			0x01000000


//
// libnxcl internal object index structure
//

struct NXC_OBJECT_INDEX
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
};


//
// HTTP request
//

enum
{
	METHOD_INVALID,
	METHOD_GET,
	METHOD_POST
};

class HttpRequest
{
private:
	String m_raw;

	int m_method;
	String m_uri;
	String m_rawQuery;
	StringMap m_query;

	BOOL IsComplete(void);
	BOOL Parse(void);
	BOOL ParseQueryString(void);
	BOOL ParseParameter(TCHAR *pszParam);

public:
	HttpRequest();
	~HttpRequest();

	BOOL Read(SOCKET);

	BOOL GetQueryParam(TCHAR *pszName, String &value);
	void SetQueryParam(TCHAR *pszName, TCHAR *pszValue);
	TCHAR *GetURI(void);
};


//
// HTTP response
//

enum ResponseCode
{
	HTTP_OK = 200,
	HTTP_MOVEDPERMANENTLY = 301,
	HTTP_FOUND = 302,
	HTTP_BADREQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_FORBIDDEN = 403,
	HTTP_NOTFOUND = 404,
	HTTP_INTERNALSERVERERROR = 500
};

class HttpResponse
{
protected:
	int m_code;
	String m_codeString;
	StringMap m_headers;
	TCHAR *m_body;
	int m_bodyLen;

public:
	HttpResponse();
	~HttpResponse();

	void SetHeader(TCHAR *pszHdr, TCHAR *pszValue) { m_headers.Set(pszHdr, pszValue); }
	void SetType(TCHAR *pszType) { m_headers.Set(_T("Content-type"), pszType); }
	void SetBody(TCHAR *data, int size = -1, BOOL bAppend = FALSE);
	void AppendBody(TCHAR *data, int size = -1) { SetBody(data, size, TRUE); }
	void SetCode(int);

	int GetCode(void) { return m_code; }
	TCHAR *GetCodeString(void) { return m_codeString; }

	char *BuildStream(int &);

	// HTML helpers
	void BeginPage(TCHAR *pszTitle);
	void EndPage(void) { SetBody(_T("</body></html>"), -1, TRUE); }
};


//
// Client session
//

enum
{
	FORM_OVERVIEW,
	FORM_OBJECTS,
	FORM_ALARMS
};

class ClientSession
{
protected:
	TCHAR m_sid[MAX_SID_LEN];
	DWORD m_dwIndex;
	NXC_SESSION m_hSession;

	void ShowMainMenu(HttpResponse &response);
	void ShowFormObjects(HttpResponse &response);
	void SendObjectTree(HttpRequest &request, HttpResponse &response);

public:
	ClientSession();
	~ClientSession();

	void GenerateSID(void);
	void SetIndex(DWORD idx) { m_dwIndex = idx; }

	BOOL IsMySID(TCHAR *sid) { return !_tcsicmp(sid, m_sid); }
	DWORD GetIndex(void) { return m_dwIndex; }

	DWORD DoLogin(TCHAR *pszLogin, TCHAR *pszPasswd);
	BOOL ProcessRequest(HttpRequest &request, HttpResponse &response);

	void ShowForm(HttpResponse &response, int nForm);
};


//
// Functions
//

void InitLog(void);
void CloseLog(void);
void WriteLog(DWORD msg, WORD wType, TCHAR *format, ...);

void DebugPrintf(DWORD dwSessionId, TCHAR *pszFormat, ...);

BOOL Initialize(void);
void Shutdown(void);
void Main(void);

void InitSessions(void);
BOOL SessionRequestHandler(HttpRequest &request, HttpResponse &response);

TCHAR *EscapeHTMLText(String &text);
void ShowFormLogin(HttpResponse &response, TCHAR *pszErrorText);

#ifdef _WIN32

void InitService(void);
void NXHTTPDInstallService(char *pszExecName);
void NXHTTPDRemoveService(void);
void NXHTTPDStartService(void);
void NXHTTPDStopService(void);
void InstallEventSource(TCHAR *pszPath);
void RemoveEventSource(void);

#endif


//
// Global variables
//

extern DWORD g_dwFlags;
extern TCHAR g_szConfigFile[];
extern TCHAR g_szLogFile[];
extern TCHAR g_szMasterServer[];
extern TCHAR g_szDocumentRoot[];
extern WORD g_wListenPort;


#endif // __NXHTTPD__H__
