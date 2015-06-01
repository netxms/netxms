/*
** NetXMS multiplatform core agent
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
** File: dcsnmp.cpp
**
**/

#include "nxagentd.h"

/**
 * SNMP targets
 */
static HashMap<uuid_t, SNMPTarget> s_snmpTargets(true);
static MUTEX s_snmpTargetsLock = MutexCreate();

/**
 * Create SNMP target from NXCP message
 */
SNMPTarget::SNMPTarget(UINT64 serverId, NXCPMessage *msg, UINT32 baseId)
{
   msg->getFieldAsBinary(baseId, m_guid, UUID_LENGTH);
   m_serverId = serverId;
   m_ipAddress = msg->getFieldAsInetAddress(baseId + 1);
   m_snmpVersion = (BYTE)msg->getFieldAsInt16(baseId + 2);
   m_port = msg->getFieldAsUInt16(baseId + 3);
   m_authType = (BYTE)msg->getFieldAsInt16(baseId + 4);
   m_encType = (BYTE)msg->getFieldAsInt16(baseId + 5);
   m_authName = msg->getFieldAsUtf8String(baseId + 6);
   m_authPassword = msg->getFieldAsUtf8String(baseId + 7);
   m_encPassword = msg->getFieldAsUtf8String(baseId + 8);
   m_transport = NULL;
}

/**
 * Create SNMP target from database record
 * Expected field order:
 *   guid,server_id,ip_address,snmp_version,port,auth_type,enc_type,auth_name,auth_pass,enc_pass
 */
SNMPTarget::SNMPTarget(DB_RESULT hResult, int row)
{
   DBGetFieldGUID(hResult, row, 0, m_guid);
   m_serverId = DBGetFieldUInt64(hResult, row, 1);
   m_ipAddress = DBGetFieldInetAddr(hResult, row, 2);
   m_snmpVersion = (BYTE)DBGetFieldLong(hResult, row, 3);
   m_port = (UINT16)DBGetFieldLong(hResult, row, 4);
   m_authType = (BYTE)DBGetFieldLong(hResult, row, 5);
   m_encType = (BYTE)DBGetFieldLong(hResult, row, 6);
   m_authName = DBGetFieldUTF8(hResult, row, 7, NULL, 0);
   m_authPassword = DBGetFieldUTF8(hResult, row, 8, NULL, 0);
   m_encPassword = DBGetFieldUTF8(hResult, row, 9, NULL, 0);
   m_transport = NULL;
}

/**
 * SNMP target destructor
 */
SNMPTarget::~SNMPTarget()
{
   safe_free(m_authName);
   safe_free(m_authPassword);
   safe_free(m_encPassword);
   delete m_transport;
}

/**
 * Save SNMP target object to database
 */
bool SNMPTarget::saveToDatabase()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("dc_snmp_targets"), _T("guid"), m_guid))
      hStmt = DBPrepare(hdb, _T("UPDATE dc_snmp_targets SET server_id=?,ip_address=?,snmp_version=?,port=?,auth_type=?,enc_type=?,auth_name=?,auth_pass=?,enc_pass=? WHERE guid=?"));
   else
      hStmt = DBPrepare(hdb, _T("INSERT INTO dc_snmp_targets (server_id,ip_address,snmp_version,port,auth_type,enc_type,auth_name,auth_pass,enc_pass,guid) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, (const TCHAR *)m_ipAddress.toString(), DB_BIND_TRANSIENT);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)m_snmpVersion);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (INT32)m_port);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (INT32)m_authType);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (INT32)m_encType);
#ifdef UNICODE
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, WideStringFromUTF8String(m_authName), DB_BIND_DYNAMIC);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, WideStringFromUTF8String(m_authPassword), DB_BIND_DYNAMIC);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, WideStringFromUTF8String(m_encPassword), DB_BIND_DYNAMIC);
#else
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_authName, DB_BIND_STATIC);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_authPassword, DB_BIND_STATIC);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_encPassword, DB_BIND_STATIC);
#endif
   TCHAR guidText[64];
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, uuid_to_string(m_guid, guidText), DB_BIND_STATIC);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Get SNMP transport (create if needed)
 */
