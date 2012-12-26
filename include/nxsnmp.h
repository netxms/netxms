/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: nxsnmp.h
**
**/

#ifndef _nxsnmp_h_
#define _nxsnmp_h_

#ifdef __cplusplus
#include <nms_common.h>
#include <nms_threads.h>
#endif

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

#define MAX_OID_LEN                 128
#define MAX_MIB_OBJECT_NAME         64
#define SNMP_DEFAULT_PORT           161
#define SNMP_MAX_CONTEXT_NAME       256
#define SNMP_MAX_ENGINEID_LEN       256
#define SNMP_DEFAULT_MSG_MAX_SIZE   65536


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
#define SNMP_ERR_UNSUPP_SEC_LEVEL   14    /* unsupported security level */
#define SNMP_ERR_TIME_WINDOW        15    /* not in time window */
#define SNMP_ERR_SEC_NAME           16    /* unknown security name */
#define SNMP_ERR_ENGINE_ID          17    /* unknown engine ID */
#define SNMP_ERR_AUTH_FAILURE       18    /* authentication failure */
#define SNMP_ERR_DECRYPTION         19    /* decryption error */
#define SNMP_ERR_BAD_RESPONSE       20    /* malformed or unexpected response from agent */


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
#define SNMP_VERSION_3     3


//
// PDU types
//

#define SNMP_INVALID_PDU         255
#define SNMP_GET_REQUEST         0
#define SNMP_GET_NEXT_REQUEST    1
#define SNMP_RESPONSE            2
#define SNMP_SET_REQUEST         3
#define SNMP_TRAP                4
#define SNMP_GET_BULK_REQUEST    5
#define SNMP_INFORM_REQUEST      6
#define SNMP_REPORT              8


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
#define ASN_RESPONSE_PDU            0xA2
#define ASN_SET_REQUEST_PDU         0xA3
#define ASN_TRAP_V1_PDU             0xA4
#define ASN_GET_BULK_REQUEST_PDU    0xA5
#define ASN_INFORM_REQUEST_PDU      0xA6
#define ASN_TRAP_V2_PDU             0xA7
#define ASN_REPORT_PDU              0xA8


//
// Security models
//

#define SNMP_SECURITY_MODEL_V1      1
#define SNMP_SECURITY_MODEL_V2C     2
#define SNMP_SECURITY_MODEL_USM     3


//
// SNMP V3 header flags
//

#define SNMP_AUTH_FLAG           0x01
#define SNMP_PRIV_FLAG           0x02
#define SNMP_REPORTABLE_FLAG     0x04


//
// SNMP v3 authentication methods
//

#define SNMP_AUTH_NONE           0
#define SNMP_AUTH_MD5            1
#define SNMP_AUTH_SHA1           2


//
// SNMP v3 encryption methods
//

#define SNMP_ENCRYPT_NONE        0
#define SNMP_ENCRYPT_DES         1
#define SNMP_ENCRYPT_AES         2


//
// MIB object access types
//

#define MIB_ACCESS_READONLY      1
#define MIB_ACCESS_READWRITE     2
#define MIB_ACCESS_WRITEONLY     3
#define MIB_ACCESS_NOACCESS      4
#define MIB_ACCESS_NOTIFY        5
#define MIB_ACCESS_CREATE        6


//
// MIB object status codes
//

#define MIB_STATUS_MANDATORY     1
#define MIB_STATUS_OPTIONAL      2
#define MIB_STATUS_OBSOLETE      3
#define MIB_STATUS_DEPRECATED    4
#define MIB_STATUS_CURRENT       5


//
// MIB object data types
//

