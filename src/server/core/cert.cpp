/* 
** NetXMS - Network Management System
** Copyright (C) 2007 Victor Kirhenshtein
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


//
// Static data
//

static X509_STORE *m_pTrustedCertStore = NULL;
static MUTEX m_mutexStoreAccess = INVALID_MUTEX_HANDLE;


//
// Create X509 certificate structure from login message
//

X509 *CertificateFromLoginMessage(CSCPMessage *pMsg)
{
	DWORD dwLen;
	BYTE *pData;
	OPENSSL_CONST BYTE *p;
	X509 *pCert = NULL;

	dwLen = pMsg->GetVariableBinary(VID_CERTIFICATE, NULL, 0);
	if (dwLen > 0)
	{
		pData = (BYTE *)malloc(dwLen);
		pMsg->GetVariableBinary(VID_CERTIFICATE, pData, dwLen);
		p = pData;
		pCert = d2i_X509(NULL, &p, dwLen);
		free(pData);
	}
	return pCert;
}


//
// Validate user's certificate
//

BOOL ValidateUserCertificate(X509 *pCert, TCHAR *pszLogin, BYTE *pChallenge, BYTE *pSignature,
									  DWORD dwSigLen, int nMappingMethod, TCHAR *pszMappingData)
{
	EVP_PKEY *pKey;
	BYTE hash[SHA1_DIGEST_SIZE];
	BOOL bValid = FALSE;

	DbgPrintf(AF_DEBUG_MISC, "Validating certificate \"%s\" for user %s", CHECK_NULL(pCert->name), pszLogin);
	MutexLock(m_mutexStoreAccess, INFINITE);

	if (m_pTrustedCertStore == NULL)
	{
		DbgPrintf(AF_DEBUG_MISC, "Cannot validate user certificate because certificate store is not initialized");
		MutexUnlock(m_mutexStoreAccess);
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
				DbgPrintf(AF_DEBUG_MISC, "Unknown key type %d in certificate \"%s\" for user %s", pKey->type, CHECK_NULL(pCert->name), pszLogin);
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
			DbgPrintf(AF_DEBUG_MISC, "Certificate \"%s\" for user %s - validation %s",
			          CHECK_NULL(pCert->name), pszLogin, bValid ? "successful" : "failed");
		}
		else
		{
			char szBuffer[256];

			DbgPrintf(AF_DEBUG_MISC, "X509_STORE_CTX_new() failed: %s", ERR_error_string(ERR_get_error(), szBuffer));
			bValid = FALSE;
		}
	}

	// Check user mapping
	if (bValid)
	{
		switch(nMappingMethod)
		{
			case USER_MAP_CERT_BY_SUBJECT:
				bValid = !_tcsicmp(CHECK_NULL(pCert->name), CHECK_NULL_EX(pszMappingData));
				break;
			default:
				DbgPrintf(AF_DEBUG_MISC, "Invalid certificate mapping method %d for user %s", nMappingMethod, pszLogin);
				bValid = FALSE;
				break;
		}
	}

	MutexUnlock(m_mutexStoreAccess);
	return bValid;
}


//
// Reload certificates from database
//

void ReloadCertificates(void)
{
	BYTE *pBinCert;
	OPENSSL_CONST BYTE *p;
	DB_RESULT hResult;
	int i, nRows, nLoaded;
	DWORD dwLen;
	X509 *pCert;
	TCHAR szBuffer[256], szSubject[256], *pszCertData;

	MutexLock(m_mutexStoreAccess, INFINITE);

	if (m_pTrustedCertStore != NULL)
		X509_STORE_free(m_pTrustedCertStore);

	m_pTrustedCertStore = X509_STORE_new();
	if (m_pTrustedCertStore != NULL)
	{
		_stprintf(szBuffer, _T("SELECT cert_data,subject FROM certificates WHERE cert_type=%d"), CERT_TYPE_TRUSTED_CA);
		hResult = DBSelect(g_hCoreDB, szBuffer);
		if (hResult != NULL)
		{
			nRows = DBGetNumRows(hResult);
			for(i = 0, nLoaded = 0; i < nRows; i++)
			{
				pszCertData = DBGetField(hResult, i, 0, NULL, 0);
				if (pszCertData != NULL)
				{
					dwLen = (DWORD)_tcslen(pszCertData);
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
							WriteLog(MSG_CANNOT_ADD_CERT, EVENTLOG_ERROR_TYPE,
										"ss", DBGetField(hResult, i, 1, szSubject, 256),
										_ERR_error_tstring(ERR_get_error(), szBuffer));
						}
					}
					else
					{
						WriteLog(MSG_CANNOT_LOAD_CERT, EVENTLOG_ERROR_TYPE,
									"ss", DBGetField(hResult, i, 1, szSubject, 256),
									_ERR_error_tstring(ERR_get_error(), szBuffer));
					}
				}
			}
			DBFreeResult(hResult);

			if (nLoaded > 0)
				WriteLog(MSG_CA_CERTIFICATES_LOADED, EVENTLOG_INFORMATION_TYPE, "d", nLoaded);
		}
	}
	else
	{
		WriteLog(MSG_CANNOT_INIT_CERT_STORE, EVENTLOG_ERROR_TYPE,
		         "s", _ERR_error_tstring(ERR_get_error(), szBuffer));
	}

	MutexUnlock(m_mutexStoreAccess);
}


//
// Certificate stuff initialization
//

void InitCertificates(void)
{
	m_mutexStoreAccess = MutexCreate();

	// Create self-signed root certificate if needed
	if (g_dwFlags & AF_INTERNAL_CA)
	{
		/* TODO: implement generation of self-signed certificate */
	}

	ReloadCertificates();
}


#else		/* _WITH_ENCRYPTION */


//
// Stub for certificate initialization
//

void InitCertificates(void)
{
}

#endif	/* _WITH_ENCRYPTION */
