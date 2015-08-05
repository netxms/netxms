/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: crypto.cpp
**
**/

#include "libnetxms.h"
#include "ice.h"

/**
 * Constants
 */
#define KEY_BUFFER_SIZE       4096

/**
 * Supported ciphers. By default, we support all ciphers compiled
 * into OpenSSL library.
 */
static UINT32 s_supportedCiphers = 
#ifdef _WITH_ENCRYPTION
#ifndef OPENSSL_NO_AES
   NXCP_SUPPORT_AES_256 |
   NXCP_SUPPORT_AES_128 |
#endif
#ifndef OPENSSL_NO_BF
   NXCP_SUPPORT_BLOWFISH_256 |
   NXCP_SUPPORT_BLOWFISH_128 |
#endif
#ifndef OPENSSL_NO_IDEA
   NXCP_SUPPORT_IDEA |
#endif
#ifndef OPENSSL_NO_DES
   NXCP_SUPPORT_3DES |
#endif
#endif   /* _WITH_ENCRYPTION */
   0;

/**
 * Static data
 */
static void (*s_debugCallback)(int, const TCHAR *, va_list args) = NULL;
static WORD s_noEncryptionFlag = 0;

#ifdef _WITH_ENCRYPTION

extern "C" typedef OPENSSL_CONST EVP_CIPHER * (*CIPHER_FUNC)();
static CIPHER_FUNC s_ciphers[NETXMS_MAX_CIPHERS] =
{
#ifndef OPENSSL_NO_AES
   EVP_aes_256_cbc,
#else
   NULL,
#endif
#ifndef OPENSSL_NO_BF
   EVP_bf_cbc,
#else
   NULL,
#endif
#ifndef OPENSSL_NO_IDEA
   EVP_idea_cbc,
#else
   NULL,
#endif
#ifndef OPENSSL_NO_DES
   EVP_des_ede3_cbc,
#else
   NULL,
#endif
#ifndef OPENSSL_NO_AES
   EVP_aes_128_cbc,
#else
   NULL,
#endif
#ifndef OPENSSL_NO_BF
   EVP_bf_cbc
#else
   NULL
#endif
};
static const TCHAR *s_cipherNames[NETXMS_MAX_CIPHERS] = { _T("AES-256"), _T("Blowfish-256"), _T("IDEA"), _T("3DES"), _T("AES-128"), _T("Blowfish-128") };
static MUTEX *s_cryptoMutexList = NULL;

/**
 * Locking callback for CRYPTO library
 */
#if defined(__SUNPRO_CC)
extern "C"
#endif
static void CryptoLockingCallback(int nMode, int nLock, const char *pszFile, int nLine)
{
   if (nMode & CRYPTO_LOCK)
      MutexLock(s_cryptoMutexList[nLock]);
   else
      MutexUnlock(s_cryptoMutexList[nLock]);
}

/**
 * ID callback for CRYPTO library
 */
#ifndef _WIN32

#if defined(__SUNPRO_CC)
extern "C"
#endif
static unsigned long CryptoIdCallback()
{
   return (unsigned long)GetCurrentThreadId();
}

#endif

#endif   /* _WITH_ENCRYPTION */

/**
 * Debug output
 */
static void CryptoDbgPrintf(int level, const TCHAR *format, ...)
{
   if (s_debugCallback == NULL)
      return;

   va_list args;
   va_start(args, format);
   s_debugCallback(level, format, args);
   va_end(args);
}

/**
 * Initialize OpenSSL library
 */
