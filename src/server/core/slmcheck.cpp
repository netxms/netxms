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
** File: slmcheck.cpp
**
**/

#include "nxcore.h"
#include "nms_objects.h"

#define QUERY_LENGTH		(512)

//
// SLM check default constructor
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
	/*
	if (m_script == NULL && m_threshold == NULL)
		m_iStatus = STATUS_UNKNOWN;
	else
		m_iStatus = STATUS_NORMAL;
	*/
}

//
// Create object from database data
//

BOOL SlmCheck::CreateFromDB(DWORD id)
{
	const int script_length = 1024;
	DWORD thresholdId;
	const int errorMsgLen = 512;
	TCHAR errorMsg[errorMsgLen];

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
	m_script = DBGetField(hResult, 0, 1, NULL, script_length);
	thresholdId = DBGetFieldLong(hResult, 0, 2);
	m_state		= DBGetFieldLong(hResult, 0, 3);
	DBGetField(hResult, 0, 4, m_reason, 255);

	if (thresholdId > 0)
	{
		// FIXME: load threshold
	}

	// Compile script if there is one
	if (m_type == check_script && m_script != NULL)
	{
		m_pCompiledScript = (NXSL_Program*)NXSLCompile(m_script, errorMsg, errorMsgLen);
		if (m_pCompiledScript == NULL)
		{
			DbgPrintf(2, _T("Check %s/%ld script compilation failed - %s"), 
				m_szName, (long)m_dwId, errorMsg); 
		}
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
	BOOL bNewObject = TRUE;
	BOOL ret = FALSE;

	LockData();

	saveCommonProperties(hdb);
   
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT check_id FROM slm_checks WHERE check_id=?"));
	if (hStmt == NULL)
		goto finish;
	DBBind(hStmt, 0, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? _T("INSERT INTO slm_checks (check_id,type,content,threshold_id,state,reason) ")
											  _T("VALUES (?,?,?,?,?,?)") :
											  _T("UPDATE slm_checks SET check_id=?,type=?,content=?,threshold_id=?,state=?,reason=? ")
											  _T("WHERE service_id=?"));
	if (hStmt == NULL)	
		goto finish;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, DWORD(m_type));
	DBBind(hStmt, 3, DB_SQLTYPE_TEXT, m_script, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_threshold ? m_threshold->getId() : 0);
	DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, DWORD(m_state));
	DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_STATIC);
	if (!bNewObject)
		DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_dwId);
	
	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		goto finish;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	saveACLToDB(hdb);
	ret = TRUE;

finish:
	// Unlock object and clear modification flag
	m_bIsModified = FALSE;
	UnlockData();
	return ret;
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
// Execute check
//

void SlmCheck::execute()
{
	NXSL_ServerEnv pEnv;
	NXSL_Value *pValue;

	switch (m_type)
	{
	case check_script:
		if (m_pCompiledScript->run(&pEnv, 0, NULL) == 0)
		{
			pValue = m_pCompiledScript->getResult();
			m_iStatus = pValue->getValueAsInt32() == 0 ? STATUS_NORMAL : STATUS_CRITICAL;
			DbgPrintf(9, _T("%s/%ld ret value %d"), m_szName, (long)m_dwId, pValue->getValueAsInt32());
		}
		else
		{
			DbgPrintf(4, _T("%s/%ld nxsl run() failed, %s"),  m_szName, (long)m_dwId, m_pCompiledScript->getErrorText());
		}
		break;
	case check_threshold:
	default:
		DbgPrintf(4, _T("execute() called for undefined check type, check %s/%ld"), m_szName, (long)m_dwId);
		m_iStatus = STATUS_UNKNOWN;
		break;
	}
}

