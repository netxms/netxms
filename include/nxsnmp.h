/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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

#include <nms_common.h>
#include <nms_threads.h>

#ifdef LIBNXSNMP_EXPORTS
#define LIBNXSNMP_EXPORTABLE __EXPORT
#else
#define LIBNXSNMP_EXPORTABLE __IMPORT
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

/**
 * Default maximum message (PDU) size. 65507 is UDPv4 limit (65535 bytes − 8-byte UDP header − 20-byte IP header)
 */
#define SNMP_DEFAULT_MSG_MAX_SIZE   ((size_t)65507)

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
#define SNMP_ERR_ABORTED            21    /* operation aborted */


//
// MIB parser error codes
//
#define SNMP_MPE_SUCCESS      0
#define SNMP_MPE_PARSE_ERROR  1

/**
 * SNMP versions
 */
enum SNMP_Version
{
   SNMP_VERSION_1       = 0,
   SNMP_VERSION_2C      = 1,
   SNMP_VERSION_3       = 3,
   SNMP_VERSION_DEFAULT = 127
};

/**
 * Get SNMP version from int
 */
inline SNMP_Version SNMP_VersionFromInt(int v)
{
   switch(v)
   {
      case 0:
         return SNMP_VERSION_1;
      case 1:
         return SNMP_VERSION_2C;
      case 3:
         return SNMP_VERSION_3;
      default:
         return SNMP_VERSION_DEFAULT;
   }
}

/**
 * SNMP command codes (PDU types)
 */
enum SNMP_Command
{
   SNMP_GET_REQUEST         = 0,
   SNMP_GET_NEXT_REQUEST    = 1,
   SNMP_RESPONSE            = 2,
   SNMP_SET_REQUEST         = 3,
   SNMP_TRAP                = 4,
   SNMP_GET_BULK_REQUEST    = 5,
   SNMP_INFORM_REQUEST      = 6,
   SNMP_REPORT              = 8,
   SNMP_INVALID_PDU         = 255
};

/**
 * SNMP PDU error codes
 */
enum SNMP_ErrorCode
{
   SNMP_PDU_ERR_SUCCESS              = 0,
   SNMP_PDU_ERR_TOO_BIG              = 1,
   SNMP_PDU_ERR_NO_SUCH_NAME         = 2,
   SNMP_PDU_ERR_BAD_VALUE            = 3,
   SNMP_PDU_ERR_READ_ONLY            = 4,
   SNMP_PDU_ERR_GENERIC              = 5,
   SNMP_PDU_ERR_NO_ACCESS            = 6,
   SNMP_PDU_ERR_WRONG_TYPE           = 7,
   SNMP_PDU_ERR_WRONG_LENGTH         = 8,
   SNMP_PDU_ERR_WRONG_ENCODING       = 9,
   SNMP_PDU_ERR_WRONG_VALUE          = 10,
   SNMP_PDU_ERR_NO_CREATON           = 11,
   SNMP_PDU_ERR_INCONSISTENT_VALUE   = 12,
   SNMP_PDU_ERR_RESOURCE_UNAVAILABLE = 13,
   SNMP_PDU_ERR_COMMIT_FAILED        = 14,
   SNMP_PDU_ERR_UNDO_FAILED          = 15,
   SNMP_PDU_ERR_AUTHORIZATION_ERROR  = 16,
   SNMP_PDU_ERR_NOT_WRITABLE         = 17,
   SNMP_PDU_ERR_INCONSISTENT_NAME    = 18
};

/**
 * ASN.1 identifier types
 */
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
#define ASN_FLOAT                   0x48
#define ASN_DOUBLE                  0x49
#define ASN_INTEGER64               0x4A
#define ASN_UINTEGER64              0x4B
#define ASN_NO_SUCH_OBJECT          0x80
#define ASN_NO_SUCH_INSTANCE        0x81
#define ASN_END_OF_MIBVIEW          0x82
#define ASN_GET_REQUEST_PDU         0xA0
#define ASN_GET_NEXT_REQUEST_PDU    0xA1
#define ASN_RESPONSE_PDU            0xA2
#define ASN_SET_REQUEST_PDU         0xA3
#define ASN_TRAP_V1_PDU             0xA4
#define ASN_GET_BULK_REQUEST_PDU    0xA5
#define ASN_INFORM_REQUEST_PDU      0xA6
#define ASN_TRAP_V2_PDU             0xA7
#define ASN_REPORT_PDU              0xA8

/**
 * ASN.1 identifier for opaque field
 */
