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
#include <nxnt.h>
#include "messages.h"

#ifdef _WIN32
#define NXHTTPD_SERVICE_NAME     _T("nxhttpd")
#define NXHTTPD_EVENT_SOURCE     _T("NetXMS Web Server")
#define NXHTTPD_SYSLOG_NAME      NXHTTPD_EVENT_SOURCE
#else
#define NXHTTPD_SYSLOG_NAME      _T("nxhttpd")
#endif

#define MAX_MODULE_NAME				128
#define MAX_SID_LEN					(SHA1_DIGEST_SIZE * 2 + 1)
#define MAX_SESSIONS					256
#define MAX_INTERFACE_TYPE			56


//
// Global application flags
//

#define AF_DAEMON				0x00000001
#define AF_USE_SYSLOG		0x00000002
#define AF_DEBUG				0x00000004
#define AF_SHUTDOWN			0x01000000


//
// Timestamp formats
//

#define TS_LONG_DATE_TIME  0
#define TS_LONG_TIME       1
#define TS_DAY_AND_MONTH   2
#define TS_MONTH           3


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

	BOOL GetQueryParam(const TCHAR *pszName, String &value);
	void SetQueryParam(const TCHAR *pszName, const TCHAR *pszValue);
	TCHAR *GetURI(void) { return m_uri; }
	TCHAR *GetRawQuery(void) { return m_rawQuery; }
	const TCHAR *GetMethodName(void);
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
	DWORD m_dwBoxRowNumber;

public:
	HttpResponse();
	~HttpResponse();

	void SetHeader(const TCHAR *pszHdr, const TCHAR *pszValue) { m_headers.Set(pszHdr, pszValue); }
	void SetType(const TCHAR *pszType) { m_headers.Set(_T("Content-type"), pszType); }
	void SetBody(const TCHAR *data, int size = -1, BOOL bAppend = FALSE);
	void AppendBody(const TCHAR *data, int size = -1) { SetBody(data, size, TRUE); }
	void SetCode(int);

	int GetCode(void) { return m_code; }
	TCHAR *GetCodeString(void) { return m_codeString; }

	char *BuildStream(int &);

	// HTML helpers
	void BeginPage(const TCHAR *pszTitle);
	void EndPage(void) { SetBody(_T("</body>\r\n</html>\r\n"), -1, TRUE); }
	void StartBox(const TCHAR *pszTitle, const TCHAR *pszClass = NULL, const TCHAR *pszId = NULL, const TCHAR *pszTableClass = NULL, BOOL bContentOnly = FALSE);
	void StartTableHeader(const TCHAR *pszClass);
	void StartBoxRow(void);
	void EndBox(BOOL bContentOnly = FALSE) { AppendBody(bContentOnly ? _T("</table>") : _T("</table></div>\r\n")); }
};


//
// Client session
//

enum
{
	FORM_OVERVIEW,
	FORM_OBJECTS,
	FORM_ALARMS,
	FORM_TOOLS,
	FORM_CONTROL_PANEL
};

enum
{
	OBJVIEW_CURRENT,
	OBJVIEW_OVERVIEW,
	OBJVIEW_ALARMS,
	OBJVIEW_LAST_VALUES,
	OBJVIEW_PERFORMANCE,
	OBJVIEW_TOOLS,
	OBJVIEW_MANAGE,
	OBJVIEW_LAST_VIEW_CODE
};

enum
{
	OBJECT_ACTION_CREATE,
	OBJECT_ACTION_DELETE,
	OBJECT_ACTION_MANAGE,
	OBJECT_ACTION_UNMANAGE,
	OBJECT_ACTION_PROPERTIES,
	OBJECT_ACTION_SECURITY
};

class ClientSession
{
protected:
	TCHAR m_sid[MAX_SID_LEN];
	DWORD m_dwIndex;
	NXC_SESSION m_hSession;
	time_t m_tmLastAccess;
	int m_nCurrObjectView;

	DWORD m_dwNumAlarms;
	NXC_ALARM *m_pAlarmList;
	MUTEX m_mutexAlarmList;
	int m_nAlarmSortMode;
	int m_nAlarmSortDir;
	DWORD m_dwCurrAlarmRoot;

	DWORD m_dwNumValues;
	NXC_DCI_VALUE *m_pValueList;
	int m_nLastValuesSortMode;
	int m_nLastValuesSortDir;

	void OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm);
	void DeleteAlarmFromList(DWORD dwAlarmId);
	void AddAlarmToList(NXC_ALARM *pAlarm);
	NXC_ALARM *FindAlarmInList(DWORD dwAlarmId);

	void ShowMainMenu(HttpResponse &response);
	void ShowFormOverview(HttpResponse &response);
	void ShowFormObjects(HttpResponse &response);
	void ShowObjectView(HttpRequest &request, HttpResponse &response);
	void ShowObjectOverview(HttpResponse &response, NXC_OBJECT *pObject);
	void ShowObjectTools(HttpResponse &response, NXC_OBJECT *pObject);
	void ShowObjectMgmt(HttpRequest &request, HttpResponse &response, NXC_OBJECT *pObject);
	void SendObjectTree(HttpRequest &request, HttpResponse &response);
	void ShowAlarmList(HttpResponse &response, NXC_OBJECT *pRootObj, BOOL bReload, const TCHAR *pszSelection);
	void SendAlarmList(HttpRequest &request, HttpResponse &response);
	void AlarmCtrlHandler(HttpRequest &request, HttpResponse &response);
	void ShowLastValues(HttpResponse &response, NXC_OBJECT *pObject, BOOL bReload);
	void SendLastValues(HttpRequest &request, HttpResponse &response);
	void ShowPieChart(HttpRequest &request, HttpResponse &response);
	void ShowFormControlPanel(HttpResponse &response);
	void ShowCtrlPanelView(HttpRequest &request, HttpResponse &response);
	void CtrlPanelServerVariables(HttpRequest &request, HttpResponse &response);
	void CtrlPanelUsers(HttpRequest &request, HttpResponse &response, BOOL bUsers);

	void JSON_SendAlarmList(HttpResponse &response);

