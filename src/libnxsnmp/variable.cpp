/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: variable.cpp
**
**/

#include "libnxsnmp.h"


//
// SNMP_Variable default constructor
//

SNMP_Variable::SNMP_Variable()
{
   m_pName = NULL;
   m_pValue = NULL;
   m_dwType = ASN_NULL;
   m_dwValueLength = 0;
}


//
// Create variable of ASN_NULL type
//

SNMP_Variable::SNMP_Variable(TCHAR *pszName)
{
   DWORD dwLength, *pdwOid;

   m_pValue = NULL;
   m_dwType = ASN_NULL;
   m_dwValueLength = 0;

   pdwOid = (DWORD *)malloc(sizeof(DWORD) * MAX_OID_LEN);
   dwLength = SNMPParseOID(pszName, pdwOid, MAX_OID_LEN);
   m_pName = new SNMP_ObjectId(dwLength, pdwOid);
   free(pdwOid);
}


//
// Create variable of ASN_NULL type
//

SNMP_Variable::SNMP_Variable(DWORD *pdwName, DWORD dwNameLen)
{
   m_pValue = NULL;
   m_dwType = ASN_NULL;
   m_dwValueLength = 0;
   m_pName = new SNMP_ObjectId(dwNameLen, pdwName);
}


//
// SNMP_Variable destructor
//

SNMP_Variable::~SNMP_Variable()
{
   delete m_pName;
   safe_free(m_pValue);
}


//
// Parse variable record in PDU
//

BOOL SNMP_Variable::Parse(BYTE *pData, DWORD dwVarLength)
{
   BYTE *pbCurrPos;
   DWORD dwType, dwLength, dwIdLength;
   SNMP_OID *oid;
   BOOL bResult = FALSE;

   // Object ID
   if (!BER_DecodeIdentifier(pData, dwVarLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      return FALSE;
   if (dwType != ASN_OBJECT_ID)
      return FALSE;

   oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
   memset(oid, 0, sizeof(SNMP_OID));
   if (BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)oid))
   {
      m_pName = new SNMP_ObjectId(oid->dwLength, oid->pdwValue);
      dwVarLength -= dwLength + dwIdLength;
      pbCurrPos += dwLength;
      bResult = TRUE;
   }
   safe_free(oid->pdwValue);
   free(oid);

   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwVarLength, &m_dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         switch(m_dwType)
         {
            case ASN_OBJECT_ID:
               oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
               memset(oid, 0, sizeof(SNMP_OID));
               if (BER_DecodeContent(m_dwType, pbCurrPos, dwLength, (BYTE *)oid))
               {
                  m_dwValueLength = oid->dwLength * sizeof(DWORD);
                  m_pValue = (BYTE *)oid->pdwValue;
                  bResult = TRUE;
               }
               else
               {
                  safe_free(oid->pdwValue);
               }
               free(oid);
               break;
            case ASN_INTEGER:
            case ASN_COUNTER32:
            case ASN_GAUGE32:
            case ASN_TIMETICKS:
            case ASN_UINTEGER32:
               m_pValue = (BYTE *)malloc(8);
               bResult = BER_DecodeContent(m_dwType, pbCurrPos, dwLength, m_pValue);
               break;
            default:
               m_dwValueLength = dwLength;
               m_pValue = (BYTE *)nx_memdup(pbCurrPos, dwLength);
               bResult = TRUE;
               break;
         }
      }
   }

   return bResult;
}


//
// Get value as unsigned integer
//

DWORD SNMP_Variable::GetValueAsUInt(void)
{
   DWORD dwValue;

   switch(m_dwType)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         dwValue = *((DWORD *)m_pValue);
         break;
      default:
         dwValue = 0;
         break;
   }

   return dwValue;
}


//
// Get value as signed integer
//

LONG SNMP_Variable::GetValueAsInt(void)
{
   LONG iValue;

   switch(m_dwType)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         iValue = *((LONG *)m_pValue);
         break;
      default:
         iValue = 0;
         break;
   }

   return iValue;
}


//
// Get value as string
//

TCHAR *SNMP_Variable::GetValueAsString(TCHAR *pszBuffer, DWORD dwBufferSize)
{
   DWORD dwLen;

   if ((pszBuffer == NULL) || (dwBufferSize == 0))
      return NULL;

   switch(m_dwType)
   {
      case ASN_INTEGER:
         _sntprintf(pszBuffer, dwBufferSize, _T("%d"), *((LONG *)m_pValue));
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         _sntprintf(pszBuffer, dwBufferSize, _T("%u"), *((DWORD *)m_pValue));
         break;
      case ASN_IP_ADDR:
         if (dwBufferSize >= 16)
            IpToStr(ntohl(*((DWORD *)m_pValue)), pszBuffer);
         else
            pszBuffer[0] = 0;
         break;
      case ASN_OBJECT_ID:
         SNMPConvertOIDToText(m_dwValueLength / sizeof(DWORD), (DWORD *)m_pValue,
                              pszBuffer, dwBufferSize);
         break;
      case ASN_OCTET_STRING:
         dwLen = min(dwBufferSize - 1, m_dwValueLength);
#ifdef UNICODE
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)m_pValue, dwLen,
                             pszBuffer, dwBufferSize);