#define ASN_OPAQUE_TAG1             0x9F  /* First byte: 0x80 | 0x1F */
#define ASN_OPAQUE_TAG2             0x30  /* Base value for second byte */
#define ASN_OPAQUE_COUNTER64        (ASN_OPAQUE_TAG2 + ASN_COUNTER64)
#define ASN_OPAQUE_DOUBLE           (ASN_OPAQUE_TAG2 + ASN_DOUBLE)
#define ASN_OPAQUE_FLOAT            (ASN_OPAQUE_TAG2 + ASN_FLOAT)
#define ASN_OPAQUE_INTEGER64        (ASN_OPAQUE_TAG2 + ASN_INTEGER64)
#define ASN_OPAQUE_UINTEGER64       (ASN_OPAQUE_TAG2 + ASN_UINTEGER64)

/**
 * Security models
 */
enum SNMP_SecurityModel
{
   SNMP_SECURITY_MODEL_V1  = 1,
   SNMP_SECURITY_MODEL_V2C = 2,
   SNMP_SECURITY_MODEL_USM = 3
};

//
// SNMP V3 header flags
//
#define SNMP_AUTH_FLAG           0x01
#define SNMP_PRIV_FLAG           0x02
#define SNMP_REPORTABLE_FLAG     0x04

/**
 * SNMP v3 authentication methods
 */
enum SNMP_AuthMethod
{
   SNMP_AUTH_NONE   = 0,
   SNMP_AUTH_MD5    = 1,
   SNMP_AUTH_SHA1   = 2,
   SNMP_AUTH_SHA224 = 3,
   SNMP_AUTH_SHA256 = 4,
   SNMP_AUTH_SHA384 = 5,
   SNMP_AUTH_SHA512 = 6
};

/**
 * SNMP v3 encryption methods
 */
enum SNMP_EncryptionMethod
{
   SNMP_ENCRYPT_NONE = 0,
   SNMP_ENCRYPT_DES = 1,
   SNMP_ENCRYPT_AES_128 = 2,
   SNMP_ENCRYPT_AES_192 = 3,
   SNMP_ENCRYPT_AES_256 = 4,
};

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
#define SG_VERBOSE            0x0001
#define SG_STRING_RESULT      0x0002
#define SG_RAW_RESULT         0x0004
#define SG_HSTRING_RESULT     0x0008
#define SG_PSTRING_RESULT     0x0010
#define SG_GET_NEXT_REQUEST   0x0020

#endif      /* NXSNMP_WITH_NET_SNMP */


#ifdef __cplusplus

class ZFile;

/**
 * SNMP codepage
 */
struct LIBNXSNMP_EXPORTABLE SNMP_Codepage
{
   char codepage[16];

   SNMP_Codepage()
   {
      codepage[0] = 0;
   }

   SNMP_Codepage(const char *cp)
   {
      if (cp != nullptr)
         strlcpy(codepage, cp, 16);
      else
         codepage[0] = 0;
   }

   /**
    * Check if this codepage object represents null value
    */
   bool isNull()
   {
      return codepage[0] == 0;
   }

   /**
    * Get effective codepage value: use override if it is not null and not empty, otherwise own value if it is not empty, otherwise null
    */
   const char *effectiveValue(const char *overrideValue) const
   {
      return ((overrideValue != nullptr) && (*overrideValue != 0)) ? overrideValue : ((codepage[0] != 0) ? codepage : nullptr);
   }
};

/**
 * MIB tree node
 */
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

   void initialize();

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
   void setName(const TCHAR *pszName) { MemFree(m_pszName); m_pszName = _tcsdup(pszName); }

   SNMP_MIBObject *getParent() { return m_pParent; }
   SNMP_MIBObject *getNext() { return m_pNext; }
   SNMP_MIBObject *getPrev() { return m_pPrev; }
   SNMP_MIBObject *getFirstChild() { return m_pFirst; }
   SNMP_MIBObject *getLastChild() { return m_pLast; }

   uint32_t getObjectId() { return m_dwOID; }
   const TCHAR *getName() { return m_pszName; }
   const TCHAR *getDescription() { return m_pszDescription; }
   int getType() { return m_iType; }
   int getStatus() { return m_iStatus; }
   int getAccess() { return m_iAccess; }

   SNMP_MIBObject *findChildByID(uint32_t oid);

   void print(int nIndent);

   // File I/O, supposed to be callsed only from libnxsnmp functions
   void writeToFile(ZFile *file, uint32_t flags);
   bool readFromFile(ZFile *file);
};

/**
 * Object identifier (OID)
 */
