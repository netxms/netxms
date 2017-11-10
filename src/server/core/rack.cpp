/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
	m_topBottomNumbering = false;
}

/**
 * Constructor for creating new object
 */
Rack::Rack(const TCHAR *name, int height) : Container(name, 0)
{
	m_height = height;
   m_topBottomNumbering = false;
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
bool Rack::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
	if (!Container::loadFromDatabase(hdb, id))
		return false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT height,top_bottom_num FROM racks WHERE id=?"));
	if (hStmt == NULL)
		return false;

	bool success = false;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			m_height = DBGetFieldLong(hResult, 0, 0);
			m_topBottomNumbering = DBGetFieldLong(hResult, 0, 1) ? true : false;
			success = true;
		}
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);
	return success;
}

/**
 * Save object to database
 */
bool Rack::saveToDatabase(DB_HANDLE hdb)
{
	if (!Container::saveToDatabase(hdb))
		return false;

	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("racks"), _T("id"), m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE racks SET height=?,top_bottom_num=? WHERE id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO racks (height,top_bottom_num,id) VALUES (?,?,?)"));
	}
	if (hStmt == NULL)
		return false;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_height);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_topBottomNumbering ? _T("1") : _T("0"), DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
	BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
	return success;
}

/**
 * Delete object from database
 */
bool Rack::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = Container::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM racks WHERE id=?"));
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Rack::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   Container::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_HEIGHT, (WORD)m_height);
   pMsg->setField(VID_TOP_BOTTOM, (INT16)(m_topBottomNumbering ? 1 : 0));
}

/**
 * Modify object from message
 */
UINT32 Rack::modifyFromMessageInternal(NXCPMessage *pRequest)
{
	if (pRequest->isFieldExist(VID_HEIGHT))
		m_height = (int)pRequest->getFieldAsUInt16(VID_HEIGHT);

   if (pRequest->isFieldExist(VID_TOP_BOTTOM))
      m_topBottomNumbering = (int)pRequest->getFieldAsBoolean(VID_TOP_BOTTOM);

   return Container::modifyFromMessageInternal(pRequest);
}

/**
 * Serialize object to JSON
 */
json_t *Rack::toJson()
{
   json_t *root = Container::toJson();
   json_object_set_new(root, "height", json_integer(m_height));
   json_object_set_new(root, "topBottomNumbering", json_boolean(m_topBottomNumbering));
   return root;
}
