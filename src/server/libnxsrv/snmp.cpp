/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Static data
//

static DWORD m_dwRequestId = 1;


//
// Generate new request ID
//

DWORD LIBNXSRV_EXPORTABLE SnmpNewRequestId()
{
   return m_dwRequestId++;
}


//
// Get value for SNMP variable
// If szOidStr is not NULL, string representation of OID is used, otherwise -
// binary representation from oidBinary and dwOidLen
//

DWORD LIBNXSRV_EXPORTABLE SnmpGet(DWORD dwVersion, SNMP_Transport *pTransport,
                                  const TCHAR *szOidStr, const DWORD *oidBinary, DWORD dwOidLen, void *pValue,
                                  DWORD dwBufferSize, DWORD dwFlags)
{
   SNMP_PDU *pRqPDU, *pRespPDU;
   DWORD dwNameLen, pdwVarName[MAX_OID_LEN], dwResult = SNMP_ERR_SUCCESS;

	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Create PDU and send request
   pRqPDU = new SNMP_PDU(SNMP_GET_REQUEST, m_dwRequestId++, dwVersion);
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
      memcpy(pdwVarName, oidBinary, dwOidLen * sizeof(DWORD));
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
                        *((DWORD *)pValue) = ntohl(pVar->GetValueAsUInt());
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


//
// Enumerate multiple values by walking through MIB, starting at given root
//

DWORD LIBNXSRV_EXPORTABLE SnmpEnumerate(DWORD dwVersion, SNMP_Transport *pTransport, const TCHAR *szRootOid,
                                        DWORD (* pHandler)(DWORD, SNMP_Variable *, SNMP_Transport *, void *),
                                        void *pUserArg, BOOL bVerbose)
{
   DWORD pdwRootName[MAX_OID_LEN], dwRootLen, pdwName[MAX_OID_LEN], dwNameLen, dwResult;
   SNMP_PDU *pRqPDU, *pRespPDU;
   BOOL bRunning = TRUE;

	if (pTransport == NULL)
		return SNMP_ERR_COMM;

   // Get root
   dwRootLen = SNMPParseOID(szRootOid, pdwRootName, MAX_OID_LEN);
   if (dwRootLen == 0)
   {
      nxlog_write(MSG_OID_PARSE_ERROR, EVENTLOG_ERROR_TYPE, "s", szRootOid);
      dwResult = SNMP_ERR_BAD_OID;
   }
   else
   {
      memcpy(pdwName, pdwRootName, dwRootLen * sizeof(DWORD));
      dwNameLen = dwRootLen;

      // Walk the MIB
      while(bRunning)
      {
         pRqPDU = new SNMP_PDU(SNMP_GET_NEXT_REQUEST, m_dwRequestId++, dwVersion);
         pRqPDU->bindVariable(new SNMP_Variable(pdwName, dwNameLen));
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
                  if ((pVar->GetName()->Length() < dwRootLen) ||
                      (memcmp(pdwRootName, pVar->GetName()->GetValue(), dwRootLen * sizeof(DWORD))) ||
                      ((pVar->GetName()->Length() == dwNameLen) &&
                       (!memcmp(pVar->GetName()->GetValue(), pdwName, pVar->GetName()->Length() * sizeof(DWORD)))))
                  {
                     bRunning = FALSE;
                     delete pRespPDU;
                     delete pRqPDU;
                     break;
                  }
                  memcpy(pdwName, pVar->GetName()->GetValue(), 
                         pVar->GetName()->Length() * sizeof(DWORD));
                  dwNameLen = pVar->GetName()->Length();

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
   }
   return dwResult;
}
