/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG _T("dc.snmp")

/**
 * SNMP targets
 */
static SharedHashMap<uuid_t, SNMPTarget> s_snmpTargets;
static Mutex s_snmpTargetsLock;

/**
 * Create SNMP target from NXCP message
 */
SNMPTarget::SNMPTarget(uint64_t serverId, const NXCPMessage& msg, uint32_t baseId)
{
   m_guid = msg.getFieldAsGUID(baseId);
   m_serverId = serverId;
   m_ipAddress = msg.getFieldAsInetAddress(baseId + 1);
   m_snmpVersion = static_cast<SNMP_Version>(msg.getFieldAsInt16(baseId + 2));
   m_port = msg.getFieldAsUInt16(baseId + 3);
   m_authType = static_cast<SNMP_AuthMethod>(msg.getFieldAsInt16(baseId + 4));
   m_encType = static_cast<SNMP_EncryptionMethod>(msg.getFieldAsInt16(baseId + 5));
   m_authName = msg.getFieldAsUtf8String(baseId + 6);
   m_authPassword = msg.getFieldAsUtf8String(baseId + 7);
   m_encPassword = msg.getFieldAsUtf8String(baseId + 8);
   m_transport = nullptr;
}

/**
 * Create SNMP target from database record
 * Expected field order:
 *   guid,server_id,ip_address,snmp_version,port,auth_type,enc_type,auth_name,auth_pass,enc_pass
 */
SNMPTarget::SNMPTarget(DB_RESULT hResult, int row)
{
   m_guid = DBGetFieldGUID(hResult, row, 0);
   m_serverId = DBGetFieldUInt64(hResult, row, 1);
   m_ipAddress = DBGetFieldInetAddr(hResult, row, 2);
   m_snmpVersion = static_cast<SNMP_Version>(DBGetFieldLong(hResult, row, 3));
   m_port = static_cast<uint16_t>(DBGetFieldLong(hResult, row, 4));
   m_authType = static_cast<SNMP_AuthMethod>(DBGetFieldLong(hResult, row, 5));
   m_encType = static_cast<SNMP_EncryptionMethod>(DBGetFieldLong(hResult, row, 6));
   m_authName = DBGetFieldUTF8(hResult, row, 7, nullptr, 0);
   m_authPassword = DBGetFieldUTF8(hResult, row, 8, nullptr, 0);
   m_encPassword = DBGetFieldUTF8(hResult, row, 9, nullptr, 0);
   m_transport = nullptr;
}

/**
 * SNMP target destructor
 */
SNMPTarget::~SNMPTarget()
{
   MemFree(m_authName);
   MemFree(m_authPassword);
   MemFree(m_encPassword);
   delete m_transport;
}

/**
 * Save SNMP target object to database
 */
bool SNMPTarget::saveToDatabase(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("dc_snmp_targets"), _T("guid"), m_guid))
      hStmt = DBPrepare(hdb, _T("UPDATE dc_snmp_targets SET server_id=?,ip_address=?,snmp_version=?,port=?,auth_type=?,enc_type=?,auth_name=?,auth_pass=?,enc_pass=? WHERE guid=?"));
   else
      hStmt = DBPrepare(hdb, _T("INSERT INTO dc_snmp_targets (server_id,ip_address,snmp_version,port,auth_type,enc_type,auth_name,auth_pass,enc_pass,guid) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, m_serverId);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, (const TCHAR *)m_ipAddress.toString(), DB_BIND_TRANSIENT);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_snmpVersion));
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_port));
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_authType));
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_encType));
#ifdef UNICODE
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, WideStringFromUTF8String(m_authName), DB_BIND_DYNAMIC);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, WideStringFromUTF8String(m_authPassword), DB_BIND_DYNAMIC);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, WideStringFromUTF8String(m_encPassword), DB_BIND_DYNAMIC);
#else
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_authName, DB_BIND_STATIC);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_authPassword, DB_BIND_STATIC);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_encPassword, DB_BIND_STATIC);
#endif
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_guid);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Get SNMP transport (create if needed)
 */