public:
	ClientSession();
	~ClientSession();

	void EventHandler(DWORD dwEvent, DWORD dwCode, void *pArg);

	void GenerateSID(void);
	void SetIndex(DWORD idx) { m_dwIndex = idx; }

	BOOL IsMySID(TCHAR *sid) { return !_tcsicmp(sid, m_sid); }
	DWORD GetIndex(void) { return m_dwIndex; }
	time_t GetLastAccessTime(void) { return m_tmLastAccess; }
	void SetLastAccessTime(void) { m_tmLastAccess = time(NULL); }

	DWORD DoLogin(TCHAR *pszLogin, TCHAR *pszPasswd);
	BOOL ProcessRequest(HttpRequest &request, HttpResponse &response);

	void ShowForm(HttpResponse &response, int nForm);

	int CompareAlarms(NXC_ALARM *p1, NXC_ALARM *p2);
	int CompareDCIValues(NXC_DCI_VALUE *p1, NXC_DCI_VALUE *p2);

	BOOL ProcessJSONRequest(HttpRequest &request, HttpResponse &response);
};


//
// Pie chart drawing class
//

#define MAX_PIE_ELEMENTS		16
#define MAX_COLORS				16
#define PIE_WIDTH					400
#define PIE_HEIGHT				192

#ifndef PI
#define PI 3.141592653589793238462643
#endif

#ifndef ROUND
#define ROUND(x) (int)floor(x + 0.5)
#endif

class PieChart
{
public:
	PieChart();
	~PieChart();

	BOOL SetValue(const TCHAR *label, double value);
	void SetNoDataLabel(const TCHAR *label);

	BOOL Build(void);
	void Clear(void);
	void *GetRawImage(void) { return m_rawData; }
	int GetRawImageSize(void) { return m_rawDataSize; }

private:
	int m_valueCount;
	TCHAR *m_labels[MAX_PIE_ELEMENTS];
	const TCHAR *m_noDataLabel;
	double m_values[MAX_PIE_ELEMENTS];
	void *m_rawData;
	int m_rawDataSize;

	int m_color[MAX_COLORS];
	int m_colorCount;
};


//
// Functions
//

void DebugPrintf(DWORD dwSessionId, const TCHAR *pszFormat, ...);

BOOL Initialize(void);
void Shutdown(void);
void Main(void);

void InitSessions(void);
BOOL SessionRequestHandler(HttpRequest &request, HttpResponse &response);

TCHAR *EscapeHTMLText(String &text);
void ShowFormLogin(HttpResponse &response, const TCHAR *pszErrorText);
void AddTableHeader(HttpResponse &response, const TCHAR *pszClass, ...);
void ShowErrorMessage(HttpResponse &response, DWORD dwError);
void ShowInfoMessage(HttpResponse &response, const TCHAR *pszText);
void ShowSuccessMessage(HttpResponse &response, const TCHAR *pszText = NULL);
void AddButton(HttpResponse &response, const TCHAR *pszSID, const TCHAR *pszImage, const TCHAR *pszDescription, const TCHAR *pszHandler);
void AddCheckbox(HttpResponse &response, int nId, const TCHAR *pszName, const TCHAR *pszDescription, const TCHAR *pszHandler, BOOL bChecked);
void AddActionLink(HttpResponse &response, const TCHAR *pszSID, const TCHAR *pszName, const TCHAR *pszImage,
						 const TCHAR *pszFunction, const TCHAR *pszArgs);
void AddActionMenu(HttpResponse &response, const TCHAR *sid, ...);

TCHAR *FormatTimeStamp(DWORD dwTimeStamp, TCHAR *pszBuffer, int iType);
DWORD *IdListFromString(const TCHAR *pszStr, DWORD *pdwCount);
BOOL IsListMember(DWORD dwId, DWORD dwCount, DWORD *pdwList);

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
extern DWORD g_dwSessionTimeout;
extern TCHAR g_szConfigFile[];
extern TCHAR g_szLogFile[];
extern TCHAR g_szMasterServer[];
extern TCHAR g_szDocumentRoot[];
extern TCHAR g_szListenAddress[];
extern WORD g_wListenPort;
extern const TCHAR *g_szStatusText[];
extern const TCHAR *g_szStatusTextSmall[];
extern const TCHAR *g_szStatusImageName[];
extern const TCHAR *g_szAlarmState[];
extern const TCHAR *g_szObjectClass[];
extern const TCHAR *g_szInterfaceTypes[];
extern CODE_TO_TEXT g_ctNodeType[];


#endif // __NXHTTPD__H__
