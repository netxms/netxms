/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: nxsnmp.h
**
**/

#ifndef _nxsnmp_h_
#define _nxsnmp_h_

#ifdef _WIN32
#ifdef LIBNXSNMP_EXPORTS
#define LIBNXSNMP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSNMP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSNMP_EXPORTABLE
#endif


//
// libnxsnmp error codes
//

#define SNMP_ERR_SUCCESS   0
#define SNMP_ERR_TIMEOUT   1
#define SNMP_ERR_PARAM     2
#define SNMP_ERR_SOCKET    3
#define SNMP_ERR_COMM      4
#define SNMP_ERR_PARSE     5


//
// SNMP versions
//

#define SNMP_VERSION_1     0
#define SNMP_VERSION_2C    1


//
// PDU types
//

#define SNMP_INVALID_PDU         -1
#define SNMP_GET_REQUEST         0
#define SNMP_GET_NEXT_REQUEST    1
#define SNMP_GET_RESPONCE        2
#define SNMP_SET_REQUEST         3
#define SNMP_TRAP                4
#define SNMP_GET_BULK_REQUEST    5
#define SNMP_INFORM_REQUEST      6


//
// ASN.1 identifier types
//

#define ASN_INTEGER                 0x02
#define ASN_BIT_STRING              0x03
#define ASN_OCTET_STRING            0x04
#define ASN_NULL                    0x05
#define ASN_OBJECT_ID               0x06
#define ASN_SEQUENCE                0x30
#define ASN_IP_ADDR                 0x40
#define ASN_COUNTER32               0x41
#define ASN_GAUGE32                 0x42
#define ASN_TIMETICKS               0x43
#define ASN_OPAQUE                  0x44
#define ASN_NSAP_ADDR               0x45
#define ASN_COUNTER64               0x46
#define ASN_UINTEGER32              0x47
#define ASN_GET_REQUEST_PDU         0xA0
#define ASN_GET_NEXT_REQUEST_PDU    0xA1
#define ASN_GET_RESPONCE_PDU        0xA2
#define ASN_SET_REQUEST_PDU         0xA3
#define ASN_TRAP_V1_PDU             0xA4
#define ASN_GET_BULK_REQUEST_PDU    0xA5
#define ASN_INFORM_REQUEST_PDU      0xA6
#define ASN_TRAP_V2_PDU             0xA7


//
// Object identifier
//

class LIBNXSNMP_EXPORTABLE SNMP_ObjectId
{
private:
   DWORD m_dwLength;
   DWORD *m_pdwValue;
   TCHAR *m_pszTextValue;

   void ConvertToText(void);

public:
   SNMP_ObjectId();
   SNMP_ObjectId(DWORD dwLength, DWORD *pdwValue);
   ~SNMP_ObjectId();

   DWORD Length(void) { return m_dwLength; }
   const DWORD *GetValue(void) { return m_pdwValue; }
   const TCHAR *GetValueAsText(void) { return CHECK_NULL(m_pszTextValue); }
};


//
// SNMP variable
//

class LIBNXSNMP_EXPORTABLE SNMP_Variable
{
private:
   SNMP_ObjectId *m_pName;
   DWORD m_dwType;
   DWORD m_dwValueLength;
   BYTE *m_pValue;

public:
   SNMP_Variable();
   SNMP_Variable(TCHAR *pszName);
   ~SNMP_Variable();

   BOOL Parse(BYTE *pData, DWORD dwVarLength);
   DWORD Encode(BYTE *pBuffer, DWORD dwBufferSize);

   SNMP_ObjectId *GetName(void) { return m_pName; }
   DWORD GetType(void) { return m_dwType; }
   DWORD GetValueLength(void) { return m_dwValueLength; }
   const BYTE *GetValue(void) { return m_pValue; }

   DWORD GetValueAsUInt(void);
   long GetValueAsInt(void);
   TCHAR *GetValueAsString(TCHAR *pszBuffer, DWORD dwBufferSize);
   SNMP_ObjectId *GetValueAsObjectId(void);
};


