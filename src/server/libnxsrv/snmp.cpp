/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: snmp.cpp
**
**/

#include "libnxsrv.h"

/**
 * Unique request ID
 */
static VolatileCounter s_requestId = 1;

/**
 * Generate new request ID
 */
UINT32 LIBNXSRV_EXPORTABLE SnmpNewRequestId()
{
   return (UINT32)InterlockedIncrement(&s_requestId);
}

/**
 * Get value for SNMP variable
 * If szOidStr is not NULL, string representation of OID is used, otherwise -
 * binary representation from oidBinary and dwOidLen
 * Note: buffer size is in bytes
 */
UINT32 LIBNXSRV_EXPORTABLE SnmpGet(int version, SNMP_Transport *transport,
                                   const TCHAR *szOidStr, const UINT32 *oidBinary, size_t dwOidLen, void *pValue,
                                   size_t bufferSize, UINT32 dwFlags)
{
   if (version != transport->getSnmpVersion())
   {
      int v = transport->getSnmpVersion();
      transport->setSnmpVersion(version);
      DbgPrintf(7, _T("SnmpGet: transport SNMP version %d changed to %d"), v, version);
      UINT32 rc = SnmpGetEx(transport, szOidStr, oidBinary, dwOidLen, pValue, bufferSize, dwFlags, NULL);
      transport->setSnmpVersion(v);
      return rc;
   }
   else
   {
      return SnmpGetEx(transport, szOidStr, oidBinary, dwOidLen, pValue, bufferSize, dwFlags, NULL);
   }
}

/**
 * Get value for SNMP variable
 * If szOidStr is not NULL, string representation of OID is used, otherwise -
 * binary representation from oidBinary and dwOidLen
 * If SG_RAW_RESULT flag given and dataLen is not NULL actial data length will be stored there
 * Note: buffer size is in bytes
 */
