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

#include <nms_common.h>
#include <nms_threads.h>

#ifdef _WIN32
#ifdef LIBNXSNMP_EXPORTS
#define LIBNXSNMP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXSNMP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXSNMP_EXPORTABLE
#endif


/***************************************************************
 Following part of the file may conflict with net-snmp includes,
 so it can be excluded by defining NXSNMP_WITH_NET_SNMP
****************************************************************/

#ifdef NXSNMP_WITH_NET_SNMP

#define SNMP_VERSION_2C    1
#define SNMP_DEFAULT_PORT  161

#else


//
// Various constants
//

#define MAX_OID_LEN           128
#define MAX_MIB_OBJECT_NAME   64
#define SNMP_DEFAULT_PORT     161


//
// OID comparision results
//

#define OID_ERROR          -1
#define OID_EQUAL          0
#define OID_NOT_EQUAL      1
#define OID_SHORTER        2
#define OID_LONGER         3


//
// libnxsnmp error codes
//

#define SNMP_ERR_SUCCESS            0     /* success */
#define SNMP_ERR_TIMEOUT            1     /* request timeout */
#define SNMP_ERR_PARAM              2     /* invalid parameters passed to function */
#define SNMP_ERR_SOCKET             3     /* unable to create socket */
#define SNMP_ERR_COMM               4     /* send/receive error */
#define SNMP_ERR_PARSE              5     /* error parsing PDU */
#define SNMP_ERR_NO_OBJECT          6     /* given object doesn't exist on agent */
#define SNMP_ERR_HOSTNAME           7     /* invalid hostname or IP address */
#define SNMP_ERR_BAD_OID            8     /* object id is incorrect */
#define SNMP_ERR_AGENT              9     /* agent returns an error */
#define SNMP_ERR_BAD_TYPE           10    /* unknown variable data type */
#define SNMP_ERR_FILE_IO            11    /* file I/O error */
#define SNMP_ERR_BAD_FILE_HEADER    12    /* file header is invalid */
#define SNMP_ERR_BAD_FILE_DATA      13    /* file data is invalid or corrupted */


//
// MIB parser error codes
//

#define SNMP_MPE_SUCCESS      0
#define SNMP_MPE_PARSE_ERROR  1


//
// SNMP versions
//

#define SNMP_VERSION_1     0
#define SNMP_VERSION_2C    1


//
// PDU types
//

#define SNMP_INVALID_PDU         255
#define SNMP_GET_REQUEST         0
#define SNMP_GET_NEXT_REQUEST    1
#define SNMP_GET_RESPONSE        2
#define SNMP_SET_REQUEST         3
#define SNMP_TRAP                4
#define SNMP_GET_BULK_REQUEST    5
#define SNMP_INFORM_REQUEST      6


//
// PDU error codes
//

#define SNMP_PDU_ERR_SUCCESS        0
#define SNMP_PDU_ERR_TOO_BIG        1
#define SNMP_PDU_ERR_NO_SUCH_NAME   2
#define SNMP_PDU_ERR_BAD_VALUE      3
#define SNMP_PDU_ERR_READ_ONLY      4
#define SNMP_PDU_ERR_GENERIC        5


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
#define ASN_NO_SUCH_OBJECT          0x80
#define ASN_NO_SUCH_INSTANCE        0x81
#define ASN_GET_REQUEST_PDU         0xA0
#define ASN_GET_NEXT_REQUEST_PDU    0xA1
#define ASN_GET_RESPONSE_PDU        0xA2
#define ASN_SET_REQUEST_PDU         0xA3
#define ASN_TRAP_V1_PDU             0xA4
#define ASN_GET_BULK_REQUEST_PDU    0xA5
#define ASN_INFORM_REQUEST_PDU      0xA6
#define ASN_TRAP_V2_PDU             0xA7


//
// MIB object access types
//

#define MIB_ACCESS_READONLY      0
#define MIB_ACCESS_READWRITE     1
#define MIB_ACCESS_WRITEONLY     5
#define MIB_ACCESS_NOACCESS      3
#define MIB_ACCESS_NOTIFY        4
#define MIB_ACCESS_CREATE        2


//
// MIB object status codes
//

#define MIB_STATUS_MANDATORY     0
#define MIB_STATUS_OPTIONAL      1
#define MIB_STATUS_OBSOLETE      2
#define MIB_STATUS_DEPRECATED    3
#define MIB_STATUS_CURRENT       4


//
// Flags for SNMPSaveMIBTree
//

#define SMT_COMPRESS_DATA        0x01
#define SMT_SKIP_DESCRIPTIONS    0x02


#endif      /* NXSNMP_WITH_NET_SNMP */


//
// MIB tree node
//

class LIBNXSNMP_EXPORTABLE SNMP_MIBObject
{
private:
   SNMP_MIBObject *m_pParent;
   SNMP_MIBObject *m_pNext;
   SNMP_MIBObject *m_pPrev;
   SNMP_MIBObject *m_pFirst;    // First child
   SNMP_MIBObject *m_pLast;     // Last child