class LIBNXSNMP_EXPORTABLE SNMP_ObjectId
{
private:
   size_t m_length;
   uint32_t *m_value;

public:
   SNMP_ObjectId();
   SNMP_ObjectId(const SNMP_ObjectId &src);
   SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t suffix);
   SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t *suffix, size_t length);
   SNMP_ObjectId(const uint32_t *value, size_t length);
   ~SNMP_ObjectId();

   SNMP_ObjectId& operator =(const SNMP_ObjectId &src);

   size_t length() const { return m_length; }
   const uint32_t *value() const { return m_value; }
   String toString() const;
   TCHAR *toString(TCHAR *buffer, size_t bufferSize) const;
   bool isValid() const { return (m_length > 0) && (m_value != NULL); }
   bool isZeroDotZero() const { return (m_length == 2) && (m_value[0] == 0) && (m_value[1] == 0); }
   uint32_t getElement(size_t index) const { return (index < m_length) ? m_value[index] : 0; }
   uint32_t getLastElement() const { return (m_length > 0) ? m_value[m_length - 1] : 0; }

   int compare(const TCHAR *oid) const;
   int compare(const uint32_t *oid, size_t length) const;
	int compare(const SNMP_ObjectId& oid) const;

   void setValue(const uint32_t *value, size_t length);
   void extend(uint32_t subId);
   void extend(const uint32_t *subId, size_t length);
   void truncate(size_t count);
   void changeElement(size_t pos, uint32_t value) { if (pos < m_length) m_value[pos] = value; }

   static SNMP_ObjectId parse(const TCHAR *oid);
};

/**
 * Size of internal data buffer for SNMP varbind
 */
#define SNMP_VARBIND_INTERNAL_BUFFER_SIZE 32

/**
 * SNMP variable (varbind)
 */
class LIBNXSNMP_EXPORTABLE SNMP_Variable
{
private:
   SNMP_ObjectId m_name;
   uint32_t m_type;
   size_t m_valueLength;
   BYTE *m_value;
   BYTE m_valueBuffer[SNMP_VARBIND_INTERNAL_BUFFER_SIZE];
   SNMP_Codepage m_codepage;

   bool decodeContent(const BYTE *data, size_t dataLength, bool enclosedInOpaque);
   void reallocValueBuffer(size_t length)
   {
      if (m_value == nullptr)
      {
         if (length <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE)
            m_value = m_valueBuffer;
         else
            m_value = MemAllocArrayNoInit<BYTE>(length);
      }
      else if (m_value == m_valueBuffer)
      {
         if (length <= SNMP_VARBIND_INTERNAL_BUFFER_SIZE)
            return;
         m_value = MemAllocArrayNoInit<BYTE>(length);
         memcpy(m_value, m_valueBuffer, SNMP_VARBIND_INTERNAL_BUFFER_SIZE);
      }
      else if (length > m_valueLength)
      {
         m_value = MemRealloc(m_value, length);
      }
      m_valueLength = length;
   }

public:
   SNMP_Variable();
   SNMP_Variable(const TCHAR *name);
   SNMP_Variable(const uint32_t *name, size_t nameLen);
   SNMP_Variable(const SNMP_ObjectId &name);
   SNMP_Variable(const SNMP_Variable *src);
   ~SNMP_Variable();

   bool decode(const BYTE *data, size_t varLength);
   size_t encode(BYTE *buffer, size_t bufferSize) const;

   const SNMP_ObjectId& getName() const { return m_name; }
   uint32_t getType() const { return m_type; }
   bool isInteger() const;
   bool isFloat() const;
   bool isString() const;
   size_t getValueLength() const { return m_valueLength; }
   const BYTE *getValue() const { return m_value; }

	size_t getRawValue(BYTE *buffer, size_t bufSize) const;
   int32_t getValueAsInt() const;
   uint32_t getValueAsUInt() const;
   int64_t getValueAsInt64() const;
   uint64_t getValueAsUInt64() const;
   double getValueAsDouble() const;
   TCHAR *getValueAsString(TCHAR *buffer, size_t bufferSize, const char *codepage = nullptr) const;
   TCHAR *getValueAsPrintableString(TCHAR *buffer, size_t bufferSize, bool *convertToHex, const char *codepage = nullptr) const;
   SNMP_ObjectId getValueAsObjectId() const;
   MacAddress getValueAsMACAddr() const;
   TCHAR *getValueAsIPAddr(TCHAR *buffer) const;
   ByteStream* getValueAsByteStream() const { return new ByteStream(m_value, m_valueLength); }

   SNMP_Variable *decodeOpaque() const;