//
// SNMP PDU
//

class LIBNXSNMP_EXPORTABLE SNMP_PDU
{
private:
   DWORD m_dwVersion;
   DWORD m_dwCommand;
   char *m_pszCommunity;
   DWORD m_dwNumVariables;
   SNMP_Variable **m_ppVarList;
   SNMP_ObjectId *m_pEnterprise;
   int m_iTrapType;
   int m_iSpecificTrap;
   DWORD m_dwTimeStamp;
   DWORD m_dwAgentAddr;
   DWORD m_dwRqId;
   DWORD m_dwErrorCode;
   DWORD m_dwErrorIndex;

   BOOL ParseVariable(BYTE *pData, DWORD dwVarLength);
   BOOL ParseVarBinds(BYTE *pData, DWORD dwPDULength);
   BOOL ParsePDU(BYTE *pData, DWORD dwPDULength);
   BOOL ParseTrapPDU(BYTE *pData, DWORD dwPDULength);

public:
   SNMP_PDU();
   SNMP_PDU(DWORD dwCommand, char *pszCommunity, DWORD dwRqId, DWORD dwVersion = SNMP_VERSION_2C);
   ~SNMP_PDU();

   BOOL Parse(BYTE *pRawData, DWORD dwRawLength);
   DWORD Encode(BYTE **ppBuffer);

   SNMP_ObjectId *GetTrapId(void) { return m_pEnterprise; }
   int GetTrapType(void) { return m_iTrapType; }
   int GetSpecificTrapType(void) { return m_iSpecificTrap; }
   DWORD GetNumVariables(void) { return m_dwNumVariables; }
   SNMP_Variable *GetVariable(DWORD dwIndex) { return (dwIndex < m_dwNumVariables) ? m_ppVarList[dwIndex] : NULL; }
   const char *GetCommunity(void) { return m_pszCommunity; }
   DWORD GetVersion(void) { return m_dwVersion; }

   DWORD GetRequestId(void) { return m_dwRqId; }
   void SetRequestId(DWORD dwId) { m_dwRqId = dwId; }

   void BindVariable(SNMP_Variable *pVar);
};


//
// SNMP transport
//

class LIBNXSNMP_EXPORTABLE SNMP_Transport
{
private:
   SOCKET m_hSocket;
   DWORD m_dwBufferSize;
   DWORD m_dwBytesInBuffer;
   DWORD m_dwBufferPos;
   BYTE *m_pBuffer;

   DWORD PreParsePDU(void);
   int RecvData(DWORD dwTimeout, struct sockaddr *pSender, int *piAddrSize);
   void ClearBuffer(void);

public:
   SNMP_Transport();
   SNMP_Transport(SOCKET hSocket);
   ~SNMP_Transport();

   int Read(SNMP_PDU **ppData, DWORD dwTimeout = INFINITE,
            struct sockaddr *pSender = NULL, int *piAddrSize = NULL);
   int Send(SNMP_PDU *pPDU, struct sockaddr *pRcpt = NULL, int iAddrLen = 0);
   DWORD DoRequest(SNMP_PDU *pRequest, SNMP_PDU **pResponce, 
                   DWORD dwTimeout = INFINITE, DWORD dwNumRetries = 1);

   BOOL CreateUDPTransport(TCHAR *pszHostName, DWORD dwHostAddr = 0, WORD wPort = 161);
};


//
// Functions
//

void LIBNXSNMP_EXPORTABLE SNMPConvertOIDToText(DWORD dwLength, DWORD *pdwValue, TCHAR *pszBuffer, DWORD dwBufferSize);
DWORD LIBNXSNMP_EXPORTABLE SNMPParseOID(TCHAR *pszText, DWORD *pdwBuffer, DWORD dwBufferSize);
BOOL LIBNXSNMP_EXPORTABLE SNMPIsCorrectOID(TCHAR *pszText);


#endif
