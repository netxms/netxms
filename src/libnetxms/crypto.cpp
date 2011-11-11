/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2010 Victor Kirhenshtein
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


//
// Constants
//

#define KEY_BUFFER_SIZE       4096


//
// Supported ciphers. By default, we support all ciphers compiled
// into OpenSSL library.
//

static DWORD m_dwSupportedCiphers = 
#ifdef _WITH_ENCRYPTION
#ifndef OPENSSL_NO_AES
   CSCP_SUPPORT_AES_256 |
#endif
#ifndef OPENSSL_NO_BF
   CSCP_SUPPORT_BLOWFISH |
#endif
#ifndef OPENSSL_NO_IDEA
   CSCP_SUPPORT_IDEA |
#endif
#ifndef OPENSSL_NO_DES
   CSCP_SUPPORT_3DES |
#endif
#endif   /* _WITH_ENCRYPTION */
   0;


//
// Static data
//

#ifdef _WITH_ENCRYPTION

typedef OPENSSL_CONST EVP_CIPHER * (*CIPHER_FUNC)(void);
static CIPHER_FUNC m_pfCipherList[NETXMS_MAX_CIPHERS] =
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
   EVP_des_ede3_cbc
#else
   NULL
#endif
};
static WORD m_wNoEncryptionFlag = 0;
static MUTEX *m_pCryptoMutexList = NULL;


//
// Locking callback for CRYPTO library
//

static void CryptoLockingCallback(int nMode, int nLock, const char *pszFile, int nLine)
{
   if (nMode & CRYPTO_LOCK)
      MutexLock(m_pCryptoMutexList[nLock]);
   else
      MutexUnlock(m_pCryptoMutexList[nLock]);
}


//
// ID callback for CRYPTO library
//

#ifndef _WIN32

static unsigned long CryptoIdCallback(void)
{
   return (unsigned long)GetCurrentThreadId();
}

#endif

#endif   /* _WITH_ENCRYPTION */


//
// Initialize OpenSSL library
//

BOOL LIBNETXMS_EXPORTABLE InitCryptoLib(DWORD dwEnabledCiphers)
{
#ifdef _WITH_ENCRYPTION
   BYTE random[8192];
   int i;

   CRYPTO_malloc_init();
   ERR_load_CRYPTO_strings();
   OpenSSL_add_all_algorithms();
   RAND_seed(random, 8192);
   m_dwSupportedCiphers &= dwEnabledCiphers;
   m_wNoEncryptionFlag = htons(MF_DONT_ENCRYPT);
   m_pCryptoMutexList = (MUTEX *)malloc(sizeof(MUTEX) * CRYPTO_num_locks());
   for(i = 0; i < CRYPTO_num_locks(); i++)
      m_pCryptoMutexList[i] = MutexCreate();
   CRYPTO_set_locking_callback(CryptoLockingCallback);
#ifndef _WIN32
   CRYPTO_set_id_callback(CryptoIdCallback);
#endif   /* _WIN32 */
#endif   /* _WITH_ENCRYPTION */
   return TRUE;
}


//
// Get supported ciphers
//

DWORD LIBNETXMS_EXPORTABLE CSCPGetSupportedCiphers(void)
{
   return m_dwSupportedCiphers;
}


//
// Encrypt message
//