   void setValueFromString(uint32_t type, const TCHAR *value, const char *codepage = nullptr);
   void setValueFromUInt32(uint32_t type, uint32_t value);
   void setValueFromObjectId(uint32_t type, const SNMP_ObjectId& value);
   void setValueFromByteArray(uint32_t type, const BYTE* buffer, size_t size);
   void setValueFromByteStream(uint32_t type, const ByteStream& bs) { setValueFromByteArray(type, bs.buffer(), bs.size()); }

   void setCodepage(const SNMP_Codepage& codepage) { m_codepage = codepage; }
};

/**
 * SNMP engine
 */
class LIBNXSNMP_EXPORTABLE SNMP_Engine
{
private:
	BYTE m_id[SNMP_MAX_ENGINEID_LEN];
	size_t m_idLen;
	uint32_t m_engineBoots;
	uint32_t m_engineTime;
	time_t m_engineTimeDiff;   // Difference between engine time and epoch time

public:
	SNMP_Engine();
	SNMP_Engine(const BYTE *id, size_t idLen, int engineBoots = 0, int engineTime = 0);
	SNMP_Engine(const SNMP_Engine *src);
   SNMP_Engine(const SNMP_Engine& src);

	const BYTE *getId() const { return m_id; }
	size_t getIdLen() const { return m_idLen; }
	uint32_t getBoots() const { return m_engineBoots; }
	uint32_t getTime() const { return m_engineTime; }
	uint32_t getAdjustedTime() const { return (m_engineTime == 0) ? 0 : static_cast<uint32_t>(time(nullptr) - m_engineTimeDiff); }

	bool equals(const SNMP_Engine& e) const { return (m_idLen == e.m_idLen) && !memcmp(m_id, e.m_id, m_idLen); }

	void setBoots(uint32_t boots) { m_engineBoots = boots; }
	void setTime(uint32_t engineTime)
	{
	   m_engineTime = engineTime;
	   m_engineTimeDiff = time(nullptr) - engineTime;
	}
   void setBootsAndTime(const SNMP_Engine& src)
   {
      m_engineBoots = src.m_engineBoots;
      m_engineTime = src.m_engineTime;
      m_engineTimeDiff = src.m_engineTimeDiff;
   }

	String toString() const;
};

/**
 * Security context
 */
class LIBNXSNMP_EXPORTABLE SNMP_SecurityContext
{
private:
   SNMP_SecurityModel m_securityModel;
	char *m_authName;	// community for V1/V2c, user for V3 USM
	char *m_authPassword;
	char *m_privPassword;
	char *m_contextName;
	BYTE m_authKey[64];
	BYTE m_privKey[64];
	bool m_validKeys;
	SNMP_Engine m_authoritativeEngine;
   SNMP_Engine m_contextEngine;
	SNMP_AuthMethod m_authMethod;
	SNMP_EncryptionMethod m_privMethod;

public:
	SNMP_SecurityContext();
	SNMP_SecurityContext(const SNMP_SecurityContext *src);
	SNMP_SecurityContext(const char *community);
	SNMP_SecurityContext(const char *user, const char *authPassword, SNMP_AuthMethod authMethod);
	SNMP_SecurityContext(const char *user, const char *authPassword, const char *encryptionPassword,
	         SNMP_AuthMethod authMethod, SNMP_EncryptionMethod encryptionMethod);
	~SNMP_SecurityContext();

	SNMP_SecurityModel getSecurityModel() const { return m_securityModel; }
	const char *getCommunity() const { return CHECK_NULL_EX_A(m_authName); }
	const char *getUser() const { return CHECK_NULL_EX_A(m_authName); }
	const char *getAuthPassword() const { return CHECK_NULL_EX_A(m_authPassword); }
	const char *getPrivPassword() const { return CHECK_NULL_EX_A(m_privPassword); }
	const char *getContextName() const { return m_contextName; }

	bool needAuthentication() const { return (m_authMethod != SNMP_AUTH_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	bool needEncryption() const { return (m_privMethod != SNMP_ENCRYPT_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	SNMP_AuthMethod getAuthMethod() const { return m_authMethod; }
	SNMP_EncryptionMethod getPrivMethod() const { return m_privMethod; }
	const BYTE *getAuthKey();
	const BYTE *getPrivKey();
   void recalculateKeys();

	json_t *toJson() const;

   void setAuthName(const char *name);
   void setCommunity(const char *community) { setAuthName(community); }
   void setUser(const char *user) { setAuthName(user); }
   void setAuthPassword(const char *password);
   void setPrivPassword(const char *password);
   void setAuthMethod(SNMP_AuthMethod method);
   void setPrivMethod(SNMP_EncryptionMethod method);
   void setSecurityModel(SNMP_SecurityModel model);
   void setContextName(const char *name);

   void setAuthoritativeEngine(const SNMP_Engine &engine);
   const SNMP_Engine& getAuthoritativeEngine() const { return m_authoritativeEngine; }

   void setContextEngine(const SNMP_Engine &engine) { m_contextEngine = engine; }
   const SNMP_Engine& getContextEngine() const { return m_contextEngine; }

   size_t getSignatureSize() const;
};

/**
 * Get signature size for selected authentication method
 */
inline size_t SNMP_SecurityContext::getSignatureSize() const
{
   switch(m_authMethod)
   {
      case SNMP_AUTH_SHA224:
         return 16;
      case SNMP_AUTH_SHA256:
         return 24;
      case SNMP_AUTH_SHA384:
         return 32;
      case SNMP_AUTH_SHA512:
         return 48;
      default:
         return 12;
   }
}

#ifdef _WIN32
template class LIBNXSNMP_EXPORTABLE ObjectArray<SNMP_Variable>;
#endif

/**
 * SNMP PDU
 */
class LIBNXSNMP_EXPORTABLE SNMP_PDU
{
private:
   SNMP_Version m_version;
   SNMP_Command m_command;
   ObjectArray<SNMP_Variable> m_variables;
   SNMP_ObjectId m_trapId;
   int m_trapType;
   int m_specificTrap;
   uint32_t m_timestamp;
   uint32_t m_dwAgentAddr;
   uint32_t m_requestId;
   uint32_t m_errorCode;
   uint32_t m_errorIndex;
   uint32_t m_msgId;
   uint32_t m_msgMaxSize;
	BYTE m_contextEngineId[SNMP_MAX_ENGINEID_LEN];
	size_t m_contextEngineIdLen;
	char m_contextName[SNMP_MAX_CONTEXT_NAME];
	BYTE m_salt[8];
	bool m_reportable;
	SNMP_Codepage m_codepage;

	// The following attributes only used by parser and
	// valid only for received PDUs
	BYTE m_flags;
	char *m_authObject;
	SNMP_Engine m_authoritativeEngine;
	SNMP_SecurityModel m_securityModel;
	BYTE m_signature[48];
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
	size_t encodeV3ScopedPDU(uint32_t pduType, BYTE *pdu, size_t pduSize, BYTE *buffer, size_t bufferSize);
	void signMessage(BYTE *msg, size_t msgLen, SNMP_SecurityContext *securityContext);
	bool decryptData(const BYTE *data, size_t length, BYTE *decryptedData, SNMP_SecurityContext *securityContext);

public:
   SNMP_PDU();
   SNMP_PDU(SNMP_Command command, uint32_t requestId, SNMP_Version version = SNMP_VERSION_2C);
   SNMP_PDU(SNMP_Command command, SNMP_Version version, const SNMP_ObjectId& trapId, uint32_t sysUpTime, uint32_t requestId);
   SNMP_PDU(const SNMP_PDU& src);
   ~SNMP_PDU();

   bool parse(const BYTE *rawData, size_t rawLength, SNMP_SecurityContext *securityContext, bool engineIdAutoupdate);
   size_t encode(BYTE **ppBuffer, SNMP_SecurityContext *securityContext);

   SNMP_Command getCommand() const { return m_command; }
   int getNumVariables() const { return m_variables.size(); }
   SNMP_Variable *getVariable(int index) { return m_variables.get(index); }
   SNMP_Version getVersion() const { return m_version; }
   SNMP_ErrorCode getErrorCode() const { return static_cast<SNMP_ErrorCode>(m_errorCode); }

   void setTrapId(const SNMP_ObjectId& id) { setTrapId(id.value(), id.length()); }
   void setTrapId(const uint32_t *value, size_t length);
   const SNMP_ObjectId& getTrapId() const { return m_trapId; }
   int getTrapType() const { return m_trapType; }
   int getSpecificTrapType() const { return m_specificTrap; }

	void setMessageId(uint32_t msgId) { m_msgId = msgId; }
	uint32_t getMessageId() const { return m_msgId; }

	bool isReportable() const { return m_reportable; }
	void setReportable(bool value) { m_reportable = value; }

	const char *getCommunity() const { return CHECK_NULL_EX_A(m_authObject); }
	const char *getUser() const { return CHECK_NULL_EX_A(m_authObject); }
	const SNMP_Engine& getAuthoritativeEngine() const { return m_authoritativeEngine; }
	SNMP_SecurityModel getSecurityModel() const { return m_securityModel; }
	int getFlags() const { return (int)m_flags; }

	uint32_t getRequestId() const { return m_requestId; }
   void setRequestId(uint32_t requestId) { m_requestId = requestId; }

	void setContextEngineId(const BYTE *id, size_t len);
	void setContextEngineId(const char *id);
	void setContextName(const char *name) { strlcpy(m_contextName, name, SNMP_MAX_CONTEXT_NAME); }
	const char *getContextName() const { return m_contextName; }
	size_t getContextEngineIdLength() const { return m_contextEngineIdLen; }
	const BYTE *getContextEngineId() const { return m_contextEngineId; }

   void setCodepage(const SNMP_Codepage& codepage);

   void bindVariable(SNMP_Variable *var)
   {
      m_variables.add(var);
      var->setCodepage(m_codepage);
   }
   void unlinkVariables();
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
	SNMP_Version m_snmpVersion;
	SNMP_Codepage m_codepage;

	uint32_t doEngineIdDiscovery(SNMP_PDU *originalRequest, uint32_t timeout, int numRetries);

public:
   SNMP_Transport();
   virtual ~SNMP_Transport();

   virtual int readMessage(SNMP_PDU **pdu, uint32_t timeout = INFINITE, struct sockaddr *sender = nullptr,
            socklen_t *addrSize = nullptr, SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = nullptr) = 0;
   virtual int sendMessage(SNMP_PDU *pdu, uint32_t timeout) = 0;
   virtual InetAddress getPeerIpAddress() = 0;
   virtual uint16_t getPort() = 0;
   virtual bool isProxyTransport() = 0;

   uint32_t doRequest(SNMP_PDU *request, SNMP_PDU **response, uint32_t timeout, int numRetries, bool engineIdDiscoveryOnly = false);
   uint32_t doRequest(SNMP_PDU *request, SNMP_PDU **response);
   uint32_t sendTrap(SNMP_PDU *trap, uint32_t timeout = INFINITE, int numRetries = 1);

	void setSecurityContext(SNMP_SecurityContext *ctx);
	SNMP_SecurityContext *getSecurityContext() { return m_securityContext; }
	const char *getCommunityString() const { return (m_securityContext != nullptr) ? m_securityContext->getCommunity() : ""; }
   const SNMP_Engine *getAuthoritativeEngine() const { return m_authoritativeEngine; }
   const SNMP_Engine *getContextEngine() const { return m_contextEngine; }

	void enableEngineIdAutoupdate(bool enabled) { m_enableEngineIdAutoupdate = enabled; }
	bool isEngineIdAutoupdateEnabled() const { return m_enableEngineIdAutoupdate; }

	void setPeerUpdatedOnRecv(bool enabled) { m_updatePeerOnRecv = enabled; }
	bool isPeerUpdatedOnRecv() const { return m_updatePeerOnRecv; }

	void setSnmpVersion(SNMP_Version version) { m_snmpVersion = version; }
	SNMP_Version getSnmpVersion() const { return m_snmpVersion; }

   void setCodepage(const char* codepage) { strlcpy(m_codepage.codepage, codepage, 16); }
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

   virtual int readMessage(SNMP_PDU **pdu, uint32_t timeout = INFINITE, struct sockaddr *sender = nullptr,
            socklen_t *addrSize = nullptr, SNMP_SecurityContext* (*contextFinder)(struct sockaddr *, socklen_t) = nullptr) override;
   virtual int sendMessage(SNMP_PDU *pdu, uint32_t timeout) override;
   virtual InetAddress getPeerIpAddress() override;
   virtual uint16_t getPort() override;
   virtual bool isProxyTransport() override;

   uint32_t createUDPTransport(const TCHAR *hostName, uint16_t port = SNMP_DEFAULT_PORT);
   uint32_t createUDPTransport(const InetAddress& hostAddr, uint16_t port = SNMP_DEFAULT_PORT);
   bool isConnected() const { return m_connected; }
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

   void buildIndex();
   SNMP_SnapshotIndexEntry *find(const uint32_t *oid, size_t oidLen) const;
   SNMP_SnapshotIndexEntry *find(const TCHAR *oid) const;
   SNMP_SnapshotIndexEntry *find(const SNMP_ObjectId& oid) const;

public:
   SNMP_Snapshot();
   virtual ~SNMP_Snapshot();

   static SNMP_Snapshot *create(SNMP_Transport *transport, const TCHAR *baseOid);
   static SNMP_Snapshot *create(SNMP_Transport *transport, const UINT32 *baseOid, size_t oidLen);

   Iterator<SNMP_Variable> begin() { return m_values->begin(); }
   EnumerationCallbackResult walk(const TCHAR *baseOid, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *userArg) const;
   EnumerationCallbackResult walk(const UINT32 *baseOid, size_t baseOidLen, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *userArg) const;

   const SNMP_Variable *get(const TCHAR *oid) const;
   const SNMP_Variable *get(const SNMP_ObjectId& oid) const;
   const SNMP_Variable *get(const uint32_t *oid, size_t oidLen) const;
   const SNMP_Variable *getNext(const TCHAR *oid) const;
   const SNMP_Variable *getNext(const SNMP_ObjectId& oid) const;
   const SNMP_Variable *getNext(const uint32_t *oid, size_t oidLen) const;
   const SNMP_Variable *first() const { return m_values->get(0); }
   const SNMP_Variable *last() const { return m_values->get(m_values->size() - 1); }

   int32_t getAsInt32(const TCHAR *oid) const { const SNMP_Variable *v = get(oid); return (v != nullptr) ? v->getValueAsInt() : 0; }
   int32_t getAsInt32(const SNMP_ObjectId& oid) const { const SNMP_Variable *v = get(oid); return (v != nullptr) ? v->getValueAsInt() : 0; }
   int32_t getAsInt32(const uint32_t *oid, size_t oidLen) const { const SNMP_Variable *v = get(oid, oidLen); return (v != nullptr) ? v->getValueAsInt() : 0; }
   uint32_t getAsUInt32(const TCHAR *oid) const { const SNMP_Variable *v = get(oid); return (v != nullptr) ? v->getValueAsUInt() : 0; }
   uint32_t getAsUInt32(const SNMP_ObjectId& oid) const { const SNMP_Variable *v = get(oid); return (v != nullptr) ? v->getValueAsUInt() : 0; }
   uint32_t getAsUInt32(const uint32_t *oid, size_t oidLen) const { const SNMP_Variable *v = get(oid, oidLen); return (v != nullptr) ? v->getValueAsUInt() : 0; }
   TCHAR *getAsString(const TCHAR *oid, TCHAR *buffer, size_t bufferSize) const { const SNMP_Variable *v = get(oid); if (v != nullptr) { v->getValueAsString(buffer, bufferSize); return buffer; } return nullptr; }
   TCHAR *getAsString(const SNMP_ObjectId& oid, TCHAR *buffer, size_t bufferSize) const { const SNMP_Variable *v = get(oid); if (v != NULL) { v->getValueAsString(buffer, bufferSize); return buffer; } return nullptr; }
   TCHAR *getAsString(const uint32_t *oid, size_t oidLen, TCHAR *buffer, size_t bufferSize) const { const SNMP_Variable *v = get(oid, oidLen); if (v != nullptr) { v->getValueAsString(buffer, bufferSize); return buffer; } return nullptr; }

   int size() const { return m_values->size(); }
   bool isEmpty() const { return m_values->size() == 0; }
};

/**
 * Functions
 */
TCHAR LIBNXSNMP_EXPORTABLE *SnmpConvertOIDToText(size_t length, const uint32_t *value, TCHAR *buffer, size_t bufferSize);
size_t LIBNXSNMP_EXPORTABLE SnmpParseOID(const TCHAR *text, uint32_t *buffer, size_t bufferSize);
bool LIBNXSNMP_EXPORTABLE SnmpIsCorrectOID(const TCHAR *oid);
size_t LIBNXSNMP_EXPORTABLE SnmpGetOIDLength(const TCHAR *oid);
uint32_t LIBNXSNMP_EXPORTABLE SnmpSaveMIBTree(const TCHAR *fileName, SNMP_MIBObject *root, uint32_t flags);
uint32_t LIBNXSNMP_EXPORTABLE SnmpLoadMIBTree(const TCHAR *fileName, SNMP_MIBObject **ppRoot);
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetMIBTreeTimestamp(const TCHAR *fileName, uint32_t *timestamp);
uint32_t LIBNXSNMP_EXPORTABLE SnmpResolveDataType(const TCHAR *type);
TCHAR LIBNXSNMP_EXPORTABLE *SnmpDataTypeName(uint32_t type, TCHAR *buffer, size_t bufferSize);

const TCHAR LIBNXSNMP_EXPORTABLE *SnmpGetErrorText(uint32_t errorCode);
const TCHAR LIBNXSNMP_EXPORTABLE *SnmpGetProtocolErrorText(SNMP_ErrorCode errorCode);

uint32_t LIBNXSNMP_EXPORTABLE SnmpNewRequestId();
void LIBNXSNMP_EXPORTABLE SnmpSetDefaultTimeout(uint32_t timeout);
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetDefaultTimeout();
void LIBNXSNMP_EXPORTABLE SnmpSetDefaultRetryCount(int numRetries);
int LIBNXSNMP_EXPORTABLE SnmpGetDefaultRetryCount();
uint32_t LIBNXSNMP_EXPORTABLE SnmpGet(SNMP_Version version, SNMP_Transport *transport, const TCHAR *oidStr, const uint32_t *oidBinary, size_t oidLen, void *value, size_t bufferSize, uint32_t flags);
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetEx(SNMP_Transport *transport, const TCHAR *oidStr, const uint32_t *oidBinary, size_t oidLen,
      void *value, size_t bufferSize, uint32_t flags, uint32_t *dataLen = nullptr, const char *codepage = nullptr);
bool LIBNXSNMP_EXPORTABLE CheckSNMPIntegerValue(SNMP_Transport *snmpTransport, const TCHAR *oid, int32_t value);
uint32_t LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors = false, bool failOnShutdown = false);
uint32_t LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors = false, bool failOnShutdown = false);
int LIBNXSNMP_EXPORTABLE SnmpWalkCount(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen);
int LIBNXSNMP_EXPORTABLE SnmpWalkCount(SNMP_Transport *transport, const TCHAR *rootOid);

uint32_t LIBNXSNMP_EXPORTABLE SnmpScanAddressRange(const InetAddress& from, const InetAddress& to, uint16_t port, SNMP_Version snmpVersion,
      const char *community, void (*callback)(const InetAddress&, uint32_t, void*), void *context);

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
static inline uint32_t SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid,
         uint32_t (*handler)(SNMP_Variable*, SNMP_Transport*, void*), void *context, bool logErrors = false, bool failOnShutdown = false)
{
   return SnmpWalk(transport, rootOid, [transport, handler, context](SNMP_Variable *v) -> uint32_t { return handler(v, transport, context); }, logErrors, failOnShutdown);
}

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
static inline uint32_t SnmpWalk(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen,
         uint32_t (*handler)(SNMP_Variable*, SNMP_Transport*, void*), void *context, bool logErrors = false, bool failOnShutdown = false)
{
   return SnmpWalk(transport, rootOid, rootOidLen, [transport, handler, context](SNMP_Variable *v) -> uint32_t { return handler(v, transport, context); }, logErrors, failOnShutdown);
}

/**
 * Wrapper function for calling SnmpWalk with specific context type
 */
template <typename C> uint32_t SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, uint32_t (*callback)(SNMP_Variable*, SNMP_Transport*, C*), C *context, bool logErrors = false, bool failOnShutdown = false)
{
   return SnmpWalk(transport, rootOid, reinterpret_cast<uint32_t (*)(SNMP_Variable*, SNMP_Transport*, void*)>(callback), context, logErrors, failOnShutdown);
}

