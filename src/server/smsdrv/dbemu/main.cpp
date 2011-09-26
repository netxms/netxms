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

static TCHAR dbDriver[MAX_STR];
static TCHAR dbName[MAX_DBPARAM_LEN];
static TCHAR dbUsername[MAX_DBPARAM_LEN];
static TCHAR dbPassword[MAX_DBPARAM_LEN];
static TCHAR dbServer[MAX_DBPARAM_LEN];
static TCHAR dbSchema[MAX_DBPARAM_LEN];
static TCHAR sqlTemplate[MAX_STR];

TCHAR *configFile;
DB_HANDLE dbh;

extern "C" BOOL EXPORT SMSDriverInit(const TCHAR *pszInitArgs)
{
	BOOL bRet = false;
	NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("DBDriver"),			CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	dbDriver },	
		{ _T("DBName"),				CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	dbName },	
		{ _T("DBLogin"),			CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	dbUsername },	
		{ _T("DBPassword"),			CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	dbPassword },
		{ _T("DBServer"),			CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	dbServer },	
		{ _T("DBSchema"),			CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	dbSchema },	
		{ _T("SqlTemplate"),		CT_STRING,	0, 0, MAX_DBPARAM_LEN, 0,	sqlTemplate },	
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	if (pszInitArgs == NULL || *pszInitArgs == 0)
	{
#ifdef _WIN32
		configFile = _tcsdup(_T("\\smsdbemu.conf"));
#else
		configFile = _tcsdup(_T("/etc/smsdbemu.conf"));
#endif
	}
	else
	{
		configFile = _tcsdup(pszInitArgs);
	}

	Config *config = new Config();
	if (config->loadIniConfig(configFile, _T("SMSDbEmu")) && config->parseTemplate(_T("SMSDbEmu"), configTemplate))
	{
		if (!DBInit(0, 0))
		{
			DbgPrintf(1, _T("%s: Unable to initialize database library"), MYNAMESTR);
			goto finish;
		}

		DB_DRIVER driver = DBLoadDriver(dbDriver, NULL, TRUE, NULL, NULL);
		if (driver == NULL)
		{
			DbgPrintf(1, _T("%s: Unable to load and initialize database driver \"%s\""), MYNAMESTR, dbDriver);
			goto finish;
		}

		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
		dbh = DBConnect(driver, dbServer, dbName, dbUsername, dbPassword, dbSchema, errorText);
		if (dbh == NULL)
		{
			DbgPrintf(1, _T("%s: Unable to connect to database %s@%s as %s: %s"), MYNAMESTR, dbName, dbServer, dbUsername, errorText);
			DBUnloadDriver(driver);
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
	TCHAR *realText = EscapeStringForXML(pszText, -1);

	DB_STATEMENT dbs = DBPrepare(dbh, sqlTemplate);
	if (dbs != NULL)
	{
		DBBind(dbs, 1, DB_SQLTYPE_VARCHAR, pszPhoneNumber, DB_BIND_STATIC);
		DBBind(dbs, 2, DB_SQLTYPE_VARCHAR, realText, DB_BIND_STATIC);
		if (!(bRet = DBExecute(dbs)))
			DbgPrintf(1, _T("%s: Cannot execute"), MYNAMESTR);
		else
			DbgPrintf(9, _T("%s: sent sms '%s' to %s"), MYNAMESTR, realText, pszPhoneNumber);
		DBFreeStatement(dbs);
	}

	return bRet;
}


extern "C" void EXPORT SMSDriverUnload()
{
	DBDisconnect(dbh);
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

