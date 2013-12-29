/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
static UINT32 m_dwSupportedCiphers = 
#ifdef _WITH_ENCRYPTION
#ifndef OPENSSL_NO_AES
   CSCP_SUPPORT_AES_256 |
   CSCP_SUPPORT_AES_128 |
#endif
#ifndef OPENSSL_NO_BF
   CSCP_SUPPORT_BLOWFISH_256 |
   CSCP_SUPPORT_BLOWFISH_128 |
#endif
#ifndef OPENSSL_NO_IDEA
   CSCP_SUPPORT_IDEA |
#endif
#ifndef OPENSSL_NO_DES
   CSCP_SUPPORT_3DES |
#endif
#endif   /* _WITH_ENCRYPTION */
   0;

/**
 * Static data
 */
#ifdef _WITH_ENCRYPTION

typedef OPENSSL_CONST EVP_CIPHER * (*CIPHER_FUNC)();
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
static WORD s_noEncryptionFlag = 0;
static MUTEX *s_cryptoMutexList = NULL;
static void (*s_debugCallback)(int, const TCHAR *, va_list args) = NULL;

/**
 * Locking callback for CRYPTO library
 */
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
BOOL LIBNETXMS_EXPORTABLE InitCryptoLib(UINT32 dwEnabledCiphers, void (*debugCallback)(int, const TCHAR *, va_list args))
{
   s_debugCallback = debugCallback;

#ifdef _WITH_ENCRYPTION
   BYTE random[8192];
   int i;

   CRYPTO_malloc_init();
   ERR_load_CRYPTO_strings();
   OpenSSL_add_all_algorithms();
   RAND_seed(random, 8192);
   s_noEncryptionFlag = htons(MF_DONT_ENCRYPT);
   s_cryptoMutexList = (MUTEX *)malloc(sizeof(MUTEX) * CRYPTO_num_locks());
   for(i = 0; i < CRYPTO_num_locks(); i++)
      s_cryptoMutexList[i] = MutexCreate();
   CRYPTO_set_locking_callback(CryptoLockingCallback);
#ifndef _WIN32
   CRYPTO_set_id_callback(CryptoIdCallback);
#endif   /* _WIN32 */

   // validate supported ciphers
   CryptoDbgPrintf(1, _T("Validating ciphers"));
   m_dwSupportedCiphers &= dwEnabledCiphers;
   UINT32 cipherBit = 1;
   for(i = 0; i < NETXMS_MAX_CIPHERS; i++, cipherBit = cipherBit << 1)
   {
      if ((m_dwSupportedCiphers & cipherBit) == 0)
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
         m_dwSupportedCiphers &= ~cipherBit;
         CryptoDbgPrintf(1, _T("   %s disabled (validation failed)"), s_cipherNames[i]);
      }
   }

   CryptoDbgPrintf(1, _T("Crypto library initialized"));

#else
   CryptoDbgPrintf(1, _T("Crypto library will not be initialized because libnetxms was built without encryption support"));
#endif   /* _WITH_ENCRYPTION */
   return TRUE;
}

/**
 * Get supported ciphers
 */
UINT32 LIBNETXMS_EXPORTABLE CSCPGetSupportedCiphers()
{
   return m_dwSupportedCiphers;
}

/**
 * Encrypt message
 */
CSCP_ENCRYPTED_MESSAGE LIBNETXMS_EXPORTABLE *CSCPEncryptMessage(NXCPEncryptionContext *pCtx, CSCP_MESSAGE *pMsg)
{
   return (pCtx != NULL) ? pCtx->encryptMessage(pMsg) : NULL;
}

/**
 * Decrypt message
 */
BOOL LIBNETXMS_EXPORTABLE CSCPDecryptMessage(NXCPEncryptionContext *pCtx, CSCP_ENCRYPTED_MESSAGE *pMsg, BYTE *pDecryptionBuffer)
{
   return (pCtx != NULL) ? pCtx->decryptMessage(pMsg, pDecryptionBuffer) : NULL;
}

/**
 * Setup encryption context
 * Function will determine is it called on initiator or responder side
 * by message code. Initiator should provide it's private key,
 * and responder should provide pointer to response message.
 */
