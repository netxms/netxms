/*
** NetXMS - Network Management System
** Copyright (C) 2021-2026 Raden Solutions
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
** File: sshkeys.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG L"ssh.keys"

#define MAX_SSH_KEY_NAME 255

static unsigned char s_sshHeader[11] = { 0x00, 0x00, 0x00, 0x07, 0x73, 0x73, 0x68, 0x2D, 0x72, 0x73, 0x61 };

/**
 * SSH key data structure
 */
class SshKeyPair
{
private:
   uint32_t m_id;
   wchar_t m_name[MAX_SSH_KEY_NAME];
   char *m_publicKey;
   char *m_privateKey;

public:
   SshKeyPair();
   SshKeyPair(DB_RESULT result, int row);
   SshKeyPair(uint32_t id, const wchar_t *name, char *publicKey, char *privateKey);
   ~SshKeyPair();

   void saveToDatabase();
   void deleteFromDatabase();

   uint32_t getId() const { return m_id; }
   const wchar_t *getName() const { return m_name; }
   const char *getPublicKey() const { return m_publicKey; }
   const char *getPrivateKey() const { return m_privateKey; }

   json_t *toJson(bool includePublicKey) const;

   static shared_ptr<SshKeyPair> generate(const wchar_t *name);
};

static SynchronizedSharedHashMap<uint32_t, SshKeyPair> s_sshKeys;

/**
 * Default constructor
 */
SshKeyPair::SshKeyPair()
{
   m_id = CreateUniqueId(IDG_SSH_KEY);
   m_name[0] = 0;
   m_publicKey = nullptr;
   m_privateKey = nullptr;
}

/**
 * Constructor form DB result
 */
SshKeyPair::SshKeyPair(DB_RESULT result, int row)
{
   m_id = DBGetFieldULong(result, row, 0);
   DBGetField(result, row, 1, m_name, MAX_SSH_KEY_NAME);
   m_publicKey = DBGetFieldUTF8(result, row, 2, nullptr, 0);
   m_privateKey = DBGetFieldUTF8(result, row, 3, nullptr, 0);
}

/**
 * Constructor from key data. Takes ownership of public and private key strings.
 */
SshKeyPair::SshKeyPair(uint32_t id, const wchar_t *name, char *publicKey, char *privateKey)
{
   if (id != 0)
      m_id = id;
   else
      m_id = CreateUniqueId(IDG_SSH_KEY);
   wcslcpy(m_name, name, MAX_SSH_KEY_NAME);
   m_publicKey = publicKey;
   m_privateKey = privateKey;
}

/**
 * SSH key data destructor
 */
SshKeyPair::~SshKeyPair()
{
   MemFree(m_publicKey);
   MemFree(m_privateKey);
}

/**
 * Convert SSH key data to JSON object
 */
json_t *SshKeyPair::toJson(bool includePublicKey) const
{
   json_t *object = json_object();
   json_object_set_new(object, "id", json_integer(m_id));
   json_object_set_new(object, "name", json_string_w(m_name));
   if (includePublicKey)
      json_object_set_new(object, "publicKey", json_string(m_publicKey));
   return object;
}

/**
 * Encode public key parts
 */
static int SshEncodeBuffer(unsigned char *encoding, int bufferLen, unsigned char* buffer)
{
   int adjustedLen = bufferLen, index;
   if (*buffer & 0x80)
   {
      adjustedLen++;
      encoding[4] = 0;
      index = 5;
   }
   else
   {
      index = 4;
   }
   encoding[0] = (unsigned char) (adjustedLen >> 24);
   encoding[1] = (unsigned char) (adjustedLen >> 16);
   encoding[2] = (unsigned char) (adjustedLen >>  8);
   encoding[3] = (unsigned char) (adjustedLen);
   memcpy(&encoding[index], buffer, bufferLen);
   return index + bufferLen;
}

/**
 * Generate SSH key
 */
shared_ptr<SshKeyPair> SshKeyPair::generate(const wchar_t *name)
{
   shared_ptr<SshKeyPair> data = make_shared<SshKeyPair>();
   _tcslcpy(data->m_name, name, MAX_SSH_KEY_NAME);
   RSA_KEY key = RSAGenerateKey(4056);
   if (key == nullptr)
   {
      nxlog_write(NXLOG_ERROR, _T("SshKeyData::generateNewKeys: failed to generate RSA key"));
      return shared_ptr<SshKeyPair>();
   }

   // Format private key to PKCS#1 and encode to base64
   StringBuffer privateKey;
   privateKey.append(_T("-----BEGIN RSA PRIVATE KEY-----\n"));
   char buf[4096];
   BYTE *pBufPos = reinterpret_cast<BYTE*>(buf);
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   uint32_t privateLen = i2d_PrivateKey(key, &pBufPos);
#else
   uint32_t privateLen = i2d_RSAPrivateKey(key, &pBufPos);
#endif
   base64_encode_alloc(buf, privateLen, &data->m_privateKey);
   privateKey.appendUtf8String(data->m_privateKey);
   privateKey.append(_T("\n-----END RSA PRIVATE KEY-----"));
   MemFree(data->m_privateKey);
   data->m_privateKey = privateKey.getUTF8String();

   // Format public key
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   BIGNUM *n = nullptr, *e = nullptr;
   EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_RSA_N, &n);
   EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_RSA_E, &e);
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
   const BIGNUM *n, *e, *d;
   RSA_get0_key(key, &n, &e, &d);
#else
   const BIGNUM *n, *e;
   n = key->n;
   e = key->e;
#endif
   if ((n == nullptr) || (e == nullptr))
   {
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
      // One can be allocated
      BN_free(n);
      BN_free(e);
#endif
      nxlog_write(NXLOG_ERROR, _T("SshKeyData::generateNewKeys: failed to generate RSA key components"));
      RSAFree(key);
      return shared_ptr<SshKeyPair>();
   }

   int nLen = BN_num_bytes(n);
   auto nBytes = MemAllocArrayNoInit<BYTE>(nLen);
   BN_bn2bin(n, nBytes);

   // reading the public exponent
   int eLen = BN_num_bytes(e);
   auto eBytes = MemAllocArrayNoInit<BYTE>(eLen);
   BN_bn2bin(e, eBytes);

   int binaryDataLength = 11 + 4 + eLen + 4 + nLen;
   // correct depending on the MSB of e and N
   if (eBytes[0] & 0x80)
      binaryDataLength++;
   if (nBytes[0] & 0x80)
      binaryDataLength++;

   auto binaryData = MemAllocArrayNoInit<BYTE>(binaryDataLength);
   memcpy(binaryData, s_sshHeader, 11);

   int index = SshEncodeBuffer(&binaryData[11], eLen, eBytes);
   SshEncodeBuffer(&binaryData[11 + index], nLen, nBytes);

   std::string publicKey("ssh-rsa ");
   char *out;
   size_t len = base64_encode_alloc(reinterpret_cast<const char*>(binaryData), binaryDataLength, &out);
   publicKey.append(out, len);

   MemFree(nBytes);
   MemFree(eBytes);
   MemFree(binaryData);
   MemFree(out);

   publicKey.append(" ");
   char utf8name[MAX_SSH_KEY_NAME * 4];
   wchar_to_utf8(name, -1, utf8name, MAX_SSH_KEY_NAME * 4);
   publicKey.append(utf8name);
   data->m_publicKey = MemCopyStringA(publicKey.c_str());

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   BN_free(n);
   BN_free(e);
#endif

   RSAFree(key);
   return data;
}

/**
 * Save SSH key data to database
 */
void SshKeyPair::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   static const wchar_t *columns[] = { L"name", L"public_key", L"private_key", nullptr };
   DB_STATEMENT stmt =  DBPrepareMerge(hdb, L"ssh_keys", L"id", m_id, columns);
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_publicKey, DB_BIND_STATIC);
      DBBind(stmt, 3, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, m_privateKey, DB_BIND_STATIC);
      DBBind(stmt, 4, DB_SQLTYPE_INTEGER, m_id);
      DBExecute(stmt);
      DBFreeStatement(stmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete SSH key from database
 */
void SshKeyPair::deleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   ExecuteQueryOnObject(hdb, m_id, L"DELETE FROM ssh_keys WHERE id=?");
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Maximum size of private key text accepted for import (16384-bit RSA key is about 13 KB)
 */
#define MAX_PRIVATE_KEY_TEXT_SIZE 16384

/**
 * String field in SSH wire format data (RFC 4251 section 5)
 */
struct SshWireString
{
   const BYTE *data;
   size_t size;

   bool equals(const char *s) const { return (size == strlen(s)) && !memcmp(data, s, size); }
   bool equals(const SshWireString& s) const { return (size == s.size) && !memcmp(data, s.data, size); }
};

/**
 * Bounds-checked reader for SSH wire format data
 */
class SshWireReader
{
private:
   const BYTE *m_data;
   size_t m_size;
   size_t m_pos;

public:
   SshWireReader(const BYTE *data, size_t size) : m_data(data), m_size(size), m_pos(0) { }

   size_t remaining() const { return m_size - m_pos; }

   bool readBytes(size_t count, SshWireString *s)
   {
      if (count > remaining())
         return false;
      s->data = m_data + m_pos;
      s->size = count;
      m_pos += count;
      return true;
   }

   bool readUInt32(uint32_t *value)
   {
      SshWireString s;
      if (!readBytes(4, &s))
         return false;
      *value = (static_cast<uint32_t>(s.data[0]) << 24) | (static_cast<uint32_t>(s.data[1]) << 16) | (static_cast<uint32_t>(s.data[2]) << 8) | s.data[3];
      return true;
   }

   bool readString(SshWireString *s)
   {
      uint32_t len;
      return readUInt32(&len) && readBytes(len, s);
   }
};

/**
 * Supported key algorithms
 */
enum class SshKeyAlgorithm
{
   RSA,
   ED25519,
   ECDSA
};

/**
 * Key type accepted for import
 */
struct SshKeyType
{
   const char *name;
   SshKeyAlgorithm algorithm;
   const char *curve;
   size_t pointSize;   // uncompressed EC point: 0x04 prefix and two coordinates
};

/**
 * Key types accepted for import
 */
static const SshKeyType s_sshKeyTypes[] =
{
   { "ssh-rsa", SshKeyAlgorithm::RSA, nullptr, 0 },
   { "ssh-ed25519", SshKeyAlgorithm::ED25519, nullptr, 0 },
   { "ecdsa-sha2-nistp256", SshKeyAlgorithm::ECDSA, "nistp256", 65 },
   { "ecdsa-sha2-nistp384", SshKeyAlgorithm::ECDSA, "nistp384", 97 },
   { "ecdsa-sha2-nistp521", SshKeyAlgorithm::ECDSA, "nistp521", 133 }
};

/**
 * Public key fields from the envelope's public key blob
 */
struct SshPublicKeyFields
{
   const SshKeyType *type;
   SshWireString typeName;
   SshWireString rsaExponent;
   SshWireString rsaModulus;
   SshWireString ed25519Key;
   SshWireString ecdsaPoint;
};

/**
 * Log reason for private key rejection
 */
static bool RejectPrivateKey(const wchar_t *reason)
{
   nxlog_debug_tag(DEBUG_TAG, 4, L"Private key rejected: %s", reason);
   return false;
}

/**
 * Check that mpint is positive, non-empty and has no redundant leading zero byte
 */
static bool IsCanonicalPositiveMpint(const SshWireString& s)
{
   if ((s.size == 0) || (s.data[0] & 0x80))
      return false;
   return (s.data[0] != 0) || ((s.size > 1) && (s.data[1] & 0x80));
}

/**
 * Read mpint and check that it is canonical and positive
 */
static bool ReadPositiveMpint(SshWireReader *reader, SshWireString *s)
{
   return reader->readString(s) && IsCanonicalPositiveMpint(*s);
}

/**
 * Parse public key blob from key envelope
 */
static bool ParsePublicKeyBlob(const SshWireString& blob, SshPublicKeyFields *fields)
{
   SshWireReader reader(blob.data, blob.size);
   if (!reader.readString(&fields->typeName))
      return RejectPrivateKey(L"malformed public key blob");

   fields->type = nullptr;
   for(size_t i = 0; i < sizeof(s_sshKeyTypes) / sizeof(s_sshKeyTypes[0]); i++)
   {
      if (fields->typeName.equals(s_sshKeyTypes[i].name))
      {
         fields->type = &s_sshKeyTypes[i];
         break;
      }
   }
   if (fields->type == nullptr)
      return RejectPrivateKey(L"unsupported key type");

   SshWireString curve;
   switch(fields->type->algorithm)
   {
      case SshKeyAlgorithm::RSA:
         if (!ReadPositiveMpint(&reader, &fields->rsaExponent) || !ReadPositiveMpint(&reader, &fields->rsaModulus))
            return RejectPrivateKey(L"invalid RSA public key");
         break;
      case SshKeyAlgorithm::ED25519:
         if (!reader.readString(&fields->ed25519Key) || (fields->ed25519Key.size != 32))
            return RejectPrivateKey(L"invalid Ed25519 public key");
         break;
      case SshKeyAlgorithm::ECDSA:
         if (!reader.readString(&curve) || !curve.equals(fields->type->curve))
            return RejectPrivateKey(L"ECDSA curve does not match key type");
         if (!reader.readString(&fields->ecdsaPoint) || (fields->ecdsaPoint.size != fields->type->pointSize) || (fields->ecdsaPoint.data[0] != 0x04))
            return RejectPrivateKey(L"invalid ECDSA public point");
         break;
   }

   if (reader.remaining() != 0)
      return RejectPrivateKey(L"extra data after public key");
   return true;
}

/**
 * Parse private section and check it against public key fields
 */
static bool ParsePrivateSection(const SshWireString& section, const SshPublicKeyFields& publicKey)
{
   if (section.size % 8 != 0)
      return RejectPrivateKey(L"private section is not aligned to cipher block size");

   SshWireReader reader(section.data, section.size);
   uint32_t check1, check2;
   if (!reader.readUInt32(&check1) || !reader.readUInt32(&check2) || (check1 != check2))
      return RejectPrivateKey(L"check integers do not match");

   SshWireString typeName;
   if (!reader.readString(&typeName) || !typeName.equals(publicKey.typeName))
      return RejectPrivateKey(L"private key type does not match public key type");

   SshWireString n, e, d, iqmp, p, q, curve, point, scalar, ed25519Public, ed25519Private;
   switch(publicKey.type->algorithm)
   {
      case SshKeyAlgorithm::RSA:
         if (!ReadPositiveMpint(&reader, &n) || !ReadPositiveMpint(&reader, &e) || !ReadPositiveMpint(&reader, &d) ||
             !ReadPositiveMpint(&reader, &iqmp) || !ReadPositiveMpint(&reader, &p) || !ReadPositiveMpint(&reader, &q))
            return RejectPrivateKey(L"invalid RSA private key");
         if (!n.equals(publicKey.rsaModulus) || !e.equals(publicKey.rsaExponent))
            return RejectPrivateKey(L"RSA private key does not match public key");
         break;
      case SshKeyAlgorithm::ED25519:
         if (!reader.readString(&ed25519Public) || (ed25519Public.size != 32) || !reader.readString(&ed25519Private) || (ed25519Private.size != 64))
            return RejectPrivateKey(L"invalid Ed25519 private key");
         if (!ed25519Public.equals(publicKey.ed25519Key) || memcmp(ed25519Private.data + 32, ed25519Public.data, 32))
            return RejectPrivateKey(L"Ed25519 private key does not match public key");
         break;
      case SshKeyAlgorithm::ECDSA:
         if (!reader.readString(&curve) || !curve.equals(publicKey.type->curve))
            return RejectPrivateKey(L"ECDSA curve does not match key type");
         if (!reader.readString(&point) || !point.equals(publicKey.ecdsaPoint))
            return RejectPrivateKey(L"ECDSA private key does not match public key");
         if (!ReadPositiveMpint(&reader, &scalar))
            return RejectPrivateKey(L"invalid ECDSA private scalar");
         break;
   }

   SshWireString comment;
   if (!reader.readString(&comment))
      return RejectPrivateKey(L"missing key comment");

   // Padding to cipher block size is 1, 2, 3, ...
   SshWireString padding;
   reader.readBytes(reader.remaining(), &padding);
   for(size_t i = 0; i < padding.size; i++)
   {
      if (padding.data[i] != static_cast<BYTE>(i + 1))
         return RejectPrivateKey(L"invalid padding");
   }
   return true;
}

/**
 * Validate decoded OpenSSH key container and find public key blob in it
 */
static bool ParseOpenSshKeyContainer(const BYTE *data, size_t size, SshWireString *publicKeyBlob, SshPublicKeyFields *publicKey)
{
   static const char magic[] = "openssh-key-v1";   // terminating zero byte is part of the magic

   SshWireReader reader(data, size);
   SshWireString s;
   if (!reader.readBytes(sizeof(magic), &s) || memcmp(s.data, magic, sizeof(magic)))
      return RejectPrivateKey(L"not an OpenSSH private key");

   if (!reader.readString(&s) || !s.equals("none"))
      return RejectPrivateKey(L"key is encrypted");
   if (!reader.readString(&s) || !s.equals("none"))
      return RejectPrivateKey(L"key is encrypted");
   if (!reader.readString(&s) || (s.size != 0))
      return RejectPrivateKey(L"unexpected KDF options");

   uint32_t keyCount;
   if (!reader.readUInt32(&keyCount) || (keyCount != 1))
      return RejectPrivateKey(L"container must hold exactly one key");

   if (!reader.readString(publicKeyBlob))
      return RejectPrivateKey(L"missing public key blob");
   if (!ParsePublicKeyBlob(*publicKeyBlob, publicKey))
      return false;

   SshWireString privateSection;
   if (!reader.readString(&privateSection))
      return RejectPrivateKey(L"missing private section");
   if (!ParsePrivateSection(privateSection, *publicKey))
      return false;

   if (reader.remaining() != 0)
      return RejectPrivateKey(L"extra data after private section");
   return true;
}

/**
 * Check if character is whitespace allowed around armored key
 */
static inline bool IsKeyTextSpace(char ch)
{
   return (ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n');
}

/**
 * Validate unencrypted private key in OpenSSH format and derive public key line from it. On success, returns
 * normalized private key text (LF line endings, no surrounding whitespace) and public key line
 * "<type> <base64 blob> <name>" in UTF-8, both to be freed by caller with MemFree. Validation is structural only:
 * key that passes it is well-formed, but key material itself is not verified.
 */
bool NXCORE_EXPORTABLE ParseOpenSshPrivateKey(const char *text, const wchar_t *name, char **privateKey, char **publicKey)
{
   static const char beginLine[] = "-----BEGIN OPENSSH PRIVATE KEY-----";
   static const char endLine[] = "-----END OPENSSH PRIVATE KEY-----";

   size_t textLength = strlen(text);
   if (textLength > MAX_PRIVATE_KEY_TEXT_SIZE)
      return RejectPrivateKey(L"key text is too long");

   const char *start = text;
   const char *end = text + textLength;
   while((start < end) && IsKeyTextSpace(*start))
      start++;
   while((end > start) && IsKeyTextSpace(*(end - 1)))
      end--;

   char *normalized = MemAllocArrayNoInit<char>(end - start + 1);
   size_t normalizedLength = 0;
   for(const char *p = start; p < end; p++)
   {
      if ((*p != '\r') || (p + 1 == end) || (*(p + 1) != '\n'))
         normalized[normalizedLength++] = *p;
   }
   normalized[normalizedLength] = 0;

   // Armor lines and base64 body without line breaks
   char *body = MemAllocArrayNoInit<char>(normalizedLength + 1);
   size_t bodyLength = 0;
   bool valid = false;
   const char *firstLineEnd = strchr(normalized, '\n');
   const char *lastLineStart = strrchr(normalized, '\n');
   if ((firstLineEnd != nullptr) && (firstLineEnd != lastLineStart) &&
       (firstLineEnd - normalized == sizeof(beginLine) - 1) && !memcmp(normalized, beginLine, sizeof(beginLine) - 1) &&
       !strcmp(lastLineStart + 1, endLine))
   {
      valid = true;
      size_t lineLength = 0;
      for(const char *p = firstLineEnd + 1; p <= lastLineStart; p++)
      {
         if (*p == '\n')
         {
            if (lineLength == 0)
            {
               valid = false;
               break;
            }
            lineLength = 0;
         }
         else if (isbase64(*p) || (*p == '='))
         {
            body[bodyLength++] = *p;
            lineLength++;
         }
         else
         {
            valid = false;
            break;
         }
      }
   }

   if (!valid)
   {
      MemFree(body);
      MemFree(normalized);
      return RejectPrivateKey(L"invalid OpenSSH private key armor or body");
   }

   // Padding characters allowed only as last one or two characters of a properly sized body
   size_t padding = 0;
   while((padding < bodyLength) && (body[bodyLength - padding - 1] == '='))
      padding++;
   if ((bodyLength % 4 != 0) || (padding > 2) || (memchr(body, '=', bodyLength - padding) != nullptr))
   {
      MemFree(body);
      MemFree(normalized);
      return RejectPrivateKey(L"invalid base64 encoding");
   }

   char *decoded;
   size_t decodedLength;
   bool success = base64_decode_alloc(body, bodyLength, &decoded, &decodedLength);
   MemFree(body);
   if (!success || (decoded == nullptr))
   {
      MemFree(normalized);
      return RejectPrivateKey(L"invalid base64 encoding");
   }

   SshWireString publicKeyBlob;
   SshPublicKeyFields publicKeyFields;
   if (!ParseOpenSshKeyContainer(reinterpret_cast<BYTE*>(decoded), decodedLength, &publicKeyBlob, &publicKeyFields))
   {
      MemFree(decoded);
      MemFree(normalized);
      return false;
   }

   char *encodedBlob;
   size_t encodedBlobLength = base64_encode_alloc(reinterpret_cast<const char*>(publicKeyBlob.data), publicKeyBlob.size, &encodedBlob);
   char *utf8name = UTF8StringFromWideString(name);

   std::string line(publicKeyFields.type->name);
   line.append(" ");
   line.append(encodedBlob, encodedBlobLength);
   line.append(" ");
   line.append(utf8name);

   MemFree(utf8name);
   MemFree(encodedBlob);
   MemFree(decoded);

   *privateKey = normalized;
   *publicKey = MemCopyStringA(line.c_str());
   return true;
}

/**
 * Data structure for fill message with SSH data
 */
struct FillMessageData
{
   uint32_t baseId;
   NXCPMessage *msg;
   bool withPublicKey;
};

/**
 * Callback to fill message with SSH key data
 */
static EnumerationCallbackResult FillMessageSshKeys(const uint32_t &key, const shared_ptr<SshKeyPair> &value, FillMessageData *data)
{
   NXCPMessage *msg = data->msg;
   msg->setField(data->baseId++, value->getId());
   msg->setField(data->baseId++, value->getName());
   if (data->withPublicKey)
      msg->setFieldFromUtf8String(data->baseId++, value->getPublicKey());
   else
      data->baseId++;
   data->baseId+=2;

   return _CONTINUE;
}

/**
 * Get SSH list
 */
void FillMessageWithSshKeys(NXCPMessage *msg, bool withPublicKey)
{
   FillMessageData data;
   data.baseId = VID_SSH_KEY_LIST_BASE;
   data.msg = msg;
   data.withPublicKey = withPublicKey;
   s_sshKeys.forEach(FillMessageSshKeys, &data);
   msg->setField(VID_SSH_KEY_COUNT, (data.baseId - VID_SSH_KEY_LIST_BASE) / 5);
}

/**
 * Load SSH keys
 */
void LoadSshKeys()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelect(hdb, L"SELECT id,name,public_key,private_key FROM ssh_keys");
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for (int i = 0; i < count; i++)
      {
         shared_ptr<SshKeyPair> key = make_shared<SshKeyPair>(result, i);
         s_sshKeys.set(key->getId(), key);
      }
      DBFreeResult(result);
   }
   else
   {
      nxlog_write(NXLOG_ERROR, _T("Error loading ssh keys configuration from database"));
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Check SSH key name: not blank and fits into key name buffer without truncation
 */
static bool IsValidSshKeyName(const wchar_t *name)
{
   size_t len = wcslen(name);
   if (len >= MAX_SSH_KEY_NAME)
      return false;
   for(size_t i = 0; i < len; i++)
      if (!iswspace(name[i]))
         return true;
   return false;
}

/**
 * Generate SSH key
 */
uint32_t NXCORE_EXPORTABLE GenerateSshKey(const wchar_t *name, uint32_t *newKeyId)
{
   if (!IsValidSshKeyName(name))
      return RCC_INVALID_OBJECT_NAME;

   shared_ptr<SshKeyPair> key = SshKeyPair::generate(name);
   if (key != nullptr)
   {
      if (newKeyId != nullptr)
         *newKeyId = key->getId();
      s_sshKeys.set(key->getId(), key);
      wchar_t threadKey[32] = L"ssh-key-";
      IntegerToString(key->getId(), &threadKey[8]);
      ThreadPoolExecuteSerialized(g_mainThreadPool, threadKey, key, &SshKeyPair::saveToDatabase);
      NotifyClientSessions(NX_NOTIFY_SSH_KEY_DATA_CHAGED, 0);
      return RCC_SUCCESS;
   }
   else
   {
      return RCC_INTERNAL_ERROR;
   }
}

/**
 * Create SSH key or update existing one. Private key text is parsed and stored together with derived public key;
 * without private key text existing key keeps its key pair and only gets renamed.
 */
static uint32_t StoreSshKey(uint32_t id, const wchar_t *name, const char *privateKeyText, uint32_t *keyId)
{
   shared_ptr<SshKeyPair> oldKey;
   if (id != 0)
   {
      oldKey = s_sshKeys.getShared(id);
      if (oldKey == nullptr)
         return RCC_INVALID_SSH_KEY_ID;
   }

   if (!IsValidSshKeyName(name))
      return RCC_INVALID_OBJECT_NAME;

   char *privateKey, *publicKey;
   if (privateKeyText != nullptr)
   {
      if (!ParseOpenSshPrivateKey(privateKeyText, name, &privateKey, &publicKey))
         return RCC_INVALID_ARGUMENT;
   }
   else if (oldKey != nullptr)
   {
      privateKey = MemCopyStringA(oldKey->getPrivateKey());
      publicKey = MemCopyStringA(oldKey->getPublicKey());
   }
   else
   {
      return RCC_INVALID_ARGUMENT;
   }

   shared_ptr<SshKeyPair> newKey = make_shared<SshKeyPair>(id, name, publicKey, privateKey);
   if (keyId != nullptr)
      *keyId = newKey->getId();

   wchar_t threadKey[32] = L"ssh-key-";
   IntegerToString(newKey->getId(), &threadKey[8]);
   ThreadPoolExecuteSerialized(g_mainThreadPool, threadKey, newKey, &SshKeyPair::saveToDatabase);
   s_sshKeys.set(newKey->getId(), newKey);
   NotifyClientSessions(NX_NOTIFY_SSH_KEY_DATA_CHAGED, 0);
   return RCC_SUCCESS;
}

/**
 * Create or edit SSH key from NXCP message. Public key sent by client is ignored.
 */
uint32_t NXCORE_EXPORTABLE CreateOrEditSshKey(const NXCPMessage& request)
{
   String name = request.getFieldAsSharedString(VID_NAME);
   char *privateKeyText = request.getFieldAsUtf8String(VID_PRIVATE_KEY);
   uint32_t rcc = StoreSshKey(request.getFieldAsUInt32(VID_SSH_KEY_ID), name, privateKeyText, nullptr);
   MemFree(privateKeyText);
   return rcc;
}

/**
 * Create or edit SSH key from JSON. Absent private key keeps existing key pair on edit; public key in request is
 * ignored.
 */
uint32_t NXCORE_EXPORTABLE CreateOrEditSshKey(uint32_t id, json_t *json, uint32_t *keyId)
{
   if ((id != 0) && !s_sshKeys.contains(id))
      return RCC_INVALID_SSH_KEY_ID;

   json_t *jsonName = json_object_get(json, "name");
   if (!json_is_string(jsonName))
      return RCC_INVALID_OBJECT_NAME;

   const char *privateKeyText = nullptr;
   json_t *jsonPrivateKey = json_object_get(json, "privateKey");
   if (jsonPrivateKey != nullptr)
   {
      if (!json_is_string(jsonPrivateKey))
         return RCC_INVALID_ARGUMENT;
      privateKeyText = json_string_value(jsonPrivateKey);
   }

   return StoreSshKey(id, String(json_string_value(jsonName), "utf8"), privateKeyText, keyId);
}

/**
 * Callback to find if deleted ssh key is used by node
 */
static bool CheckIfSshKeyIsUsed(NetObj *object, uint32_t *data)
{
   return static_cast<Node *>(object)->getSshKeyId() == (*data);
}

/**
 * Delete SSH key from database
 */
uint32_t NXCORE_EXPORTABLE DeleteSshKey(uint32_t id, bool forceDelete, SharedObjectArray<NetObj> **references)
{
   shared_ptr<SshKeyPair> key = s_sshKeys.getShared(id);
   if (key == nullptr)
      return RCC_INVALID_SSH_KEY_ID;

   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxNodeById.findAll(CheckIfSshKeyIsUsed, &id);
   if (objects->isEmpty() || forceDelete)
   {
      for (int i = 0; i < objects->size(); i++)
      {
         static_cast<Node *>(objects->get(i))->clearSshKey();
      }
      wchar_t threadKey[32] = L"ssh-key-";
      IntegerToString(id, &threadKey[8]);
      ThreadPoolExecuteSerialized(g_mainThreadPool, threadKey, key, &SshKeyPair::deleteFromDatabase);
      s_sshKeys.remove(id);
      NotifyClientSessions(NX_NOTIFY_SSH_KEY_DATA_CHAGED, 0);
      return RCC_SUCCESS;
   }

   if (references != nullptr)
      *references = objects.release();
   return RCC_SSH_KEY_IN_USE;
}

/**
 * Find SSH key by id and fill message with private key
 */
void NXCORE_EXPORTABLE FindSshKeyById(uint32_t id, NXCPMessage *msg)
{
   shared_ptr<SshKeyPair> key = s_sshKeys.getShared(id);
   if (key != nullptr)
   {
      msg->setFieldFromUtf8String(VID_PUBLIC_KEY, key->getPublicKey());
      msg->setFieldFromUtf8String(VID_PRIVATE_KEY, key->getPrivateKey());
      msg->setField(VID_RCC, ERR_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, ERR_INVALID_SSH_KEY_ID);
   }
}

/**
 * Get all SSH keys as JSON array
 */
json_t NXCORE_EXPORTABLE *GetSshKeysAsJson(bool includePublicKey)
{
   json_t *output = json_array();
   s_sshKeys.forEach(
      [output, includePublicKey](const uint32_t &id, const shared_ptr<SshKeyPair> &key) -> EnumerationCallbackResult
      {
         json_array_append_new(output, key->toJson(includePublicKey));
         return _CONTINUE;
      });
   return output;
}

/**
 * Get SSH key by ID as JSON
 */
json_t NXCORE_EXPORTABLE *GetSshKeyByIdAsJson(uint32_t id)
{
   shared_ptr<SshKeyPair> key = s_sshKeys.getShared(id);
   return (key != nullptr) ? key->toJson(true) : nullptr;
}
