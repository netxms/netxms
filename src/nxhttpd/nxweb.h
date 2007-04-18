#if !defined(AFX_NXWEB_H__07A21931_F638_4A98_914C_70D09408483E__INCLUDED_)
#define AFX_NXWEB_H__07A21931_F638_4A98_914C_70D09408483E__INCLUDED_

#pragma warning (disable : 4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>

#include <nms_common.h>
#include <nms_util.h>
#include <nms_cscp.h>
#include <nxclapi.h>

#include "nxhttpd.h"
#include "nxpie.h"

//////////////////////////////////////////////////////////////////////////

class Session
{
public:
	Session()
	{
		sid = "";
		userName = "";
		lastSeen = 0;
	}

	~Session()
	{
	}

	string sid;
	string userName;
	time_t lastSeen;
	NXC_SESSION handle;
};

//////////////////////////////////////////////////////////////////////////

typedef map<string, Session *> tSessionsMap;

//////////////////////////////////////////////////////////////////////////

struct NXC_OBJECT_INDEX
{
   DWORD dwKey;
   NXC_OBJECT *pObject;
};

//////////////////////////////////////////////////////////////////////////

class NxWeb : public NxHttpd
{
public:
	NxWeb();
	virtual ~NxWeb();

	bool HandleRequest(HttpRequest &, HttpResponse &);

private:
	MUTEX m_sessionsMutex;
	tSessionsMap sessions;

	Session *CheckSession(std::string);

	bool DoLogin(HttpRequest, HttpResponse &, string &);
	bool DoLogout(Session *);
	bool DoOverview(Session *, HttpRequest, HttpResponse &);
	bool DoAlarms(Session *, HttpRequest, HttpResponse &);
	bool DoObjects(Session *, HttpRequest, HttpResponse &);
	bool DoObjectsList(Session *, HttpRequest, HttpResponse &);
	bool DoObjectInfo(Session *, HttpRequest, HttpResponse &);

	//////////////////////////////////////////////////////////////////////////
	
	string _Login(HttpRequest, std::string);
	string _Overview(HttpRequest, std::string, NXC_SERVER_STATS *);
	string _Alarms(HttpRequest, Session *);
	string _Objects(HttpRequest, std::string);
};


//
// Functions
//

BOOL Initialize(void);
void Shutdown(void);
THREAD_RESULT THREAD_CALL Main(void *);

#ifdef _WIN32

void InitService(void);
void NXHTTPDInstallService(char *pszExecName);
void NXHTTPDRemoveService(void);
void NXHTTPDStartService(void);
void NXHTTPDStopService(void);

#endif


//
// Global variables
//

extern char g_szConfigFile[MAX_PATH];
extern char g_szLogFile[MAX_PATH];
extern char g_szMasterServer[MAX_OBJECT_NAME];


#endif // !defined(AFX_NXWEB_H__07A21931_F638_4A98_914C_70D09408483E__INCLUDED_)