bool LIBNETXMS_EXPORTABLE InitCryptoLib(UINT32 dwEnabledCiphers, void (*debugCallback)(int, const TCHAR *, va_list args))
{
   s_debugCallback = debugCallback;
   s_noEncryptionFlag = htons(MF_DONT_ENCRYPT);

#ifdef _WITH_ENCRYPTION
   BYTE random[8192];
   int i;

   CRYPTO_malloc_init();
   ERR_load_CRYPTO_strings();
   OpenSSL_add_all_algorithms();
   RAND_seed(random, 8192);
   s_cryptoMutexList = (MUTEX *)malloc(sizeof(MUTEX) * CRYPTO_num_locks());
   for(i = 0; i < CRYPTO_num_locks(); i++)
      s_cryptoMutexList[i] = MutexCreate();
   CRYPTO_set_locking_callback(CryptoLockingCallback);
#ifndef _WIN32
   CRYPTO_set_id_callback(CryptoIdCallback);
#endif   /* _WIN32 */

   // validate supported ciphers
   CryptoDbgPrintf(1, _T("Validating ciphers"));
   s_supportedCiphers &= dwEnabledCiphers;
   UINT32 cipherBit = 1;
   for(i = 0; i < NETXMS_MAX_CIPHERS; i++, cipherBit = cipherBit << 1)
   {
      if ((s_supportedCiphers & cipherBit) == 0)
      {
         CryptoDbgPrintf(1, _T("   %s disabled (config)"), s_cipherNames[i]);
         continue;
      }
      NXCPEncryptionContext *ctx = NXCPEncryptionContext::create(cipherBit);
      if (ctx != NULL)
      {
         delete ctx;
         CryptoDbgPrintf(1, _T("   %s enabled"), s_cipherNames[i]);
      }
      else
      {
         s_supportedCiphers &= ~cipherBit;
         CryptoDbgPrintf(1, _T("   %s disabled (validation failed)"), s_cipherNames[i]);
      }
   }

   CryptoDbgPrintf(1, _T("Crypto library initialized"));

#else
   CryptoDbgPrintf(1, _T("Crypto library will not be initialized because libnetxms was built without encryption support"));
#endif   /* _WITH_ENCRYPTION */
   return true;
}

/**
 * Get supported ciphers
 */
UINT32 LIBNETXMS_EXPORTABLE NXCPGetSupportedCiphers()
{
   return s_supportedCiphers;
}

/**
 * Get supported ciphers as text
 */
String LIBNETXMS_EXPORTABLE NXCPGetSupportedCiphersAsText()
{
   String s;
   UINT32 cipherBit = 1;
   for(int i = 0; i < NETXMS_MAX_CIPHERS; i++, cipherBit = cipherBit << 1)
   {
      if ((s_supportedCiphers & cipherBit) == 0)
      {
         continue;
      }
      NXCPEncryptionContext *ctx = NXCPEncryptionContext::create(cipherBit);
      if (ctx != NULL)
      {
         delete ctx;
         if (s.length() > 0)
            s.append(_T(", "));
         s.append(s_cipherNames[i]);
      }
   }
   return s;
}

/**
 * Encrypt message
 */
NXCP_ENCRYPTED_MESSAGE LIBNETXMS_EXPORTABLE *NXCPEncryptMessage(NXCPEncryptionContext *pCtx, NXCP_MESSAGE *msg)
{
   return (pCtx != NULL) ? pCtx->encryptMessage(msg) : NULL;
}

/**
 * Decrypt message
 */
bool LIBNETXMS_EXPORTABLE NXCPDecryptMessage(NXCPEncryptionContext *pCtx, NXCP_ENCRYPTED_MESSAGE *msg, BYTE *pDecryptionBuffer)
{
   return (pCtx != NULL) ? pCtx->decryptMessage(msg, pDecryptionBuffer) : false;
}

/**
 * Setup encryption context
 * Function will determine is it called on initiator or responder side
 * by message code. Initiator should provide it's private key,
 * and responder should provide pointer to response message.
 */
