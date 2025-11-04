/* 
** NetXMS - Network Management System
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
** File: util.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Unique request ID
 */
static VolatileCounter s_requestId = 0;

/**
 * Generate new request ID
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpNewRequestId()
{
   return static_cast<uint32_t>(InterlockedIncrement(&s_requestId) & 0x7FFFFFFF);
}

/**
 * Default timeout for utility finctions
 */
static uint32_t s_snmpTimeout = 1500;

/**
 * Set SNMP timeout
 */
void LIBNXSNMP_EXPORTABLE SnmpSetDefaultTimeout(uint32_t timeout)
{
   s_snmpTimeout = (timeout >= 10) ? timeout : 10;
   nxlog_debug_tag(LIBNXSNMP_DEBUG_TAG, 4, _T("SNMP request timeout set to %u milliseconds"), s_snmpTimeout);
}

/**
 * Get SNMP timeout
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetDefaultTimeout()
{
   return s_snmpTimeout;
}

/**
 * Get value for SNMP variable
 * If szOidStr is not NULL, string representation of OID is used, otherwise -
 * binary representation from oidBinary and dwOidLen
 * Note: buffer size is in bytes
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpGet(SNMP_Version version, SNMP_Transport *transport, const TCHAR *oidStr, const uint32_t *oidBinary, size_t oidLen, void *value, size_t bufferSize, uint32_t flags)
{
   if (version != transport->getSnmpVersion())
   {
      SNMP_Version v = transport->getSnmpVersion();
      transport->setSnmpVersion(version);
      uint32_t rc = SnmpGetEx(transport, oidStr, oidBinary, oidLen, value, bufferSize, flags, nullptr, nullptr);
      transport->setSnmpVersion(v);
      return rc;
   }
   return SnmpGetEx(transport, oidStr, oidBinary, oidLen, value, bufferSize, flags, nullptr, nullptr);
}

/**
 * Get value for SNMP variable
 * Note: buffer size is in bytes
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpGet(SNMP_Version version, SNMP_Transport *transport, const SNMP_ObjectId& oid, void *value, size_t bufferSize, uint32_t flags)
{
   if (version != transport->getSnmpVersion())
   {
      SNMP_Version v = transport->getSnmpVersion();
      transport->setSnmpVersion(version);
      uint32_t rc = SnmpGetEx(transport, nullptr, oid.value(), oid.length(), value, bufferSize, flags, nullptr, nullptr);
      transport->setSnmpVersion(v);
      return rc;
   }
   return SnmpGetEx(transport, nullptr, oid.value(), oid.length(), value, bufferSize, flags, nullptr, nullptr);
}

/**
 * Get value for SNMP variable
 * If szOidStr is not NULL, string representation of OID is used, otherwise -
 * binary representation from oidBinary and dwOidLen
 * If SG_RAW_RESULT flag given and dataLen is not NULL actual data length will be stored there
 * Note: buffer size is in bytes
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetEx(SNMP_Transport *pTransport, const TCHAR *oidStr, const uint32_t *oidBinary, size_t oidLen,
      void *value, size_t bufferSize, uint32_t flags, uint32_t *dataLen, const char *codepage)
{
	if (pTransport == nullptr)
		return SNMP_ERR_COMM;

	uint32_t result = SNMP_ERR_SUCCESS;

   // Parse text OID
   uint32_t varName[MAX_OID_LEN];
   size_t nameLength;
   if (oidStr != nullptr)
   {
      nameLength = SnmpParseOID(oidStr, varName, MAX_OID_LEN);
      if (nameLength == 0)
      {
         InetAddress a = pTransport->getPeerIpAddress();
         if (flags & SG_VERBOSE)
         {
            TCHAR temp[64];
            nxlog_debug_tag(LIBNXSNMP_DEBUG_TAG, 5,
                     _T("Error parsing SNMP OID \"%s\" in SnmpGetEx (destination IP address %s)"), oidStr, a.toString(temp));
         }
         result = SNMP_ERR_BAD_OID;
      }
   }
   else
   {
      memcpy(varName, oidBinary, oidLen * sizeof(uint32_t));
      nameLength = oidLen;
   }

   if (result == SNMP_ERR_SUCCESS)   // Still no errors
   {
      SNMP_PDU requestPDU((flags & SG_GET_NEXT_REQUEST) ? SNMP_GET_NEXT_REQUEST : SNMP_GET_REQUEST, (uint32_t)InterlockedIncrement(&s_requestId) & 0x7FFFFFFF, pTransport->getSnmpVersion());
      requestPDU.bindVariable(new SNMP_Variable(varName, nameLength));
      SNMP_PDU *responsePDU;
      result = pTransport->doRequest(&requestPDU, &responsePDU);

      // Analyze response
      if (result == SNMP_ERR_SUCCESS)
      {
         if ((responsePDU->getNumVariables() > 0) &&
             (responsePDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *pVar = responsePDU->getVariable(0);

            if ((pVar->getType() != ASN_NO_SUCH_OBJECT) &&
                (pVar->getType() != ASN_NO_SUCH_INSTANCE) &&
                (pVar->getType() != ASN_END_OF_MIBVIEW) &&
                (!(flags & SG_GET_NEXT_REQUEST) || (pVar->getName().compare(varName, nameLength) == OID_LONGER)))
            {
               if (flags & SG_RAW_RESULT)
               {
						pVar->getRawValue((BYTE *)value, bufferSize);
                  if (dataLen != nullptr)
                     *dataLen = (UINT32)pVar->getValueLength();
               }
               else if (flags & SG_OBJECT_ID_RESULT)
               {
                  *static_cast<SNMP_ObjectId*>(value) = pVar->getValueAsObjectId();
               }
               else if (flags & SG_HSTRING_RESULT)
               {
						size_t rawLen = (bufferSize - sizeof(TCHAR)) / 2 / sizeof(TCHAR);
						BYTE *raw = static_cast<BYTE*>(SNMP_MemAlloc(rawLen));
						rawLen = (int)pVar->getRawValue(raw, rawLen);
						BinToStr(raw, rawLen, (TCHAR *)value);
						SNMP_MemFree(raw, rawLen);
               }
               else if (flags & SG_STRING_RESULT)
               {
                  pVar->getValueAsString((TCHAR *)value, bufferSize / sizeof(TCHAR), codepage);
               }
               else if (flags & SG_PSTRING_RESULT)
               {
						bool convert = true;
                  pVar->getValueAsPrintableString((TCHAR *)value, bufferSize / sizeof(TCHAR), &convert, codepage);
               }
               else
               {
                  switch(pVar->getType())
                  {
                     case ASN_INTEGER:
                        if (bufferSize >= sizeof(int32_t))
                           *((int32_t *)value) = pVar->getValueAsInt();
                        break;
                     case ASN_COUNTER32:
                     case ASN_GAUGE32:
                     case ASN_TIMETICKS:
                     case ASN_UINTEGER32:
                        if (bufferSize >= sizeof(uint32_t))
                           *((UINT32 *)value) = pVar->getValueAsUInt();
                        break;
                     case ASN_INTEGER64:
                        if (bufferSize >= sizeof(int64_t))
                           *((int64_t *)value) = pVar->getValueAsInt64();
                        else if (bufferSize >= sizeof(int32_t))
                           *((int32_t *)value) = pVar->getValueAsInt();
                        break;
                     case ASN_COUNTER64:
                     case ASN_UINTEGER64:
                        if (bufferSize >= sizeof(uint64_t))
                           *((uint64_t *)value) = pVar->getValueAsUInt64();
                        else if (bufferSize >= sizeof(uint32_t))
                           *((uint32_t *)value) = pVar->getValueAsUInt();
                        break;
                     case ASN_FLOAT:
                     case ASN_DOUBLE:
                        if (bufferSize >= sizeof(double))
                           *((double *)value) = pVar->getValueAsDouble();
                        else if (bufferSize >= sizeof(float))
                           *((float *)value) = static_cast<float>(pVar->getValueAsDouble());
                        break;
                     case ASN_IP_ADDR:
                        if (bufferSize >= sizeof(uint32_t))
                           *((uint32_t *)value) = ntohl(pVar->getValueAsUInt());
                        break;
                     case ASN_OCTET_STRING:
                        pVar->getValueAsString((TCHAR *)value, bufferSize / sizeof(TCHAR), codepage);
                        break;
                     case ASN_OBJECT_ID:
                        pVar->getValueAsString((TCHAR *)value, bufferSize / sizeof(TCHAR));
                        break;
                     case ASN_NULL:
                        result = SNMP_ERR_NO_OBJECT;
                        break;
                     default:
                        nxlog_write_tag(NXLOG_WARNING, LIBNXSNMP_DEBUG_TAG, _T("Unknown SNMP varbind type %u in GET response PDU"), pVar->getType());
                        result = SNMP_ERR_BAD_TYPE;
                        break;
                  }
               }
            }
            else
            {
               result = SNMP_ERR_NO_OBJECT;
            }
         }
         else
         {
            if (responsePDU->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
               result = SNMP_ERR_NO_OBJECT;
            else
               result = SNMP_ERR_AGENT;
         }
         delete responsePDU;
      }
      else
      {
         if (flags & SG_VERBOSE)
            nxlog_debug_tag(LIBNXSNMP_DEBUG_TAG, 7, _T("Error %u processing SNMP GET request"), result);
      }
   }

   return result;
}

/**
 * Check if specified SNMP variable set to specified value.
 * If variable doesn't exist at all, will return false
 */
