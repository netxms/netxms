/*
** NetXMS - Network Management System
** Copyright (C) 2020-2025 Raden Solutions
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
 * Check zone access for user
 */
static inline bool CheckZoneAccess(int32_t zoneUIN, uint32_t userId)
{
   if ((zoneUIN == ALL_ZONES) || (userId == 0))
      return true;
   shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
   return (zone != nullptr) ? zone->checkAccessRights(userId, OBJECT_ACCESS_READ) : false;
}

/**
 * Get list of configured SNMP communities for given zone into NXCP message
 */
void ZoneCommunityListToMessage(int32_t zoneUIN, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT community FROM snmp_communities WHERE zone=? ORDER BY id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         uint32_t fieldId = VID_COMMUNITY_STRING_LIST_BASE;
         msg->setField(VID_NUM_STRINGS, (UINT32)count);
         TCHAR buffer[256];
         for(int i = 0; i < count; i++)
         {
            DBGetField(hResult, i, 0, buffer, 256);
            msg->setField(fieldId++, buffer);
         }
         DBFreeResult(hResult);
         msg->setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg->setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
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
void FullCommunityListToMessage(uint32_t userId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT zone,community FROM snmp_communities ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      int dbRecordCount = DBGetNumRows(hResult);
      uint32_t elementCount = 0;
      uint32_t communityFieldId = VID_COMMUNITY_STRING_LIST_BASE;
      uint32_t zoneFieldId = VID_COMMUNITY_STRING_ZONE_LIST_BASE;
      for(int i = 0; i < dbRecordCount; i++)
      {
         int32_t zoneUIN = DBGetFieldULong(hResult, i, 0);
         if (CheckZoneAccess(zoneUIN, userId))
         {
            TCHAR buffer[256];
            msg->setField(communityFieldId++, DBGetField(hResult, i, 1, buffer, 256));
            msg->setField(zoneFieldId++, zoneUIN);
            elementCount++;
         }
      }
      DBFreeResult(hResult);
      msg->setField(VID_NUM_STRINGS, elementCount);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update list of well-known SNMP community strings from NXCP message
 */
uint32_t UpdateCommunityList(const NXCPMessage& request, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!DBBegin(hdb))
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   uint32_t rcc;
   if (ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM snmp_communities WHERE zone=?")))
   {
      rcc = RCC_SUCCESS;
      int count = request.getFieldAsInt32(VID_NUM_STRINGS);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_communities (zone,id,community) VALUES(?,?,?)"), count > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
            uint32_t fieldId = VID_COMMUNITY_STRING_LIST_BASE;
            for (int i = 0; i < count; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
               if (!DBExecute(hStmt))
               {
                  rcc = RCC_DB_FAILURE;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
      }
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   if (rcc == RCC_SUCCESS)
   {
      DBCommit(hdb);
      NotifyClientSessions(NX_NOTIFY_COMMUNITIES_CONFIG_CHANGED, zoneUIN);
   }
   else
   {
      DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Get list of configured SNMP USM credentials for given zone into NXCP message
 */
void ZoneUsmCredentialsListToMessage(int32_t zoneUIN, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   TCHAR query[256];
   _sntprintf(query, 255, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password,comments FROM usm_credentials WHERE zone=%d ORDER BY id ASC"), zoneUIN);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      TCHAR buffer[MAX_DB_STRING];
      int count = DBGetNumRows(hResult);
      msg->setField(VID_NUM_RECORDS, static_cast<uint32_t>(count));
      uint32_t fieldId = VID_USM_CRED_LIST_BASE;
      for(int i = 0; i < count; i++, fieldId += 3)
      {
         DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);  // security name
         msg->setField(fieldId++, buffer);

         msg->setField(fieldId++, static_cast<uint16_t>(DBGetFieldLong(hResult, i, 1))); // auth method
         msg->setField(fieldId++, static_cast<uint16_t>(DBGetFieldLong(hResult, i, 2))); // priv method

         DBGetField(hResult, i, 3, buffer, MAX_DB_STRING);  // auth password
         msg->setField(fieldId++, buffer);

         DBGetField(hResult, i, 4, buffer, MAX_DB_STRING);  // priv password
         msg->setField(fieldId++, buffer);

         msg->setField(fieldId++, zoneUIN); // zone ID

         TCHAR comments[256];
         msg->setField(fieldId++, DBGetField(hResult, i, 5, comments, 256)); //comment
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
void FullUsmCredentialsListToMessage(uint32_t userId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password,zone,comments FROM usm_credentials ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      TCHAR buffer[MAX_DB_STRING];
      int dbRecordCount = DBGetNumRows(hResult);
      uint32_t elementCount = 0;
      uint32_t fieldId = VID_USM_CRED_LIST_BASE;
      for(int i = 0; i < dbRecordCount; i++)
      {
         int32_t zoneUIN = DBGetFieldULong(hResult, i, 5);
         if (CheckZoneAccess(zoneUIN, userId))
         {
            DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);  // security name
            msg->setField(fieldId++, buffer);

            msg->setField(fieldId++, (WORD)DBGetFieldLong(hResult, i, 1)); // auth method
            msg->setField(fieldId++, (WORD)DBGetFieldLong(hResult, i, 2)); // priv method

            DBGetField(hResult, i, 3, buffer, MAX_DB_STRING);  // auth password
            msg->setField(fieldId++, buffer);

            DBGetField(hResult, i, 4, buffer, MAX_DB_STRING);  // priv password
            msg->setField(fieldId++, buffer);

            msg->setField(fieldId++, zoneUIN); // zone ID

            TCHAR comments[256];
            msg->setField(fieldId++, DBGetField(hResult, i, 6, comments, 256)); //comment

            elementCount++;
            fieldId += 3;
         }
      }
      DBFreeResult(hResult);

      msg->setField(VID_NUM_RECORDS, elementCount);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update list of well-known SNMPv3 USM credentials from NXCP message
 */
uint32_t UpdateUsmCredentialsList(const NXCPMessage& request, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!DBBegin(hdb))
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   uint32_t rcc;
   if (ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM usm_credentials WHERE zone=?")))
   {
      rcc = RCC_SUCCESS;
      int count = request.getFieldAsInt32(VID_NUM_RECORDS);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO usm_credentials (zone,id,user_name,auth_method,priv_method,auth_password,priv_password,comments) VALUES(?,?,?,?,?,?,?,?)"), count > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
            uint32_t fieldId = VID_USM_CRED_LIST_BASE;
            for (int i = 0; i < count; i++, fieldId += 4)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (int)request.getFieldAsUInt16(fieldId++)); // Auth method
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (int)request.getFieldAsUInt16(fieldId++)); // Priv method
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
               DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
               DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
               if (!DBExecute(hStmt))
               {
                  rcc = RCC_DB_FAILURE;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
      }
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   if (rcc == RCC_SUCCESS)
   {
      DBCommit(hdb);
      NotifyClientSessions(NX_NOTIFY_USM_CONFIG_CHANGED, zoneUIN);
   }
   else
   {
      DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * In-memory cache of well-known ports
 */
struct WellKnownPort
{
   TCHAR tag[16];
   int32_t zoneUIN;
   int32_t id;
   uint16_t port;
};
static StructArray<WellKnownPort> s_wellKnownPorts;
static Mutex s_wellKnownPortsLock(MutexType::FAST);

/**
 * Get list of well-known ports for given zone and tag
 * @param tag if no ports found, tag based default values are returned: "snmp" - 161, "ssh" - 22
 */
IntegerArray<uint16_t> GetWellKnownPorts(const TCHAR *tag, int32_t zoneUIN)
{
   IntegerArray<uint16_t> ports;

   s_wellKnownPortsLock.lock();
   for(int i = 0; i < s_wellKnownPorts.size(); i++)
   {
      WellKnownPort *p = s_wellKnownPorts.get(i);
      if (((zoneUIN == -1) || (p->zoneUIN == zoneUIN)) && !_tcscmp(p->tag, tag))
      {
         ports.add(p->port);
      }
   }
   s_wellKnownPortsLock.unlock();

   if (ports.size() == 0)
   {
      if (!_tcscmp(tag, _T("agent")))
         ports.add(AGENT_LISTEN_PORT);
      else if (!_tcscmp(tag, _T("snmp")))
         ports.add(161);
      else if(!_tcscmp(tag, _T("ssh")))
         ports.add(22);
      else if(!_tcscmp(tag, _T("vnc")))
         ports.add(5900);
   }

   return ports;
}

/**
 * Get list of configured ports for all zones into NXCP message
 */
void FullWellKnownPortListToMessage(const TCHAR *tag, uint32_t userId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT zone,port FROM well_known_ports WHERE tag=? ORDER BY zone,id"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int dbRecordCount = DBGetNumRows(hResult);
         uint32_t elementCount = 0;
         uint32_t fieldId = VID_ZONE_PORT_LIST_BASE;
         for(int i = 0; i < dbRecordCount; i++)
         {
            int32_t zoneUIN = DBGetFieldLong(hResult, i, 0);
            if (CheckZoneAccess(zoneUIN, userId))
            {
               msg->setField(fieldId++, static_cast<uint16_t>(DBGetFieldLong(hResult, i, 1)));
               msg->setField(fieldId++, zoneUIN);
               fieldId += 8;
               elementCount++;
            }
         }
         msg->setField(VID_ZONE_PORT_COUNT, elementCount);
         msg->setField(VID_RCC, RCC_SUCCESS);
         DBFreeResult(hResult);
      }
      else
      {
         msg->setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured ports for given zone into NXCP message
 */
void ZoneWellKnownPortListToMessage(const TCHAR *tag, int32_t zoneUIN, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT port FROM well_known_ports WHERE tag=? AND zone=? ORDER BY id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, zoneUIN);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         uint32_t fieldId = VID_ZONE_PORT_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            msg->setField(fieldId++, static_cast<uint16_t>(DBGetFieldLong(hResult, i, 0)));
         }
         msg->setField(VID_ZONE_PORT_COUNT, (UINT32)count);
         msg->setField(VID_RCC, RCC_SUCCESS);
         DBFreeResult(hResult);
      }
      else
      {
         msg->setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update list of well-known ports from NXCP message
 */
uint32_t UpdateWellKnownPortList(const NXCPMessage& request, const TCHAR *tag, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!DBBegin(hdb))
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   StructArray<WellKnownPort> newPorts(0, 32);

   uint32_t rcc;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM well_known_ports WHERE tag=? AND zone=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, zoneUIN);
      if (DBExecute(hStmt))
      {
         rcc = RCC_SUCCESS;
         int count = request.getFieldAsInt32(VID_ZONE_PORT_COUNT);
         if (count > 0)
         {
            DB_STATEMENT hStmt2 = DBPrepare(hdb, _T("INSERT INTO well_known_ports (id,port,zone,tag) VALUES(?,?,?,?)"), count > 1);
            if (hStmt2 != nullptr)
            {
               DBBind(hStmt2, 3, DB_SQLTYPE_INTEGER, zoneUIN);
               DBBind(hStmt2, 4, DB_SQLTYPE_VARCHAR, tag, DB_BIND_STATIC);

               uint32_t fieldId = VID_ZONE_PORT_LIST_BASE;
               for(int i = 0; i < count; i++)
               {
                  uint16_t portNumber = request.getFieldAsUInt16(fieldId++);
                  DBBind(hStmt2, 1, DB_SQLTYPE_INTEGER, i + 1);
                  DBBind(hStmt2, 2, DB_SQLTYPE_INTEGER, portNumber);
                  if (!DBExecute(hStmt2))
                  {
                     rcc = RCC_DB_FAILURE;
                     break;
                  }

                  WellKnownPort *p = newPorts.addPlaceholder();
                  p->id = i + 1;
                  p->port = portNumber;
                  _tcslcpy(p->tag, tag, 16);
                  p->zoneUIN = zoneUIN;
               }
               DBFreeStatement(hStmt2);
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }
         }
         else
         {
            rcc = RCC_SUCCESS;
         }
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   if (rcc == RCC_SUCCESS)
   {
      DBCommit(hdb);

      s_wellKnownPortsLock.lock();
      for(int i = 0; i < s_wellKnownPorts.size(); i++)
      {
         WellKnownPort *p = s_wellKnownPorts.get(i);
         if ((p->zoneUIN == zoneUIN) && !_tcscmp(p->tag, tag))
         {
            s_wellKnownPorts.remove(i);
            i--;
         }
      }
      s_wellKnownPorts.addAll(newPorts);
      s_wellKnownPortsLock.unlock();

      NotifyClientSessions(NX_NOTIFY_PORTS_CONFIG_CHANGED, zoneUIN);
   }
   else
   {
      DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Load well known port list into memory on startup
 */
void LoadWellKnownPortList()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT tag,zone,id,port FROM well_known_ports ORDER BY zone,id"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         WellKnownPort *p = s_wellKnownPorts.addPlaceholder();
         DBGetField(hResult, i, 0, p->tag, 16);
         p->zoneUIN = DBGetFieldInt32(hResult, i, 1);
         p->id = DBGetFieldInt32(hResult, i, 2);
         p->port = DBGetFieldUInt16(hResult, i, 3);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of configured agent secrets for all zones into NXCP message
 */
void FullAgentSecretListToMessage(uint32_t userId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT zone,secret FROM shared_secrets ORDER BY zone DESC, id ASC"));
   if (hResult != nullptr)
   {
      int dbRecordCount = DBGetNumRows(hResult);
      uint32_t elementCount = 0;
      uint32_t fieldId = VID_SHARED_SECRET_LIST_BASE;
      for(int i = 0; i < dbRecordCount; i++)
      {
         int32_t zoneUIN = DBGetFieldULong(hResult, i, 0);
         if (CheckZoneAccess(zoneUIN, userId))
         {
            TCHAR buffer[MAX_SECRET_LENGTH];
            msg->setField(fieldId++, DBGetField(hResult, i, 1, buffer, MAX_SECRET_LENGTH));
            msg->setField(fieldId++, zoneUIN);
            fieldId += 8;
            elementCount++;
         }
      }
      DBFreeResult(hResult);
      msg->setField(VID_NUM_ELEMENTS, elementCount);
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
void ZoneAgentSecretListToMessage(int32_t zoneUIN, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT secret FROM shared_secrets WHERE zone=? ORDER BY id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         uint32_t fieldId = VID_SHARED_SECRET_LIST_BASE;
         msg->setField(VID_NUM_ELEMENTS, (UINT32)count);
         for(int i = 0; i < count; i++)
         {
            TCHAR buffer[MAX_SECRET_LENGTH];
            msg->setField(fieldId++, DBGetField(hResult, i, 0, buffer, MAX_SECRET_LENGTH));
         }
         DBFreeResult(hResult);
         msg->setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg->setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update list of well-known agent secrets from NXCP message
 */
uint32_t UpdateAgentSecretList(const NXCPMessage& request, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!DBBegin(hdb))
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   uint32_t rcc;
   if (ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM shared_secrets WHERE zone=?")))
   {
      rcc = RCC_SUCCESS;
      int count = request.getFieldAsInt32(VID_NUM_ELEMENTS);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO shared_secrets (zone,id,secret) VALUES(?,?,?)"), count > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
            uint32_t fieldId = VID_SHARED_SECRET_LIST_BASE;
            for (int i = 0; i < count; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId++), DB_BIND_DYNAMIC);
               if (!DBExecute(hStmt))
               {
                  rcc = RCC_DB_FAILURE;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
      }
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   if (rcc == RCC_SUCCESS)
   {
      DBCommit(hdb);
      NotifyClientSessions(NX_NOTIFY_SECRET_CONFIG_CHANGED, zoneUIN);
   }
   else
   {
      DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Get list of SSH credentials for given zone + all zones credentials (zone = -1)
 */
StructArray<SSHCredentials> GetSSHCredentials(int32_t zoneUIN)
{
   StructArray<SSHCredentials> credentials;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT login,password,key_id FROM ssh_credentials WHERE (zone_uin=? OR zone_uin=-1) ORDER BY zone_uin DESC, id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            SSHCredentials *c = credentials.addPlaceholder();
            DBGetField(hResult, i, 0, c->login, MAX_USER_NAME);
            DBGetField(hResult, i, 1, c->password, MAX_PASSWORD);
            c->keyId = DBGetFieldULong(hResult, i, 2);
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return credentials;
}

/**
 * Get list of SSH credentials into NXCP message
 * @param zoneUIN zone UIN or -1 for "all zones" credentials
 */
void ZoneSSHCredentialsListToMessage(int32_t zoneUIN, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT login,password,key_id FROM ssh_credentials WHERE zone_uin=? ORDER BY zone_uin DESC, id ASC"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         TCHAR loginBuff[MAX_USER_NAME];
         TCHAR passwordBuff[MAX_PASSWORD];

         int count = DBGetNumRows(hResult);
         msg->setField(VID_NUM_ELEMENTS, count);

         uint32_t fieldId = VID_ELEMENT_LIST_BASE;
         for (int i = 0; i < count; i++, fieldId += 7)
         {
            msg->setField(fieldId++, DBGetField(hResult, i, 0, loginBuff, MAX_USER_NAME));
            msg->setField(fieldId++, DBGetField(hResult, i, 1, passwordBuff, MAX_PASSWORD));
            msg->setField(fieldId++, DBGetFieldLong(hResult, i, 2));
         }

         DBFreeResult(hResult);
         msg->setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg->setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get list of SSH credentials into NXCP message
 */
void FullSSHCredentialsListToMessage(uint32_t userId, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT zone_uin,login,password,key_id FROM ssh_credentials ORDER BY zone_uin,id"));
   if (hResult != nullptr)
   {
      TCHAR loginBuff[MAX_USER_NAME];
      TCHAR passwordBuff[MAX_PASSWORD];

      int dbRecordCount = DBGetNumRows(hResult);
      uint32_t elementCount = 0;
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for (int i = 0; i < dbRecordCount; i++)
      {
         int32_t zoneUIN = DBGetFieldLong(hResult, i, 0);
         if (CheckZoneAccess(zoneUIN, userId))
         {
            msg->setField(fieldId++, zoneUIN);
            msg->setField(fieldId++, DBGetField(hResult, i, 1, loginBuff, MAX_USER_NAME));
            msg->setField(fieldId++, DBGetField(hResult, i, 2, passwordBuff, MAX_PASSWORD));
            msg->setField(fieldId++, DBGetFieldLong(hResult, i, 3));
            fieldId += 6;
            elementCount++;
         }
      }

      DBFreeResult(hResult);

      msg->setField(VID_NUM_ELEMENTS, elementCount);
      msg->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Update list of well-known SSH credentials from NXCP message
 */
uint32_t UpdateSSHCredentials(const NXCPMessage& request, int32_t zoneUIN)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!DBBegin(hdb))
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   uint32_t rcc;
   if (ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM ssh_credentials WHERE zone_uin=?")))
   {
      rcc = RCC_SUCCESS;
      int count = request.getFieldAsInt32(VID_NUM_ELEMENTS);
      if (count > 0)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO ssh_credentials (zone_uin,id,login,password,key_id) VALUES(?,?,?,?,?)"), count > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, zoneUIN);
            uint32_t fieldId = VID_ELEMENT_LIST_BASE;
            for (int i = 0; i < count; i++, fieldId += 10)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i + 1);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId), DB_BIND_DYNAMIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, request.getFieldAsString(fieldId + 1), DB_BIND_DYNAMIC);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, request.getFieldAsInt32(fieldId + 2));
               if (!DBExecute(hStmt))
               {
                  rcc = RCC_DB_FAILURE;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
      }
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   if (rcc == RCC_SUCCESS)
   {
      DBCommit(hdb);
      NotifyClientSessions(NX_NOTIFY_SSH_CREDENTIALS_CHANGED, zoneUIN);
   }
   else
   {
      DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}