CSCP_ENCRYPTED_MESSAGE LIBNETXMS_EXPORTABLE *CSCPEncryptMessage(NXCPEncryptionContext *pCtx, CSCP_MESSAGE *pMsg)
{
#ifdef _WITH_ENCRYPTION
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;
   CSCP_ENCRYPTED_PAYLOAD_HEADER header;
   int nSize;
   EVP_CIPHER_CTX cipher;
   DWORD dwMsgSize;

   if (pMsg->wFlags & m_wNoEncryptionFlag)
      return (CSCP_ENCRYPTED_MESSAGE *)nx_memdup(pMsg, ntohl(pMsg->dwSize));

   if (m_pfCipherList[pCtx->getCipher()] == NULL)
      return NULL;   // Unsupported cipher

   EVP_EncryptInit(&cipher, m_pfCipherList[pCtx->getCipher()](), pCtx->getSessionKey(), pCtx->getIV());
   EVP_CIPHER_CTX_set_key_length(&cipher, pCtx->getKeyLength());

   dwMsgSize = ntohl(pMsg->dwSize);
   pEnMsg = (CSCP_ENCRYPTED_MESSAGE *)malloc(dwMsgSize + CSCP_ENCRYPTION_HEADER_SIZE + 
                     EVP_CIPHER_block_size(EVP_CIPHER_CTX_cipher(&cipher)) + 8);
   pEnMsg->wCode = htons(CMD_ENCRYPTED_MESSAGE);
   pEnMsg->nReserved = 0;

   header.dwChecksum = htonl(CalculateCRC32((BYTE *)pMsg, dwMsgSize, 0));
   header.dwReserved = 0;
   EVP_EncryptUpdate(&cipher, pEnMsg->data, &nSize, (BYTE *)&header, CSCP_EH_ENCRYPTED_BYTES);
   dwMsgSize = nSize;
   EVP_EncryptUpdate(&cipher, pEnMsg->data + dwMsgSize, &nSize, (BYTE *)pMsg, ntohl(pMsg->dwSize));
   dwMsgSize += nSize;
   EVP_EncryptFinal(&cipher, pEnMsg->data + dwMsgSize, &nSize);
   dwMsgSize += nSize + CSCP_EH_UNENCRYPTED_BYTES;
   EVP_CIPHER_CTX_cleanup(&cipher);

   if (dwMsgSize % 8 != 0)
   {
      pEnMsg->nPadding = (BYTE)(8 - (dwMsgSize % 8));
      dwMsgSize += pEnMsg->nPadding;
   }
   else
   {
      pEnMsg->nPadding = 0;
   }
   pEnMsg->dwSize = htonl(dwMsgSize);

   return pEnMsg;
#else    /* _WITH_ENCRYPTION */
   return NULL;
#endif
}


//
// Decrypt message
//

BOOL LIBNETXMS_EXPORTABLE CSCPDecryptMessage(NXCPEncryptionContext *pCtx,
                                             CSCP_ENCRYPTED_MESSAGE *pMsg,
                                             BYTE *pDecryptionBuffer)
{
#ifdef _WITH_ENCRYPTION
   int nSize;
   EVP_CIPHER_CTX cipher;
   DWORD dwChecksum, dwMsgSize;
   CSCP_MESSAGE *pClearMsg;

   if (m_pfCipherList[pCtx->getCipher()] == NULL)
      return FALSE;   // Unsupported cipher

   pMsg->dwSize = ntohl(pMsg->dwSize);
   EVP_DecryptInit(&cipher, m_pfCipherList[pCtx->getCipher()](), pCtx->getSessionKey(), pCtx->getIV());
   EVP_CIPHER_CTX_set_key_length(&cipher, pCtx->getKeyLength());
   EVP_DecryptUpdate(&cipher, pDecryptionBuffer, &nSize, pMsg->data,
                     pMsg->dwSize - CSCP_EH_UNENCRYPTED_BYTES - pMsg->nPadding);
   EVP_DecryptFinal(&cipher, pDecryptionBuffer + nSize, &nSize);
   EVP_CIPHER_CTX_cleanup(&cipher);

   pClearMsg = (CSCP_MESSAGE *)(pDecryptionBuffer + CSCP_EH_ENCRYPTED_BYTES);
   dwMsgSize = ntohl(pClearMsg->dwSize);
   if (dwMsgSize > pMsg->dwSize)
      return FALSE;  // Message decrypted incorrectly, because it can't be larger than encrypted
   dwChecksum = CalculateCRC32((BYTE *)pClearMsg, dwMsgSize, 0);
   if (dwChecksum != ntohl(((CSCP_ENCRYPTED_PAYLOAD_HEADER *)pDecryptionBuffer)->dwChecksum))
      return FALSE;  // Bad checksum

   memcpy(pMsg, pClearMsg, dwMsgSize);
   return TRUE;
#else    /* _WITH_ENCRYPTION */
   return FALSE;
#endif
}


//
// Setup encryption context
// Function will determine is it called on initiator or responder side
// by message code. Initiator should provide it's private key,
// and responder should provide pointer to response message.
//

