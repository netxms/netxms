/*
** NetXMS - Network Management System
** Copyright (C) 2020 Raden Solutions
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
** File: network_cred.cpp
**
**/

#include "nxcore.h"

/**
 * Get list of configured SNMP communities for given zone into NXCP message
 */
void GetZoneCommunityList(NXCPMessage *msg, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 255, _T("SELECT community FROM snmp_communities WHERE zone=%d ORDER BY id ASC"), zoneUIN);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      UINT32 stringBase = VID_COMMUNITY_STRING_LIST_BASE;
      msg->setField(VID_NUM_STRINGS, (UINT32)count);
      TCHAR buffer[256];
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, buffer, 256);
         msg->setField(stringBase++, buffer);
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured SNMP communities for all zones into NXCP message
 */
void GetFullCommunityList(NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT community,zone FROM snmp_communities ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      UINT32 stringBase = VID_COMMUNITY_STRING_LIST_BASE, zoneBase = VID_COMMUNITY_STRING_ZONE_LIST_BASE;
      msg->setField(VID_NUM_STRINGS, (UINT32)count);
      TCHAR buffer[256];
      for(int i = 0; i < count; i++)
      {
         DBGetField(hResult, i, 0, buffer, 256);
         msg->setField(stringBase++, buffer);
         msg->setField(zoneBase++, DBGetFieldULong(hResult, i, 1));
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured SNMP USM credentials for given zone into NXCP message
 */
void GetZoneUsmCredentialList(NXCPMessage *msg, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 255, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password,comments FROM usm_credentials WHERE zone=%d ORDER BY id ASC"), zoneUIN);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      TCHAR buffer[MAX_DB_STRING];
      int count = DBGetNumRows(hResult);
      msg->setField(VID_NUM_RECORDS, (UINT32)count);
      for(int i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 3)
      {
         DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);  // security name
         msg->setField(id++, buffer);

         msg->setField(id++, (WORD)DBGetFieldLong(hResult, i, 1)); // auth method
         msg->setField(id++, (WORD)DBGetFieldLong(hResult, i, 2)); // priv method

         DBGetField(hResult, i, 3, buffer, MAX_DB_STRING);  // auth password
         msg->setField(id++, buffer);

         DBGetField(hResult, i, 4, buffer, MAX_DB_STRING);  // priv password
         msg->setField(id++, buffer);

         msg->setField(id++, zoneUIN); // zone ID

         TCHAR comments[256];
         msg->setField(id++, DBGetField(hResult, i, 5, comments, 256)); //comment
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured SNMP USM credentials for all zones into NXCP message
 */
void GetFullUsmCredentialList(NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password,zone,comments FROM usm_credentials ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      TCHAR buffer[MAX_DB_STRING];
      int count = DBGetNumRows(hResult);
      msg->setField(VID_NUM_RECORDS, (UINT32)count);
      for(int i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 3)
      {
         DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);  // security name
         msg->setField(id++, buffer);

         msg->setField(id++, (WORD)DBGetFieldLong(hResult, i, 1)); // auth method
         msg->setField(id++, (WORD)DBGetFieldLong(hResult, i, 2)); // priv method

         DBGetField(hResult, i, 3, buffer, MAX_DB_STRING);  // auth password
         msg->setField(id++, buffer);

         DBGetField(hResult, i, 4, buffer, MAX_DB_STRING);  // priv password
         msg->setField(id++, buffer);

         msg->setField(id++, DBGetFieldULong(hResult, i, 5)); // zone ID

         TCHAR comments[256];
         msg->setField(id++, DBGetField(hResult, i, 6, comments, 256)); //comment
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured SNMP ports for all zones into NXCP message
 */
void GetFullSnmpPortList(NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT port,zone FROM snmp_ports ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      UINT32 baseId = VID_ZONE_SNMP_PORT_LIST_BASE;
      msg->setField(VID_ZONE_SNMP_PORT_COUNT, (UINT32)count);
      for(int i = 0; i < count; i++, baseId +=8)
      {
         msg->setField(baseId++, (uint16_t)DBGetFieldLong(hResult, i, 0));
         msg->setField(baseId++, DBGetFieldULong(hResult, i, 1));
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured SNMP ports for given zone into NXCP message
 */
void GetZoneSnmpPortList(NXCPMessage *msg, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 255, _T("SELECT port FROM snmp_ports WHERE zone=%d ORDER BY id ASC"), zoneUIN);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      UINT32 baseId = VID_ZONE_SNMP_PORT_LIST_BASE;
      msg->setField(VID_ZONE_SNMP_PORT_COUNT, (UINT32)count);
      for(int i = 0; i < count; i++)
      {
         msg->setField(baseId++, (uint16_t)DBGetFieldLong(hResult, i, 0));
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);

}

/**
 * Get list of configured agent secrets for all zones into NXCP message
 */
void GetFullAgentSecretList(NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT secret,zone FROM shared_secrets ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      UINT32 baseId = VID_SHARED_SECRET_LIST_BASE;
      msg->setField(VID_NUM_ELEMENTS, (UINT32)count);
      for(int i = 0; i < count; i++, baseId +=8)
      {
         TCHAR buffer[MAX_SECRET_LENGTH];
         msg->setField(baseId++, DBGetField(hResult, i, 0, buffer, MAX_SECRET_LENGTH));
         msg->setField(baseId++, DBGetFieldULong(hResult, i, 1));
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured agent secrets for all zones into NXCP message
 */
void GetZoneAgentSecretList(NXCPMessage *msg, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 255, _T("SELECT secret FROM shared_secrets WHERE zone=%d ORDER BY id ASC"), zoneUIN);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      UINT32 baseId = VID_SHARED_SECRET_LIST_BASE;
      msg->setField(VID_NUM_ELEMENTS, (UINT32)count);
      for(int i = 0; i < count; i++)
      {
         TCHAR buffer[MAX_SECRET_LENGTH];
         msg->setField(baseId++, DBGetField(hResult, i, 0, buffer, MAX_SECRET_LENGTH));
      }
      DBFreeResult(hResult);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