UINT32 LIBNETXMS_EXPORTABLE SetupEncryptionContext(NXCPMessage *msg, 
                                                  NXCPEncryptionContext **ppCtx,
                                                  NXCPMessage **ppResponse,
                                                  RSA *pPrivateKey, int nNXCPVersion)
{
   UINT32 dwResult = RCC_NOT_IMPLEMENTED;

	*ppCtx = NULL;
#ifdef _WITH_ENCRYPTION
   if (msg->getCode() == CMD_REQUEST_SESSION_KEY)
   {
      UINT32 dwCiphers;

      *ppResponse = new NXCPMessage(nNXCPVersion);
      (*ppResponse)->setCode(CMD_SESSION_KEY);
      (*ppResponse)->setId(msg->getId());
      (*ppResponse)->disableEncryption();

      dwCiphers = msg->getFieldAsUInt32(VID_SUPPORTED_ENCRYPTION) & s_supportedCiphers;
      if (dwCiphers == 0)
      {
         (*ppResponse)->setField(VID_RCC, RCC_NO_CIPHERS);
         dwResult = RCC_NO_CIPHERS;
      }
      else
      {
			BYTE *pBufPos, ucKeyBuffer[KEY_BUFFER_SIZE];
			RSA *pServerKey;

			*ppCtx = NXCPEncryptionContext::create(dwCiphers);
         if (*ppCtx != NULL)
         {
            // Encrypt key
            int size = msg->getFieldAsBinary(VID_PUBLIC_KEY, ucKeyBuffer, KEY_BUFFER_SIZE);
            pBufPos = ucKeyBuffer;
            pServerKey = d2i_RSAPublicKey(NULL, (OPENSSL_CONST BYTE **)&pBufPos, size);
            if (pServerKey != NULL)
            {
               (*ppResponse)->setField(VID_RCC, RCC_SUCCESS);
               
               size = RSA_public_encrypt((*ppCtx)->getKeyLength(), (*ppCtx)->getSessionKey(), ucKeyBuffer, pServerKey, RSA_PKCS1_OAEP_PADDING);
               (*ppResponse)->setField(VID_SESSION_KEY, ucKeyBuffer, (UINT32)size);
               (*ppResponse)->setField(VID_KEY_LENGTH, (WORD)(*ppCtx)->getKeyLength());
               
               int ivLength = EVP_CIPHER_iv_length(s_ciphers[(*ppCtx)->getCipher()]());
               if ((ivLength <= 0) || (ivLength > EVP_MAX_IV_LENGTH))
                  ivLength = EVP_MAX_IV_LENGTH;
               size = RSA_public_encrypt(ivLength, (*ppCtx)->getIV(), ucKeyBuffer, pServerKey, RSA_PKCS1_OAEP_PADDING);
               (*ppResponse)->setField(VID_SESSION_IV, ucKeyBuffer, (UINT32)size);
               (*ppResponse)->setField(VID_IV_LENGTH, (WORD)ivLength);

               (*ppResponse)->setField(VID_CIPHER, (WORD)(*ppCtx)->getCipher());
               RSA_free(pServerKey);
               dwResult = RCC_SUCCESS;
            }
            else
            {
               (*ppResponse)->setField(VID_RCC, RCC_INVALID_PUBLIC_KEY);
               dwResult = RCC_INVALID_PUBLIC_KEY;
            }
         }
         else
         {
            (*ppResponse)->setField(VID_RCC, RCC_ENCRYPTION_ERROR);
            dwResult = RCC_ENCRYPTION_ERROR;
         }
      }
   }
   else if (msg->getCode() == CMD_SESSION_KEY)
   {
      dwResult = msg->getFieldAsUInt32(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
			*ppCtx = NXCPEncryptionContext::create(msg, pPrivateKey);
			if (*ppCtx == NULL)
			{
				dwResult = RCC_INVALID_SESSION_KEY;
			}
      }
   }

   if ((dwResult != RCC_SUCCESS) && (*ppCtx != NULL))
   {
      delete *ppCtx;
      *ppCtx = NULL;
   }
#else
   if (msg->getCode() == CMD_REQUEST_SESSION_KEY)
   {
      *ppResponse = new NXCPMessage(nNXCPVersion);
      (*ppResponse)->SetCode(CMD_SESSION_KEY);
      (*ppResponse)->SetId(msg->getId());
      (*ppResponse)->disableEncryption();
      (*ppResponse)->setField(VID_RCC, dwResult);
   }
#endif

   return dwResult;
}

/**
 * Prepare session key request message
 */
void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(NXCPMessage *msg, RSA *pServerKey, bool useX509Format)
{
#ifdef _WITH_ENCRYPTION
   int iLen;
   BYTE *pKeyBuffer, *pBufPos;

   msg->setCode(CMD_REQUEST_SESSION_KEY);
   msg->setField(VID_SUPPORTED_ENCRYPTION, s_supportedCiphers);

	if (useX509Format)
	{
		iLen = i2d_RSA_PUBKEY(pServerKey, NULL);
		pKeyBuffer = (BYTE *)malloc(iLen);
		pBufPos = pKeyBuffer;
		i2d_RSA_PUBKEY(pServerKey, &pBufPos);
	}
	else
	{
		iLen = i2d_RSAPublicKey(pServerKey, NULL);
		pKeyBuffer = (BYTE *)malloc(iLen);
		pBufPos = pKeyBuffer;
		i2d_RSAPublicKey(pServerKey, &pBufPos);
	}
	msg->setField(VID_PUBLIC_KEY, pKeyBuffer, iLen);
   free(pKeyBuffer);
#endif
}