   DWORD m_dwOID;
   TCHAR *m_pszName;
   TCHAR *m_pszDescription;
   int m_iType;
   int m_iStatus;
   int m_iAccess;

   void Initialize(void);

public:
   SNMP_MIBObject(void);
   SNMP_MIBObject(DWORD dwOID, TCHAR *pszName);
   SNMP_MIBObject(DWORD dwOID, TCHAR *pszName, int iType, 
                  int iStatus, int iAccess, TCHAR *pszDescription);
   ~SNMP_MIBObject();

   void SetParent(SNMP_MIBObject *pObject) { m_pParent = pObject; }
   void AddChild(SNMP_MIBObject *pObject);
   void SetInfo(int iType, int iStatus, int iAccess, TCHAR *pszDescription);

   SNMP_MIBObject *Parent(void) { return m_pParent; }
   SNMP_MIBObject *Next(void) { return m_pNext; }
   SNMP_MIBObject *Prev(void) { return m_pPrev; }
   SNMP_MIBObject *FirstChild(void) { return m_pFirst; }
   SNMP_MIBObject *LastChild(void) { return m_pLast; }

   SNMP_MIBObject *FindChildByID(DWORD dwOID);

   void Print(int nIndent);

   void WriteToFile(FILE *pFile, DWORD dwFlags);
   BOOL ReadFromFile(FILE *pFile);
};


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
   void SetValue(DWORD *pdwValue, DWORD dwLength);

   int Compare(TCHAR *pszOid);
   int Compare(DWORD *pdwOid, DWORD dwLen);
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
   SNMP_Variable(DWORD *pdwName, DWORD dwNameLen);
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
   TCHAR *GetValueAsMACAddr(TCHAR *pszBuffer);
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
   BOOL ParseTrap2PDU(BYTE *pData, DWORD dwPDULength);

public:
   SNMP_PDU();
   SNMP_PDU(DWORD dwCommand, char *pszCommunity, DWORD dwRqId, DWORD dwVersion = SNMP_VERSION_2C);
   ~SNMP_PDU();

   BOOL Parse(BYTE *pRawData, DWORD dwRawLength);
   DWORD Encode(BYTE **ppBuffer);

   DWORD GetCommand(void) { return m_dwCommand; }
   SNMP_ObjectId *GetTrapId(void) { return m_pEnterprise; }
   int GetTrapType(void) { return m_iTrapType; }
   int GetSpecificTrapType(void) { return m_iSpecificTrap; }
   DWORD GetNumVariables(void) { return m_dwNumVariables; }
   SNMP_Variable *GetVariable(DWORD dwIndex) { return (dwIndex < m_dwNumVariables) ? m_ppVarList[dwIndex] : NULL; }
   const char *GetCommunity(void) { return m_pszCommunity; }
   DWORD GetVersion(void) { return m_dwVersion; }
   DWORD GetErrorCode(void) { return m_dwErrorCode; }

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
   int RecvData(DWORD dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize);
   void ClearBuffer(void);

public:
   SNMP_Transport();
   SNMP_Transport(SOCKET hSocket);
   ~SNMP_Transport();

   int Read(SNMP_PDU **ppData, DWORD dwTimeout = INFINITE,
            struct sockaddr *pSender = NULL, socklen_t *piAddrSize = NULL);
   int Send(SNMP_PDU *pPDU, struct sockaddr *pRcpt = NULL, socklen_t iAddrLen = 0);
   DWORD DoRequest(SNMP_PDU *pRequest, SNMP_PDU **pResponse, 
                   DWORD dwTimeout = INFINITE, DWORD dwNumRetries = 1);

   DWORD CreateUDPTransport(TCHAR *pszHostName, DWORD dwHostAddr = 0, WORD wPort = SNMP_DEFAULT_PORT);
};


//
// Functions
//

void LIBNXSNMP_EXPORTABLE SNMPConvertOIDToText(DWORD dwLength, DWORD *pdwValue, TCHAR *pszBuffer, DWORD dwBufferSize);
DWORD LIBNXSNMP_EXPORTABLE SNMPParseOID(const TCHAR *pszText, DWORD *pdwBuffer, DWORD dwBufferSize);
BOOL LIBNXSNMP_EXPORTABLE SNMPIsCorrectOID(const TCHAR *pszText);
const TCHAR LIBNXSNMP_EXPORTABLE *SNMPGetErrorText(DWORD dwError);
DWORD LIBNXSNMP_EXPORTABLE SNMPSaveMIBTree(TCHAR *pszFile, SNMP_MIBObject *pRoot, DWORD dwFlags);
DWORD LIBNXSNMP_EXPORTABLE SNMPLoadMIBTree(TCHAR *pszFile, SNMP_MIBObject **ppRoot);


#endif