UINT32 LIBNXSRV_EXPORTABLE SnmpGetEx(SNMP_Transport *pTransport,
                                     const TCHAR *szOidStr, const UINT32 *oidBinary, size_t dwOidLen, void *pValue,
                                     size_t bufferSize, UINT32 dwFlags, UINT32 *dataLen)
{
   SNMP_PDU *pRqPDU, *pRespPDU;
   UINT32 pdwVarName[MAX_OID_LEN], dwResult = SNMP_ERR_SUCCESS;
   size_t nameLength;

	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Create PDU and send request
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, (UINT32)InterlockedIncrement(&s_requestId), pTransport->getSnmpVersion());
   if (szOidStr != NULL)
   {
      nameLength = SNMPParseOID(szOidStr, pdwVarName, MAX_OID_LEN);
      if (nameLength == 0)
      {
         InetAddress a = pTransport->getPeerIpAddress();
         nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "ssA", szOidStr, _T("SnmpGet"), &a);
         dwResult = SNMP_ERR_BAD_OID;
      }
   }
   else
   {
      memcpy(pdwVarName, oidBinary, dwOidLen * sizeof(UINT32));
      nameLength = dwOidLen;
   }

   if (dwResult == SNMP_ERR_SUCCESS)   // Still no errors
   {
      pRqPDU->bindVariable(new SNMP_Variable(pdwVarName, nameLength));
      dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_snmpTimeout, 3);

      // Analyze response
      if (dwResult == SNMP_ERR_SUCCESS)
      {
         if ((pRespPDU->getNumVariables() > 0) &&
             (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *pVar = pRespPDU->getVariable(0);

            if ((pVar->getType() != ASN_NO_SUCH_OBJECT) &&
                (pVar->getType() != ASN_NO_SUCH_INSTANCE))
            {
               if (dwFlags & SG_RAW_RESULT)
               {
						pVar->getRawValue((BYTE *)pValue, bufferSize);
                  if (dataLen != NULL)
                     *dataLen = (UINT32)pVar->getValueLength();
               }
               else if (dwFlags & SG_HSTRING_RESULT)
               {
						size_t rawLen = (bufferSize - sizeof(TCHAR)) / 2 / sizeof(TCHAR);
						BYTE *raw = (BYTE *)malloc(rawLen);
						rawLen = (int)pVar->getRawValue(raw, rawLen);
						BinToStr(raw, rawLen, (TCHAR *)pValue);
						free(raw);
               }
               else if (dwFlags & SG_STRING_RESULT)
               {
                  pVar->getValueAsString((TCHAR *)pValue, bufferSize / sizeof(TCHAR));
               }
               else if (dwFlags & SG_PSTRING_RESULT)
               {
						bool convert = true;
                  pVar->getValueAsPrintableString((TCHAR *)pValue, bufferSize / sizeof(TCHAR), &convert);
               }
               else
               {
                  switch(pVar->getType())
                  {
                     case ASN_INTEGER:
                     case ASN_UINTEGER32:
                     case ASN_COUNTER32:
                     case ASN_GAUGE32:
                     case ASN_TIMETICKS:
                        *((INT32 *)pValue) = pVar->getValueAsInt();
                        break;
                     case ASN_IP_ADDR:
                        *((UINT32 *)pValue) = ntohl(pVar->getValueAsUInt());
                        break;
                     case ASN_OCTET_STRING:
                        pVar->getValueAsString((TCHAR *)pValue, bufferSize / sizeof(TCHAR));
                        break;
                     case ASN_OBJECT_ID:
                        pVar->getValueAsString((TCHAR *)pValue, bufferSize / sizeof(TCHAR));
                        break;
                     case ASN_NULL:
                        dwResult = SNMP_ERR_NO_OBJECT;
                        break;
                     default:
                        nxlog_write(MSG_SNMP_UNKNOWN_TYPE, NXLOG_WARNING, "x", pVar->getType());
                        dwResult = SNMP_ERR_BAD_TYPE;
                        break;
                  }
               }
            }
            else
            {
               dwResult = SNMP_ERR_NO_OBJECT;
            }
         }
         else
         {
            if (pRespPDU->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
               dwResult = SNMP_ERR_NO_OBJECT;
            else
               dwResult = SNMP_ERR_AGENT;
         }
         delete pRespPDU;
      }
      else
      {
         if (dwFlags & SG_VERBOSE)
            nxlog_write(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", dwResult);
      }
   }

   delete pRqPDU;
   return dwResult;
}

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
UINT32 LIBNXSRV_EXPORTABLE SnmpWalk(UINT32 dwVersion, SNMP_Transport *pTransport, const TCHAR *szRootOid,
                                   UINT32 (* pHandler)(UINT32, SNMP_Variable *, SNMP_Transport *, void *),
                                   void *pUserArg, BOOL bVerbose)
{
	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Get root
	UINT32 pdwRootName[MAX_OID_LEN];
   size_t dwRootLen = SNMPParseOID(szRootOid, pdwRootName, MAX_OID_LEN);
   if (dwRootLen == 0)
   {
      InetAddress a = pTransport->getPeerIpAddress();
      nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "ssA", szRootOid, _T("SnmpWalk"), &a);
      return SNMP_ERR_BAD_OID;
   }

	// First OID to request
   UINT32 pdwName[MAX_OID_LEN];
   memcpy(pdwName, pdwRootName, dwRootLen * sizeof(UINT32));
   size_t nameLength = dwRootLen;

   // Walk the MIB
   UINT32 dwResult;
   BOOL bRunning = TRUE;
   UINT32 firstObjectName[MAX_OID_LEN];
   size_t firstObjectNameLen = 0;
   while(bRunning)
   {
      SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, (UINT32)InterlockedIncrement(&s_requestId), dwVersion);
      pRqPDU->bindVariable(new SNMP_Variable(pdwName, nameLength));
	   SNMP_PDU *pRespPDU;
      dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_snmpTimeout, 3);

      // Analyze response
      if (dwResult == SNMP_ERR_SUCCESS)
      {
         if ((pRespPDU->getNumVariables() > 0) &&
             (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *pVar = pRespPDU->getVariable(0);

            if ((pVar->getType() != ASN_NO_SUCH_OBJECT) &&
                (pVar->getType() != ASN_NO_SUCH_INSTANCE))
            {
               // Should we stop walking?
					// Some buggy SNMP agents may return first value after last one
					// (Toshiba Strata CTX do that for example), so last check is here
               if ((pVar->getName()->getLength() < dwRootLen) ||
                   (memcmp(pdwRootName, pVar->getName()->getValue(), dwRootLen * sizeof(UINT32))) ||
						 (pVar->getName()->compare(pdwName, nameLength) == OID_EQUAL) ||
						 (pVar->getName()->compare(firstObjectName, firstObjectNameLen) == OID_EQUAL))
               {
                  bRunning = FALSE;
                  delete pRespPDU;
                  delete pRqPDU;
                  break;
               }
               nameLength = pVar->getName()->getLength();
               memcpy(pdwName, pVar->getName()->getValue(), nameLength * sizeof(UINT32));
					if (firstObjectNameLen == 0)
					{
						firstObjectNameLen = nameLength;
						memcpy(firstObjectName, pdwName, nameLength * sizeof(UINT32));
					}

               // Call user's callback function for processing
               dwResult = pHandler(dwVersion, pVar, pTransport, pUserArg);
               if (dwResult != SNMP_ERR_SUCCESS)
               {
                  bRunning = FALSE;
               }
            }
            else
            {
               // Consider no object/no instance as end of walk signal instead of failure
               bRunning = FALSE;
            }
         }
         else
         {
            if (pRespPDU->getErrorCode() == SNMP_PDU_ERR_NO_SUCH_NAME)
               dwResult = SNMP_ERR_NO_OBJECT;
            else
               dwResult = SNMP_ERR_AGENT;
            bRunning = FALSE;
         }
         delete pRespPDU;
      }
      else
      {
         if (bVerbose)
            nxlog_write(MSG_SNMP_GET_ERROR, EVENTLOG_ERROR_TYPE, "d", dwResult);
         bRunning = FALSE;
      }
      delete pRqPDU;
   }
   return dwResult;
}
