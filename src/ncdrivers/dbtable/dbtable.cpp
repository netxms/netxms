/* 
** NetXMS - Network Management System
** SMS driver emulator for databases
** Copyright (C) 2011-2019 Raden Solutions
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
** File: dbtable.cpp
**
**/

#include "dbtable.h"

#define DEBUG_TAG _T("ncd.dbtable")

/**
 * DBTable driver class
 */
class DBTableDriver : public NCDriver
{
private:
	TCHAR m_dbDriver[MAX_PATH];
	TCHAR m_dbName[MAX_DB_NAME];
	TCHAR m_dbUsername[MAX_DB_LOGIN];
	TCHAR m_dbPassword[MAX_DB_PASSWORD];
	TCHAR m_dbServer[MAX_PATH];
	TCHAR m_dbSchema[MAX_DB_NAME];
	TCHAR m_sqlTemplate[1024];
	UINT32 m_maxNumberLength;
	UINT32 m_maxMessageLength;
	DB_DRIVER m_driver;
	DB_HANDLE m_dbh;

	DBTableDriver();

public:
   static DBTableDriver *createFromConfig(Config *config);
   virtual ~DBTableDriver();
   virtual int send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body) override;
};


DBTableDriver::DBTableDriver()
{
	_tcscpy(m_dbDriver, _T("sqlite.ddr"));
	_tcscpy(m_dbName, _T("netxms"));
	_tcscpy(m_dbUsername, _T("netxms"));
	_tcscpy(m_dbPassword, _T(""));
	_tcscpy(m_dbServer, _T("localhost"));
	_tcscpy(m_dbSchema, _T(""));
	_tcscpy(m_sqlTemplate, _T(""));
	m_maxNumberLength = 32;
	m_maxMessageLength = 255;
	m_driver = NULL;
	m_dbh = NULL;
}

/**
 * Create driver from config
 */
DBTableDriver *DBTableDriver::createFromConfig(Config *config)
{
	DBTableDriver *d= new DBTableDriver();
	bool bRet = false;
	NX_CFG_TEMPLATE configTemplate[] = 
	{
		{ _T("DBDriver"), CT_STRING, 0, 0, sizeof(d->m_dbDriver) / sizeof(TCHAR), 0, d->m_dbDriver },	
		{ _T("DBName"), CT_STRING, 0, 0, sizeof(d->m_dbName) / sizeof(TCHAR), 0, d->m_dbName },	
		{ _T("DBLogin"), CT_STRING, 0, 0, sizeof(d->m_dbUsername) / sizeof(TCHAR), 0,	d->m_dbUsername },	
		{ _T("DBPassword"), CT_STRING, 0, 0, sizeof(d->m_dbPassword) / sizeof(TCHAR), 0,	d->m_dbPassword },
		{ _T("DBServer"), CT_STRING, 0, 0, sizeof(d->m_dbServer) / sizeof(TCHAR), 0, d->m_dbServer },	
		{ _T("DBSchema"), CT_STRING, 0, 0, sizeof(d->m_dbSchema) / sizeof(TCHAR), 0, d->m_dbSchema },	
		{ _T("MaxMessageLength"), CT_LONG, 0, 0, 0, 0, &(d->m_maxMessageLength) },	
		{ _T("MaxNumberLength"), CT_LONG, 0, 0, 0, 0, &(d->m_maxNumberLength) },	
		{ _T("QueryTemplate"), CT_STRING, 0, 0, sizeof(d->m_sqlTemplate) / sizeof(TCHAR), 0, d->m_sqlTemplate },	
		{ _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
	};

	if (config->parseTemplate(_T("DbTable"), configTemplate))
	{
		d->m_driver = DBLoadDriver(d->m_dbDriver, NULL, NULL, NULL);
		if (d->m_driver == NULL)
		{
			nxlog_debug_tag(DEBUG_TAG, 1, _T("Unable to load and initialize database driver \"%s\""), d->m_dbDriver);
			goto finish;
		}

      DecryptPassword(d->m_dbUsername, d->m_dbPassword, d->m_dbPassword, MAX_DB_PASSWORD);

		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
		d->m_dbh = DBConnect(d->m_driver, d->m_dbServer, d->m_dbName, d->m_dbUsername, d->m_dbPassword, d->m_dbSchema, errorText);
		if (d->m_dbh == NULL) // Do not fail, just report
			nxlog_debug_tag(DEBUG_TAG, 1, _T("Unable to connect to database %s@%s as %s: %s"), d->m_dbName, d->m_dbServer, d->m_dbUsername, errorText);

		bRet = true;
	}

finish:
	if (!bRet && d->m_driver != NULL)
		DBUnloadDriver(d->m_driver);

	if(bRet)
	{
		return d;
	}
	else
	{
		delete d;
		return NULL;
	}
}

/**
 * Send message
 */
int DBTableDriver::send(const TCHAR* recipient, const TCHAR* subject, const TCHAR* body)
{
   int result = -1;

	if (m_dbh == NULL)
	{
		TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
		m_dbh = DBConnect(m_driver, m_dbServer, m_dbName, m_dbUsername, m_dbPassword, m_dbSchema, errorText);
		if (m_dbh == NULL)
			nxlog_debug_tag(DEBUG_TAG, 1, _T("Unable to connect to database %s@%s as %s: %s"), m_dbName, m_dbServer, m_dbUsername, errorText);
	}

	if (m_dbh != NULL)
	{
		DB_STATEMENT dbs = DBPrepare(m_dbh, m_sqlTemplate);
		if (dbs != NULL)
		{
			DBBind(dbs, 1, DB_SQLTYPE_VARCHAR, recipient, DB_BIND_STATIC, m_maxNumberLength);
			DBBind(dbs, 2, DB_SQLTYPE_VARCHAR, body, DB_BIND_STATIC, m_maxMessageLength);
         if (!DBExecute(dbs))
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot execute"));
         }
         else
         {
            result = 0;
            nxlog_debug_tag(DEBUG_TAG, 8, _T("sent sms '%s' to %s"), body, recipient);
         }
			DBFreeStatement(dbs);
		}
	}

   return result;
}

/**
 * Destructor
 */
DBTableDriver::~DBTableDriver()
{
	DBDisconnect(m_dbh);
	DBUnloadDriver(m_driver);
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(DBTable, NULL)
{
   return DBTableDriver::createFromConfig(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif

