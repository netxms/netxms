/*
** NetXMS - Network Management System
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
** File: nxsnmp.h
**
**/

#ifndef _nxsnmp_h_
#define _nxsnmp_h_

#include <nms_common.h>
#include <nms_threads.h>
#include <initializer_list>

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
#define SNMP_ERR_SNAPSHOT_TOO_BIG   22    /* snapshot size limit reached */


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
static inline SNMP_Version SNMP_VersionFromInt(int v)
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
#define SG_OBJECT_ID_RESULT   0x0040

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

   uint32_t m_dwOID;
   TCHAR *m_pszName;
   TCHAR *m_pszDescription;
	TCHAR *m_pszTextualConvention;
	TCHAR *m_index;
   int m_iType;
   int m_iStatus;
   int m_iAccess;

   void initialize();

public:
   SNMP_MIBObject();
   SNMP_MIBObject(uint32_t oid, const TCHAR *name);
   SNMP_MIBObject(uint32_t oid, const TCHAR *name, int type,
                  int status, int access, const TCHAR *description,
						const TCHAR *textualConvention, const TCHAR *index);
   ~SNMP_MIBObject();

   void setParent(SNMP_MIBObject *pObject) { m_pParent = pObject; }
   void addChild(SNMP_MIBObject *pObject);
   void setInfo(int type, int status, int access, const TCHAR *description, const TCHAR *textualConvention, const TCHAR *index);
   void setName(const TCHAR *name) { MemFree(m_pszName); m_pszName = MemCopyString(name); }

   SNMP_MIBObject *getParent() { return m_pParent; }
   SNMP_MIBObject *getNext() { return m_pNext; }
   SNMP_MIBObject *getPrev() { return m_pPrev; }
   SNMP_MIBObject *getFirstChild() { return m_pFirst; }
   SNMP_MIBObject *getLastChild() { return m_pLast; }

   uint32_t getObjectId() const { return m_dwOID; }
   const TCHAR *getName() const { return m_pszName; }
   const TCHAR *getDescription() const { return m_pszDescription; }
   const TCHAR *getIndex() const { return m_index; }
   int getType() const { return m_iType; }
   int getStatus() const { return m_iStatus; }
   int getAccess() const { return m_iAccess; }

   SNMP_MIBObject *findChildByID(uint32_t oid);

   void print(int nIndent) const;

   // File I/O, supposed to be callsed only from libnxsnmp functions
   void writeToFile(ZFile *file, uint32_t flags);
   bool readFromFile(ZFile *file);
};

/**
 * OID parsing functions
 */
TCHAR LIBNXSNMP_EXPORTABLE *SnmpConvertOIDToText(size_t length, const uint32_t *value, TCHAR *buffer, size_t bufferSize);
size_t LIBNXSNMP_EXPORTABLE SnmpParseOID(const TCHAR *text, uint32_t *buffer, size_t bufferSize);
bool LIBNXSNMP_EXPORTABLE SnmpIsCorrectOID(const TCHAR *oid);
size_t LIBNXSNMP_EXPORTABLE SnmpGetOIDLength(const TCHAR *oid);

#define SNMP_OID_INTERNAL_BUFFER_SIZE  32

/**
 * Object identifier (OID)
 */
class LIBNXSNMP_EXPORTABLE SNMP_ObjectId
{
private:
   size_t m_length;
   uint32_t *m_value;
   uint32_t m_internalBuffer[SNMP_OID_INTERNAL_BUFFER_SIZE];

public:
   /**
    * Create empty OID with length 0
    */
   SNMP_ObjectId()
   {
      m_length = 0;
      m_value = m_internalBuffer;
   }

   /**
    * Copy constructor
    */
   SNMP_ObjectId(const SNMP_ObjectId& src)
   {
      m_length = src.m_length;
      if (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE)
      {
         m_value = m_internalBuffer;
         memcpy(m_value, src.m_value, m_length * sizeof(uint32_t));
      }
      else
      {
         m_value = MemCopyArray(src.m_value, m_length);
      }
   }

   /**
    * Move constructor
    */
   SNMP_ObjectId(SNMP_ObjectId&& src)
   {
      m_length = src.m_length;
      if (src.m_value != src.m_internalBuffer)
      {
         m_value = src.m_value;
         src.m_value = src.m_internalBuffer;
      }
      else
      {
         m_value = m_internalBuffer;
         memcpy(m_value, src.m_value, m_length * sizeof(uint32_t));
      }
      src.m_length = 0;
   }

   SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t suffix);
   SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t *suffix, size_t length);

   /**
    * Create OID from existing binary value
    */
   SNMP_ObjectId(const uint32_t *value, size_t length)
   {
      m_length = length;
      if (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE)
      {
         m_value = m_internalBuffer;
         memcpy(m_value, value, length * sizeof(uint32_t));
      }
      else
      {
         m_value = MemCopyArray(value, length);
      }
   }

   /**
    * Create OID from existing binary value
    */
   SNMP_ObjectId(std::initializer_list<uint32_t> value)
   {
      m_length = value.size();
      if (m_length > 0)
      {
         m_value = (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE) ? m_internalBuffer : MemAllocArrayNoInit<uint32_t>(m_length);
#if __cpp_lib_nonmember_container_access
         memcpy(m_value, std::data(value), m_length * sizeof(uint32_t));
#else
         m_value = MemAllocArrayNoInit<uint32_t>(m_length);
         uint32_t *p = m_value;
         for(uint32_t n : value)
            *p++ = n;
#endif
      }
      else
      {
         m_value = m_internalBuffer;
      }
   }

   ~SNMP_ObjectId()
   {
      if (m_value != m_internalBuffer)
         MemFree(m_value);
   }

   SNMP_ObjectId& operator =(const SNMP_ObjectId& src);
   SNMP_ObjectId& operator =(SNMP_ObjectId&& src);

   size_t length() const { return m_length; }
   const uint32_t *value() const { return m_value; }
   String toString() const
   {
      TCHAR buffer[MAX_OID_LEN * 5];
      SnmpConvertOIDToText(m_length, m_value, buffer, MAX_OID_LEN * 5);
      return String(buffer);
   }
   TCHAR *toString(TCHAR *buffer, size_t bufferSize) const
   {
      SnmpConvertOIDToText(m_length, m_value, buffer, bufferSize);
      return buffer;
   }
   bool isValid() const { return (m_length > 0) && (m_value != nullptr); }
   bool isZeroDotZero() const { return (m_length == 2) && (m_value[0] == 0) && (m_value[1] == 0); }
   uint32_t getElement(size_t index) const { return (index < m_length) ? m_value[index] : 0; }
   uint32_t getLastElement() const { return (m_length > 0) ? m_value[m_length - 1] : 0; }

   int compare(const TCHAR *oid) const;
   int compare(const uint32_t *oid, size_t length) const;
   int compare(std::initializer_list<uint32_t> oid) const
   {
#if __cpp_lib_nonmember_container_access
      return compare(std::data(oid), oid.size());
#else
      return compare(SNMP_ObjectId(oid));
#endif
   }
   int compare(const SNMP_ObjectId& oid) const
   {
      return compare(oid.value(), oid.length());
   }

   bool equals(const TCHAR *oid) const
   {
      return compare(oid) == OID_EQUAL;
   }
   bool equals(const uint32_t *oid, size_t length) const
   {
      if (m_length != length)
         return false;
      return compare(oid, length) == OID_EQUAL;
   }
   bool equals(std::initializer_list<uint32_t> oid) const
   {
      if (m_length != oid.size())
         return false;
      return compare(oid) == OID_EQUAL;
   }
   bool equals(const SNMP_ObjectId& oid) const
   {
      if (m_length != oid.m_length)
         return false;
      return compare(oid) == OID_EQUAL;
   }

   bool startsWith(const TCHAR *oid) const
   {
      int rc = compare(oid);
      return (rc == OID_LONGER) || (rc == OID_EQUAL);
   }
   bool startsWith(const uint32_t *oid, size_t length) const
   {
      if (m_length < length)
         return false;
      int rc = compare(oid, length);
      return (rc == OID_LONGER) || (rc == OID_EQUAL);
   }
   bool startsWith(std::initializer_list<uint32_t> oid) const
   {
      if (m_length < oid.size())
         return false;
      int rc = compare(oid);
      return (rc == OID_LONGER) || (rc == OID_EQUAL);
   }
   bool startsWith(const SNMP_ObjectId& oid) const
   {
      if (m_length < oid.m_length)
         return false;
      int rc = compare(oid);
      return (rc == OID_LONGER) || (rc == OID_EQUAL);
   }

   bool follows(const TCHAR *oid) const
   {
      int rc = compare(oid);
      return (rc == OID_LONGER) || (rc == OID_FOLLOWING);
   }
   bool follows(const uint32_t *oid, size_t length) const
   {
      int rc = compare(oid, length);
      return (rc == OID_LONGER) || (rc == OID_FOLLOWING);
   }
   bool follows(std::initializer_list<uint32_t> oid) const
   {
      int rc = compare(oid);
      return (rc == OID_LONGER) || (rc == OID_FOLLOWING);
   }
   bool follows(const SNMP_ObjectId& oid) const
   {
      int rc = compare(oid);
      return (rc == OID_LONGER) || (rc == OID_FOLLOWING);
   }

   void setValue(const uint32_t *value, size_t length);
   void extend(uint32_t subId);
   void extend(const uint32_t *subId, size_t length);
   void truncate(size_t count);
   void changeElement(size_t pos, uint32_t value)
   {
      if (pos < m_length)
         m_value[pos] = value;
   }

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
   SNMP_Variable(const TCHAR *name, uint32_t type = ASN_NULL);
   SNMP_Variable(const uint32_t *name, size_t nameLen, uint32_t type = ASN_NULL);
   SNMP_Variable(const SNMP_ObjectId &name, uint32_t type = ASN_NULL);
   SNMP_Variable(std::initializer_list<uint32_t> name, uint32_t type = ASN_NULL);
   SNMP_Variable(const SNMP_Variable& src);
   SNMP_Variable(SNMP_Variable&& src);
   ~SNMP_Variable();

   SNMP_Variable& operator=(const SNMP_Variable& src);
   SNMP_Variable& operator=(SNMP_Variable&& src);

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
	SNMP_Engine()
	{
	   m_idLen = 0;
	   m_engineBoots = 0;
	   m_engineTime = 0;
	   m_engineTimeDiff = 0;
	}
	SNMP_Engine(const BYTE *id, size_t idLen, int engineBoots = 0, int engineTime = 0)
	{
	   m_idLen = std::min(idLen, SNMP_MAX_ENGINEID_LEN);
	   memcpy(m_id, id, m_idLen);
	   m_engineBoots = engineBoots;
	   m_engineTime = engineTime;
	   m_engineTimeDiff = time(nullptr) - engineTime;
	}

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

	String toString() const
	{
	   TCHAR buffer[SNMP_MAX_ENGINEID_LEN * 2 + 1];
	   BinToStr(m_id, m_idLen, buffer);
	   return String(buffer);
	}
};

/**
 * Security context
 */
class LIBNXSNMP_EXPORTABLE SNMP_SecurityContext
{
private:
   SNMP_SecurityModel m_securityModel;
   char *m_community;
	char *m_userName;
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
	const char *getCommunity() const { return CHECK_NULL_EX_A(m_community); }
	const char *getUserName() const { return CHECK_NULL_EX_A(m_userName); }
	const char *getAuthName() const { return (m_securityModel == SNMP_SECURITY_MODEL_USM) ? getUserName() : getCommunity(); }
	const char *getAuthPassword() const { return CHECK_NULL_EX_A(m_authPassword); }
	const char *getPrivPassword() const { return CHECK_NULL_EX_A(m_privPassword); }
	const char *getContextName() const { return m_contextName; }

	bool needAuthentication() const { return (m_authMethod != SNMP_AUTH_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	bool needEncryption() const { return (m_privMethod != SNMP_ENCRYPT_NONE) && (m_authoritativeEngine.getIdLen() != 0); }
	SNMP_AuthMethod getAuthMethod() const { return m_authMethod; }
	SNMP_EncryptionMethod getPrivMethod() const { return m_privMethod; }
	const BYTE *getAuthKey()
	{
	   if (!m_validKeys)
	      recalculateKeys();
	   return m_authKey;
	}
	const BYTE *getPrivKey()
	{
	   if (!m_validKeys)
	      recalculateKeys();
	   return m_privKey;
	}
   void recalculateKeys();

	json_t *toJson() const;

   void setCommunity(const char *community)
   {
      MemFree(m_community);
      m_community = MemCopyStringA(CHECK_NULL_EX_A(community));
   }
   void setUserName(const char *userName)
   {
      MemFree(m_userName);
      m_userName = MemCopyStringA(CHECK_NULL_EX_A(userName));
   }
   void setAuthPassword(const char *password);
   void setPrivPassword(const char *password);
   void setAuthMethod(SNMP_AuthMethod method);
   void setPrivMethod(SNMP_EncryptionMethod method);
   void setSecurityModel(SNMP_SecurityModel model);

   void setContextName(const char *name)
   {
      MemFree(m_contextName);
      m_contextName = MemCopyStringA(name);
   }

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
 * Buffer for PDU encoding
 */
typedef Buffer<BYTE, 4096> SNMP_PDUBuffer;

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

   bool parseVariable(const BYTE *data, size_t varLength);
   bool parseVarBinds(const BYTE *pduData, size_t pduLength);
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
   size_t encode(SNMP_PDUBuffer *outBuffer, SNMP_SecurityContext *securityContext);

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
	SNMP_Transport()
	{
	   m_authoritativeEngine = nullptr;
	   m_contextEngine = nullptr;
	   m_securityContext = nullptr;
	   m_enableEngineIdAutoupdate = false;
	   m_updatePeerOnRecv = false;
	   m_reliable = false;
	   m_snmpVersion = SNMP_VERSION_2C;
	}
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
   uint16_t m_port;
	bool m_connected;
   size_t m_bytesInBuffer;
   BYTE *m_buffer;
   BYTE m_localBuffer[2048];

   size_t preParsePDU();
   int recvData(uint32_t timeout, struct sockaddr *sender, socklen_t *addrSize);
   void clearBuffer()
   {
      m_bytesInBuffer = 0;
   }

public:
   SNMP_UDPTransport() : SNMP_Transport()
   {
      m_port = SNMP_DEFAULT_PORT;
      m_hSocket = INVALID_SOCKET;
      m_bytesInBuffer = 0;
      m_buffer = m_localBuffer;
      m_connected = false;
   }
   SNMP_UDPTransport(SOCKET hSocket) : SNMP_Transport()
   {
      m_port = SNMP_DEFAULT_PORT;
      m_hSocket = hSocket;
      m_bytesInBuffer = 0;
      m_buffer = m_localBuffer;
      m_connected = false;
   }
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

   void expandBuffer()
   {
      if (m_buffer == m_localBuffer)
         m_buffer = static_cast<BYTE*>(MemAlloc(SNMP_DEFAULT_MSG_MAX_SIZE));
   }
};

struct SNMP_SnapshotIndexEntry;

/**
 * SNMP snapshot
 */
class LIBNXSNMP_EXPORTABLE SNMP_Snapshot
{
private:
   ObjectArray<SNMP_Variable> m_values;
   SNMP_SnapshotIndexEntry *m_index;
   SNMP_SnapshotIndexEntry *m_indexData;
   MemoryPool m_pool;

   void buildIndex();
   SNMP_SnapshotIndexEntry *find(const uint32_t *oid, size_t oidLen) const;
   SNMP_SnapshotIndexEntry *find(const TCHAR *oid) const;

public:
   SNMP_Snapshot();
   ~SNMP_Snapshot()
   {
      MemFree(m_indexData);
   }

   static SNMP_Snapshot *create(SNMP_Transport *transport, const TCHAR *baseOid, size_t limit = 0);
   static SNMP_Snapshot *create(SNMP_Transport *transport, const uint32_t *baseOid, size_t oidLen, size_t limit = 0);
   static inline SNMP_Snapshot *create(SNMP_Transport *transport, const SNMP_ObjectId& baseOid, size_t limit = 0)
   {
      return create(transport, baseOid.value(), baseOid.length(), limit);
   }
   static inline SNMP_Snapshot *create(SNMP_Transport *transport, std::initializer_list<uint32_t> baseOid, size_t limit = 0)
   {
#if __cpp_lib_nonmember_container_access
      return create(transport, std::data(baseOid), baseOid.size(), limit);
#else
      return create(transport, SNMP_ObjectId(baseOid), limit);
#endif
   }

   Iterator<SNMP_Variable> begin() { return m_values.begin(); }

   EnumerationCallbackResult walk(const TCHAR *baseOid, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *context) const;
   EnumerationCallbackResult walk(const uint32_t *baseOid, size_t baseOidLen, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *context) const;
   EnumerationCallbackResult walk(const SNMP_ObjectId& baseOid, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *context) const
   {
      return walk(baseOid.value(), baseOid.length(), handler, context);
   }
   EnumerationCallbackResult walk(std::initializer_list<uint32_t> baseOid, EnumerationCallbackResult (*handler)(const SNMP_Variable *, const SNMP_Snapshot *, void *), void *context) const
   {
#if __cpp_lib_nonmember_container_access
      return walk(std::data(baseOid), baseOid.size(), handler, context);
#else
      return walk(SNMP_ObjectId(baseOid), handler, context);
#endif
   }

   EnumerationCallbackResult walk(const TCHAR *baseOid, std::function<EnumerationCallbackResult(const SNMP_Variable*)> callback) const;
   EnumerationCallbackResult walk(const uint32_t *baseOid, size_t baseOidLen, std::function<EnumerationCallbackResult(const SNMP_Variable*)> callback) const;
   EnumerationCallbackResult walk(const SNMP_ObjectId& baseOid, std::function<EnumerationCallbackResult(const SNMP_Variable*)> callback) const
   {
      return walk(baseOid.value(), baseOid.length(), callback);
   }
   EnumerationCallbackResult walk(std::initializer_list<uint32_t> baseOid, std::function<EnumerationCallbackResult(const SNMP_Variable*)> callback) const
   {
#if __cpp_lib_nonmember_container_access
      return walk(std::data(baseOid), baseOid.size(), callback);
#else
      return walk(SNMP_ObjectId(baseOid), callback);
#endif
   }

   const SNMP_Variable *get(const TCHAR *oid) const;
   const SNMP_Variable *get(const uint32_t *oid, size_t oidLen) const;
   const SNMP_Variable *get(const SNMP_ObjectId& oid) const
   {
      return get(oid.value(), oid.length());
   }
   const SNMP_Variable *get(std::initializer_list<uint32_t> oid) const
   {
#if __cpp_lib_nonmember_container_access
      return get(std::data(oid), oid.size());
#else
      return get(SNMP_ObjectId(oid));
#endif
   }

   const SNMP_Variable *getNext(const TCHAR *oid) const;
   const SNMP_Variable *getNext(const uint32_t *oid, size_t oidLen) const;
   const SNMP_Variable *getNext(const SNMP_ObjectId& oid) const
   {
      return getNext(oid.value(), oid.length());
   }
   const SNMP_Variable *getNext(std::initializer_list<uint32_t> oid) const
   {
#if __cpp_lib_nonmember_container_access
      return getNext(std::data(oid), oid.size());
#else
      return getNext(SNMP_ObjectId(oid));
#endif
   }

   const SNMP_Variable *first() const { return m_values.get(0); }
   const SNMP_Variable *last() const { return m_values.get(m_values.size() - 1); }

   int32_t getAsInt32(const TCHAR *oid) const
   {
      const SNMP_Variable *v = get(oid);
      return (v != nullptr) ? v->getValueAsInt() : 0;
   }
   int32_t getAsInt32(const uint32_t *oid, size_t oidLen) const
   {
      const SNMP_Variable *v = get(oid, oidLen);
      return (v != nullptr) ? v->getValueAsInt() : 0;
   }
   int32_t getAsInt32(const SNMP_ObjectId& oid) const
   {
      const SNMP_Variable *v = get(oid);
      return (v != nullptr) ? v->getValueAsInt() : 0;
   }
   int32_t getAsInt32(std::initializer_list<uint32_t> oid) const
   {
#if __cpp_lib_nonmember_container_access
      return getAsInt32(std::data(oid), oid.size());
#else
      return getAsInt32(SNMP_ObjectId(oid));
#endif
   }
   uint32_t getAsUInt32(const TCHAR *oid) const
   {
      const SNMP_Variable *v = get(oid);
      return (v != nullptr) ? v->getValueAsUInt() : 0;
   }
   uint32_t getAsUInt32(const uint32_t *oid, size_t oidLen) const
   {
      const SNMP_Variable *v = get(oid, oidLen);
      return (v != nullptr) ? v->getValueAsUInt() : 0;
   }
   uint32_t getAsUInt32(const SNMP_ObjectId& oid) const
   {
      const SNMP_Variable *v = get(oid);
      return (v != nullptr) ? v->getValueAsUInt() : 0;
   }
   uint32_t getAsUInt32(std::initializer_list<uint32_t> oid) const
   {
#if __cpp_lib_nonmember_container_access
      return getAsUInt32(std::data(oid), oid.size());
#else
      return getAsUInt32(SNMP_ObjectId(oid));
#endif
   }
   TCHAR *getAsString(const TCHAR *oid, TCHAR *buffer, size_t bufferSize) const
   {
      const SNMP_Variable *v = get(oid);
      if (v != nullptr)
      {
         v->getValueAsString(buffer, bufferSize);
         return buffer;
      }
      return nullptr;
   }
   TCHAR *getAsString(const uint32_t *oid, size_t oidLen, TCHAR *buffer, size_t bufferSize) const
   {
      const SNMP_Variable *v = get(oid, oidLen);
      if (v != nullptr)
      {
         v->getValueAsString(buffer, bufferSize);
         return buffer;
      }
      return nullptr;
   }
   TCHAR *getAsString(const SNMP_ObjectId& oid, TCHAR *buffer, size_t bufferSize) const
   {
      return getAsString(oid.value(), oid.length(), buffer, bufferSize);
   }
   TCHAR *getAsString(std::initializer_list<uint32_t> oid, TCHAR *buffer, size_t bufferSize) const
   {
#if __cpp_lib_nonmember_container_access
      return getAsString(std::data(oid), oid.size(), buffer, bufferSize);
#else
      return getAsString(SNMP_ObjectId(oid), buffer, bufferSize);
#endif
   }

   int size() const { return m_values.size(); }
   bool isEmpty() const { return m_values.isEmpty(); }
};

/**
 * Functions
 */
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
uint32_t LIBNXSNMP_EXPORTABLE SnmpGet(SNMP_Version version, SNMP_Transport *transport, const SNMP_ObjectId& oid, void *value, size_t bufferSize, uint32_t flags);
uint32_t LIBNXSNMP_EXPORTABLE SnmpGet(SNMP_Version version, SNMP_Transport *transport, const TCHAR *oidStr, const uint32_t *oidBinary, size_t oidLen, void *value, size_t bufferSize, uint32_t flags);
uint32_t LIBNXSNMP_EXPORTABLE SnmpGetEx(SNMP_Transport *transport, const TCHAR *oidStr, const uint32_t *oidBinary, size_t oidLen,
      void *value, size_t bufferSize, uint32_t flags, uint32_t *dataLen = nullptr, const char *codepage = nullptr);
bool LIBNXSNMP_EXPORTABLE CheckSNMPIntegerValue(SNMP_Transport *snmpTransport, const TCHAR *oid, int32_t value);
bool LIBNXSNMP_EXPORTABLE CheckSNMPIntegerValue(SNMP_Transport *snmpTransport, std::initializer_list<uint32_t> oid, int32_t value);
uint32_t LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors = false, bool failOnShutdown = false);
uint32_t LIBNXSNMP_EXPORTABLE SnmpWalk(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors = false, bool failOnShutdown = false);
int LIBNXSNMP_EXPORTABLE SnmpWalkCount(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen);
int LIBNXSNMP_EXPORTABLE SnmpWalkCount(SNMP_Transport *transport, const TCHAR *rootOid);

uint32_t LIBNXSNMP_EXPORTABLE SnmpScanAddressRange(const InetAddress& from, const InetAddress& to, uint16_t port, SNMP_Version snmpVersion,
      const char *community, void (*callback)(const InetAddress&, uint32_t, void*), void *context);

/**
 * Get value for SNMP variable
 * Note: buffer size is in bytes
 */
static inline uint32_t SnmpGet(SNMP_Version version, SNMP_Transport *transport, std::initializer_list<uint32_t> oid, void *value, size_t bufferSize, uint32_t flags)
{
#if __cpp_lib_nonmember_container_access
   return SnmpGet(version, transport, nullptr, std::data(oid), oid.size(), value, bufferSize, flags);
#else
   return SnmpGet(version, transport, SNMP_ObjectId(oid), value, bufferSize, flags);
#endif
}

/**
 * Get value for SNMP variable
 * If SG_RAW_RESULT flag given and dataLen is not NULL actual data length will be stored there
 * Note: buffer size is in bytes
 */
static inline uint32_t SnmpGetEx(SNMP_Transport *transport, std::initializer_list<uint32_t> oid,
      void *value, size_t bufferSize, uint32_t flags, uint32_t *dataLen = nullptr, const char *codepage = nullptr)
{
#if __cpp_lib_nonmember_container_access
   return SnmpGetEx(transport, nullptr, std::data(oid), oid.size(), value, bufferSize, flags, dataLen, codepage);
#else
   SNMP_ObjectId oidObject(oid);
   return SnmpGetEx(transport, nullptr, oidObject.value(), oidObject.length(), value, bufferSize, flags, dataLen, codepage);
#endif
}

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
 * Enumerate multiple values by walking through MIB, starting at given root
 */
static inline uint32_t SnmpWalk(SNMP_Transport *transport, const SNMP_ObjectId& rootOid,
         uint32_t (*handler)(SNMP_Variable*, SNMP_Transport*, void*), void *context, bool logErrors = false, bool failOnShutdown = false)
{
   return SnmpWalk(transport, rootOid.value(), rootOid.length(), [transport, handler, context](SNMP_Variable *v) -> uint32_t { return handler(v, transport, context); }, logErrors, failOnShutdown);
}

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
static inline uint32_t SnmpWalk(SNMP_Transport *transport, const SNMP_ObjectId& rootOid, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors = false, bool failOnShutdown = false)
{
   return SnmpWalk(transport, rootOid.value(), rootOid.length(), handler, logErrors, failOnShutdown);
}

/**
 * Enumerate multiple values by walking through MIB, starting at given root
 */
static inline uint32_t SnmpWalk(SNMP_Transport *transport, std::initializer_list<uint32_t> rootOid, std::function<uint32_t (SNMP_Variable*)> handler, bool logErrors = false, bool failOnShutdown = false)
{
#if __cpp_lib_nonmember_container_access
   return SnmpWalk(transport, std::data(rootOid), rootOid.size(), handler, logErrors, failOnShutdown);
#else
   return SnmpWalk(transport, SNMP_ObjectId(rootOid), handler, logErrors, failOnShutdown);
#endif
}

/**
 * Wrapper function for calling SnmpWalk with specific context type
 */
template <typename C> static inline uint32_t SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, uint32_t (*callback)(SNMP_Variable*, SNMP_Transport*, C*), C *context, bool logErrors = false, bool failOnShutdown = false)
{
   return SnmpWalk(transport, rootOid, reinterpret_cast<uint32_t (*)(SNMP_Variable*, SNMP_Transport*, void*)>(callback), context, logErrors, failOnShutdown);
}

/**
 * Wrapper function for calling SnmpWalk with specific context type
 */
template <typename C> static inline uint32_t SnmpWalk(SNMP_Transport *transport, std::initializer_list<uint32_t> rootOid, uint32_t (*callback)(SNMP_Variable*, SNMP_Transport*, C*), C *context, bool logErrors = false, bool failOnShutdown = false)
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
template <typename T> static inline uint32_t SnmpWalk(SNMP_Transport *transport, const TCHAR *rootOid, T *object, uint32_t (T::*func)(SNMP_Variable*, SNMP_Transport*))
{
   __SnmpWalk_WrapperData<T> data(object, func);
   return SnmpWalk(transport, rootOid, __SnmpWalk_Wrapper<T>, &data);
}

/**
 * Wrapper function for calling SnmpWalk on object
 */
template <typename T> static inline uint32_t SnmpWalk(SNMP_Transport *transport, const uint32_t *rootOid, size_t rootOidLen, T *object, uint32_t (T::*func)(SNMP_Variable*, SNMP_Transport*))
{
   __SnmpWalk_WrapperData<T> data(object, func);
   return SnmpWalk(transport, rootOid, rootOidLen, __SnmpWalk_Wrapper<T>, &data);
}

/**
 * Wrapper function for calling SnmpWalk on object
 */
template <typename T> static inline uint32_t SnmpWalk(SNMP_Transport *transport, const SNMP_ObjectId& rootOid, T *object, uint32_t (T::*func)(SNMP_Variable*, SNMP_Transport*))
{
   __SnmpWalk_WrapperData<T> data(object, func);
   return SnmpWalk(transport, rootOid, __SnmpWalk_Wrapper<T>, &data);
}

/**
 * Wrapper function for calling SnmpWalk on object
 */
template <typename T> static inline uint32_t SnmpWalk(SNMP_Transport *transport, std::initializer_list<uint32_t> rootOid, T *object, uint32_t (T::*func)(SNMP_Variable*, SNMP_Transport*))
{
   __SnmpWalk_WrapperData<T> data(object, func);
#if __cpp_lib_nonmember_container_access
   return SnmpWalk(transport, std::data(rootOid), rootOid.size(), __SnmpWalk_Wrapper<T>, &data);
#else
   return SnmpWalk(transport, SNMP_ObjectId(rootOid), __SnmpWalk_Wrapper<T>, &data);
#endif
}

/**
 * Wrapper for SnmpWalkCount for using initializer list
 */
static inline int SnmpWalkCount(SNMP_Transport *transport, const SNMP_ObjectId& rootOid)
{
   return SnmpWalkCount(transport, rootOid.value(), rootOid.length());
}

/**
 * Wrapper for SnmpWalkCount for using initializer list
 */
static inline int SnmpWalkCount(SNMP_Transport *transport, std::initializer_list<uint32_t> rootOid)
{
#if __cpp_lib_nonmember_container_access
   return SnmpWalkCount(transport, std::data(rootOid), rootOid.size());
#else
   return SnmpWalkCount(transport, SNMP_ObjectId(rootOid));
#endif
}

#endif   /* __cplusplus */

/** @} */

#endif
