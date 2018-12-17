/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: swpkg.cpp
**
**/

#include "nxcore.h"

/**
 * Extract value from source table cell
 */
#define EXTRACT_VALUE(name, field) \
	{ \
		if (!_tcsicmp(table->getColumnName(i), _T(name))) \
		{ \
			const TCHAR *value = table->getAsString(row, i); \
			pkg->field = MemCopyString(value); \
			continue; \
		} \
	}

/**
 * Create from agent table
 *
 * @param table table received from agent
 * @param row row number in a table
 * @return new object on success, NULL on parse error
 */
SoftwarePackage *SoftwarePackage::createFromTableRow(const Table *table, int row)
{
   SoftwarePackage *pkg = new SoftwarePackage();
   for(int i = 0; i < table->getNumColumns(); i++)
   {
      EXTRACT_VALUE("NAME", m_name);
      EXTRACT_VALUE("VERSION", m_version);
      EXTRACT_VALUE("VENDOR", m_vendor);
      EXTRACT_VALUE("URL", m_url);
      EXTRACT_VALUE("DESCRIPTION", m_description);

      if (!_tcsicmp(table->getColumnName(i), _T("DATE")))
         pkg->m_date = (time_t)table->getAsInt(row, i);
   }

   if (pkg->m_name == NULL)
   {
      delete pkg;
      return NULL;
   }
   return pkg;
}

/**
 * Constructor
 *
 * @param table table received from agent
 * @param row row number in a table
 */
SoftwarePackage::SoftwarePackage()
{
	m_name = NULL;
	m_version = NULL;
	m_vendor = NULL;
	m_date = 0;
	m_url = NULL;
	m_description = NULL;
	m_changeCode = CHANGE_NONE;
}

/**
 * Constructor to load from database
 *
 * @param database query result
 * @param row id
 */
SoftwarePackage::SoftwarePackage(DB_RESULT result, int row)
{
   m_name = DBGetField(result, row, 0, NULL, 0);
   m_version = DBGetField(result, row, 1, NULL, 0);
   m_vendor = DBGetField(result, row, 2, NULL, 0);
   m_date = (time_t)DBGetFieldULong(result, row, 3);
   m_url = DBGetField(result, row, 4, NULL, 0);
   m_description = DBGetField(result, row, 5, NULL, 0);
   m_changeCode = CHANGE_NONE;
}

/**
 * Destructor
 */
SoftwarePackage::~SoftwarePackage()
{
	free(m_name);
	free(m_version);
	free(m_vendor);
	free(m_url);
	free(m_description);
}

/**
 * Fill NXCP message with package data
 */
void SoftwarePackage::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
	UINT32 varId = baseId;
	msg->setField(varId++, CHECK_NULL_EX(m_name));
	msg->setField(varId++, CHECK_NULL_EX(m_version));
	msg->setField(varId++, CHECK_NULL_EX(m_vendor));
	msg->setField(varId++, static_cast<UINT32>(m_date));
	msg->setField(varId++, CHECK_NULL_EX(m_url));
	msg->setField(varId++, CHECK_NULL_EX(m_description));
}

/**
 * Save software package data to database
 */
bool SoftwarePackage::saveToDatabase(DB_HANDLE hdb, UINT32 nodeId) const
{
   bool result = false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO software_inventory (name,version,vendor,date,url,description,node_id) VALUES (?,?,?,?,?,?,?)"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_version, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<UINT32>(m_date));
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_url, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, nodeId);

      result = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   return result;
}