#define MIB_TYPE_OTHER                 0
#define MIB_TYPE_IMPORT_ITEM           1
#define MIB_TYPE_OBJID                 2
#define MIB_TYPE_BITSTRING             3
#define MIB_TYPE_INTEGER               4
#define MIB_TYPE_INTEGER32             5
#define MIB_TYPE_INTEGER64             6
#define MIB_TYPE_UNSIGNED32            7
#define MIB_TYPE_COUNTER               8
#define MIB_TYPE_COUNTER32             9
#define MIB_TYPE_COUNTER64             10
#define MIB_TYPE_GAUGE                 11
#define MIB_TYPE_GAUGE32               12
#define MIB_TYPE_TIMETICKS             13
#define MIB_TYPE_OCTETSTR              14
#define MIB_TYPE_OPAQUE                15
#define MIB_TYPE_IPADDR                16
#define MIB_TYPE_PHYSADDR              17
#define MIB_TYPE_NETADDR               18
#define MIB_TYPE_NAMED_TYPE            19
#define MIB_TYPE_SEQID                 20
#define MIB_TYPE_SEQUENCE              21
#define MIB_TYPE_CHOICE                22
#define MIB_TYPE_TEXTUAL_CONVENTION    23
#define MIB_TYPE_MACRO_DEFINITION      24
#define MIB_TYPE_MODCOMP               25
#define MIB_TYPE_TRAPTYPE              26
#define MIB_TYPE_NOTIFTYPE             27
#define MIB_TYPE_MODID                 28
#define MIB_TYPE_NSAPADDRESS           29
#define MIB_TYPE_AGENTCAP              30
#define MIB_TYPE_UINTEGER              31
#define MIB_TYPE_NULL                  32
#define MIB_TYPE_OBJGROUP              33
#define MIB_TYPE_NOTIFGROUP            34


//
// Flags for SNMPSaveMIBTree
//

#define SMT_COMPRESS_DATA        0x01
#define SMT_SKIP_DESCRIPTIONS    0x02


#endif      /* NXSNMP_WITH_NET_SNMP */


#ifdef __cplusplus

//
// MIB tree node
//

class ZFile;

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
	TCHAR *m_pszTextualConvention;
   int m_iType;
   int m_iStatus;
   int m_iAccess;

   void Initialize();

public:
   SNMP_MIBObject();
   SNMP_MIBObject(DWORD dwOID, const TCHAR *pszName);
   SNMP_MIBObject(DWORD dwOID, const TCHAR *pszName, int iType, 
                  int iStatus, int iAccess, const TCHAR *pszDescription,
						const TCHAR *pszTextualConvention);
   ~SNMP_MIBObject();

   void setParent(SNMP_MIBObject *pObject) { m_pParent = pObject; }
   void addChild(SNMP_MIBObject *pObject);
   void setInfo(int iType, int iStatus, int iAccess, const TCHAR *pszDescription, const TCHAR *pszTextualConvention);
   void setName(const TCHAR *pszName) { safe_free(m_pszName); m_pszName = _tcsdup(pszName); }

   SNMP_MIBObject *getParent() { return m_pParent; }
   SNMP_MIBObject *getNext() { return m_pNext; }
   SNMP_MIBObject *getPrev() { return m_pPrev; }
   SNMP_MIBObject *getFirstChild() { return m_pFirst; }
   SNMP_MIBObject *getLastChild() { return m_pLast; }

   DWORD getObjectId() { return m_dwOID; }
   const TCHAR *getName() { return m_pszName; }
   const TCHAR *getDescription() { return m_pszDescription; }
   int getType() { return m_iType; }
   int getStatus() { return m_iStatus; }
   int getAccess() { return m_iAccess; }

   SNMP_MIBObject *findChildByID(DWORD dwOID);

   void print(int nIndent);

   // File I/O, supposed to be callsed only from libnxsnmp functions
   void writeToFile(ZFile *pFile, DWORD dwFlags);
   BOOL readFromFile(ZFile *pFile);
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

   void convertToText();

public:
   SNMP_ObjectId();
   SNMP_ObjectId(DWORD dwLength, const DWORD *pdwValue);
   ~SNMP_ObjectId();

   DWORD getLength() { return m_dwLength; }
   const DWORD *getValue() { return m_pdwValue; }
   const TCHAR *getValueAsText() { return CHECK_NULL(m_pszTextValue); }
   void setValue(DWORD *pdwValue, DWORD dwLength);
   void extend(DWORD dwSubId);

   int compare(TCHAR *pszOid);
   int compare(DWORD *pdwOid, DWORD dwLen);
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
   SNMP_Variable(const TCHAR *pszName);
   SNMP_Variable(DWORD *pdwName, DWORD dwNameLen);
   ~SNMP_Variable();

   BOOL Parse(BYTE *pData, DWORD dwVarLength);
   DWORD Encode(BYTE *pBuffer, DWORD dwBufferSize);

   SNMP_ObjectId *GetName() { return m_pName; }
   DWORD GetType() { return m_dwType; }
   DWORD GetValueLength() { return m_dwValueLength; }
   const BYTE *GetValue() { return m_pValue; }

	size_t getRawValue(BYTE *buffer, size_t bufSize);
   DWORD GetValueAsUInt();
   LONG GetValueAsInt();
   TCHAR *GetValueAsString(TCHAR *pszBuffer, DWORD dwBufferSize);
   TCHAR *getValueAsPrintableString(TCHAR *buffer, DWORD bufferSize, bool *convertToHex);
   SNMP_ObjectId *GetValueAsObjectId();
   TCHAR *GetValueAsMACAddr(TCHAR *pszBuffer);
   TCHAR *GetValueAsIPAddr(TCHAR *pszBuffer);

   void SetValueFromString(DWORD dwType, const TCHAR *pszValue);
};


