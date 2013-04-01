/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: rack.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
Rack::Rack() : Container()
{
	m_height = 42;
}

/**
 * Constructor for creating new object
 */
Rack::Rack(const TCHAR *name, int height) : Container(name, 0)
{
	m_height = height;
}

/**
 * Destructor
 */
Rack::~Rack()
{
}

/**
 * Create object from database data
 */
BOOL Rack::CreateFromDB(DWORD id)
{
	if (!Container::CreateFromDB(id))
		return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT height FROM racks WHERE id=?"));
	if (hStmt == NULL)
		return FALSE;

	BOOL success = FALSE;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			m_height = DBGetFieldLong(hResult, 0, 0);
			success = TRUE;
		}
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);
	return success;
}

/**
 * Save object to database
 */
BOOL Rack::SaveToDB(DB_HANDLE hdb)
{
	if (!Container::SaveToDB(hdb))
		return FALSE;

	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("racks"), _T("id"), m_dwId))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE racks SET height=? WHERE id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO racks (height,id) VALUES (?,?)"));
	}
	if (hStmt == NULL)
		return FALSE;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_height);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
	BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
	return success;
}

/**
 * Delete object from database
 */
BOOL Rack::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = Container::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM racks WHERE id=%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}

/**
 * Create NXCP message with object's data
 */
void Rack::CreateMessage(CSCPMessage *pMsg)
{
   Container::CreateMessage(pMsg);
   pMsg->SetVariable(VID_HEIGHT, (WORD)m_height);
}

/**
 * Modify object from message
 */
DWORD Rack::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

	if (pRequest->IsVariableExist(VID_HEIGHT))
		m_height = (int)pRequest->GetVariableShort(VID_HEIGHT);

   return Container::ModifyFromMessage(pRequest, TRUE);
}
