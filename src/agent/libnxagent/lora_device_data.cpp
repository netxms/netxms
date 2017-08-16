/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Raden Solutions
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
** File: lora_device_data.cpp
**
**/

#include "libnxagent.h"

/**
 * Create new Lora Device Data object from NXCPMessage
 */
LoraDeviceData::LoraDeviceData(NXCPMessage *request)
{
   m_guid = request->getFieldAsGUID(VID_GUID);
   if (request->getFieldAsUInt32(VID_REG_TYPE) == 0) // OTAA
      m_devEui = request->getFieldAsMacAddress(VID_MAC_ADDR);
   else
   {
      char devAddr[12];
      request->getFieldAsMBString(VID_DEVICE_ADDRESS, devAddr, 12);
      m_devAddr = MacAddress::parse(devAddr);
   }

   memset(m_payload, 0, 36);
   m_decoder = request->getFieldAsInt32(VID_DECODER);
   m_dataRate[0] = 0;
   m_rssi = 1;
   m_snr = -100;
   m_freq = 0;
   m_fcnt = 0;
   m_port = 0;
   m_lastContact = 0;
}

/**
 * Create Lora Device Data object from DB record
 */
LoraDeviceData::LoraDeviceData(DB_RESULT result, int row)
{
   m_guid = DBGetFieldGUID(result, row, 0);
   m_devAddr = DBGetFieldMacAddr(result, row, 1);
   m_devEui = DBGetFieldMacAddr(result, row, 2);
   m_decoder = DBGetFieldULong(result, row, 3);
   m_lastContact = DBGetFieldULong(result, row, 4);

   memset(m_payload, 0, 36);
   m_dataRate[0] = 0;
   m_rssi = 0;
   m_snr = -100;
   m_freq = 0;
   m_fcnt = 0;
   m_port = 0;
}

/**
 * Update Lora device data object in DB
 */
UINT32 LoraDeviceData::saveToDB(bool isNew) const
{
   UINT32 rcc = ERR_IO_FAILURE;

      DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
      DB_STATEMENT hStmt;
      if (isNew)
         hStmt = DBPrepare(hdb, _T("INSERT INTO device_decoder_map(devAddr,devEui,decoder,last_contact,guid) VALUES (?,?,?,?,?)"));
      else
         hStmt = DBPrepare(hdb, _T("UPDATE device_decoder_map SET devAddr=?,devEui=?,decoder=?,last_contact=? WHERE guid=?"));

      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_devAddr.length() > 0 ? (const TCHAR*)m_devAddr.toString(MAC_ADDR_FLAT_STRING) : _T(""), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_devEui.length() > 0 ? (const TCHAR*)m_devEui.toString(MAC_ADDR_FLAT_STRING) : _T(""), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_decoder);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)m_lastContact);
         DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_guid);
         if (DBExecute(hStmt))
            rcc = ERR_SUCCESS;
         else
            rcc = ERR_EXEC_FAILED;

         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);

      return rcc;
}

/**
 * Remove device from local map and DB
 */
UINT32 LoraDeviceData::deleteFromDB() const
{
   UINT32 rcc = ERR_IO_FAILURE;

   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM device_decoder_map WHERE guid=?"));

   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_guid);
      if (DBExecute(hStmt))
         rcc = ERR_SUCCESS;
      else
         rcc = ERR_EXEC_FAILED;

      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return rcc;
}