SNMP_Transport *SNMPTarget::getTransport(uint16_t port)
{
   if (m_transport != nullptr)
      return m_transport;

   m_transport = new SNMP_UDPTransport;
	static_cast<SNMP_UDPTransport*>(m_transport)->createUDPTransport(m_ipAddress, (port != 0) ? port : m_port);
   m_transport->setSnmpVersion(m_snmpVersion);
   SNMP_SecurityContext *ctx = (m_snmpVersion == SNMP_VERSION_3) ? new SNMP_SecurityContext(m_authName, m_authPassword, m_encPassword, m_authType, m_encType) : new SNMP_SecurityContext(m_authName);
   m_transport->setSecurityContext(ctx);
   return m_transport;
}

/**
 * Add (or replace) SNMP target information
 */
void UpdateSnmpTarget(shared_ptr<SNMPTarget> target)
{
   s_snmpTargetsLock.lock();
   s_snmpTargets.set(target->getGuid().getValue(), target);
   s_snmpTargetsLock.unlock();
}

/**
 * Get value from SNMP node
 */
uint32_t GetSnmpValue(const uuid& target, uint16_t port, SNMP_Version version, const TCHAR *oid, TCHAR *value, int interpretRawValue)
{
   s_snmpTargetsLock.lock();
   shared_ptr<SNMPTarget> t = s_snmpTargets.getShared(target.getValue());
   if (t == nullptr)
   {
      s_snmpTargetsLock.unlock();

      TCHAR buffer[64];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMP target with guid %s not found"), target.toString(buffer));
      return ERR_INTERNAL_ERROR;
   }
   s_snmpTargetsLock.unlock();

   SNMP_Transport *snmp = t->getTransport(port);
   uint32_t rcc;

   if (interpretRawValue == SNMP_RAWTYPE_NONE)
   {
      rcc = SnmpGetEx(snmp, oid, nullptr, 0, value, MAX_RESULT_LENGTH * sizeof(TCHAR), SG_PSTRING_RESULT, nullptr);
   }
   else
   {
		BYTE rawValue[1024];
		memset(rawValue, 0, 1024);
      rcc = SnmpGetEx(snmp, oid, nullptr, 0, rawValue, 1024, SG_RAW_RESULT, nullptr);
		if (rcc == SNMP_ERR_SUCCESS)
		{
			switch(interpretRawValue)
			{
				case SNMP_RAWTYPE_INT32:
				   IntegerToString(static_cast<int32_t>(ntohl(*((uint32_t*)rawValue))), value);
					break;
				case SNMP_RAWTYPE_UINT32:
               IntegerToString(static_cast<uint32_t>(ntohl(*((uint32_t*)rawValue))), value);
					break;
				case SNMP_RAWTYPE_INT64:
               IntegerToString(static_cast<int64_t>(ntohq(*((uint64_t*)rawValue))), value);
					break;
				case SNMP_RAWTYPE_UINT64:
               IntegerToString(ntohq(*((uint64_t*)rawValue)), value);
					break;
				case SNMP_RAWTYPE_DOUBLE:
					_sntprintf(value, MAX_RESULT_LENGTH, _T("%f"), ntohd(*((double *)rawValue)));
					break;
				case SNMP_RAWTYPE_IP_ADDR:
					IpToStr(ntohl(*((uint32_t*)rawValue)), value);
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

   return (rcc == SNMP_ERR_SUCCESS) ? ERR_SUCCESS :
      ((rcc == SNMP_ERR_NO_OBJECT) ? ERR_UNKNOWN_METRIC : ERR_INTERNAL_ERROR);
}

/**
 * Read one row for SNMP table
 */
static uint32_t ReadSNMPTableRow(SNMP_Transport *snmp, const SNMP_ObjectId *rowOid, size_t baseOidLen,
         uint32_t index, const ObjectArray<SNMPTableColumnDefinition> &columns, Table *table)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   for(int i = 0; i < columns.size(); i++)
   {
      const SNMPTableColumnDefinition *c = columns.get(i);
      if (c->getSnmpOid().isValid())
      {
         uint32_t oid[MAX_OID_LEN];
         size_t oidLen = c->getSnmpOid().length();
         memcpy(oid, c->getSnmpOid().value(), oidLen * sizeof(uint32_t));
         if (rowOid != nullptr)
         {
            size_t suffixLen = rowOid->length() - baseOidLen;
            memcpy(&oid[oidLen], rowOid->value() + baseOidLen, suffixLen * sizeof(uint32_t));
            oidLen += suffixLen;
         }
         else
         {
            oid[oidLen++] = index;
         }
         request.bindVariable(new SNMP_Variable(oid, oidLen));
      }
   }

   SNMP_PDU *response;
   uint32_t rc = snmp->doRequest(&request, &response);
   if (rc == SNMP_ERR_SUCCESS)
   {
      if ((response->getNumVariables() >= columns.size()) &&
          (response->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
      {
         table->addRow();
         for(int i = 0; i < response->getNumVariables(); i++)
         {
            SNMP_Variable *v = response->getVariable(i);
            if ((v != nullptr) && (v->getType() != ASN_NO_SUCH_OBJECT) && (v->getType() != ASN_NO_SUCH_INSTANCE))
            {
               const SNMPTableColumnDefinition *c = columns.get(i);
               if ((c != NULL) && c->isConvertSnmpStringToHex())
               {
                  size_t size = v->getValueLength();
                  TCHAR *buffer = MemAllocString(size * 2 + 1);
                  BinToStr(v->getValue(), size, buffer);
                  table->setPreallocated(i, buffer);
               }
               else
               {
                  bool convert = false;
                  TCHAR buffer[1024];
                  table->set(i, v->getValueAsPrintableString(buffer, 1024, &convert));
               }
            }
         }
      }
      delete response;
   }
   return rc;
}

/**
 * Get table from SNMP node
 */
uint32_t GetSnmpTable(const uuid& target, uint16_t port, SNMP_Version version, const TCHAR *oid,
         const ObjectArray<SNMPTableColumnDefinition> &columns, Table *value)
{
   s_snmpTargetsLock.lock();
   shared_ptr<SNMPTarget> t = s_snmpTargets.getShared(target.getValue());
   if (t == nullptr)
   {
      s_snmpTargetsLock.unlock();

      TCHAR buffer[64];
      nxlog_debug_tag(DEBUG_TAG, 6, _T("SNMP target with guid %s not found"), target.toString(buffer));
      return ERR_INTERNAL_ERROR;
   }
   s_snmpTargetsLock.unlock();

   SNMP_Transport *snmp = t->getTransport(port);

   ObjectArray<SNMP_ObjectId> oidList(64, 64, Ownership::True);
   uint32_t rcc = SnmpWalk(snmp, oid,
      [] (SNMP_Variable *varbind, SNMP_Transport *snmp, void *arg) -> uint32_t {
         static_cast<ObjectArray<SNMP_ObjectId>*>(arg)->add(new SNMP_ObjectId(varbind->getName()));
         return SNMP_ERR_SUCCESS;
      }, &oidList);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      for(int i = 0; i < columns.size(); i++)
      {
         const SNMPTableColumnDefinition *c = columns.get(i);
         if (c->getSnmpOid().isValid())
            value->addColumn(c->getName(), c->getDataType(), c->getDisplayName(), c->isInstanceColumn());
      }

      size_t baseOidLen = SnmpGetOIDLength(oid);
      for(int i = 0; i < oidList.size(); i++)
      {
         rcc = ReadSNMPTableRow(snmp, oidList.get(i), baseOidLen, 0, columns, value);
         if (rcc != SNMP_ERR_SUCCESS)
            break;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("GetSnmpTable: SNMP walk on %s failed (%s)"), oid, SnmpGetErrorText(rcc));
   }

   return (rcc == SNMP_ERR_SUCCESS) ? ERR_SUCCESS :
      ((rcc == SNMP_ERR_NO_OBJECT) ? ERR_UNKNOWN_METRIC : ERR_INTERNAL_ERROR);
}
