/* 
** NetXMS - Network Management System
** SMS driver emulator for databases
** Copyright (C) 2011 NetXMS Team
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: main.cpp
**
**/

#include "dbemu.h"

static TCHAR s_dbDriver[MAX_PATH];
static TCHAR s_dbName[MAX_DB_NAME];
static TCHAR s_dbUsername[MAX_DB_LOGIN];
static TCHAR s_dbPassword[MAX_DB_PASSWORD];
static TCHAR s_dbServer[MAX_PATH];
static TCHAR s_dbSchema[MAX_DB_NAME];
static TCHAR s_sqlTemplate[1024];
static DB_DRIVER s_driver;
static DB_HANDLE s_dbh;

extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *pszInitArgs)
{
	BOOL bRet = false;
	static NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("DBDriver"), CT_STRING, 0, 0, sizeof(s_dbDriver) / sizeof(TCHAR), 0, s_dbDriver },	
		{ _T("DBName"), CT_STRING, 0, 0, sizeof(s_dbName) / sizeof(TCHAR), 0, s_dbName },	
		{ _T("DBLogin"), CT_STRING, 0, 0, sizeof(s_dbUsername) / sizeof(TCHAR), 0,	s_dbUsername },	
		{ _T("DBPassword"), CT_STRING, 0, 0, sizeof(s_dbPassword) / sizeof(TCHAR), 0,	s_dbPassword },
		{ _T("DBServer"), CT_STRING, 0, 0, sizeof(s_dbServer) / sizeof(TCHAR), 0, s_dbServer },	
		{ _T("DBSchema"), CT_STRING, 0, 0, sizeof(s_dbSchema) / sizeof(TCHAR), 0, s_dbSchema },	
		{ _T("QueryTemplate"), CT_STRING, 0, 0, sizeof(s_sqlTemplate) / sizeof(TCHAR), 0, s_sqlTemplate },	
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	const TCHAR *configFile;
	if (pszInitArgs == NULL || *pszInitArgs == 0)
	{
#ifdef _WIN32
		configFile = _T("C:\\smsdbemu.conf");
#else
		configFile = _T("/etc/smsdbemu.conf");
#endif
	}
	else
	{
		configFile = pszInitArgs;
	}

	Config *config = new Config();
	if (config->loadIniConfig(configFile, _T("SMSDbEmu")) && config->parseTemplate(_T("SMSDbEmu"), configTemplate))
	{
		s_driver = DBLoadDriver(s_dbDriver, NULL, TRUE, NULL, NULL);
		if (s_driver == NULL)
		{
			DbgPrintf(1, _T("%s: Unable to load and initialize database driver \"%s\""), MYNAMESTR, s_dbDriver);
			goto finish;
		}

		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
		s_dbh = DBConnect(s_driver, s_dbServer, s_dbName, s_dbUsername, s_dbPassword, s_dbSchema, errorText);
		if (s_dbh == NULL)
		{
			DbgPrintf(1, _T("%s: Unable to connect to database %s@%s as %s: %s"), MYNAMESTR, s_dbName, s_dbServer, s_dbUsername, errorText);
			DBUnloadDriver(s_driver);
			goto finish;
		}	
		bRet = true;
	}

finish:
	return bRet;
}

extern "C" BOOL EXPORT SMSDriverSend(const TCHAR *pszPhoneNumber, const TCHAR *pszText)
{
	BOOL bRet = false;

	DB_STATEMENT dbs = DBPrepare(s_dbh, s_sqlTemplate);
	if (dbs != NULL)
	{
		DBBind(dbs, 1, DB_SQLTYPE_VARCHAR, pszPhoneNumber, DB_BIND_STATIC);
		DBBind(dbs, 2, DB_SQLTYPE_VARCHAR, pszText, DB_BIND_STATIC);
		if (!(bRet = DBExecute(dbs)))
			DbgPrintf(1, _T("%s: Cannot execute"), MYNAMESTR);
		else
			DbgPrintf(8, _T("%s: sent sms '%s' to %s"), MYNAMESTR, pszText, pszPhoneNumber);
		DBFreeStatement(dbs);
	}

	return bRet;
}


extern "C" void EXPORT SMSDriverUnload()
{
	DBDisconnect(s_dbh);
	DBUnloadDriver(s_driver);
}


//
// DLL Entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif

