/*
** NetXMS - Network Management System
** Copyright (C) 2021-2024 Raden Solutions
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

#define MAX_SSH_KEY_NAME 255

static unsigned char s_sshHeader[11] = { 0x00, 0x00, 0x00, 0x07, 0x73, 0x73, 0x68, 0x2D, 0x72, 0x73, 0x61};

/**
 * SSH key data structure
 */
class SshKeyPair
{
private:
   uint32_t m_id;
   TCHAR m_name[MAX_SSH_KEY_NAME];
   TCHAR *m_publicKey;
   char *m_privateKey;

public:
   static shared_ptr<SshKeyPair> generate(const TCHAR *name);

   SshKeyPair();
   SshKeyPair(const NXCPMessage& msg);
   SshKeyPair(DB_RESULT result, int row);
   ~SshKeyPair();

   void saveToDatabase();
   void deleteFromDatabase();

   uint32_t getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getPublicKey() const { return m_publicKey; }
   const char *getPrivateKey() const { return m_privateKey; }

   void updateKeys(const shared_ptr<SshKeyPair> &data);
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
 * Constructor from NXCP message
 */
SshKeyPair::SshKeyPair(const NXCPMessage& msg)
{
   if (msg.getFieldAsInt32(VID_SSH_KEY_ID) != 0)
      m_id = msg.getFieldAsInt32(VID_SSH_KEY_ID);
   else
      m_id = CreateUniqueId(IDG_SSH_KEY);
   msg.getFieldAsString(VID_NAME, m_name, MAX_SSH_KEY_NAME);
   m_publicKey = msg.getFieldAsString(VID_PUBLIC_KEY);
   m_privateKey = msg.getFieldAsMBString(VID_PRIVATE_KEY);
}

/**
 * Constructor form DB result
 */
SshKeyPair::SshKeyPair(DB_RESULT result, int row)
{
   m_id = DBGetFieldULong(result, row, 0);
   DBGetField(result, row, 1, m_name, MAX_SSH_KEY_NAME);
   m_publicKey = DBGetField(result, row, 2, nullptr, 0);
   m_privateKey = DBGetFieldA(result, row, 3, nullptr, 0);
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
shared_ptr<SshKeyPair> SshKeyPair::generate(const TCHAR *name)
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

   StringBuffer publicKey;
   publicKey.append(_T("ssh-rsa "));
   char *out;
   size_t len = base64_encode_alloc(reinterpret_cast<const char*>(binaryData), binaryDataLength, &out);
   publicKey.appendUtf8String(out, len);

   MemFree(nBytes);
   MemFree(eBytes);
   MemFree(binaryData);
   MemFree(out);

   publicKey.append(_T(" "));
   publicKey.append(name);
   data->m_publicKey = MemCopyString(publicKey.getBuffer());

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
   static const TCHAR *columns[] = { _T("name"), _T("public_key"), _T("private_key"), nullptr };
   DB_STATEMENT stmt =  DBPrepareMerge(hdb, _T("ssh_keys"), _T("id"), m_id, columns);
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, m_publicKey, DB_BIND_STATIC);
      DBBind(stmt, 3, DB_CTYPE_STRING, TStringFromUTF8String(m_privateKey), DB_BIND_DYNAMIC);
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
   ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM ssh_keys WHERE id=?"));
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Copy key data form old object
 */
void SshKeyPair::updateKeys(const shared_ptr<SshKeyPair> &data)
{
   m_publicKey = MemCopyString(data->m_publicKey);
   m_privateKey = MemCopyStringA(data->m_privateKey);
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
      msg->setField(data->baseId++, value->getPublicKey());
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
   DB_RESULT result = DBSelect(hdb, _T("SELECT id,name,public_key,private_key FROM ssh_keys"));
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
 * Generate SSH key
 */
uint32_t GenerateSshKey(const TCHAR *name)
{
   shared_ptr<SshKeyPair> key = SshKeyPair::generate(name);
   if (key != nullptr)
   {
      s_sshKeys.set(key->getId(), key);
      TCHAR threadKey[32];
      _sntprintf(threadKey, 32, _T("SshKey_%d"), key->getId());
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
 * Generate SSH key
 */
void CreateOrEditSshKey(const NXCPMessage& request)
{
   uint32_t id = request.getFieldAsInt32(VID_SSH_KEY_ID);
   shared_ptr<SshKeyPair> newKey = make_shared<SshKeyPair>(request);

   if (!request.isFieldExist(VID_PRIVATE_KEY) && (id != 0))
   {
      shared_ptr<SshKeyPair> oldKey = s_sshKeys.getShared(id);
      newKey->updateKeys(oldKey);
   }
   TCHAR threadKey[32];
   _sntprintf(threadKey, 32, _T("SshKey_%d"), newKey->getId());
   ThreadPoolExecuteSerialized(g_mainThreadPool, threadKey, newKey, &SshKeyPair::saveToDatabase);
   s_sshKeys.set(newKey->getId(), newKey);
   NotifyClientSessions(NX_NOTIFY_SSH_KEY_DATA_CHAGED, 0);
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
void DeleteSshKey(NXCPMessage *response, uint32_t id, bool forceDelete)
{
   shared_ptr<SshKeyPair> key = s_sshKeys.getShared(id);
   if (key != nullptr)
   {
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxNodeById.findAll(CheckIfSshKeyIsUsed, &id);
      if (objects->isEmpty() || forceDelete)
      {
         for (int i = 0; i < objects->size(); i++)
         {
            static_cast<Node *>(objects->get(i))->clearSshKey();
         }
         TCHAR threadKey[32];
         _sntprintf(threadKey, 32, _T("SshKey_%d"), key->getId());
         ThreadPoolExecuteSerialized(g_mainThreadPool, threadKey, key, &SshKeyPair::deleteFromDatabase);
         s_sshKeys.remove(key->getId());
         NotifyClientSessions(NX_NOTIFY_SSH_KEY_DATA_CHAGED, 0);
         response->setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         IntegerArray<uint32_t> ids;
         for (int i = 0; i < objects->size(); i ++)
            ids.add(objects->get(i)->getId());
         response->setFieldFromInt32Array(VID_OBJECT_LIST, ids);
         response->setField(VID_RCC, RCC_SSH_KEY_IN_USE);
      }
   }
   else
   {
      response->setField(VID_RCC, RCC_INVALID_SSH_KEY_ID);
   }
}

/**
 * Find SSH key by id and fill message with private key
 */
void FindSshKeyById(uint32_t id, NXCPMessage *msg)
{
   shared_ptr<SshKeyPair> key = s_sshKeys.getShared(id);
   if (key != nullptr)
   {
      msg->setField(VID_PUBLIC_KEY, key->getPublicKey());
      msg->setFieldFromUtf8String(VID_PRIVATE_KEY, key->getPrivateKey());
      msg->setField(VID_RCC, ERR_SUCCESS);
   }
   else
   {
      msg->setField(VID_RCC, ERR_INVALID_SSH_KEY_ID);
   }
}
