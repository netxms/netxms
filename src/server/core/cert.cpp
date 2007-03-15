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


//
// Static data
//

static X509_STORE *m_pTrustedCertStore = NULL;
static X509_LOOKUP *m_pCertLookup = NULL;


//
// Create X509 certificate structure from login message
//

X509 *CertificateFromLoginMessage(CSCPMessage *pMsg)
{
	DWORD dwLen;
	BYTE *pData;
	const BYTE *p;
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

	if (g_dwFlags & AF_CERT_AUTH_DISABLED)
	{
		DbgPrintf(AF_DEBUG_MISC, "Cannot validate user certificate because certificate authentication is disabled");
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

	return bValid;
}


//
// Certificate stuff initialization
//

BOOL InitCertificates(void)
{
	TCHAR szError[256], szCertFile[MAX_PATH];
	BOOL bSuccess = FALSE;

	// Root certificate file
	_tcscpy(szCertFile, g_szDataDir);
	_tcscat(szCertFile, DFILE_ROOT_CERT);

	// Create self-signed root certificate if needed
	if (g_dwFlags & AF_INTERNAL_CA)
	{
		if (_taccess(szCertFile, 0) != 0)
		{
			/* TODO: implement generation of self-signed certificate */
		}
	}

	m_pTrustedCertStore = X509_STORE_new();
	if (m_pTrustedCertStore != NULL)
	{
		m_pCertLookup = X509_STORE_add_lookup(m_pTrustedCertStore, X509_LOOKUP_file());
		if (m_pCertLookup != NULL)
		{
			if (X509_LOOKUP_load_file(m_pCertLookup, szCertFile, X509_FILETYPE_PEM))
			{
				bSuccess = TRUE;
			}
			else
			{
				WriteLog(MSG_CANNOT_LOAD_ROOT_CERT, EVENTLOG_ERROR_TYPE,
							"ss", szCertFile, ERR_error_string(ERR_get_error(), szError));
			}
		}
		else
		{
			WriteLog(MSG_CANNOT_ADD_CERT_LOOKUP, EVENTLOG_ERROR_TYPE,
						"s", ERR_error_string(ERR_get_error(), szError));
		}
	}
	else
	{
		WriteLog(MSG_CANNOT_INIT_CERT_STORE, EVENTLOG_ERROR_TYPE,
		         "s", ERR_error_string(ERR_get_error(), szError));
	}
	return bSuccess;
}


#else		/* _WITH_ENCRYPTION */


//
// Stub for certificate initialization
//

BOOL InitCertificates(void)
{
	return TRUE;
}

#endif	/* _WITH_ENCRYPTION */
