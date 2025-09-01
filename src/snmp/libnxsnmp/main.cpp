/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Convert OID element to string. Returns pointer to character following last digit.
 */
static inline TCHAR *OIDElementToString(uint32_t value, TCHAR *dest)
{
   TCHAR *p = dest;
   TCHAR buffer[64];
   TCHAR *t = buffer;
   do
   {
      uint32_t rem = value % 10;
      *t++ = static_cast<uint32_t>((rem < 10) ? (rem + '0') : (rem - 10 + 'a'));
      value = value / 10;
   } while(value > 0);

   t--;
   while(t >= buffer)
      *p++ = *t--;
   return p;
}

/**
 * Convert OID to text. Returns pointer to buffer.
 */
TCHAR LIBNXSNMP_EXPORTABLE *SnmpConvertOIDToText(size_t length, const uint32_t *value, TCHAR *buffer, size_t bufferSize)
{
   TCHAR *p = buffer;
   size_t count = 0;
   for(size_t i = 0; (i < length) && (count < bufferSize); i++)
   {
      if (i > 0)
         *p++ = '.';
      p = OIDElementToString(value[i], p);
   }
   *p = 0;
	return buffer;
}

/**
 * Parse OID in text into binary format
 * Will return 0 if OID is invalid or empty, and OID length (in UINT32s) on success
 * Buffer size should be given in number of UINT32s
 */
size_t LIBNXSNMP_EXPORTABLE SnmpParseOID(const TCHAR *text, uint32_t *buffer, size_t bufferSize)
{
   TCHAR *pCurr = (TCHAR *)text, *pEnd, szNumber[32];
   size_t length = 0;
   int iNumLen;

   if (*pCurr == 0)
      return 0;

   // Skip initial dot if present
   if (*pCurr == _T('.'))
      pCurr++;

   for(pEnd = pCurr; (*pEnd != 0) && (length < bufferSize); pCurr = pEnd + 1)
   {
      for(iNumLen = 0, pEnd = pCurr; (*pEnd >= _T('0')) && (*pEnd <= _T('9')); pEnd++, iNumLen++);
      if ((iNumLen > 15) || ((*pEnd != _T('.')) && (*pEnd != 0)))
         return 0;   // Number is definitely too large or not a number
      memcpy(szNumber, pCurr, sizeof(TCHAR) * iNumLen);
      szNumber[iNumLen] = 0;
      buffer[length++] = _tcstoul(szNumber, nullptr, 10);
   }
   return length;
}

/**
 * Check if given OID is syntaxically correct
 */
bool LIBNXSNMP_EXPORTABLE SnmpIsCorrectOID(const TCHAR *oid)
{
   uint32_t buffer[MAX_OID_LEN];
   size_t len = SnmpParseOID(oid, buffer, MAX_OID_LEN);
   return (len > 0);
}

/**
 * Check if given OID is syntaxically correct
 */
size_t LIBNXSNMP_EXPORTABLE SnmpGetOIDLength(const TCHAR *oid)
{
   uint32_t buffer[MAX_OID_LEN];
   return SnmpParseOID(oid, buffer, MAX_OID_LEN);
}

/**
 * Get text for libnxsnmp error code
 */
const TCHAR LIBNXSNMP_EXPORTABLE *SnmpGetErrorText(uint32_t errorCode)
{
   static const TCHAR *errorText[] =
   {
      _T("Operation completed successfully"),
      _T("Request timed out"),
      _T("Invalid parameters passed to function"),
      _T("Unable to create socket"),
      _T("Communication error"),
      _T("Error parsing PDU"),
      _T("No such object"),
      _T("Invalid hostname or IP address"),
      _T("OID is incorrect"),
      _T("Agent error"),
      _T("Unknown variable data type"),
      _T("File I/O error"),
      _T("Invalid file header"),
      _T("Invalid or corrupted file data"),
      _T("Unsupported security level"),
      _T("Not in time window"),
      _T("Unknown security name"),
      _T("Unknown engine ID"),
      _T("Authentication failure"),
      _T("Decryption error"),
      _T("Malformed or unexpected response from agent"),
      _T("Operation aborted"),
      _T("Snapshot size limit reached")
   };
   return (errorCode <= SNMP_ERR_SNAPSHOT_TOO_BIG) ? errorText[errorCode] : _T("Unknown error");
}