//
// SNMP engine
//

class LIBNXSNMP_EXPORTABLE SNMP_Engine
{
private:
	BYTE m_id[SNMP_MAX_ENGINEID_LEN];
	int m_idLen;
	int m_engineBoots;
	int m_engineTime;

public:
	SNMP_Engine();
	SNMP_Engine(BYTE *id, int idLen, int engineBoots = 0, int engineTime = 0);
	SNMP_Engine(SNMP_Engine *src);
	~SNMP_Engine();

	BYTE *getId() { return m_id; }
	int getIdLen() { return m_idLen; }
	int getBoots() { return m_engineBoots; }
	int getTime() { return m_engineTime; }

	void setBoots(int boots) { m_engineBoots = boots; }
	void setTime(int engineTime) { m_engineTime = engineTime; }
};


//
// Security context
//

class LIBNXSNMP_EXPORTABLE SNMP_SecurityContext
{
private:
	int m_securityModel;
	char *m_authName;	// community for V1/V2c, user for V3 USM
	char *m_authPassword;
	char *m_privPassword;
	BYTE m_authKeyMD5[16];
	BYTE m_authKeySHA1[20];
	BYTE m_privKey[20];
	SNMP_Engine m_authoritativeEngine;
	int m_authMethod;
	int m_privMethod;

	void recalculateKeys();

public:
	SNMP_SecurityContext();
	SNMP_SecurityContext(SNMP_SecurityContext *src);
	SNMP_SecurityContext(const char *user);
	SNMP_SecurityContext(const char *user, const char *authPassword, int authMethod);
	SNMP_SecurityContext(const char *user, const char *authPassword, const char *encryptionPassword,
	                     int authMethod, int encryptionMethod);
	~SNMP_SecurityContext();

	int getSecurityModel() { return m_securityModel; }
	const char *getCommunity() { return CHECK_NULL_EX_A(m_authName); }
	const char *getUser() { return CHECK_NULL_EX_A(m_authName); }
	const char *getAuthPassword() { return CHECK_NULL_EX_A(m_authPassword); }
	const char *getPrivPassword() { return CHECK_NULL_EX_A(m_privPassword); }

