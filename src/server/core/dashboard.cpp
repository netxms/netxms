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
** File: netmap.cpp
**
**/

#include "nxcore.h"


/**
 * Default constructor
 */
Dashboard::Dashboard() : Container()
{
	m_elements = new ObjectArray<DashboardElement>();
	m_elements->setOwner(true);
	m_numColumns = 1;
	m_options = 0;
	m_iStatus = STATUS_NORMAL;
}

/**
 * Constructor for creating new dashboard object
 */
Dashboard::Dashboard(const TCHAR *name) : Container(name, 0)
{
	m_elements = new ObjectArray<DashboardElement>();
	m_elements->setOwner(true);
	m_numColumns = 1;
	m_options = 0;
	m_iStatus = STATUS_NORMAL;
}

/**
 * Destructor
 */
Dashboard::~Dashboard()
{
	delete m_elements;
}

/**
 * Redefined status calculation
 */
void Dashboard::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}

/**
 * Create object from database
 */
BOOL Dashboard::CreateFromDB(UINT32 dwId)
{
	if (!Container::CreateFromDB(dwId))
		return FALSE;

	m_iStatus = STATUS_NORMAL;

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT num_columns,options FROM dashboards WHERE id=%d"), (int)dwId);
	DB_RESULT hResult = DBSelect(g_hCoreDB, query);
	if (hResult == NULL)
		return FALSE;
	if (DBGetNumRows(hResult) > 0)
	{
		m_numColumns = (int)DBGetFieldLong(hResult, 0, 0);
		m_options = DBGetFieldULong(hResult, 0, 1);
	}
	DBFreeResult(hResult);

	_sntprintf(query, 256, _T("SELECT element_type,element_data,layout_data FROM dashboard_elements ")
								  _T("WHERE dashboard_id=%d ORDER BY element_id"), (int)dwId);
	hResult = DBSelect(g_hCoreDB, query);
	if (hResult == NULL)
		return FALSE;

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		DashboardElement *e = new DashboardElement;
		e->m_type = (int)DBGetFieldLong(hResult, i, 0);
		e->m_data = DBGetField(hResult, i, 1, NULL, 0);
		e->m_layout = DBGetField(hResult, i, 2, NULL, 0);
		m_elements->add(e);
	}

	DBFreeResult(hResult);
	return TRUE;
}

/**
 * Save object to database
 */
BOOL Dashboard::SaveToDB(DB_HANDLE hdb)
{
	LockData();

	// Check for object's existence in database
   DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("dashboards"), _T("id"), m_dwId))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE dashboards SET num_columns=?,options=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dashboards (num_columns,options,id) VALUES (?,?,?)"));
   }
   if (hStmt == NULL)
      goto fail;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT32)m_numColumns);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_options);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_dwId);
   if (!DBExecute(hStmt))
		goto fail;
   DBFreeStatement(hStmt);

   // Save elements
   hStmt = DBPrepare(hdb, _T("DELETE FROM dashboard_elements WHERE dashboard_id=?"));
   if (hStmt == NULL)
		goto fail;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
   if (!DBExecute(hStmt))
		goto fail;
   DBFreeStatement(hStmt);

   hStmt = DBPrepare(hdb, _T("INSERT INTO dashboard_elements (dashboard_id,element_id,element_type,element_data,layout_data) VALUES (?,?,?,?,?)"));
   if (hStmt == NULL)
		goto fail;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
   for(int i = 0; i < m_elements->size(); i++)
   {
		DashboardElement *element = m_elements->get(i);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)i);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)element->m_type);
      DBBind(hStmt, 4, DB_SQLTYPE_TEXT, element->m_data, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, element->m_layout, DB_BIND_STATIC);
      if (!DBExecute(hStmt))
			goto fail;
   }

   DBFreeStatement(hStmt);
	UnlockData();
	return Container::SaveToDB(hdb);

fail:
   if (hStmt != NULL)
      DBFreeStatement(hStmt);
	UnlockData();
	return FALSE;
}

/**
 * Delete object from database
 */
bool Dashboard::deleteFromDB(DB_HANDLE hdb)
{
   bool success = Container::deleteFromDB(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dashboards WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dashboard_elements WHERE dashboard_id=?"));
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Dashboard::CreateMessage(CSCPMessage *msg)
{
	Container::CreateMessage(msg);
	msg->SetVariable(VID_NUM_COLUMNS, (WORD)m_numColumns);
	msg->SetVariable(VID_FLAGS, m_options);
	msg->SetVariable(VID_NUM_ELEMENTS, (UINT32)m_elements->size());

	UINT32 varId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_elements->size(); i++)
	{
		DashboardElement *element = m_elements->get(i);
		msg->SetVariable(varId++, (WORD)element->m_type);
		msg->SetVariable(varId++, CHECK_NULL_EX(element->m_data));
		msg->SetVariable(varId++, CHECK_NULL_EX(element->m_layout));
		varId += 7;
	}
}

/**
 * Modify object from NXCP message
 */
UINT32 Dashboard::ModifyFromMessage(CSCPMessage *request, BOOL alreadyLocked)
{
	if (!alreadyLocked)
		LockData();

	if (request->isFieldExist(VID_NUM_COLUMNS))
		m_numColumns = (int)request->GetVariableShort(VID_NUM_COLUMNS);

	if (request->isFieldExist(VID_FLAGS))
		m_options = (int)request->GetVariableLong(VID_FLAGS);

	if (request->isFieldExist(VID_NUM_ELEMENTS))
	{
		m_elements->clear();

		int count = (int)request->GetVariableLong(VID_NUM_ELEMENTS);
		UINT32 varId = VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			DashboardElement *e = new DashboardElement;
			e->m_type = (int)request->GetVariableShort(varId++);
			e->m_data = request->GetVariableStr(varId++);
			e->m_layout = request->GetVariableStr(varId++);
			varId += 7;
			m_elements->add(e);
		}
	}

	return Container::ModifyFromMessage(request, TRUE);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Dashboard::showThresholdSummary()
{
	return false;
}