/**
 * Get text for protocol error code
 */
const TCHAR LIBNXSNMP_EXPORTABLE *SnmpGetProtocolErrorText(SNMP_ErrorCode errorCode)
{
   static const TCHAR *errorText[] =
   {
      _T("Success"),
      _T("Response is too big"),
      _T("No such name"),
      _T("Bad value"),
      _T("Read only variable"),
      _T("Generic error"),
      _T("No access"),
      _T("Wrong type"),
      _T("Wrong length"),
      _T("Wrong encoding"),
      _T("Wrong value"),
      _T("Creation not allowed"),
      _T("Inconsistent value"),
      _T("Resource unavailable"),
      _T("Commit failed"),
      _T("Undo failed"),
      _T("Authorization error"),
      _T("Not writable"),
      _T("Inconsistent name")
   };
   return ((static_cast<int>(errorCode) >= 0) && (static_cast<int>(errorCode) <= SNMP_PDU_ERR_INCONSISTENT_NAME)) ? errorText[errorCode] : _T("Unknown error");
}

/**
 * SNMP type names
 */
static CodeLookupElement s_typeList[] =
{
   { ASN_BIT_STRING, _T("BIT STRING") },
   { ASN_COUNTER32, _T("COUNTER32") },
   { ASN_COUNTER64, _T("COUNTER64") },
   { ASN_DOUBLE, _T("DOUBLE") },
   { ASN_FLOAT, _T("FLOAT") },
   { ASN_GAUGE32, _T("GAUGE32") },
   { ASN_INTEGER, _T("INTEGER") },
   { ASN_INTEGER, _T("INT") },
   { ASN_INTEGER64, _T("INTEGER64") },
   { ASN_IP_ADDR, _T("IP ADDRESS") },
   { ASN_IP_ADDR, _T("IPADDR") },
   { ASN_NSAP_ADDR, _T("NSAP ADDRESS") },
   { ASN_NULL, _T("NULL") },
   { ASN_OBJECT_ID, _T("OBJECT IDENTIFIER") },
   { ASN_OBJECT_ID, _T("OID") },
   { ASN_OCTET_STRING, _T("STRING") },
   { ASN_OCTET_STRING, _T("OCTET STRING") },
   { ASN_OPAQUE, _T("OPAQUE") },
   { ASN_SEQUENCE, _T("SEQUENCE") },
   { ASN_TIMETICKS, _T("TIMETICKS") },
   { ASN_UINTEGER32, _T("UINTEGER32") },
   { ASN_UINTEGER32, _T("UINT32") },
   { ASN_UINTEGER64, _T("UINTEGER64") },
   { 0, nullptr }
};

/**
 * Resolve text representation of data type to integer value
 */
uint32_t LIBNXSNMP_EXPORTABLE SnmpResolveDataType(const TCHAR *type)
{
   for(int i = 0; s_typeList[i].text != nullptr; i++)
      if (!_tcsicmp(s_typeList[i].text, type))
         return s_typeList[i].code;
   return ASN_NULL;
}

/**
 * Get type name
 */
TCHAR LIBNXSNMP_EXPORTABLE *SnmpDataTypeName(uint32_t type, TCHAR *buffer, size_t bufferSize)
{
	for(int i = 0; s_typeList[i].text != nullptr; i++)
		if (static_cast<uint32_t>(s_typeList[i].code) == type)
		{
			_tcslcpy(buffer, s_typeList[i].text, bufferSize);
			return buffer;
		}

	_sntprintf(buffer, bufferSize, _T("0x%02x"), type);
	return buffer;
}

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