SNMP_Transport *SNMPTarget::getTransport(UINT16 port)
{
   if (m_transport != NULL)
      return m_transport;

   m_transport = new SNMP_UDPTransport;
	((SNMP_UDPTransport *)m_transport)->createUDPTransport(m_ipAddress, (port != 0) ? port : m_port);
   m_transport->setSnmpVersion(m_snmpVersion);
   SNMP_SecurityContext *ctx = new SNMP_SecurityContext(m_authName, m_authPassword, m_encPassword, m_authType, m_encType);
	ctx->setSecurityModel((m_snmpVersion == SNMP_VERSION_3) ? SNMP_SECURITY_MODEL_USM : SNMP_SECURITY_MODEL_V2C);
   m_transport->setSecurityContext(ctx);
   return m_transport;
}

/**
 * Update SNMP target information
 */
void UpdateSnmpTarget(SNMPTarget *target)
{
   MutexLock(s_snmpTargetsLock);
   s_snmpTargets.set(target->getGuid(), target);
   target->saveToDatabase();
   MutexUnlock(s_snmpTargetsLock);
}

/**
 * Get value from SNMP node
 */
bool GetSnmpValue(const uuid_t& target, UINT16 port, const TCHAR *oid, TCHAR *value, int interpretRawValue)
{
   bool success = false;

   MutexLock(s_snmpTargetsLock);
   SNMPTarget *t = s_snmpTargets.get(target);
   if (t != NULL)
   {
      SNMP_Transport *snmp = t->getTransport(port);
      UINT32 rcc;

      if (interpretRawValue == SNMP_RAWTYPE_NONE)
      {
         rcc = SnmpGetEx(snmp, oid, NULL, 0, value, MAX_RESULT_LENGTH * sizeof(TCHAR), SG_PSTRING_RESULT, NULL);
      }
      else
      {
			BYTE rawValue[1024];
			memset(rawValue, 0, 1024);
         rcc = SnmpGetEx(snmp, oid, NULL, 0, rawValue, 1024, SG_RAW_RESULT, NULL);
			if (rcc == SNMP_ERR_SUCCESS)
			{
				switch(interpretRawValue)
				{
					case SNMP_RAWTYPE_INT32:
						_sntprintf(value, MAX_RESULT_LENGTH, _T("%d"), ntohl(*((LONG *)rawValue)));
						break;
					case SNMP_RAWTYPE_UINT32:
						_sntprintf(value, MAX_RESULT_LENGTH, _T("%u"), ntohl(*((UINT32 *)rawValue)));
						break;
					case SNMP_RAWTYPE_INT64:
						_sntprintf(value, MAX_RESULT_LENGTH, INT64_FMT, (INT64)ntohq(*((INT64 *)rawValue)));
						break;
					case SNMP_RAWTYPE_UINT64:
						_sntprintf(value, MAX_RESULT_LENGTH, UINT64_FMT, ntohq(*((QWORD *)rawValue)));
						break;
					case SNMP_RAWTYPE_DOUBLE:
						_sntprintf(value, MAX_RESULT_LENGTH, _T("%f"), ntohd(*((double *)rawValue)));
						break;
					case SNMP_RAWTYPE_IP_ADDR:
						IpToStr(ntohl(*((UINT32 *)rawValue)), value);
						break;
					case SNMP_RAWTYPE_MAC_ADDR:
						MACToStr(rawValue, value);
						break;
					default:
						value[0] = 0;
						break;
				}
			}
      }
      success = (rcc == SNMP_ERR_SUCCESS);
   }
   else
   {
      TCHAR buffer[64];
      DebugPrintf(INVALID_INDEX, 6, _T("SNMP target with guid %s not found"), uuid_to_string(target, buffer));
   }
   MutexUnlock(s_snmpTargetsLock);
   
   return success;
}