/**
 * Load RSA key(s) from file
 */
RSA LIBNETXMS_EXPORTABLE *LoadRSAKeys(const TCHAR *pszKeyFile)
{
#ifdef _WITH_ENCRYPTION
   FILE *fp;
   BYTE *pKeyBuffer, *pBufPos, hash[SHA1_DIGEST_SIZE];
   UINT32 dwLen;
   RSA *pKey = NULL;

   fp = _tfopen(pszKeyFile, _T("rb"));
   if (fp != NULL)
   {
      if (fread(&dwLen, 1, sizeof(UINT32), fp) == sizeof(UINT32) && dwLen < 10 * 1024)
      {
         pKeyBuffer = (BYTE *)malloc(dwLen);
         pBufPos = pKeyBuffer;
         if (fread(pKeyBuffer, 1, dwLen, fp) == dwLen)
         {
            BYTE hash2[SHA1_DIGEST_SIZE];

            if (fread(hash, 1, SHA1_DIGEST_SIZE, fp) == SHA1_DIGEST_SIZE)
            {
               CalculateSHA1Hash(pKeyBuffer, dwLen, hash2);
               if (!memcmp(hash, hash2, SHA1_DIGEST_SIZE))
               {
                  pKey = d2i_RSAPublicKey(NULL, (OPENSSL_CONST BYTE **)&pBufPos, dwLen);
                  if (pKey != NULL)
                  {
                     if (d2i_RSAPrivateKey(&pKey, (OPENSSL_CONST BYTE **)&pBufPos,
                                           dwLen - CAST_FROM_POINTER((pBufPos - pKeyBuffer), UINT32)) == NULL)
                     {
                        RSA_free(pKey);
                        pKey = NULL;
                     }
                  }
               }
            }
         }
         free(pKeyBuffer);
      }
      fclose(fp);
   }

   return pKey;
#else
   return NULL;
#endif
}

/**
 * Create signature for message using certificate (MS CAPI version)
 * Paraeters:
 *    msg and dwMsgLen - message to sign and it's length
 *    pCert - certificate
 *    pBuffer - output buffer
 *    dwBufSize - buffer size
 *    pdwSigLen - actual signature size
 */
#ifdef _WIN32

BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *msg, UINT32 dwMsgLen, const CERT_CONTEXT *pCert,
												          BYTE *pBuffer, UINT32 dwBufSize, UINT32 *pdwSigLen)
{
	BOOL bFreeProv, bRet = FALSE;
	DWORD i, j, dwLen, dwKeySpec;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	BYTE *pTemp;

	if (CryptAcquireCertificatePrivateKey(pCert, CRYPT_ACQUIRE_COMPARE_KEY_FLAG, NULL, &hProv, &dwKeySpec, &bFreeProv))
	{
		if (CryptCreateHash(hProv, CALG_SHA1, NULL, 0, &hHash))
		{
			if (CryptHashData(hHash, msg, dwMsgLen, 0))
			{
				dwLen = dwBufSize;
				pTemp = (BYTE *)malloc(dwBufSize);;
				bRet = CryptSignHash(hHash, dwKeySpec, NULL, 0, pTemp, &dwLen);
				*pdwSigLen = dwLen;
				// we have to reverse the byte-order in the result from CryptSignHash()
				for(i = 0, j = dwLen - 1; i < dwLen; i++, j--)
					pBuffer[i] = pTemp[j];			
			}
			CryptDestroyHash(hHash);
		}

		if (bFreeProv)
			CryptReleaseContext(hProv, 0);
	}
	return bRet;
}

#endif

/**
 * Encrypt data block with ICE
 */
void LIBNETXMS_EXPORTABLE ICEEncryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key)
{
	ICE_KEY *ice = ice_key_create(1);
	ice_key_set(ice, key);

	int stopPos = inLen - (inLen % 8);
	for(int pos = 0; pos < stopPos; pos += 8)
		ice_key_encrypt(ice, &in[pos], &out[pos]);

	if (stopPos < inLen)
	{
		BYTE plainText[8], encrypted[8];

		memcpy(plainText, &in[stopPos], inLen - stopPos);
		ice_key_encrypt(ice, plainText, encrypted);
		memcpy(&out[stopPos], encrypted, inLen - stopPos);
	}

	ice_key_destroy(ice);
}

/**
 * Decrypt data block with ICE
 */
void LIBNETXMS_EXPORTABLE ICEDecryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key)
{
	ICE_KEY *ice = ice_key_create(1);
	ice_key_set(ice, key);

	int stopPos = inLen - (inLen % 8);
	for(int pos = 0; pos < stopPos; pos += 8)
		ice_key_decrypt(ice, &in[pos], &out[pos]);

	if (stopPos < inLen)
	{
		BYTE plainText[8], encrypted[8];

		memcpy(encrypted, &in[stopPos], inLen - stopPos);
		ice_key_decrypt(ice, encrypted, plainText);
		memcpy(&out[stopPos], plainText, inLen - stopPos);
	}

	ice_key_destroy(ice);
}

/**
 * Encryption context constructor
 */
NXCPEncryptionContext::NXCPEncryptionContext()
{
   m_sessionKey = NULL;
#ifdef _WITH_ENCRYPTION
   EVP_CIPHER_CTX_init(&m_encryptor);
   EVP_CIPHER_CTX_init(&m_decryptor);
   m_encryptorLock = MutexCreate();
#endif
}

/**
 * Encryption context destructor
 */
NXCPEncryptionContext::~NXCPEncryptionContext()
{
   safe_free(m_sessionKey);
#ifdef _WITH_ENCRYPTION
   EVP_CIPHER_CTX_cleanup(&m_encryptor);
   EVP_CIPHER_CTX_cleanup(&m_decryptor);
   MutexDestroy(m_encryptorLock);
#endif
}

/**
 * Create encryption context from CMD_SESSION_KEY NXCP message
 */
NXCPEncryptionContext *NXCPEncryptionContext::create(NXCPMessage *msg, RSA *privateKey)
{
#ifdef _WITH_ENCRYPTION
   BYTE ucKeyBuffer[KEY_BUFFER_SIZE], ucSessionKey[KEY_BUFFER_SIZE];
   UINT32 dwKeySize;
   int nSize, nIVLen;
	NXCPEncryptionContext *ctx = new NXCPEncryptionContext;

   int cipher = (int)msg->getFieldAsUInt16(VID_CIPHER);
   if (ctx->initCipher(cipher))
   {
      if (ctx->m_keyLength == (int)msg->getFieldAsUInt16(VID_KEY_LENGTH))
      {
         ctx->m_sessionKey = (BYTE *)malloc(ctx->m_keyLength);

         // Decrypt session key
         dwKeySize = msg->getFieldAsBinary(VID_SESSION_KEY, ucKeyBuffer, KEY_BUFFER_SIZE);
         nSize = RSA_private_decrypt(dwKeySize, ucKeyBuffer, ucSessionKey, privateKey, RSA_PKCS1_OAEP_PADDING);
         if (nSize == ctx->m_keyLength)
         {
            memcpy(ctx->m_sessionKey, ucSessionKey, nSize);

            // Decrypt session IV
            nIVLen = msg->getFieldAsUInt16(VID_IV_LENGTH);
            if (nIVLen == 0)  // Versions prior to 0.2.13 don't send IV length, assume 16
               nIVLen = 16;
            dwKeySize = msg->getFieldAsBinary(VID_SESSION_IV, ucKeyBuffer, KEY_BUFFER_SIZE);
            nSize = RSA_private_decrypt(dwKeySize, ucKeyBuffer, ucSessionKey, privateKey, RSA_PKCS1_OAEP_PADDING);
            if ((nSize == nIVLen) &&
                (nIVLen <= EVP_CIPHER_iv_length(s_ciphers[ctx->m_cipher]())))
            {
               memcpy(ctx->m_iv, ucSessionKey, min(EVP_MAX_IV_LENGTH, nIVLen));
            }
            else
            {
               CryptoDbgPrintf(6, _T("NXCPEncryptionContext::create: IV decryption failed"));
               delete_and_null(ctx);
            }
         }
         else
         {
            CryptoDbgPrintf(6, _T("NXCPEncryptionContext::create: session key decryption failed"));
            delete_and_null(ctx);
         }
      }
      else
      {
         CryptoDbgPrintf(6, _T("NXCPEncryptionContext::create: key length mismatch (remote: %d local: %d)"), (int)msg->getFieldAsUInt16(VID_KEY_LENGTH), ctx->m_keyLength);
         delete_and_null(ctx);
      }
   }
   else
   {
      CryptoDbgPrintf(6, _T("NXCPEncryptionContext::create: initCipher(%d) call failed"), cipher);
      delete_and_null(ctx);
   }
	return ctx;
#else
	return new NXCPEncryptionContext;
#endif
}