DWORD LIBNETXMS_EXPORTABLE SetupEncryptionContext(CSCPMessage *pMsg, 
                                                  NXCPEncryptionContext **ppCtx,
                                                  CSCPMessage **ppResponse,
                                                  RSA *pPrivateKey, int nNXCPVersion)
{
   DWORD dwResult = RCC_NOT_IMPLEMENTED;

	*ppCtx = NULL;
#ifdef _WITH_ENCRYPTION
   if (pMsg->GetCode() == CMD_REQUEST_SESSION_KEY)
   {
      DWORD dwCiphers;

      *ppResponse = new CSCPMessage(nNXCPVersion);
      (*ppResponse)->SetCode(CMD_SESSION_KEY);
      (*ppResponse)->SetId(pMsg->GetId());
      (*ppResponse)->DisableEncryption();

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
			DWORD dwKeySize;

			*ppCtx = NXCPEncryptionContext::create(dwCiphers);

         // Encrypt key
         dwKeySize = pMsg->GetVariableBinary(VID_PUBLIC_KEY, ucKeyBuffer, KEY_BUFFER_SIZE);
         pBufPos = ucKeyBuffer;
         pServerKey = d2i_RSAPublicKey(NULL, (OPENSSL_CONST BYTE **)&pBufPos, dwKeySize);
         if (pServerKey != NULL)
         {
            (*ppResponse)->SetVariable(VID_RCC, RCC_SUCCESS);
            dwKeySize = RSA_public_encrypt((*ppCtx)->getKeyLength(), (*ppCtx)->getSessionKey(),
                                           ucKeyBuffer, pServerKey, RSA_PKCS1_OAEP_PADDING);
            (*ppResponse)->SetVariable(VID_SESSION_KEY, ucKeyBuffer, dwKeySize);
            dwKeySize = RSA_public_encrypt(EVP_MAX_IV_LENGTH, (*ppCtx)->getIV(),
                                           ucKeyBuffer, pServerKey, RSA_PKCS1_OAEP_PADDING);
            (*ppResponse)->SetVariable(VID_SESSION_IV, ucKeyBuffer, dwKeySize);
            (*ppResponse)->SetVariable(VID_CIPHER, (WORD)(*ppCtx)->getCipher());
            (*ppResponse)->SetVariable(VID_KEY_LENGTH, (WORD)(*ppCtx)->getKeyLength());
            (*ppResponse)->SetVariable(VID_IV_LENGTH, (WORD)EVP_MAX_IV_LENGTH);
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
      (*ppResponse)->DisableEncryption();
      (*ppResponse)->SetVariable(VID_RCC, dwResult);
   }
#endif

   return dwResult;
}


//
// Prepare session key request message
//

void LIBNETXMS_EXPORTABLE PrepareKeyRequestMsg(CSCPMessage *pMsg, RSA *pServerKey)
{
#ifdef _WITH_ENCRYPTION
   int iLen;
   BYTE *pKeyBuffer, *pBufPos;

   pMsg->SetCode(CMD_REQUEST_SESSION_KEY);
   pMsg->SetVariable(VID_SUPPORTED_ENCRYPTION, m_dwSupportedCiphers);

   iLen = i2d_RSAPublicKey(pServerKey, NULL);
   pKeyBuffer = (BYTE *)malloc(iLen);
   pBufPos = pKeyBuffer;
   i2d_RSAPublicKey(pServerKey, &pBufPos);
   pMsg->SetVariable(VID_PUBLIC_KEY, pKeyBuffer, iLen);
   free(pKeyBuffer);
#endif
}


//
// Load RSA key(s) from file
//

RSA LIBNETXMS_EXPORTABLE *LoadRSAKeys(const TCHAR *pszKeyFile)
{
#ifdef _WITH_ENCRYPTION
   FILE *fp;
   BYTE *pKeyBuffer, *pBufPos, hash[SHA1_DIGEST_SIZE];
   DWORD dwLen;
   RSA *pKey = NULL;

   fp = _tfopen(pszKeyFile, _T("rb"));
   if (fp != NULL)
   {
      if (fread(&dwLen, 1, sizeof(DWORD), fp) == sizeof(DWORD) && dwLen < 10 * 1024)
      {
         pKeyBuffer = (BYTE *)malloc(dwLen);
         pBufPos = pKeyBuffer;
         if (fread(pKeyBuffer, 1, dwLen, fp) == dwLen)
         {
            BYTE hash2[SHA1_DIGEST_SIZE];

            fread(hash, 1, SHA1_DIGEST_SIZE, fp);
            CalculateSHA1Hash(pKeyBuffer, dwLen, hash2);
            if (!memcmp(hash, hash2, SHA1_DIGEST_SIZE))
            {
               pKey = d2i_RSAPublicKey(NULL, (OPENSSL_CONST BYTE **)&pBufPos, dwLen);
               if (pKey != NULL)
               {
                  if (d2i_RSAPrivateKey(&pKey, (OPENSSL_CONST BYTE **)&pBufPos,
                                        dwLen - CAST_FROM_POINTER((pBufPos - pKeyBuffer), DWORD)) == NULL)
                  {
                     RSA_free(pKey);
                     pKey = NULL;
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


//
// Create signature for message using certificate (MS CAPI version)
// Paraeters:
//    pMsg and dwMsgLen - message to sign and it's length
//    pCert - certificate
//    pBuffer - output buffer
//    dwBufSize - buffer size
//    pdwSigLen - actual signature size
//

#ifdef _WIN32

BOOL LIBNETXMS_EXPORTABLE SignMessageWithCAPI(BYTE *pMsg, DWORD dwMsgLen, const CERT_CONTEXT *pCert,
												          BYTE *pBuffer, DWORD dwBufSize, DWORD *pdwSigLen)
{
	BOOL bFreeProv, bRet = FALSE;
	DWORD i, j, dwLen, dwKeySpec;
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;
	BYTE *pTemp;

	if (CryptAcquireCertificatePrivateKey(pCert, CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
		                                   NULL, &hProv, &dwKeySpec, &bFreeProv))
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


//
// Encrypt data block with ICE
//

void LIBNETXMS_EXPORTABLE ICEEncryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key)
{
	ICE_KEY *ice = ice_key_create(1);
	ice_key_set(ice, key);

	int stopPos = inLen - (inLen % 8);
	const BYTE *curr = in;
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


//
// Decrypt data block with ICE
//

void LIBNETXMS_EXPORTABLE ICEDecryptData(const BYTE *in, int inLen, BYTE *out, const BYTE *key)
{
	ICE_KEY *ice = ice_key_create(1);
	ice_key_set(ice, key);

	int stopPos = inLen - (inLen % 8);
	const BYTE *curr = in;
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


//
// Encryption context constructor
//

NXCPEncryptionContext::NXCPEncryptionContext()
{
	m_sessionKey = NULL;
}


//
// Encryption context destructor
//

NXCPEncryptionContext::~NXCPEncryptionContext()
{
	safe_free(m_sessionKey);
}


//
// Create encryption context from CMD_SESSION_KEY NXCP message
//

NXCPEncryptionContext *NXCPEncryptionContext::create(CSCPMessage *msg, RSA *privateKey)
{
   BYTE ucKeyBuffer[KEY_BUFFER_SIZE], ucSessionKey[KEY_BUFFER_SIZE];
   DWORD dwKeySize;
   int nSize, nIVLen;
	NXCPEncryptionContext *ctx = new NXCPEncryptionContext;

   ctx->m_cipher = msg->GetVariableShort(VID_CIPHER);
   ctx->m_keyLength = msg->GetVariableShort(VID_KEY_LENGTH);
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
          (nIVLen <= EVP_CIPHER_iv_length(m_pfCipherList[ctx->m_cipher]())))
      {
         memcpy(ctx->m_iv, ucSessionKey, min(EVP_MAX_IV_LENGTH, nIVLen));
      }
      else
      {
         delete_and_null(ctx);
      }
   }
   else
   {
      delete_and_null(ctx);
   }
	return ctx;
}


//
// Create encryption context from CMD_REQUEST_SESSION_KEY NXCP message
//

NXCPEncryptionContext *NXCPEncryptionContext::create(DWORD ciphers)
{
	NXCPEncryptionContext *ctx = new NXCPEncryptionContext;

   // Select cipher
   if (ciphers & CSCP_SUPPORT_AES_256)
   {
      ctx->m_cipher = CSCP_CIPHER_AES_256;
      ctx->m_keyLength = 32;
   }
   else if (ciphers & CSCP_SUPPORT_BLOWFISH)
   {
      ctx->m_cipher = CSCP_CIPHER_BLOWFISH;
      ctx->m_keyLength = 32;
   }
   else if (ciphers & CSCP_SUPPORT_IDEA)
   {
      ctx->m_cipher = CSCP_CIPHER_IDEA;
      ctx->m_keyLength = 16;
   }
   else if (ciphers & CSCP_SUPPORT_3DES)
   {
      ctx->m_cipher = CSCP_CIPHER_3DES;
      ctx->m_keyLength = 24;
   }

   // Generate key
   ctx->m_sessionKey = (BYTE *)malloc(ctx->m_keyLength);
   RAND_bytes(ctx->m_sessionKey, ctx->m_keyLength);
   RAND_bytes(ctx->m_iv, EVP_MAX_IV_LENGTH);

	return ctx;
}
