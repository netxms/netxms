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
** File: nodelink.cpp
**
**/

#include "nxcore.h"
#include "nms_objects.h"

#define QUERY_LENGTH		(512)

//
// NodeLink default constructor
//

NodeLink::NodeLink() : Container()
{
	_tcscpy(m_szName, _T("Default"));
	m_nodeId = 0;
}


//
// Constructor for new nodelink object
//

NodeLink::NodeLink(const TCHAR *name, DWORD nodeId) : Container(name, 0)
{
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
	m_nodeId = nodeId;
}


//
// Nodelink class destructor
//

NodeLink::~NodeLink()
{
}


//
// Calculate status for compound object based on childs status
// ! Copy/paste from BizService::calculateCompoundStatus()
//

void NodeLink::calculateCompoundStatus(BOOL bForcedRecalc /*= FALSE*/)
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

BOOL NodeLink::CreateFromDB(DWORD id)
{
	const int script_length = 1024;
	m_dwId = id;

	if (!Container::CreateFromDB(id))
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

	m_nodeId	= DBGetFieldLong(hResult, 0, 0);
	if (m_nodeId <= 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load nodelink object %ld - node id is missing"), (long)m_dwId);
		return FALSE;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	return TRUE;
}


//
// Save service to database
//

BOOL NodeLink::SaveToDB(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;

	LockData();

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT nodelink_id FROM node_links WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from node_links"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? _T("INSERT INTO node_links (node_id,nodelink_id) VALUES (?,?)") :
											  _T("UPDATE node_links SET node_id=? WHERE nodelink_id=?"));
	if (hStmt == NULL)	
	{
		// DbgPrintf(4, _T("Cannot prepare %s from node_links"), bNewObject ? _T("insert") : _T("update"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_nodeId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dwId);
	if (!DBExecute(hStmt))
	{
		// DbgPrintf(4, _T("Cannot execute %s on node_links"), bNewObject ? _T("insert") : _T("update"));
		DBFreeStatement(hStmt);
		return FALSE;
	}
	DBFreeStatement(hStmt);

	saveACLToDB(hdb);

	// Unlock object and clear modification flag
	m_bIsModified = FALSE;
	UnlockData();
	return Container::SaveToDB(hdb);
}


//
// Delete object from database
//

BOOL NodeLink::DeleteFromDB()
{
	TCHAR szQuery[QUERY_LENGTH];
	BOOL bSuccess;

	bSuccess = Container::DeleteFromDB();
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
	pMsg->SetVariable(VID_NODE_ID, m_nodeId);
}


//
// Modify object from message
//

DWORD NodeLink::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	if (pRequest->IsVariableExist(VID_NODE_ID))
	{
		m_nodeId = pRequest->GetVariableLong(VID_NODE_ID);
	}

	return NetObj::ModifyFromMessage(pRequest, TRUE);
}

//
// Execute underlying checks for this node link
//

void NodeLink::execute()
{
	DbgPrintf(9, _T("NodeLink::execute() started for id %ld"), long(m_dwId));

   LockChildList(FALSE);
	for (int i = 0; i < int(m_dwChildCount); i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_SLMCHECK)
			((SlmCheck*)m_pChildList[i])->execute();
	}
   UnlockChildList();

	DbgPrintf(9, _T("NodeLink::execute() finished for id %ld"), long(m_dwId));
}