/**
 * Initialize cipher
 */
bool NXCPEncryptionContext::initCipher(int cipher)
{
#ifdef _WITH_ENCRYPTION
   if (s_ciphers[cipher] == NULL)
      return false;   // Unsupported cipher

   if (!EVP_EncryptInit_ex(&m_encryptor, s_ciphers[cipher](), NULL, NULL, NULL))
      return false;
   if (!EVP_DecryptInit_ex(&m_decryptor, s_ciphers[cipher](), NULL, NULL, NULL))
      return false;

   switch(cipher)
   {
      case NXCP_CIPHER_AES_256:
         m_keyLength = 32;
         break;
      case NXCP_CIPHER_AES_128:
         m_keyLength = 16;
         break;
      case NXCP_CIPHER_BLOWFISH_256:
         m_keyLength = 32;
         break;
      case NXCP_CIPHER_BLOWFISH_128:
         m_keyLength = 16;
         break;
      case NXCP_CIPHER_IDEA:
         m_keyLength = 16;
         break;
      case NXCP_CIPHER_3DES:
         m_keyLength = 24;
         break;
      default:
         return false;
   }

   if (!EVP_CIPHER_CTX_set_key_length(&m_encryptor, m_keyLength) || !EVP_CIPHER_CTX_set_key_length(&m_decryptor, m_keyLength))
      return false;

   // This check is needed because at least some OpenSSL versions return no error
   // from EVP_CIPHER_CTX_set_key_length but still not change key length
   if ((EVP_CIPHER_CTX_key_length(&m_encryptor) != m_keyLength) || (EVP_CIPHER_CTX_key_length(&m_decryptor) != m_keyLength))
      return false;

   m_cipher = cipher;
   return true;
#else
   return false;
#endif
}

/**
 * Create encryption context from CMD_REQUEST_SESSION_KEY NXCP message
 */
NXCPEncryptionContext *NXCPEncryptionContext::create(UINT32 ciphers)
{
	NXCPEncryptionContext *ctx = new NXCPEncryptionContext;

#ifdef _WITH_ENCRYPTION
   // Select cipher
   bool selected = false;

   if (ciphers & NXCP_SUPPORT_AES_256)
   {
      selected = ctx->initCipher(NXCP_CIPHER_AES_256);
   }
   
   if (!selected && (ciphers & NXCP_SUPPORT_BLOWFISH_256))
   {
      selected = ctx->initCipher(NXCP_CIPHER_BLOWFISH_256);
   }
   
   if (!selected && (ciphers & NXCP_SUPPORT_AES_128))
   {
      selected = ctx->initCipher(NXCP_CIPHER_AES_128);
   }
   
   if (!selected && (ciphers & NXCP_SUPPORT_BLOWFISH_128))
   {
      selected = ctx->initCipher(NXCP_CIPHER_BLOWFISH_128);
   }
   
   if (!selected && (ciphers & NXCP_SUPPORT_IDEA))
   {
      selected = ctx->initCipher(NXCP_CIPHER_IDEA);
   }
   
   if (!selected && (ciphers & NXCP_SUPPORT_3DES))
   {
      selected = ctx->initCipher(NXCP_CIPHER_3DES);
   }

   if (!selected)
   {
      delete ctx;
      return NULL;
   }

   // Generate key
   ctx->m_sessionKey = (BYTE *)malloc(ctx->m_keyLength);
   RAND_bytes(ctx->m_sessionKey, ctx->m_keyLength);
   RAND_bytes(ctx->m_iv, EVP_MAX_IV_LENGTH);
#endif

	return ctx;
}

