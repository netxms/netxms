/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: report.cpp
**
**/

#include "nxcore.h"


//
// Redefined status calculation for report group
//

void ReportGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


//
// Constructor
//

Report::Report() : NetObj()
{
   m_iStatus = STATUS_NORMAL;
	m_definition = NULL;
}


//
// Constructor
//

Report::Report(const TCHAR *name) : NetObj()
{
   m_iStatus = STATUS_NORMAL;
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
	m_definition = NULL;
}


//
// Destructor
//

Report::~Report()
{
	safe_free(m_definition);
}


//
// Redefined status calculation for reports
//

void Report::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


//
// Save to database
//

BOOL Report::SaveToDB(DB_HANDLE hdb)
{
	TCHAR query[1024];
	DB_STATEMENT hStmt;

	LockData();

	bool isNewObject = true;
   DB_RESULT hResult = NULL;

	if (!saveCommonProperties(hdb))
		goto fail;

	// Check for object's existence in database
   _sntprintf(query, 256, _T("SELECT id FROM reports WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         isNewObject = false;
      DBFreeResult(hResult);
   }

	if (isNewObject)
	{
      hStmt = DBPrepare(hdb, _T("INSERT INTO reports (id,definition) VALUES (?,?)"));
		if (hStmt == NULL)
			goto fail;
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 2, DB_SQLTYPE_TEXT, CHECK_NULL_EX(m_definition), DB_BIND_STATIC);
	}
	else
	{
      hStmt = DBPrepare(hdb, _T("UPDATE reports SET definition=? WHERE id=?"));
		if (hStmt == NULL)
			goto fail;
		DBBind(hStmt, 1, DB_SQLTYPE_TEXT, CHECK_NULL_EX(m_definition), DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
	}

	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		goto fail;
	}
	DBFreeStatement(hStmt);

	if (!saveACLToDB(hdb))
		goto fail;

	m_bIsModified = FALSE;
	UnlockData();
	return TRUE;

fail:
	UnlockData();
	return FALSE;
}


//
// Delete from database
//

BOOL Report::DeleteFromDB()
{
	TCHAR query[256];

	_sntprintf(query, 256, _T("DELETE FROM reports WHERE id=%d"), m_dwId);
	QueueSQLRequest(query);
	return TRUE;
}


//
// Load from database
//

BOOL Report::CreateFromDB(DWORD dwId)
{
	m_dwId = dwId;

	if (!loadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for report object %d"), dwId);
      return FALSE;
   }

   if (!m_bIsDeleted)
   {
		DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT definition FROM reports WHERE id=?"));
		if (hStmt == NULL)
			return FALSE;

		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult == NULL)
		{
			DBFreeStatement(hStmt);
			return FALSE;
		}

		if (DBGetNumRows(hResult) > 0)
		{
			safe_free(m_definition);
			m_definition = DBGetField(hResult, 0, 0, NULL, 0);
		}
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);

	   loadACLFromDB();
	}

	m_iStatus = STATUS_NORMAL;

	return TRUE;
}


//
// Fill NXCP message with object's data
//

void Report::CreateMessage(CSCPMessage *msg)
{
	NetObj::CreateMessage(msg);
	msg->SetVariable(VID_REPORT_DEFINITION, CHECK_NULL_EX(m_definition));
}


//
// Update network map object from NXCP message
//

DWORD Report::ModifyFromMessage(CSCPMessage *request, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	if (request->IsVariableExist(VID_REPORT_DEFINITION))
	{
		safe_free(m_definition);
		m_definition = request->GetVariableStr(VID_REPORT_DEFINITION);
	}

	return NetObj::ModifyFromMessage(request, TRUE);
}


//
// Start report execution
// Returns assigned job ID
//

DWORD Report::execute(StringMap *parameters, DWORD userId)
{
	ReportJob *job = new ReportJob(this, parameters, userId);
	AddJob(job);
	return job->getId();
}