/**
 * Wrapper data for SnmpWalk
 */
template <typename T> class __SnmpWalk_WrapperData
{
public:
   T *m_object;
   uint32_t (T::*m_func)(SNMP_Variable *, SNMP_Transport *);

   __SnmpWalk_WrapperData(T *object, uint32_t (T::*func)(SNMP_Variable *, SNMP_Transport *))
   {
      m_object = object;
      m_func = func;
   }
};

/**
 * Callback wrapper for calling SnmpWalk on object
 */
template <typename T> uint32_t __SnmpWalk_Wrapper(SNMP_Variable *v, SNMP_Transport *transport, void *arg)
{
   __SnmpWalk_WrapperData<T> *wd = static_cast<__SnmpWalk_WrapperData<T>*>(arg);
   return ((*wd->m_object).*(wd->m_func))(v, transport);
}

/**
 * Wrapper function for calling SnmpWalk on object
 */
template <typename T> uint32_t SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, T *object, uint32_t (T::*func)(SNMP_Variable*, SNMP_Transport*))
{
   __SnmpWalk_WrapperData<T> data(object, func);
   return SnmpWalk(transport, rootOid, __SnmpWalk_Wrapper<T>, &data);
}

/**
 * Wrapper function for calling SnmpWalk on object
 */
template <typename T> uint32_t SnmpWalk(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen, T *object, uint32_t (T::*func)(SNMP_Variable*, SNMP_Transport*))
{
   __SnmpWalk_WrapperData<T> data(object, func);
   return SnmpWalk(transport, rootOid, rootOidLen, __SnmpWalk_Wrapper<T>, &data);
}

#endif   /* __cplusplus */

/** @} */

#endif