/**
 * Encrypt message
 */
NXCP_ENCRYPTED_MESSAGE *NXCPEncryptionContext::encryptMessage(NXCP_MESSAGE *msg)
{
   if (msg->flags & s_noEncryptionFlag)
      return (NXCP_ENCRYPTED_MESSAGE *)nx_memdup(msg, ntohl(msg->size));

#ifdef _WITH_ENCRYPTION
   MutexLock(m_encryptorLock);

   if (!EVP_EncryptInit_ex(&m_encryptor, NULL, NULL, m_sessionKey, m_iv))
   {
      MutexUnlock(m_encryptorLock);
      return NULL;
   }

   UINT32 msgSize = ntohl(msg->size);
   NXCP_ENCRYPTED_MESSAGE *emsg = 
      (NXCP_ENCRYPTED_MESSAGE *)malloc(msgSize + NXCP_ENCRYPTION_HEADER_SIZE + EVP_CIPHER_block_size(EVP_CIPHER_CTX_cipher(&m_encryptor)) + 8);
   emsg->code = htons(CMD_ENCRYPTED_MESSAGE);
   emsg->reserved = 0;

   NXCP_ENCRYPTED_PAYLOAD_HEADER header;
   header.dwChecksum = htonl(CalculateCRC32((BYTE *)msg, msgSize, 0));
   header.dwReserved = 0;

   int dataSize;
   EVP_EncryptUpdate(&m_encryptor, emsg->data, &dataSize, (BYTE *)&header, NXCP_EH_ENCRYPTED_BYTES);
   msgSize = dataSize;
   EVP_EncryptUpdate(&m_encryptor, emsg->data + msgSize, &dataSize, (BYTE *)msg, ntohl(msg->size));
   msgSize += dataSize;
   EVP_EncryptFinal_ex(&m_encryptor, emsg->data + msgSize, &dataSize);
   msgSize += dataSize + NXCP_EH_UNENCRYPTED_BYTES;

   MutexUnlock(m_encryptorLock);

   if (msgSize % 8 != 0)
   {
      emsg->padding = (BYTE)(8 - (msgSize % 8));
      msgSize += emsg->padding;
   }
   else
   {
      emsg->padding = 0;
   }
   emsg->size = htonl(msgSize);

   return emsg;
#else    /* _WITH_ENCRYPTION */
   return NULL;
#endif
}

/**
 * Decrypt message
 */
bool NXCPEncryptionContext::decryptMessage(NXCP_ENCRYPTED_MESSAGE *msg, BYTE *decryptionBuffer)
{
#ifdef _WITH_ENCRYPTION
   if (!EVP_DecryptInit_ex(&m_decryptor, NULL, NULL, m_sessionKey, m_iv))
      return false;

   msg->size = ntohl(msg->size);
   int dataSize;
   EVP_DecryptUpdate(&m_decryptor, decryptionBuffer, &dataSize, msg->data,
                     msg->size - NXCP_EH_UNENCRYPTED_BYTES - msg->padding);
   EVP_DecryptFinal(&m_decryptor, decryptionBuffer + dataSize, &dataSize);

   NXCP_MESSAGE *clearMsg = (NXCP_MESSAGE *)(decryptionBuffer + NXCP_EH_ENCRYPTED_BYTES);
   UINT32 msgSize = ntohl(clearMsg->size);
   if (msgSize > msg->size)
      return false;  // Message decrypted incorrectly, because it can't be larger than encrypted
   UINT32 crc32 = CalculateCRC32((BYTE *)clearMsg, msgSize, 0);
   if (crc32 != ntohl(((NXCP_ENCRYPTED_PAYLOAD_HEADER *)decryptionBuffer)->dwChecksum))
      return false;  // Bad checksum

   memcpy(msg, clearMsg, msgSize);
   return true;
#else    /* _WITH_ENCRYPTION */
   return false;
#endif
}

/**
 * Generate random bytes
 */
void LIBNETXMS_EXPORTABLE GenerateRandomBytes(BYTE *buffer, size_t size)
{
#ifdef _WITH_ENCRYPTION
   RAND_bytes(buffer, (int)size);
#else
   srand((unsigned int)time(NULL));
   for(size_t i = 0; i < size; i++)
      buffer[i] = (BYTE)(rand() % 256);
#endif
}
