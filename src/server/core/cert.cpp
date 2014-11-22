/* 
** NetXMS - Network Management System
** Copyright (C) 2007-2010 Victor Kirhenshtein
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
** File: cert.cpp
**
**/

#include "nxcore.h"

#ifdef _WITH_ENCRYPTION

// WARNING! this hack works only for d2i_X509(); be carefull when adding new code
#ifdef OPENSSL_CONST
# undef OPENSSL_CONST
#endif
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
# define OPENSSL_CONST const
#else
# define OPENSSL_CONST
#endif

/**
 * Static data
 */
static X509_STORE *m_pTrustedCertStore = NULL;
static MUTEX m_mutexStoreAccess = INVALID_MUTEX_HANDLE;

/**
 * Create X509 certificate structure from login message
 */
X509 *CertificateFromLoginMessage(NXCPMessage *pMsg)
{
	UINT32 dwLen;
	BYTE *pData;
	OPENSSL_CONST BYTE *p;
	X509 *pCert = NULL;

	dwLen = pMsg->getFieldAsBinary(VID_CERTIFICATE, NULL, 0);
	if (dwLen > 0)
	{
		pData = (BYTE *)malloc(dwLen);
		pMsg->getFieldAsBinary(VID_CERTIFICATE, pData, dwLen);
		p = pData;
		pCert = d2i_X509(NULL, &p, dwLen);
		free(pData);
	}
	return pCert;
}

/**
 * Check public key
 */
static BOOL CheckPublicKey(EVP_PKEY *key, const TCHAR *mappingData)
{
	int pkeyLen;
	unsigned char *ucBuf, *uctempBuf;
	TCHAR *pkeyText;
	BOOL valid;
	
	pkeyLen = i2d_PublicKey(key, NULL);
	ucBuf = (unsigned char *)malloc(pkeyLen +1);
	uctempBuf = ucBuf;
	i2d_PublicKey(key, &uctempBuf);
	
	pkeyText = (TCHAR *)malloc((pkeyLen * 2 + 1) * sizeof(TCHAR));
	BinToStr(ucBuf, pkeyLen, pkeyText);

	valid = !_tcscmp(pkeyText, mappingData);

	free(ucBuf);
	free(pkeyText);

	return valid;
}

/**
 * Check ciertificate's CN
 */
static BOOL CheckCommonName(X509 *cert, const TCHAR *cn)
{
   X509_NAME *subject = X509_get_subject_name(cert);
   if (subject == NULL)
      return FALSE;

   int idx = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
   if (idx == -1)
      return FALSE;

   X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, idx);
   if (entry == NULL)
      return FALSE;

   ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
   if (data == NULL)
      return FALSE;

   unsigned char *utf8CertCN;
   ASN1_STRING_to_UTF8(&utf8CertCN, data);
#ifdef UNICODE
   DbgPrintf(3, _T("Certificate CN=\"%hs\", user CN=\"%s\""), utf8CertCN, cn);
   char *utf8UserCN = UTF8StringFromWideString(cn);
   BOOL success = !stricmp((char *)utf8CertCN, utf8UserCN);
   free(utf8UserCN);
#else
   DbgPrintf(3, _T("Certificate CN=\"%s\", user CN=\"%s\""), utf8CertCN, cn);
   BOOL success = !stricmp((char *)utf8CertCN, cn);
#endif
   OPENSSL_free(utf8CertCN);
   return success;
}

/**
 * Validate user's certificate
 */
