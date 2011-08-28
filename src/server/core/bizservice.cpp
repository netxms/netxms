/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 NetXMS Team
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
** File: bizservice.cpp
**
**/

#include "nxcore.h"
#include "nms_objects.h"

#define QUERY_LENGTH		(512)

//
// Service default constructor
//

BizService::BizService()
     :Container()
{
	m_dwId = 0;
	m_busy = false;
	m_lastPollTime = time_t(0);
	_tcscpy(m_szName, _T("Default"));
}


//
// Constructor for new service object
//

BizService::BizService(const TCHAR *name)
     :Container(name, 0)
{
	m_dwId = 0;
	m_busy = false;
	m_lastPollTime = time_t(0);
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
}


//
// Service class destructor
//

BizService::~BizService()
{
}

//
// Calculate status for compound object based on childs status
//

void BizService::calculateCompoundStatus(BOOL bForcedRecalc /*= FALSE*/)
{
	int i, iCount, iMostCriticalStatus;
	int iOldStatus = m_iStatus;

	// Calculate own status by selecting the most critical status of the kids
	LockChildList(FALSE);
	for(i = 0, iCount = 0, iMostCriticalStatus = -1; i < int(m_dwChildCount); i++)
	{
		int iChildStatus = m_pChildList[i]->Status();
		if ((iChildStatus < STATUS_UNKNOWN) &&
			(iChildStatus > iMostCriticalStatus))
		{
			iMostCriticalStatus = iChildStatus;
			iCount++;
		}
	}
	m_iStatus = (iCount > 0) ? iMostCriticalStatus : STATUS_UNKNOWN;
	UnlockChildList();

	// Cause parent object(s) to recalculate it's status
	if ((iOldStatus != m_iStatus) || bForcedRecalc)
	{
		LockParentList(FALSE);
		for(i = 0; i < int(m_dwParentCount); i++)
			m_pParentList[i]->calculateCompoundStatus();
		UnlockParentList();
		Modify();   /* LOCK? */
	}
}

//
// Create object from database data
//

BOOL BizService::CreateFromDB(DWORD id)
{
	m_dwId = id;

	if (!loadCommonProperties())
		return FALSE;

	// now it doesn't make any sense but hopefully will do in the future
	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from business_services"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load biz service object %ld - record missing"), (long)m_dwId);
		return FALSE;
	}

	// m_svcStatus	= DBGetFieldULong(hResult, 0, 0);

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	// Load access list
	loadACLFromDB();

	return TRUE;
}


//
// Save service to database
//

BOOL BizService::SaveToDB(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;

	LockData();

	saveCommonProperties(hdb);
   
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
		return FALSE;
	DBBind(hStmt, 0, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? _T("INSERT INTO business_services (service_id) VALUES (?)") :
											  _T("UPDATE business_services SET service_id=service_id WHERE service_id=?"));
	if (hStmt == NULL)	
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	saveACLToDB(hdb);

	// Unlock object and clear modification flag
	m_bIsModified = FALSE;
	UnlockData();
	return TRUE;
}


//
// Delete object from database
//

BOOL BizService::DeleteFromDB()
{
   TCHAR szQuery[QUERY_LENGTH];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, QUERY_LENGTH, _T("DELETE FROM business_services WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }

   return bSuccess;
}


//
// Create CSCP message with object's data
//

void BizService::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   // Calling just a base method should do fine
}


//
// Modify object from message
//

DWORD BizService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // ... and here too

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}

bool BizService::isReadyForPolling()
{
	return time(NULL) - m_lastPollTime > g_dwSlmPollingInterval && !m_busy;
}

void BizService::lockForPolling()
{
	m_busy = true;
}

//
// A callback for poller threads
//

void BizService::poll( ClientSession *pSession, DWORD dwRqId, int nPoller )
{
	m_lastPollTime = time(NULL);

	// Loop through the kids and execute their either scripts or thresholds
	for (int i = 0; i < int(m_dwChildCount); i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_SLMCHECK)
			((SlmCheck*)m_pChildList[i])->execute();
	}

	// Set the status based on what the kids' been up to
	calculateCompoundStatus();

	m_busy = false;
}