UINT32 LIBNETXMS_EXPORTABLE SetupEncryptionContext(CSCPMessage *pMsg, 
                                                  NXCPEncryptionContext **ppCtx,
                                                  CSCPMessage **ppResponse,
                                                  RSA *pPrivateKey, int nNXCPVersion)
{
   UINT32 dwResult = RCC_NOT_IMPLEMENTED;

	*ppCtx = NULL;
#ifdef _WITH_ENCRYPTION
   if (pMsg->GetCode() == CMD_REQUEST_SESSION_KEY)
   {
      UINT32 dwCiphers;

      *ppResponse = new CSCPMessage(nNXCPVersion);
      (*ppResponse)->SetCode(CMD_SESSION_KEY);
      (*ppResponse)->SetId(pMsg->GetId());
      (*ppResponse)->disableEncryption();

      dwCiphers = pMsg->GetVariableLong(VID_SUPPORTED_ENCRYPTION) & m_dwSupportedCiphers;
      if (dwCiphers == 0)
      {
         (*ppResponse)->SetVariable(VID_RCC, RCC_NO_CIPHERS);
         dwResult = RCC_NO_CIPHERS;
      }
      else
      {
			BYTE *pBufPos, ucKeyBuffer[KEY_BUFFER_SIZE];
			RSA *pServerKey;

			*ppCtx = NXCPEncryptionContext::create(dwCiphers);

         // Encrypt key
         int size = pMsg->GetVariableBinary(VID_PUBLIC_KEY, ucKeyBuffer, KEY_BUFFER_SIZE);
         pBufPos = ucKeyBuffer;
         pServerKey = d2i_RSAPublicKey(NULL, (OPENSSL_CONST BYTE **)&pBufPos, size);
         if (pServerKey != NULL)
         {
            (*ppResponse)->SetVariable(VID_RCC, RCC_SUCCESS);
            
            size = RSA_public_encrypt((*ppCtx)->getKeyLength(), (*ppCtx)->getSessionKey(), ucKeyBuffer, pServerKey, RSA_PKCS1_OAEP_PADDING);
            (*ppResponse)->SetVariable(VID_SESSION_KEY, ucKeyBuffer, (UINT32)size);
            (*ppResponse)->SetVariable(VID_KEY_LENGTH, (WORD)(*ppCtx)->getKeyLength());
            
            int ivLength = EVP_CIPHER_iv_length(s_ciphers[(*ppCtx)->getCipher()]());
            if ((ivLength <= 0) || (ivLength > EVP_MAX_IV_LENGTH))
               ivLength = EVP_MAX_IV_LENGTH;
            size = RSA_public_encrypt(ivLength, (*ppCtx)->getIV(), ucKeyBuffer, pServerKey, RSA_PKCS1_OAEP_PADDING);
            (*ppResponse)->SetVariable(VID_SESSION_IV, ucKeyBuffer, (UINT32)size);
            (*ppResponse)->SetVariable(VID_IV_LENGTH, (WORD)ivLength);

            (*ppResponse)->SetVariable(VID_CIPHER, (WORD)(*ppCtx)->getCipher());
            RSA_free(pServerKey);
            dwResult = RCC_SUCCESS;
         }
         else
         {
            (*ppResponse)->SetVariable(VID_RCC, RCC_INVALID_PUBLIC_KEY);
            dwResult = RCC_INVALID_PUBLIC_KEY;
         }
      }
   }
   else if (pMsg->GetCode() == CMD_SESSION_KEY)
   {
      dwResult = pMsg->GetVariableLong(VID_RCC);
      if (dwResult == RCC_SUCCESS)
      {
			*ppCtx = NXCPEncryptionContext::create(pMsg, pPrivateKey);
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
   if (pMsg->GetCode() == CMD_REQUEST_SESSION_KEY)
   {
      *ppResponse = new CSCPMessage(nNXCPVersion);
      (*ppResponse)->SetCode(CMD_SESSION_KEY);
      (*ppResponse)->SetId(pMsg->GetId());
      (*ppResponse)->disableEncryption();
      (*ppResponse)->SetVariable(VID_RCC, dwResult);
   }
#endif

   return dwResult;
}

/**
 * Prepare session key request message
 */
void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(CSCPMessage *pMsg, RSA *pServerKey, bool useX509Format)
{
#ifdef _WITH_ENCRYPTION
   int iLen;
   BYTE *pKeyBuffer, *pBufPos;

   pMsg->SetCode(CMD_REQUEST_SESSION_KEY);
   pMsg->SetVariable(VID_SUPPORTED_ENCRYPTION, m_dwSupportedCiphers);

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
	pMsg->SetVariable(VID_PUBLIC_KEY, pKeyBuffer, iLen);
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
 *    pMsg and dwMsgLen - message to sign and it's length
 *    pCert - certificate
 *    pBuffer - output buffer
 *    dwBufSize - buffer size
 *    pdwSigLen - actual signature size
 */
#ifdef _WIN32

BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, UINT32 dwMsgLen, const CERT_CONTEXT *pCert,
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
			if (CryptHashData(hHash, pMsg, dwMsgLen, 0))
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
   EVP_CIPHER_CTX_init(&m_encryptor);
   EVP_CIPHER_CTX_init(&m_decryptor);
}

/**
 * Encryption context destructor
 */
NXCPEncryptionContext::~NXCPEncryptionContext()
{
	safe_free(m_sessionKey);
   EVP_CIPHER_CTX_cleanup(&m_encryptor);
   EVP_CIPHER_CTX_cleanup(&m_decryptor);
}

/**
 * Create encryption context from CMD_SESSION_KEY NXCP message
 */
NXCPEncryptionContext *NXCPEncryptionContext::create(CSCPMessage *msg, RSA *privateKey)
{
#ifdef _WITH_ENCRYPTION
   BYTE ucKeyBuffer[KEY_BUFFER_SIZE], ucSessionKey[KEY_BUFFER_SIZE];
   UINT32 dwKeySize;
   int nSize, nIVLen;
	NXCPEncryptionContext *ctx = new NXCPEncryptionContext;

   int cipher = (int)msg->GetVariableShort(VID_CIPHER);
   if (ctx->initCipher(cipher))
   {
      if (ctx->m_keyLength == (int)msg->GetVariableShort(VID_KEY_LENGTH))
      {
         ctx->m_sessionKey = (BYTE *)malloc(ctx->m_keyLength);

         // Decrypt session key
         dwKeySize = msg->GetVariableBinary(VID_SESSION_KEY, ucKeyBuffer, KEY_BUFFER_SIZE);
         nSize = RSA_private_decrypt(dwKeySize, ucKeyBuffer, ucSessionKey, privateKey, RSA_PKCS1_OAEP_PADDING);
         if (nSize == ctx->m_keyLength)
         {
            memcpy(ctx->m_sessionKey, ucSessionKey, nSize);

            // Decrypt session IV
            nIVLen = msg->GetVariableShort(VID_IV_LENGTH);
            if (nIVLen == 0)  // Versions prior to 0.2.13 don't send IV length, assume 16
               nIVLen = 16;
            dwKeySize = msg->GetVariableBinary(VID_SESSION_IV, ucKeyBuffer, KEY_BUFFER_SIZE);
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
         CryptoDbgPrintf(6, _T("NXCPEncryptionContext::create: key length mismatch (remote: %d local: %d)"), (int)msg->GetVariableShort(VID_KEY_LENGTH), ctx->m_keyLength);
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
      case CSCP_CIPHER_AES_256:
         m_keyLength = 32;
         break;
      case CSCP_CIPHER_AES_128:
         m_keyLength = 16;
         break;
      case CSCP_CIPHER_BLOWFISH_256:
         m_keyLength = 32;
         break;
      case CSCP_CIPHER_BLOWFISH_128:
         m_keyLength = 16;
         break;
      case CSCP_CIPHER_IDEA:
         m_keyLength = 16;
         break;
      case CSCP_CIPHER_3DES:
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

   if (ciphers & CSCP_SUPPORT_AES_256)
   {
      selected = ctx->initCipher(CSCP_CIPHER_AES_256);
   }
   
   if (!selected && (ciphers & CSCP_SUPPORT_BLOWFISH_256))
   {
      selected = ctx->initCipher(CSCP_CIPHER_BLOWFISH_256);
   }
   
   if (!selected && (ciphers & CSCP_SUPPORT_AES_128))
   {
      selected = ctx->initCipher(CSCP_CIPHER_AES_128);
   }
   
   if (!selected && (ciphers & CSCP_SUPPORT_BLOWFISH_128))
   {
      selected = ctx->initCipher(CSCP_CIPHER_BLOWFISH_128);
   }
   
   if (!selected && (ciphers & CSCP_SUPPORT_IDEA))
   {
      selected = ctx->initCipher(CSCP_CIPHER_IDEA);
   }
   
   if (!selected && (ciphers & CSCP_SUPPORT_3DES))
   {
      selected = ctx->initCipher(CSCP_CIPHER_3DES);
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
CSCP_ENCRYPTED_MESSAGE *NXCPEncryptionContext::encryptMessage(CSCP_MESSAGE *msg)
{
   if (msg->wFlags & s_noEncryptionFlag)
      return (CSCP_ENCRYPTED_MESSAGE *)nx_memdup(msg, ntohl(msg->dwSize));

#ifdef _WITH_ENCRYPTION
   if (!EVP_EncryptInit_ex(&m_encryptor, NULL, NULL, m_sessionKey, m_iv))
      return NULL;

   UINT32 msgSize = ntohl(msg->dwSize);
   CSCP_ENCRYPTED_MESSAGE *emsg = 
      (CSCP_ENCRYPTED_MESSAGE *)malloc(msgSize + CSCP_ENCRYPTION_HEADER_SIZE + EVP_CIPHER_block_size(EVP_CIPHER_CTX_cipher(&m_encryptor)) + 8);
   emsg->wCode = htons(CMD_ENCRYPTED_MESSAGE);
   emsg->nReserved = 0;

   CSCP_ENCRYPTED_PAYLOAD_HEADER header;
   header.dwChecksum = htonl(CalculateCRC32((BYTE *)msg, msgSize, 0));
   header.dwReserved = 0;

   int dataSize;
   EVP_EncryptUpdate(&m_encryptor, emsg->data, &dataSize, (BYTE *)&header, CSCP_EH_ENCRYPTED_BYTES);
   msgSize = dataSize;
   EVP_EncryptUpdate(&m_encryptor, emsg->data + msgSize, &dataSize, (BYTE *)msg, ntohl(msg->dwSize));
   msgSize += dataSize;
   EVP_EncryptFinal_ex(&m_encryptor, emsg->data + msgSize, &dataSize);
   msgSize += dataSize + CSCP_EH_UNENCRYPTED_BYTES;

   if (msgSize % 8 != 0)
   {
      emsg->nPadding = (BYTE)(8 - (msgSize % 8));
      msgSize += emsg->nPadding;
   }
   else
   {
      emsg->nPadding = 0;
   }
   emsg->dwSize = htonl(msgSize);

   return emsg;
#else    /* _WITH_ENCRYPTION */
   return NULL;
#endif
}

/**
 * Decrypt message
 */
bool NXCPEncryptionContext::decryptMessage(CSCP_ENCRYPTED_MESSAGE *msg, BYTE *decryptionBuffer)
{
#ifdef _WITH_ENCRYPTION
   if (!EVP_DecryptInit_ex(&m_decryptor, NULL, NULL, m_sessionKey, m_iv))
      return false;

   msg->dwSize = ntohl(msg->dwSize);
   int dataSize;
   EVP_DecryptUpdate(&m_decryptor, decryptionBuffer, &dataSize, msg->data,
                     msg->dwSize - CSCP_EH_UNENCRYPTED_BYTES - msg->nPadding);
   EVP_DecryptFinal(&m_decryptor, decryptionBuffer + dataSize, &dataSize);

   CSCP_MESSAGE *clearMsg = (CSCP_MESSAGE *)(decryptionBuffer + CSCP_EH_ENCRYPTED_BYTES);
   UINT32 msgSize = ntohl(clearMsg->dwSize);
   if (msgSize > msg->dwSize)
      return false;  // Message decrypted incorrectly, because it can't be larger than encrypted
   UINT32 crc32 = CalculateCRC32((BYTE *)clearMsg, msgSize, 0);
   if (crc32 != ntohl(((CSCP_ENCRYPTED_PAYLOAD_HEADER *)decryptionBuffer)->dwChecksum))
      return false;  // Bad checksum

   memcpy(msg, clearMsg, msgSize);
   return true;
#else    /* _WITH_ENCRYPTION */
   return false;
#endif
}