bool LIBNXSNMP_EXPORTABLE CheckSNMPIntegerValue(SNMP_Transport *snmpTransport, const TCHAR *oid, int32_t value)
{
   int32_t buffer;
   if (SnmpGet(snmpTransport->getSnmpVersion(), snmpTransport, oid, nullptr, 0, &buffer, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
      return buffer == value;
   return false;
}

/**
 * Check if specified SNMP variable set to specified value.
 * If variable doesn't exist at all, will return false
 */
bool LIBNXSNMP_EXPORTABLE CheckSNMPIntegerValue(SNMP_Transport *snmpTransport, std::initializer_list<uint32_t> oid, int32_t value)
{
   int32_t buffer;
   if (SnmpGet(snmpTransport->getSnmpVersion(), snmpTransport, oid, &buffer, sizeof(int32_t), 0) == SNMP_ERR_SUCCESS)
      return buffer == value;
   return false;
}

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors, bool failOnShutdown)
{
   if (transport == nullptr)
      return SNMP_ERR_COMM;

   uint32_t rootOidBin[MAX_OID_LEN];
   size_t rootOidLen = SnmpParseOID(rootOid, rootOidBin, MAX_OID_LEN);
   if (rootOidLen == 0)
   {
      if (logErrors)
      {
         InetAddress a = transport->getPeerIpAddress();
         TCHAR temp[64];
         nxlog_debug_tag(LIBNXSNMP_DEBUG_TAG, 5, _T("Error parsing SNMP OID \"%s\" in SnmpWalk (destination IP address %s)"), rootOid, a.toString(temp));
      }
      return SNMP_ERR_BAD_OID;
   }

   return SnmpWalk(transport, rootOidBin, rootOidLen, handler, logErrors, failOnShutdown);
}

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors, bool failOnShutdown)
{
   if (transport == nullptr)
      return SNMP_ERR_COMM;

   // First OID to request
   uint32_t objectName[MAX_OID_LEN];
   memcpy(objectName, rootOid, rootOidLen * sizeof(uint32_t));
   size_t nameLength = rootOidLen;

   // Walk the MIB
   uint32_t result;
   bool running = true;
   while(running)
   {
      if (failOnShutdown && IsShutdownInProgress())
      {
         result = SNMP_ERR_ABORTED;
         break;
      }

      SNMP_PDU requestPDU(SNMP_GET_NEXT_REQUEST, static_cast<uint32_t>(InterlockedIncrement(&s_requestId)) & 0x7FFFFFFF, transport->getSnmpVersion());
      requestPDU.bindVariable(new SNMP_Variable(objectName, nameLength));
      SNMP_PDU *responsePDU;
      result = transport->doRequest(&requestPDU, &responsePDU);

      // Analyze response
      if (result == SNMP_ERR_SUCCESS)
      {
         if ((responsePDU->getNumVariables() > 0) &&
             (responsePDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *var = responsePDU->getVariable(0);

            if ((var->getType() != ASN_NO_SUCH_OBJECT) &&
                (var->getType() != ASN_NO_SUCH_INSTANCE) &&
                (var->getType() != ASN_END_OF_MIBVIEW))
            {
               // Should we stop walking?
               // Some buggy SNMP agents may return first row of the table being walked
               // after the last one, (Toshiba Strata CTX does this for example), so last check is here
               if ((var->getName().length() < rootOidLen) ||
                   memcmp(rootOid, var->getName().value(), rootOidLen * sizeof(uint32_t)) ||
                   !var->getName().follows(objectName, nameLength))
               {
                  delete responsePDU;
                  break;
               }
               nameLength = var->getName().length();
               memcpy(objectName, var->getName().value(), nameLength * sizeof(uint32_t));

               // Call user's callback function for processing
               result = handler(var);
               if (result != SNMP_ERR_SUCCESS)
               {
                  running = false;
               }
            }
            else
            {
               // Consider no object/no instance as end of walk signal instead of failure
               running = false;
            }
         }
         else
         {
            // Some SNMP agents sends NO_SUCH_NAME PDU error after last element in MIB
            if (responsePDU->getErrorCode() != SNMP_PDU_ERR_NO_SUCH_NAME)
               result = SNMP_ERR_AGENT;
            running = false;
         }
         delete responsePDU;
      }
      else
      {
         nxlog_debug_tag(LIBNXSNMP_DEBUG_TAG, 7, _T("Error %u processing SNMP GET request"), result);
         running = false;
      }
   }
   return result;
}

/**
 * Handler for counting walk
 */
static uint32_t WalkCountHandler(SNMP_Variable *var, SNMP_Transport *transport, void *context)
{
   (*static_cast<int*>(context))++;
   return SNMP_ERR_SUCCESS;
}

/**
 * Count number of objects under given root. Returns -1 on error.
 */
int LIBNXSNMP_EXPORTABLE SnmpWalkCount(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen)
{
   int count = 0;
   if (SnmpWalk(transport, rootOid, rootOidLen, WalkCountHandler, &count) != SNMP_ERR_SUCCESS)
      return -1;
   return count;
}

/**
 * Count number of objects under given root. Returns -1 on error.
 */
int LIBNXSNMP_EXPORTABLE SnmpWalkCount(SNMP_Transport *transport, const TCHAR *rootOid)
{
   int count = 0;
   if (SnmpWalk(transport, rootOid, WalkCountHandler, &count) != SNMP_ERR_SUCCESS)
      return -1;
   return count;
}
