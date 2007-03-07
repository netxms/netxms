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

BOOL ValidateUserCertificate(X509 *pCert, TCHAR *pszLogin, BYTE *pChallenge, BYTE *pSignature, DWORD dwSigLen)
{
	EVP_PKEY *pKey;
	BYTE hash[SHA1_DIGEST_SIZE];
	BOOL bValid = FALSE;

	DbgPrintf(AF_DEBUG_MISC, "Validating certificate \"%s\" for user %s", CHECK_NULL(pCert->name), pszLogin);

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

	return bValid;
}


#endif	/* _WITH_ENCRYPTION */
