/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
** File: vobject.cpp
**
**/

#include "nxcore.h"

/**
 * Versionable object constructor
 */
VersionableObject::VersionableObject(NetObj *_this)
{
   m_this = _this;
   m_version = 0;
}

/**
 * Create NXCP message with object's data
 */
void VersionableObject::fillMessage(NXCPMessage *msg)
{
   msg->setField(VID_VERSION, m_version);
}

/**
 * Save object to database
 */
bool VersionableObject::saveToDatabase(DB_HANDLE hdb)
{
   bool success = false;

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("versionable_object"), _T("object_id"), m_this->getId()))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE versionable_object SET version=? WHERE object_id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO versionable_object (version,object_id) VALUES (?,?)"));
   }
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_version);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_this->getId());
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   return success;
}

/**
 * Delete object from database
 */
bool VersionableObject::deleteFromDatabase(DB_HANDLE hdb)
{
   return m_this->executeQueryOnObject(hdb, _T("DELETE FROM versionable_object WHERE object_id=?"));
}

/**
 * Load object from database
 */
bool VersionableObject::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   TCHAR szQuery[256];

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT version FROM versionable_object WHERE object_id=%d"), m_this->getId());
   DB_RESULT hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;

   m_version = DBGetFieldLong(hResult, 0, 0);
   DBFreeResult(hResult);
   return true;
}

/**
 * Update object from imported configuration
 */
void VersionableObject::updateFromImport(ConfigEntry *config)
{
   m_version = config->getSubEntryValueAsUInt(_T("version"), 0, m_version);
}

/**
 * Serialize object to JSON
 */
void VersionableObject::toJson(json_t *root)
{
   json_object_set_new(root, "version", json_integer(m_version));
}