BOOL ValidateUserCertificate(X509 *pCert, const TCHAR *pszLogin, BYTE *pChallenge, BYTE *pSignature,
									  UINT32 dwSigLen, int nMappingMethod, const TCHAR *pszMappingData)
{
	EVP_PKEY *pKey;
	BYTE hash[SHA1_DIGEST_SIZE];
	BOOL bValid = FALSE;

#ifdef UNICODE
	WCHAR *certSubject = WideStringFromMBString(CHECK_NULL_A(pCert->name));
#else
#define certSubject (CHECK_NULL(pCert->name))
#endif

	DbgPrintf(3, _T("Validating certificate \"%s\" for user %s"), certSubject, pszLogin);
	MutexLock(m_mutexStoreAccess);

	if (m_pTrustedCertStore == NULL)
	{
		DbgPrintf(3, _T("Cannot validate user certificate because certificate store is not initialized"));
		MutexUnlock(m_mutexStoreAccess);
#ifdef UNICODE
		free(certSubject);
#endif
		return FALSE;
	}

	// Validate signature
	pKey = X509_get_pubkey(pCert);
	if (pKey != NULL)
	{
		CalculateSHA1Hash(pChallenge, CLIENT_CHALLENGE_SIZE, hash);
		switch(pKey->type)
		{
			case EVP_PKEY_RSA:
				bValid = RSA_verify(NID_sha1, hash, SHA1_DIGEST_SIZE, pSignature, dwSigLen, pKey->pkey.rsa);
				break;
			default:
				DbgPrintf(3, _T("Unknown key type %d in certificate \"%s\" for user %s"), pKey->type, certSubject, pszLogin);
				break;
		}
	}

	// Validate certificate
	if (bValid)
	{
		X509_STORE_CTX *pStore;

		pStore = X509_STORE_CTX_new();
		if (pStore != NULL)
		{
			X509_STORE_CTX_init(pStore, m_pTrustedCertStore, pCert, NULL);
			bValid = X509_verify_cert(pStore);
			X509_STORE_CTX_free(pStore);
			DbgPrintf(3, _T("Certificate \"%s\" for user %s - validation %s"),
			          certSubject, pszLogin, bValid ? _T("successful") : _T("failed"));
		}
		else
		{
			TCHAR szBuffer[256];

			DbgPrintf(3, _T("X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), szBuffer));
			bValid = FALSE;
		}
	}

	// Check user mapping
	if (bValid)
	{
		switch(nMappingMethod)
		{
			case USER_MAP_CERT_BY_SUBJECT:
				bValid = !_tcsicmp(certSubject, CHECK_NULL_EX(pszMappingData));
				break;
			case USER_MAP_CERT_BY_PUBKEY:
				bValid = CheckPublicKey(pKey, CHECK_NULL_EX(pszMappingData));
				break;
			case USER_MAP_CERT_BY_CN:
            bValid = CheckCommonName(pCert, ((pszMappingData != NULL) && (*pszMappingData != 0)) ? pszMappingData : pszLogin);
				break;
			default:
				DbgPrintf(3, _T("Invalid certificate mapping method %d for user %s"), nMappingMethod, pszLogin);
				bValid = FALSE;
				break;
		}
	}

	MutexUnlock(m_mutexStoreAccess);

#ifdef UNICODE
	free(certSubject);
#endif

	return bValid;
#undef certSubject
}


//
// Reload certificates from database
//

void ReloadCertificates()
{
	BYTE *pBinCert;
	OPENSSL_CONST BYTE *p;
	DB_RESULT hResult;
	int i, nRows, nLoaded;
	UINT32 dwLen;
	X509 *pCert;
	TCHAR szBuffer[256], szSubject[256], *pszCertData;

	MutexLock(m_mutexStoreAccess);

	if (m_pTrustedCertStore != NULL)
		X509_STORE_free(m_pTrustedCertStore);

	m_pTrustedCertStore = X509_STORE_new();
	if (m_pTrustedCertStore != NULL)
	{
		_sntprintf(szBuffer, 256, _T("SELECT cert_data,subject FROM certificates WHERE cert_type=%d"), CERT_TYPE_TRUSTED_CA);
		hResult = DBSelect(g_hCoreDB, szBuffer);
		if (hResult != NULL)
		{
			nRows = DBGetNumRows(hResult);
			for(i = 0, nLoaded = 0; i < nRows; i++)
			{
				pszCertData = DBGetField(hResult, i, 0, NULL, 0);
				if (pszCertData != NULL)
				{
					dwLen = (UINT32)_tcslen(pszCertData);
					pBinCert = (BYTE *)malloc(dwLen);
					StrToBin(pszCertData, pBinCert, dwLen);
					free(pszCertData);
					p = pBinCert;
					pCert = d2i_X509(NULL, &p, dwLen);
					free(pBinCert);
					if (pCert != NULL)
					{
						if (X509_STORE_add_cert(m_pTrustedCertStore, pCert))
						{
							nLoaded++;
						}
						else
						{
							nxlog_write(MSG_CANNOT_ADD_CERT, EVENTLOG_ERROR_TYPE,
										"ss", DBGetField(hResult, i, 1, szSubject, 256),
										_ERR_error_tstring(ERR_get_error(), szBuffer));
						}
					}
					else
					{
						nxlog_write(MSG_CANNOT_LOAD_CERT, EVENTLOG_ERROR_TYPE,
									"ss", DBGetField(hResult, i, 1, szSubject, 256),
									_ERR_error_tstring(ERR_get_error(), szBuffer));
					}
				}
			}
			DBFreeResult(hResult);

			if (nLoaded > 0)
				nxlog_write(MSG_CA_CERTIFICATES_LOADED, EVENTLOG_INFORMATION_TYPE, "d", nLoaded);
		}
	}
	else
	{
		nxlog_write(MSG_CANNOT_INIT_CERT_STORE, EVENTLOG_ERROR_TYPE, "s", _ERR_error_tstring(ERR_get_error(), szBuffer));
	}

	MutexUnlock(m_mutexStoreAccess);
}

/**
 * Certificate stuff initialization
 */
void InitCertificates()
{
	m_mutexStoreAccess = MutexCreate();
	ReloadCertificates();
}

#else		/* _WITH_ENCRYPTION */

/**
 * Stub for certificate initialization
 */
void InitCertificates()
{
}

#endif	/* _WITH_ENCRYPTION */