#else
         memcpy(pszBuffer, m_pValue, dwLen);
#endif
         pszBuffer[dwLen] = 0;
         break;
      default:
         pszBuffer[0] = 0;
         break;
   }
   return pszBuffer;
}


//
// Get value as object id
//

SNMP_ObjectId *SNMP_Variable::GetValueAsObjectId(void)
{
   SNMP_ObjectId *oid = NULL;

   if (m_dwType == ASN_OBJECT_ID)
   {
      oid = new SNMP_ObjectId(m_dwValueLength / sizeof(DWORD), (DWORD *)m_pValue);
   }
   return oid;
}


//
// Get value as MAC address
//

TCHAR *SNMP_Variable::GetValueAsMACAddr(TCHAR *pszBuffer)
{
   int i;
   TCHAR *pszPos;

   // MAC address usually encoded as octet string
   if ((m_dwType == ASN_OCTET_STRING) && (m_dwValueLength >= 6))
   {
      for(i = 0, pszPos = pszBuffer; i < 6; i++, pszPos += 3)
         _stprintf(pszPos, _T("%02X:"), m_pValue[i]);
      *(pszPos - 1) = 0;
   }
   else
   {
      _tcscpy(pszBuffer, _T("00:00:00:00:00:00"));
   }
   return pszBuffer;
}


//
// Encode variable using BER
// Normally buffer provided should be at least m_dwValueLength + (name_length * 4) + 12 bytes
// Return value is number of bytes actually used in buffer
//

DWORD SNMP_Variable::Encode(BYTE *pBuffer, DWORD dwBufferSize)
{
   DWORD dwBytes, dwWorkBufSize;
   BYTE *pWorkBuf;

   dwWorkBufSize = m_dwValueLength + m_pName->Length() * 4 + 16;
   pWorkBuf = (BYTE *)malloc(dwWorkBufSize);
   dwBytes = BER_Encode(ASN_OBJECT_ID, (BYTE *)m_pName->GetValue(), 
                        m_pName->Length() * sizeof(DWORD), 
                        pWorkBuf, dwWorkBufSize);
   dwBytes += BER_Encode(m_dwType, m_pValue, m_dwValueLength, 
                         pWorkBuf + dwBytes, dwWorkBufSize - dwBytes);
   dwBytes = BER_Encode(ASN_SEQUENCE, pWorkBuf, dwBytes, pBuffer, dwBufferSize);
   free(pWorkBuf);
   return dwBytes;
}


//
// Set variable from string
//

void SNMP_Variable::SetValueFromString(DWORD dwType, TCHAR *pszValue)
{
   DWORD *pdwBuffer, dwLen;

   m_dwType = dwType;
   switch(m_dwType)
   {
      case ASN_INTEGER:
         m_dwValueLength = sizeof(LONG);
         m_pValue = (BYTE *)realloc(m_pValue, m_dwValueLength);
         *((LONG *)m_pValue) = _tcstol(pszValue, NULL, 0);
         break;
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
         m_dwValueLength = sizeof(DWORD);
         m_pValue = (BYTE *)realloc(m_pValue, m_dwValueLength);
         *((DWORD *)m_pValue) = _tcstoul(pszValue, NULL, 0);
         break;
      case ASN_IP_ADDR:
         m_dwValueLength = sizeof(DWORD);
         m_pValue = (BYTE *)realloc(m_pValue, m_dwValueLength);
         *((DWORD *)m_pValue) = _t_inet_addr(pszValue);
         break;
      case ASN_OBJECT_ID:
         pdwBuffer = (DWORD *)malloc(sizeof(DWORD) * 256);
         dwLen = SNMPParseOID(pszValue, pdwBuffer, 256);
         if (dwLen > 0)
         {
            m_dwValueLength = dwLen * sizeof(DWORD);
            safe_free(m_pValue);
            m_pValue = (BYTE *)nx_memdup(pdwBuffer, m_dwValueLength);
         }
         else
         {
            // OID parse error, set to .ccitt.zeroDotZero (.0.0)
            m_dwValueLength = sizeof(DWORD) * 2;
            m_pValue = (BYTE *)realloc(m_pValue, m_dwValueLength);
            memset(m_pValue, 0, m_dwValueLength);
         }
         break;
      case ASN_OCTET_STRING:
         m_dwValueLength = _tcslen(pszValue);
#ifdef UNICODE
         m_pValue = (BYTE *)realloc(m_pValue, m_dwValueLength);
         WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                             pszValue, m_dwValueLength, (char *)m_pValue,
                             m_dwValueLength, NULL, NULL);
#else
         safe_free(m_pValue);
         m_pValue = (BYTE *)nx_memdup(pszValue, m_dwValueLength);
#endif
         break;
      default:
         break;
   }
}
