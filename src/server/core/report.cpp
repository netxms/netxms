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
	m_parameters = new ObjectArray<ReportParameter>(8, 8, true);
}


//
// Destructor
//

Report::~Report()
{
	delete m_parameters;
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
      _sntprintf(query, 1024, _T("INSERT INTO reports (id,definition) VALUES (%d,'!--empty--!')"), m_dwId);
		if (!DBQuery(hdb, query))
			goto fail;
	}

	if (!saveACLToDB(hdb))
		goto fail;

   // Save parameters
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM report_parameters WHERE report_id=?"));
	if (hStmt == NULL)
		goto fail;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
   if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		goto fail;
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(hdb, _T("INSERT INTO report_parameters (report_id,param_id,name,description,data_type,default_value) VALUES (?,?,?,?,?,?)"));
	if (hStmt == NULL)
		goto fail;

	for(int i = 0; i < m_parameters->size(); i++)
	{
		ReportParameter *p = m_parameters->get(i);
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)i);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, p->getName(), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(p->getDescription()), DB_BIND_STATIC);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)p->getDataType());
		DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, CHECK_NULL_EX(p->getDefaultValue()), DB_BIND_STATIC);
		if (!DBExecute(hStmt))
		{
			DBFreeStatement(hStmt);
			goto fail;
		}
	}

	DBFreeStatement(hStmt);

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
	_sntprintf(query, 256, _T("DELETE FROM report_parameters WHERE report_id=%d"), m_dwId);
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
	   loadACLFromDB();

	   // Load parameters
      DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT name,description,data_type,default_value FROM report_parameters WHERE report_id=? ORDER BY param_id"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
				int count = DBGetNumRows(hResult);
				for(int i = 0; i < count; i++)
					m_parameters->add(new ReportParameter(hResult, i));
				DBFreeResult(hResult);
			}
			DBFreeStatement(hStmt);
		}
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
}


//
// Update network map object from NXCP message
//

DWORD Report::ModifyFromMessage(CSCPMessage *request, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	return NetObj::ModifyFromMessage(request, TRUE);
}
