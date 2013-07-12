/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
 */
UINT32 LIBNXSRV_EXPORTABLE SnmpGet(UINT32 dwVersion, SNMP_Transport *pTransport,
                                  const TCHAR *szOidStr, const UINT32 *oidBinary, UINT32 dwOidLen, void *pValue,
                                  UINT32 dwBufferSize, UINT32 dwFlags)
{
   SNMP_PDU *pRqPDU, *pRespPDU;
   UINT32 dwNameLen, pdwVarName[MAX_OID_LEN], dwResult = SNMP_ERR_SUCCESS;

	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Create PDU and send request
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, (UINT32)InterlockedIncrement(&s_requestId), dwVersion);
   if (szOidStr != NULL)
   {
      dwNameLen = SNMPParseOID(szOidStr, pdwVarName, MAX_OID_LEN);
      if (dwNameLen == 0)
      {
         nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szOidStr);
         dwResult = SNMP_ERR_BAD_OID;
      }
   }
   else
   {
      memcpy(pdwVarName, oidBinary, dwOidLen * sizeof(UINT32));
      dwNameLen = dwOidLen;
   }

   if (dwResult == SNMP_ERR_SUCCESS)   // Still no errors
   {
      pRqPDU->bindVariable(new SNMP_Variable(pdwVarName, dwNameLen));
      dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);

      // Analyze response
      if (dwResult == SNMP_ERR_SUCCESS)
      {
         if ((pRespPDU->getNumVariables() > 0) &&
             (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *pVar = pRespPDU->getVariable(0);

            if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
            {
               if (dwFlags & SG_RAW_RESULT)
               {
						pVar->getRawValue((BYTE *)pValue, dwBufferSize);
               }
               else if (dwFlags & SG_HSTRING_RESULT)
               {
						int rawLen = (dwBufferSize - 1) / 2;
						BYTE *raw = (BYTE *)malloc(rawLen);
						rawLen = (int)pVar->getRawValue(raw, rawLen);
						BinToStr(raw, rawLen, (TCHAR *)pValue);
						free(raw);
               }
               else if (dwFlags & SG_STRING_RESULT)
               {
                  pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
               }
               else if (dwFlags & SG_PSTRING_RESULT)
               {
						bool convert = true;
                  pVar->getValueAsPrintableString((TCHAR *)pValue, dwBufferSize, &convert);
               }
               else
               {
                  switch(pVar->GetType())
                  {
                     case ASN_INTEGER:
                     case ASN_UINTEGER32:
                     case ASN_COUNTER32:
                     case ASN_GAUGE32:
                     case ASN_TIMETICKS:
                        *((LONG *)pValue) = pVar->GetValueAsInt();
                        break;
                     case ASN_IP_ADDR:
                        *((UINT32 *)pValue) = ntohl(pVar->GetValueAsUInt());
                        break;
                     case ASN_OCTET_STRING:
                        pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                        break;
                     case ASN_OBJECT_ID:
                        pVar->GetValueAsString((TCHAR *)pValue, dwBufferSize);
                        break;
                     default:
                        nxlog_write(MSG_SNMP_UNKNOWN_TYPE, EVENTLOG_ERROR_TYPE, "x", pVar->GetType());
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
   UINT32 dwRootLen = SNMPParseOID(szRootOid, pdwRootName, MAX_OID_LEN);
   if (dwRootLen == 0)
   {
      nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szRootOid);
      return SNMP_ERR_BAD_OID;
   }

	// First OID to request
   UINT32 pdwName[MAX_OID_LEN];
   memcpy(pdwName, pdwRootName, dwRootLen * sizeof(UINT32));
   UINT32 dwNameLen = dwRootLen;

   // Walk the MIB
   UINT32 dwResult;
   BOOL bRunning = TRUE;
   UINT32 firstObjectName[MAX_OID_LEN];
   UINT32 firstObjectNameLen = 0;
   while(bRunning)
   {
      SNMP_PDU *pRqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, (UINT32)InterlockedIncrement(&s_requestId), dwVersion);
      pRqPDU->bindVariable(new SNMP_Variable(pdwName, dwNameLen));
	   SNMP_PDU *pRespPDU;
      dwResult = pTransport->doRequest(pRqPDU, &pRespPDU, g_dwSNMPTimeout, 3);

      // Analyze response
      if (dwResult == SNMP_ERR_SUCCESS)
      {
         if ((pRespPDU->getNumVariables() > 0) &&
             (pRespPDU->getErrorCode() == SNMP_PDU_ERR_SUCCESS))
         {
            SNMP_Variable *pVar = pRespPDU->getVariable(0);

            if ((pVar->GetType() != ASN_NO_SUCH_OBJECT) &&
                (pVar->GetType() != ASN_NO_SUCH_INSTANCE))
            {
               // Should we stop walking?
					// Some buggy SNMP agents may return first value after last one
					// (Toshiba Strata CTX do that for example), so last check is here
               if ((pVar->GetName()->getLength() < dwRootLen) ||
                   (memcmp(pdwRootName, pVar->GetName()->getValue(), dwRootLen * sizeof(UINT32))) ||
						 (pVar->GetName()->compare(pdwName, dwNameLen) == OID_EQUAL) ||
						 (pVar->GetName()->compare(firstObjectName, firstObjectNameLen) == OID_EQUAL))
               {
                  bRunning = FALSE;
                  delete pRespPDU;
                  delete pRqPDU;
                  break;
               }
               dwNameLen = pVar->GetName()->getLength();
               memcpy(pdwName, pVar->GetName()->getValue(), dwNameLen * sizeof(UINT32));
					if (firstObjectNameLen == 0)
					{
						firstObjectNameLen = dwNameLen;
						memcpy(firstObjectName, pdwName, dwNameLen * sizeof(UINT32));
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
               dwResult = SNMP_ERR_NO_OBJECT;
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
