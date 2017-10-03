/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/** \ingroup SNMP
 * @{
 */

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

#define MAX_OID_LEN                 ((size_t)128)
#define MAX_MIB_OBJECT_NAME         ((size_t)64)
#define SNMP_DEFAULT_PORT           161
#define SNMP_MAX_CONTEXT_NAME       ((size_t)256)
#define SNMP_MAX_ENGINEID_LEN       ((size_t)256)
#define SNMP_DEFAULT_MSG_MAX_SIZE   ((size_t)65536)


//
// OID comparision results
//

#define OID_ERROR          -1
#define OID_EQUAL          0
#define OID_PRECEDING      1
#define OID_FOLLOWING      2
#define OID_SHORTER        3
#define OID_LONGER         4

/**
 * libnxsnmp error codes
 */
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

/**
 * Flags for SNMPSaveMIBTree
 */
#define SMT_COMPRESS_DATA        0x01
#define SMT_SKIP_DESCRIPTIONS    0x02

/**
 * Flags for SnmpGet
 */
#define SG_VERBOSE        0x0001
#define SG_STRING_RESULT  0x0002
#define SG_RAW_RESULT     0x0004
#define SG_HSTRING_RESULT 0x0008
#define SG_PSTRING_RESULT 0x0010

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

   UINT32 m_dwOID;
   TCHAR *m_pszName;
   TCHAR *m_pszDescription;
	TCHAR *m_pszTextualConvention;
   int m_iType;
   int m_iStatus;
   int m_iAccess;

   void Initialize();

public:
   SNMP_MIBObject();
   SNMP_MIBObject(UINT32 dwOID, const TCHAR *pszName);
   SNMP_MIBObject(UINT32 dwOID, const TCHAR *pszName, int iType,
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

   UINT32 getObjectId() { return m_dwOID; }
   const TCHAR *getName() { return m_pszName; }
   const TCHAR *getDescription() { return m_pszDescription; }
   int getType() { return m_iType; }
   int getStatus() { return m_iStatus; }
   int getAccess() { return m_iAccess; }

   SNMP_MIBObject *findChildByID(UINT32 dwOID);

   void print(int nIndent);

   // File I/O, supposed to be callsed only from libnxsnmp functions
   void writeToFile(ZFile *pFile, UINT32 dwFlags);
   BOOL readFromFile(ZFile *pFile);
};

/**
 * Object identifier (OID)
 */
class LIBNXSNMP_EXPORTABLE SNMP_ObjectId
{
private:
   size_t m_length;
   UINT32 *m_value;

public:
   SNMP_ObjectId();
   SNMP_ObjectId(const SNMP_ObjectId &src);
   SNMP_ObjectId(const UINT32 *value, size_t length);
   ~SNMP_ObjectId();

   SNMP_ObjectId& operator =(const SNMP_ObjectId &src);

   size_t length() const { return m_length; }
   const UINT32 *value() const { return m_value; }
   String toString() const;
   TCHAR *toString(TCHAR *buffer, size_t bufferSize) const;
   bool isValid() const { return (m_length > 0) && (m_value != NULL); }
   bool isZeroDotZero() const { return (m_length == 2) && (m_value[0] == 0) && (m_value[1] == 0); }
   UINT32 getElement(size_t index) const { return (index < m_length) ? m_value[index] : 0; }

   int compare(const TCHAR *oid) const;
   int compare(const UINT32 *oid, size_t length) const;
	int compare(const SNMP_ObjectId& oid) const;

   void setValue(const UINT32 *value, size_t length);
   void extend(UINT32 subId);
   void extend(const UINT32 *subId, size_t length);
   void truncate(size_t count);
   void changeElement(size_t pos, UINT32 value) { if (pos < m_length) m_value[pos] = value; }

   static SNMP_ObjectId parse(const TCHAR *oid);
};

/**
 * SNMP variable (varbind)
 */
