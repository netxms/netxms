/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Raden Solutions
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
 * Default constructor
 */
HardwareComponent::HardwareComponent()
{
   m_type[0] = 0;
   m_changeCode = CHANGE_NONE;
   m_index = 0;
   m_vendor[0] = 0;
   m_model[0] = 0;
   m_capacity = 0;
   m_serial[0] = 0;
}

/**
 * Create hardware component form database
 */
HardwareComponent::HardwareComponent(DB_RESULT result, int row)
{
   DBGetField(result, row, 0, m_type, MAX_HARDWARE_NAME);
   m_index = DBGetFieldULong(result, row, 1);
   DBGetField(result, row, 2, m_vendor, MAX_HARDWARE_NAME);
   DBGetField(result, row, 3, m_model, MAX_HARDWARE_NAME);
   m_capacity = DBGetFieldULong(result, row, 4);
   DBGetField(result, row, 5, m_serial, MAX_HARDWARE_NAME);
   m_changeCode = CHANGE_NONE;
}

HardwareComponent::HardwareComponent(const Table *table, int row)
{
   for(int i = 0; i < table->getNumColumns(); i++)
   {
      if (!_tcsicmp(table->getColumnName(i), _T("TYPE")))
         _tcslcpy(m_type, table->getAsString(row, i), MAX_HARDWARE_NAME);
      else if (!_tcsicmp(table->getColumnName(i), _T("HANDLE")))
         m_index = table->getAsUInt(row, i);
      else if (!_tcsicmp(table->getColumnName(i), _T("MANUFACTURER")))
         _tcslcpy(m_vendor, table->getAsString(row, i), MAX_HARDWARE_NAME);
      else if (!_tcsicmp(table->getColumnName(i), _T("PART_NUMBER")))
         _tcslcpy(m_model, table->getAsString(row, i), MAX_HARDWARE_NAME);
      else if (!_tcsicmp(table->getColumnName(i), _T("MAX_SPEED")))
         m_capacity = table->getAsUInt(row, i);
      else if (!_tcsicmp(table->getColumnName(i), _T("SERIAL_NUMBER")))
         _tcslcpy(m_serial, table->getAsString(row, i), MAX_HARDWARE_NAME);
   }

   m_changeCode = CHANGE_NONE;
}
/**
 * Fill NXCPMessage
 */
void HardwareComponent::fillMessage(NXCPMessage *msg, UINT32 baseId) const
{
   UINT32 varId = baseId;
   msg->setField(varId++, m_type);
   msg->setField(varId++, m_index);
   msg->setField(varId++, m_vendor);
   msg->setField(varId++, m_model);
   msg->setField(varId++, m_capacity);
   msg->setField(varId++, m_serial);
}

/**
 * Save to database
 */
bool HardwareComponent::saveToDatabase(DB_HANDLE hdb, UINT32 nodeId) const
{
   bool result = false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO hardware_inventory (component_type,component_index,vendor,model,capacity,serial,node_id) VALUES (?,?,?,?,?,?,?)"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_type, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_index);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_vendor, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_model, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_capacity);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_serial, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, nodeId);

      result = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   return result;
}