	bool needAuthentication() { return (m_authMethod != SNMP_AUTH_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	bool needEncryption() { return (m_privMethod != SNMP_ENCRYPT_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	int getAuthMethod() { return m_authMethod; }
	int getPrivMethod() { return m_privMethod; }
	BYTE *getAuthKeyMD5() { return m_authKeyMD5; }
	BYTE *getAuthKeySHA1() { return m_authKeySHA1; }
	BYTE *getPrivKey() { return m_privKey; }

	void setAuthName(const char *name);
	void setCommunity(const char *community) { setAuthName(community); }
	void setUser(const char *user) { setAuthName(user); }
	void setAuthPassword(const char *password);
	void setPrivPassword(const char *password);
	void setAuthMethod(int method) { m_authMethod = method; }
	void setPrivMethod(int method) { m_privMethod = method; }
	void setSecurityModel(int model) { m_securityModel = model; }

	void setAuthoritativeEngine(SNMP_Engine &engine);
	SNMP_Engine& getAuthoritativeEngine() { return m_authoritativeEngine; }
};


//
// SNMP PDU
//

class LIBNXSNMP_EXPORTABLE SNMP_PDU
{
private:
   DWORD m_dwVersion;
   DWORD m_dwCommand;
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
	DWORD m_msgId;
	DWORD m_msgMaxSize;
	BYTE m_contextEngineId[SNMP_MAX_ENGINEID_LEN];
	int m_contextEngineIdLen;
	char m_contextName[SNMP_MAX_CONTEXT_NAME];
	BYTE m_salt[8];
	bool m_reportable;
	
	// The following attributes only used by parser and
	// valid only for received PDUs
	BYTE m_flags;
	char *m_authObject;
	SNMP_Engine m_authoritativeEngine;
	int m_securityModel;
	BYTE m_signature[12];

   BOOL parseVariable(BYTE *pData, DWORD dwVarLength);
   BOOL parseVarBinds(BYTE *pData, DWORD dwPDULength);
   BOOL parsePdu(BYTE *pdu, DWORD pduLength);
   BOOL parseTrapPDU(BYTE *pData, DWORD dwPDULength);
   BOOL parseTrap2PDU(BYTE *pData, DWORD dwPDULength);
   BOOL parsePduContent(BYTE *pData, DWORD dwPDULength);
   BOOL parseV3Header(BYTE *pData, DWORD dwPDULength);
   BOOL parseV3SecurityUsm(BYTE *pData, DWORD dwPDULength);
   BOOL parseV3ScopedPdu(BYTE *pData, DWORD dwPDULength);
	BOOL validateSignedMessage(BYTE *msg, DWORD msgLen, SNMP_SecurityContext *securityContext);
	DWORD encodeV3Header(BYTE *buffer, DWORD bufferSize, SNMP_SecurityContext *securityContext);
	DWORD encodeV3SecurityParameters(BYTE *buffer, DWORD bufferSize, SNMP_SecurityContext *securityContext);
	DWORD encodeV3ScopedPDU(DWORD pduType, BYTE *pdu, DWORD pduSize, BYTE *buffer, DWORD bufferSize);
	void signMessage(BYTE *msg, DWORD msgLen, SNMP_SecurityContext *securityContext);
	BOOL decryptData(BYTE *data, DWORD length, BYTE *decryptedData, SNMP_SecurityContext *securityContext);

public:
   SNMP_PDU();
   SNMP_PDU(DWORD dwCommand, DWORD dwRqId, DWORD dwVersion = SNMP_VERSION_2C);
   ~SNMP_PDU();

   BOOL parse(BYTE *pRawData, DWORD dwRawLength, SNMP_SecurityContext *securityContext, bool engineIdAutoupdate);
   DWORD encode(BYTE **ppBuffer, SNMP_SecurityContext *securityContext);

   DWORD getCommand() { return m_dwCommand; }
   SNMP_ObjectId *getTrapId() { return m_pEnterprise; }
   int getTrapType() { return m_iTrapType; }
   int getSpecificTrapType() { return m_iSpecificTrap; }
   DWORD getNumVariables() { return m_dwNumVariables; }
   SNMP_Variable *getVariable(DWORD dwIndex) { return (dwIndex < m_dwNumVariables) ? m_ppVarList[dwIndex] : NULL; }
   DWORD getVersion() { return m_dwVersion; }
   DWORD getErrorCode() { return m_dwErrorCode; }

	void setMessageId(DWORD msgId) { m_msgId = msgId; }
	DWORD getMessageId() { return m_msgId; }

	bool isReportable() { return m_reportable; }
	void setReportable(bool value) { m_reportable = value; }

	const char *getCommunity() { return (m_authObject != NULL) ? m_authObject : ""; }
	const char *getUser() { return (m_authObject != NULL) ? m_authObject : ""; }
	SNMP_Engine& getAuthoritativeEngine() { return m_authoritativeEngine; }
	int getSecurityModel() { return m_securityModel; }
	int getFlags() { return (int)m_flags; }

   DWORD getRequestId() { return m_dwRqId; }
   void setRequestId(DWORD dwId) { m_dwRqId = dwId; }

	void setContextEngineId(BYTE *id, int len);
	void setContextEngineId(const char *id);
	void setContextName(const char *name);
	const char *getContextName() { return m_contextName; }
	int getContextEngineIdLength() { return m_contextEngineIdLen; }
	BYTE *getContextEngineId() { return m_contextEngineId; }

	void unlinkVariables() { safe_free_and_null(m_ppVarList); m_dwNumVariables = 0; }
   void bindVariable(SNMP_Variable *pVar);
};


//
// Generic SNMP transport
//

class LIBNXSNMP_EXPORTABLE SNMP_Transport
{
protected:
	SNMP_SecurityContext *m_securityContext;
	SNMP_Engine *m_authoritativeEngine;
	SNMP_Engine *m_contextEngine;
	bool m_enableEngineIdAutoupdate;
	bool m_updatePeerOnRecv;
	int m_snmpVersion;

public:
   SNMP_Transport();
   virtual ~SNMP_Transport();

   virtual int readMessage(SNMP_PDU **data, DWORD timeout = INFINITE,
                           struct sockaddr *sender = NULL, socklen_t *addrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL)
	{
		return -1;
	}
   virtual int sendMessage(SNMP_PDU *pdu)
	{
		return -1;
	}

   DWORD doRequest(SNMP_PDU *request, SNMP_PDU **response, 
                   DWORD timeout = INFINITE, int numRetries = 1);

	void setSecurityContext(SNMP_SecurityContext *ctx);
	SNMP_SecurityContext *getSecurityContext() { return m_securityContext; }
	const char *getCommunityString() { return (m_securityContext != NULL) ? m_securityContext->getCommunity() : ""; }

	void enableEngineIdAutoupdate(bool enabled) { m_enableEngineIdAutoupdate = enabled; }
	bool isEngineIdAutoupdateEnabled() { return m_enableEngineIdAutoupdate; }

	void setPeerUpdatedOnRecv(bool enabled) { m_updatePeerOnRecv = enabled; }
	bool isPeerUpdatedOnRecv() { return m_updatePeerOnRecv; }

	void setSnmpVersion(int version) { m_snmpVersion = version; }
	int getSnmpVersion() { return m_snmpVersion; }
};


//
// UDP SNMP transport
//

class LIBNXSNMP_EXPORTABLE SNMP_UDPTransport : public SNMP_Transport
{
private:
   SOCKET m_hSocket;
   struct sockaddr_in m_peerAddr;
	bool m_connected;
   DWORD m_dwBufferSize;
   DWORD m_dwBytesInBuffer;
   DWORD m_dwBufferPos;
   BYTE *m_pBuffer;

   DWORD preParsePDU();
   int recvData(DWORD dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize);
   void clearBuffer();

public:
   SNMP_UDPTransport();
   SNMP_UDPTransport(SOCKET hSocket);
   virtual ~SNMP_UDPTransport();

   virtual int readMessage(SNMP_PDU **data, DWORD timeout = INFINITE,
                           struct sockaddr *sender = NULL, socklen_t *addrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL);
   virtual int sendMessage(SNMP_PDU *pPDU);

   DWORD createUDPTransport(TCHAR *pszHostName, DWORD dwHostAddr = 0, WORD wPort = SNMP_DEFAULT_PORT);
	bool isConnected() { return m_connected; }
};


//
// Functions
//

TCHAR LIBNXSNMP_EXPORTABLE *SNMPConvertOIDToText(DWORD dwLength, DWORD *pdwValue, TCHAR *pszBuffer, DWORD dwBufferSize);
DWORD LIBNXSNMP_EXPORTABLE SNMPParseOID(const TCHAR *pszText, DWORD *pdwBuffer, DWORD dwBufferSize);
BOOL LIBNXSNMP_EXPORTABLE SNMPIsCorrectOID(const TCHAR *pszText);
const TCHAR LIBNXSNMP_EXPORTABLE *SNMPGetErrorText(DWORD dwError);
DWORD LIBNXSNMP_EXPORTABLE SNMPSaveMIBTree(const TCHAR *pszFile, SNMP_MIBObject *pRoot, DWORD dwFlags);
DWORD LIBNXSNMP_EXPORTABLE SNMPLoadMIBTree(const TCHAR *pszFile, SNMP_MIBObject **ppRoot);
DWORD LIBNXSNMP_EXPORTABLE SNMPGetMIBTreeTimestamp(const TCHAR *pszFile, DWORD *pdwTimestamp);
DWORD LIBNXSNMP_EXPORTABLE SNMPResolveDataType(const TCHAR *pszType);
TCHAR LIBNXSNMP_EXPORTABLE *SNMPDataTypeName(DWORD type, TCHAR *buffer, size_t bufferSize);

#endif   /* __cplusplus */

#endif