class LIBNXSNMP_EXPORTABLE SNMP_Variable
{
private:
   SNMP_ObjectId m_name;
   UINT32 m_type;
   size_t m_valueLength;
   BYTE *m_value;

public:
   SNMP_Variable();
   SNMP_Variable(const TCHAR *name);
   SNMP_Variable(const UINT32 *name, size_t nameLen);
   SNMP_Variable(const SNMP_ObjectId &name);
   SNMP_Variable(const SNMP_Variable *src);
   ~SNMP_Variable();

   bool parse(const BYTE *data, size_t varLength);
   size_t encode(BYTE *buffer, size_t bufferSize);

   const SNMP_ObjectId& getName() const { return m_name; }
   UINT32 getType() const { return m_type; }
   size_t getValueLength() const { return m_valueLength; }
   const BYTE *getValue() const { return m_value; }

	size_t getRawValue(BYTE *buffer, size_t bufSize) const;
   UINT32 getValueAsUInt() const;
   LONG getValueAsInt() const;
   TCHAR *getValueAsString(TCHAR *buffer, size_t bufferSize) const;
   TCHAR *getValueAsPrintableString(TCHAR *buffer, size_t bufferSize, bool *convertToHex) const;
   SNMP_ObjectId getValueAsObjectId() const;
   TCHAR *getValueAsMACAddr(TCHAR *buffer) const;
   TCHAR *getValueAsIPAddr(TCHAR *buffer) const;

   void setValueFromString(UINT32 type, const TCHAR *value);
};

/**
 * SNMP engine
 */
class LIBNXSNMP_EXPORTABLE SNMP_Engine
{
private:
	BYTE m_id[SNMP_MAX_ENGINEID_LEN];
	size_t m_idLen;
	int m_engineBoots;
	int m_engineTime;

public:
	SNMP_Engine();
	SNMP_Engine(BYTE *id, size_t idLen, int engineBoots = 0, int engineTime = 0);
	SNMP_Engine(const SNMP_Engine *src);
	~SNMP_Engine();

	const BYTE *getId() const { return m_id; }
	size_t getIdLen() const { return m_idLen; }
	int getBoots() const { return m_engineBoots; }
	int getTime() const { return m_engineTime; }

	void setBoots(int boots) { m_engineBoots = boots; }
	void setTime(int engineTime) { m_engineTime = engineTime; }
};

/**
 * Security context
 */
class LIBNXSNMP_EXPORTABLE SNMP_SecurityContext
{
private:
	int m_securityModel;
	char *m_authName;	// community for V1/V2c, user for V3 USM
	char *m_authPassword;
	char *m_privPassword;
	char *m_contextName;
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
	SNMP_SecurityContext(const char *community);
	SNMP_SecurityContext(const char *user, const char *authPassword, int authMethod);
	SNMP_SecurityContext(const char *user, const char *authPassword, const char *encryptionPassword,
	                     int authMethod, int encryptionMethod);
	~SNMP_SecurityContext();

	int getSecurityModel() { return m_securityModel; }
	const char *getCommunity() { return CHECK_NULL_EX_A(m_authName); }
	const char *getUser() { return CHECK_NULL_EX_A(m_authName); }
	const char *getAuthPassword() { return CHECK_NULL_EX_A(m_authPassword); }
	const char *getPrivPassword() { return CHECK_NULL_EX_A(m_privPassword); }
	const char *getContextName() { return m_contextName; }

