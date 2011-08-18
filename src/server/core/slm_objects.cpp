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
** File: slm_objects.cpp
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
	_tcscpy(m_szName, _T("Default"));
}


//
// Constructor for new service object
//

BizService::BizService(const TCHAR *name)
     :Container(name, 0)
{
	m_dwId = 0;
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
}


//
// Service class destructor
//

BizService::~BizService()
{
}

void BizService::calculateCompoundStatus(BOOL bForcedRecalc /*= FALSE*/)
{
	int i, iCount, iMostCriticalStatus;

	LockChildList(FALSE);

	for(i = 0, iCount = 0, iMostCriticalStatus = -1; i < m_dwChildCount; i++)
	{
		int iChildStatus = m_pChildList[i]->PropagatedStatus();
		if ((iChildStatus < STATUS_UNKNOWN) &&
			(iChildStatus > iMostCriticalStatus))
		{
			iMostCriticalStatus = iChildStatus;
			iCount++;
		}
	}
	m_iStatus = (iCount > 0) ? iMostCriticalStatus : STATUS_UNKNOWN;

	UnlockChildList();
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
	TCHAR szQuery[QUERY_LENGTH];

	LockData();

	saveCommonProperties(hdb);
   
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from business_services"));
		return FALSE;
	}
	DBBind(hStmt, 0, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	if (bNewObject)
		nx_strncpy(szQuery, _T("INSERT INTO business_services (service_id) VALUES (?)"), QUERY_LENGTH);
	else
		nx_strncpy(szQuery, _T("UPDATE business_services SET service_id=service_id WHERE service_id=?"), QUERY_LENGTH);
	hStmt = DBPrepare(g_hCoreDB, szQuery);
	if (hStmt == NULL)	
	{
		DbgPrintf(4, _T("Cannot prepare %s from business_services"), bNewObject ? _T("insert") : _T("update"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	if (!DBExecute(hStmt))
	{
		DbgPrintf(4, _T("Cannot execute %s on business_services"), bNewObject ? _T("insert") : _T("update"));
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
   // pMsg->SetVariable(VID_ID, m_dwId);
   // pMsg->SetVariable(VID_STATUS, m_svcStatus);
}


//
// Modify object from message
//

DWORD BizService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

	// if (pRequest->IsVariableExist(VID_STATUS))
	//	m_svcStatus = pRequest->GetVariableLong(VID_STATUS);

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}



//
// SLMCheck default constructor
//

SlmCheck::SlmCheck()
:NetObj()
{
	_tcscpy(m_szName, _T("Default"));
	m_state = FALSE;
	m_script = NULL;
	m_threshold = NULL;
	m_reason[0] = _T('\0');
}


//
// Constructor for new check object
//

SlmCheck::SlmCheck(const TCHAR *name)
:NetObj()
{
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
	m_state = FALSE;
	m_script = NULL;
	m_threshold = NULL;
	m_reason[0] = _T('\0');
}


//
// Service class destructor
//

SlmCheck::~SlmCheck()
{
	safe_delete_and_null(m_threshold);
	safe_free_and_null(m_script);
}

void SlmCheck::calculateCompoundStatus(BOOL bForcedRecalc /*= FALSE*/)
{
	m_iStatus = STATUS_NORMAL;
}

//
// Create object from database data
//

BOOL SlmCheck::CreateFromDB(DWORD id)
{
	const int script_length = 1024;
	DWORD thresholdId;
	m_dwId = id;

	if (!loadCommonProperties())
		return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT type,content,threshold_id,state,reason FROM slm_checks WHERE check_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from slm_checks"));
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
		DbgPrintf(4, _T("Cannot load check object %ld - record missing"), (long)m_dwId);
		return FALSE;
	}

	m_type		= SlmCheck::CheckType(DBGetFieldLong(hResult, 0, 0));
	DBGetField(hResult, 0, 1, m_script, script_length);
	thresholdId = DBGetFieldLong(hResult, 0, 2);
	m_state		= DBGetFieldLong(hResult, 0, 3);
	DBGetField(hResult, 0, 4, m_reason, 255);

	if (thresholdId > 0)
	{
		// FIXME: load threshold
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	// Load access list
	loadACLFromDB();

	return TRUE;
}


//
// Save service to database
//

BOOL SlmCheck::SaveToDB(DB_HANDLE hdb)
{
	return TRUE;
}


//
// Delete object from database
//

BOOL SlmCheck::DeleteFromDB()
{
	TCHAR szQuery[QUERY_LENGTH];
	BOOL bSuccess;

	bSuccess = NetObj::DeleteFromDB();
	if (bSuccess)
	{
		_sntprintf(szQuery, QUERY_LENGTH, _T("DELETE FROM slm_checks WHERE check_id=%d"), m_dwId);
		QueueSQLRequest(szQuery);
	}

	return bSuccess;
}


//
// Create CSCP message with object's data
//

void SlmCheck::CreateMessage(CSCPMessage *pMsg)
{
	NetObj::CreateMessage(pMsg);
	// pMsg->SetVariable(VID_ID, m_dwId);
	// pMsg->SetVariable(VID_STATUS, m_svcStatus);
}


//
// Modify object from message
//

DWORD SlmCheck::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	// if (pRequest->IsVariableExist(VID_STATUS))
	//	m_svcStatus = pRequest->GetVariableLong(VID_STATUS);

	return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// NodeLink default constructor
//

NodeLink::NodeLink()
:NetObj()
{
	_tcscpy(m_szName, _T("Default"));
}


//
// Constructor for new nodelink object
//

NodeLink::NodeLink(const TCHAR *name)
:NetObj()
{
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
}


//
// Nodelink class destructor
//

NodeLink::~NodeLink()
{
	safe_delete_and_null(m_node);
}

void NodeLink::calculateCompoundStatus(BOOL bForcedRecalc /*= FALSE*/)
{
	m_iStatus = STATUS_NORMAL;
}

//
// Create object from database data
//

BOOL NodeLink::CreateFromDB(DWORD id)
{
	const int script_length = 1024;
	DWORD nodeId;
	m_dwId = id;

	if (!loadCommonProperties())
		return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT node_id FROM node_links WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from node_links"));
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
		DbgPrintf(4, _T("Cannot load nodelink object %ld - record missing"), (long)m_dwId);
		return FALSE;
	}

	nodeId	= DBGetFieldLong(hResult, 0, 0);
	if (nodeId > 0)
	{
		m_node = new Node;
		m_node->CreateFromDB(nodeId);
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	// Load access list
	loadACLFromDB();

	return TRUE;
}


//
// Save service to database
//

BOOL NodeLink::SaveToDB(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;
	TCHAR szQuery[QUERY_LENGTH];

	LockData();

	saveCommonProperties(hdb);

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT nodelink_id FROM node_links WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from node_links"));
		return FALSE;
	}
	DBBind(hStmt, 0, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	if (bNewObject)
		nx_strncpy(szQuery, _T("INSERT INTO node_links (node_id, nodelink_id) VALUES (?, ?)"), QUERY_LENGTH);
	else
		nx_strncpy(szQuery, _T("UPDATE node_links SET node_id=? WHERE nodelink_id=?"), QUERY_LENGTH);
	hStmt = DBPrepare(g_hCoreDB, szQuery);
	if (hStmt == NULL)	
	{
		DbgPrintf(4, _T("Cannot prepare %s from node_links"), bNewObject ? _T("insert") : _T("update"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_node == NULL ? 0 : m_node->Id());
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
	if (!DBExecute(hStmt))
	{
		DbgPrintf(4, _T("Cannot execute %s on node_links"), bNewObject ? _T("insert") : _T("update"));
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

BOOL NodeLink::DeleteFromDB()
{
	TCHAR szQuery[QUERY_LENGTH];
	BOOL bSuccess;

	bSuccess = NetObj::DeleteFromDB();
	if (bSuccess)
	{
		_sntprintf(szQuery, QUERY_LENGTH, _T("DELETE FROM node_links WHERE nodelink_id=%d"), m_dwId);
		QueueSQLRequest(szQuery);
	}

	return bSuccess;
}


//
// Create CSCP message with object's data
//

void NodeLink::CreateMessage(CSCPMessage *pMsg)
{
	NetObj::CreateMessage(pMsg);
	// pMsg->SetVariable(VID_ID, m_dwId);
	// pMsg->SetVariable(VID_STATUS, m_svcStatus);
}


//
// Modify object from message
//

DWORD NodeLink::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	// if (pRequest->IsVariableExist(VID_STATUS))
	//	m_svcStatus = pRequest->GetVariableLong(VID_STATUS);

	return NetObj::ModifyFromMessage(pRequest, TRUE);
}