	bool needAuthentication() { return (m_authMethod != SNMP_AUTH_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	bool needEncryption() { return (m_privMethod != SNMP_ENCRYPT_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	int getAuthMethod() { return m_authMethod; }
	int getPrivMethod() { return m_privMethod; }
	BYTE *getAuthKeyMD5() { return m_authKeyMD5; }
	BYTE *getAuthKeySHA1() { return m_authKeySHA1; }
	BYTE *getPrivKey() { return m_privKey; }

	json_t *toJson() const;

	void setAuthName(const char *name);
	void setCommunity(const char *community) { setAuthName(community); }
	void setUser(const char *user) { setAuthName(user); }
	void setAuthPassword(const char *password);
	void setPrivPassword(const char *password);
	void setAuthMethod(int method) { m_authMethod = method; }
	void setPrivMethod(int method) { m_privMethod = method; }
	void setSecurityModel(int model);
	void setContextName(const TCHAR *name);
#ifdef UNICODE
	void setContextNameA(const char *name);
#else
	void setContextNameA(const char *name) { setContextName(name); }
#endif

	void setAuthoritativeEngine(const SNMP_Engine &engine);
	const SNMP_Engine& getAuthoritativeEngine() { return m_authoritativeEngine; }
};

/**
 * SNMP PDU
 */
class LIBNXSNMP_EXPORTABLE SNMP_PDU
{
private:
   UINT32 m_version;
   UINT32 m_command;
   ObjectArray<SNMP_Variable> *m_variables;
   SNMP_ObjectId *m_pEnterprise;
   int m_trapType;
   int m_specificTrap;
   UINT32 m_dwTimeStamp;
   UINT32 m_dwAgentAddr;
   UINT32 m_dwRqId;
   UINT32 m_dwErrorCode;
   UINT32 m_dwErrorIndex;
	UINT32 m_msgId;
	UINT32 m_msgMaxSize;
	BYTE m_contextEngineId[SNMP_MAX_ENGINEID_LEN];
	size_t m_contextEngineIdLen;
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
	size_t m_signatureOffset;

   bool parseVariable(const BYTE *pData, size_t varLength);
   bool parseVarBinds(const BYTE *pData, size_t pduLength);
   bool parsePdu(const BYTE *pdu, size_t pduLength);
   bool parseTrapPDU(const BYTE *pData, size_t pduLength);
   bool parseTrap2PDU(const BYTE *pData, size_t pduLength);
   bool parsePduContent(const BYTE *pData, size_t pduLength);
   bool parseV3Header(const BYTE *pData, size_t pduLength);
   bool parseV3SecurityUsm(const BYTE *pData, size_t pduLength, const BYTE *rawMsg);
   bool parseV3ScopedPdu(const BYTE *pData, size_t pduLength);
	bool validateSignedMessage(const BYTE *msg, size_t msgLen, SNMP_SecurityContext *securityContext);
	size_t encodeV3Header(BYTE *buffer, size_t bufferSize, SNMP_SecurityContext *securityContext);
	size_t encodeV3SecurityParameters(BYTE *buffer, size_t bufferSize, SNMP_SecurityContext *securityContext);
	size_t encodeV3ScopedPDU(UINT32 pduType, BYTE *pdu, size_t pduSize, BYTE *buffer, size_t bufferSize);
	void signMessage(BYTE *msg, size_t msgLen, SNMP_SecurityContext *securityContext);
	bool decryptData(const BYTE *data, size_t length, BYTE *decryptedData, SNMP_SecurityContext *securityContext);

public:
   SNMP_PDU();
   SNMP_PDU(UINT32 dwCommand, UINT32 dwRqId, UINT32 dwVersion = SNMP_VERSION_2C);
   SNMP_PDU(SNMP_PDU *src);
   ~SNMP_PDU();

   bool parse(const BYTE *rawData, size_t rawLength, SNMP_SecurityContext *securityContext, bool engineIdAutoupdate);
   size_t encode(BYTE **ppBuffer, SNMP_SecurityContext *securityContext);

   UINT32 getCommand() { return m_command; }
   SNMP_ObjectId *getTrapId() { return m_pEnterprise; }
   int getTrapType() { return m_trapType; }
   int getSpecificTrapType() { return m_specificTrap; }
   int getNumVariables() { return m_variables->size(); }
   SNMP_Variable *getVariable(int index) { return m_variables->get(index); }
   UINT32 getVersion() { return m_version; }
   UINT32 getErrorCode() { return m_dwErrorCode; }

	void setMessageId(UINT32 msgId) { m_msgId = msgId; }
	UINT32 getMessageId() { return m_msgId; }

	bool isReportable() { return m_reportable; }
	void setReportable(bool value) { m_reportable = value; }

	const char *getCommunity() { return (m_authObject != NULL) ? m_authObject : ""; }
	const char *getUser() { return (m_authObject != NULL) ? m_authObject : ""; }
	SNMP_Engine& getAuthoritativeEngine() { return m_authoritativeEngine; }
	int getSecurityModel() { return m_securityModel; }
	int getFlags() { return (int)m_flags; }

   UINT32 getRequestId() { return m_dwRqId; }
   void setRequestId(UINT32 dwId) { m_dwRqId = dwId; }

	void setContextEngineId(const BYTE *id, size_t len);
	void setContextEngineId(const char *id);
	void setContextName(const char *name);
	const char *getContextName() { return m_contextName; }
	size_t getContextEngineIdLength() { return m_contextEngineIdLen; }
	BYTE *getContextEngineId() { return m_contextEngineId; }

   void unlinkVariables() { m_variables->setOwner(false); m_variables->clear(); m_variables->setOwner(true); }
   void bindVariable(SNMP_Variable *pVar);
};

/**
 * Generic SNMP transport
 */
class LIBNXSNMP_EXPORTABLE SNMP_Transport
{
protected:
	SNMP_SecurityContext *m_securityContext;
	SNMP_Engine *m_authoritativeEngine;
	SNMP_Engine *m_contextEngine;
	bool m_enableEngineIdAutoupdate;
	bool m_updatePeerOnRecv;
	bool m_reliable;
	int m_snmpVersion;

public:
   SNMP_Transport();
   virtual ~SNMP_Transport();

   virtual int readMessage(SNMP_PDU **data, UINT32 timeout = INFINITE,
                           struct sockaddr *sender = NULL, socklen_t *addrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL) = 0;
   virtual int sendMessage(SNMP_PDU *pdu) = 0;
   virtual InetAddress getPeerIpAddress() = 0;
   virtual UINT16 getPort() = 0;
   virtual bool isProxyTransport() = 0;

   UINT32 doRequest(SNMP_PDU *request, SNMP_PDU **response, UINT32 timeout = INFINITE, int numRetries = 1);

	void setSecurityContext(SNMP_SecurityContext *ctx);
	SNMP_SecurityContext *getSecurityContext() { return m_securityContext; }
	const char *getCommunityString() { return (m_securityContext != NULL) ? m_securityContext->getCommunity() : ""; }
   const SNMP_Engine *getAuthoritativeEngine() { return m_authoritativeEngine; }

	void enableEngineIdAutoupdate(bool enabled) { m_enableEngineIdAutoupdate = enabled; }
	bool isEngineIdAutoupdateEnabled() { return m_enableEngineIdAutoupdate; }

	void setPeerUpdatedOnRecv(bool enabled) { m_updatePeerOnRecv = enabled; }
	bool isPeerUpdatedOnRecv() { return m_updatePeerOnRecv; }

	void setSnmpVersion(int version) { m_snmpVersion = version; }
	int getSnmpVersion() { return m_snmpVersion; }
};

/**
 * UDP SNMP transport
 */
class LIBNXSNMP_EXPORTABLE SNMP_UDPTransport : public SNMP_Transport
{
protected:
   SOCKET m_hSocket;
   SockAddrBuffer m_peerAddr;
	bool m_connected;
   size_t m_dwBufferSize;
   size_t m_dwBytesInBuffer;
   size_t m_dwBufferPos;
   BYTE *m_pBuffer;
   UINT16 m_port;

   size_t preParsePDU();
   int recvData(UINT32 dwTimeout, struct sockaddr *pSender, socklen_t *piAddrSize);
   void clearBuffer();

public:
   SNMP_UDPTransport();
   SNMP_UDPTransport(SOCKET hSocket);
   virtual ~SNMP_UDPTransport();

   virtual int readMessage(SNMP_PDU **data, UINT32 timeout = INFINITE,
                           struct sockaddr *sender = NULL, socklen_t *addrSize = NULL,
	                        SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = NULL);
   virtual int sendMessage(SNMP_PDU *pPDU);
   virtual InetAddress getPeerIpAddress();
   virtual UINT16 getPort();
   virtual bool isProxyTransport();

   UINT32 createUDPTransport(const TCHAR *hostName, UINT16 port = SNMP_DEFAULT_PORT);
   UINT32 createUDPTransport(const InetAddress& hostAddr, UINT16 port = SNMP_DEFAULT_PORT);
	bool isConnected() { return m_connected; }
};

struct SNMP_SnapshotIndexEntry;

/**
 * SNMP snapshot
 */
class LIBNXSNMP_EXPORTABLE SNMP_Snapshot
{
private:
   ObjectArray<SNMP_Variable> *m_values;
   SNMP_SnapshotIndexEntry *m_index;

   static UINT32 callback(SNMP_Variable *var, SNMP_Transport *transport, void *arg);

   void buildIndex();
   SNMP_SnapshotIndexEntry *find(const UINT32 *oid, size_t oidLen) const;
   SNMP_SnapshotIndexEntry *find(const TCHAR *oid) const;
   SNMP_SnapshotIndexEntry *find(const SNMP_ObjectId& oid) const;

public:
   SNMP_Snapshot();
   virtual ~SNMP_Snapshot();

   static SNMP_Snapshot *create(SNMP_Transport *transport, const TCHAR *baseOid);
   static SNMP_Snapshot *create(SNMP_Transport *transport, const UINT32 *baseOid, size_t oidLen);

   Iterator<SNMP_Variable> *iterator() { return m_values->iterator(); }
   EnumerationCallbackResult walk(const TCHAR *baseOid, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *userArg) const;
   EnumerationCallbackResult walk(const UINT32 *baseOid, size_t baseOidLen, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *userArg) const;

   const SNMP_Variable *get(const TCHAR *oid) const;
   const SNMP_Variable *get(const SNMP_ObjectId& oid) const;
   const SNMP_Variable *get(const UINT32 *oid, size_t oidLen) const;
   const SNMP_Variable *getNext(const TCHAR *oid) const;
   const SNMP_Variable *getNext(const SNMP_ObjectId& oid) const;
   const SNMP_Variable *getNext(const UINT32 *oid, size_t oidLen) const;
   const SNMP_Variable *first() const { return m_values->get(0); }
   const SNMP_Variable *last() const { return m_values->get(m_values->size() - 1); }

   UINT32 getAsInt32(const TCHAR *oid) const { const SNMP_Variable *v = get(oid); return (v != NULL) ? v->getValueAsInt() : 0; }
   UINT32 getAsInt32(const SNMP_ObjectId& oid) const { const SNMP_Variable *v = get(oid); return (v != NULL) ? v->getValueAsInt() : 0; }
   UINT32 getAsInt32(const UINT32 *oid, size_t oidLen) const { const SNMP_Variable *v = get(oid, oidLen); return (v != NULL) ? v->getValueAsInt() : 0; }
   UINT32 getAsUInt32(const TCHAR *oid) const { const SNMP_Variable *v = get(oid); return (v != NULL) ? v->getValueAsUInt() : 0; }
   UINT32 getAsUInt32(const SNMP_ObjectId& oid) const { const SNMP_Variable *v = get(oid); return (v != NULL) ? v->getValueAsUInt() : 0; }
   UINT32 getAsUInt32(const UINT32 *oid, size_t oidLen) const { const SNMP_Variable *v = get(oid, oidLen); return (v != NULL) ? v->getValueAsUInt() : 0; }
   TCHAR *getAsString(const TCHAR *oid, TCHAR *buffer, size_t bufferSize) const { const SNMP_Variable *v = get(oid); if (v != NULL) { v->getValueAsString(buffer, bufferSize); return buffer; } return NULL; }
   TCHAR *getAsString(const SNMP_ObjectId& oid, TCHAR *buffer, size_t bufferSize) const { const SNMP_Variable *v = get(oid); if (v != NULL) { v->getValueAsString(buffer, bufferSize); return buffer; } return NULL; }
   TCHAR *getAsString(const UINT32 *oid, size_t oidLen, TCHAR *buffer, size_t bufferSize) const { const SNMP_Variable *v = get(oid, oidLen); if (v != NULL) { v->getValueAsString(buffer, bufferSize); return buffer; } return NULL; }

   int size() const { return m_values->size(); }
   bool isEmpty() const { return m_values->size() == 0; }
};

/**
 * Functions
 */
TCHAR LIBNXSNMP_EXPORTABLE *SNMPConvertOIDToText(size_t length, const UINT32 *value, TCHAR *buffer, size_t bufferSize);
size_t LIBNXSNMP_EXPORTABLE SNMPParseOID(const TCHAR *text, UINT32 *buffer, size_t bufferSize);
bool LIBNXSNMP_EXPORTABLE SNMPIsCorrectOID(const TCHAR *oid);
size_t LIBNXSNMP_EXPORTABLE SNMPGetOIDLength(const TCHAR *oid);
const TCHAR LIBNXSNMP_EXPORTABLE *SNMPGetErrorText(UINT32 dwError);
UINT32 LIBNXSNMP_EXPORTABLE SNMPSaveMIBTree(const TCHAR *fileName, SNMP_MIBObject *pRoot, UINT32 dwFlags);
UINT32 LIBNXSNMP_EXPORTABLE SNMPLoadMIBTree(const TCHAR *fileName, SNMP_MIBObject **ppRoot);
UINT32 LIBNXSNMP_EXPORTABLE SNMPGetMIBTreeTimestamp(const TCHAR *fileName, UINT32 *pdwTimestamp);
UINT32 LIBNXSNMP_EXPORTABLE SNMPResolveDataType(const TCHAR *pszType);
TCHAR LIBNXSNMP_EXPORTABLE *SNMPDataTypeName(UINT32 type, TCHAR *buffer, size_t bufferSize);

UINT32 LIBNXSNMP_EXPORTABLE SnmpNewRequestId();
void LIBNXSNMP_EXPORTABLE SnmpSetMessageIds(DWORD msgParseError, DWORD msgTypeError, DWORD msgGetError);
void LIBNXSNMP_EXPORTABLE SnmpSetDefaultTimeout(UINT32 timeout);
UINT32 LIBNXSNMP_EXPORTABLE SnmpGetDefaultTimeout();
UINT32 LIBNXSNMP_EXPORTABLE SnmpGet(int version, SNMP_Transport *transport,
                                    const TCHAR *szOidStr, const UINT32 *oidBinary, size_t oidLen, void *pValue,
                                    size_t bufferSize, UINT32 dwFlags);
UINT32 LIBNXSNMP_EXPORTABLE SnmpGetEx(SNMP_Transport *pTransport,
                                      const TCHAR *szOidStr, const UINT32 *oidBinary, size_t oidLen, void *pValue,
                                      size_t bufferSize, UINT32 dwFlags, UINT32 *dataLen);
UINT32 LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid,
						                   UINT32 (* handler)(SNMP_Variable *, SNMP_Transport *, void *),
                                     void *userArg, bool logErrors = false);
UINT32 LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const UINT32 *rootOid, size_t rootOidLen,
                                     UINT32 (* handler)(SNMP_Variable *, SNMP_Transport *, void *),
                                     void *userArg, bool logErrors = false);

#endif   /* __cplusplus */

/** @} */

#endif
