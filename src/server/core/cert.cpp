/* 
** NetXMS - Network Management System
** Copyright (C) 2007-2017 Victor Kirhenshtein
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
#include <nxcrypto.h>

#define DEBUG_TAG    _T("crypto.cert")

#ifdef _WITH_ENCRYPTION

// WARNING! this hack works only for d2i_X509(); be careful when adding new code
#ifdef OPENSSL_CONST
# undef OPENSSL_CONST
#endif
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
# define OPENSSL_CONST const
#else
# define OPENSSL_CONST
#endif

#if OPENSSL_VERSION_NUMBER < 0x10000000L
inline int EVP_PKEY_id(EVP_PKEY *key)
{
   return key->type;
}
#endif

#if OPENSSL_VERSION_NUMBER < 0x10100000L
inline ASN1_TIME *X509_getm_notBefore(const X509 *x)
{
   return X509_get_notBefore(x);
}

inline ASN1_TIME *X509_getm_notAfter(const X509 *x)
{
   return X509_get_notAfter(x);
}
#endif

/**
 * Server certificate file information
 */
TCHAR *g_serverCACertificatesPath = NULL;
TCHAR g_serverCertificatePath[MAX_PATH] = _T("");
TCHAR g_serverCertificateKeyPath[MAX_PATH] = _T("");
char g_serverCertificatePassword[MAX_PASSWORD] = "";

/**
 * Server certificate
 */
static ObjectRefArray<X509> s_serverCACertificates(8, 8);
static X509 *s_serverCertificate = NULL;
static EVP_PKEY *s_serverCertificateKey = NULL;

/**
 * Trusted CA certificate store
 */
static X509_STORE *s_trustedCertificateStore = NULL;
static Mutex s_certificateStoreLock;

/**
 * Issue certificate signed with server's certificate
 */
X509 *IssueCertificate(X509_REQ *request, const char *ou, const char *cn, int days)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: new certificate request (CN override: %hs, OU override: %hs)"),
            (cn != NULL) ? cn : "<not set>", (ou != NULL) ? ou : "<not set>");

   X509_NAME *requestSubject = X509_REQ_get_subject_name(request);
   if (requestSubject == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot get subject from certificate request"));
      return NULL;
   }

   X509 *cert = X509_new();
   if (cert == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_new() failed"));
      return NULL;
   }

   if (X509_set_version(cert, 2) != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_version() failed"));
      X509_free(cert);
      return NULL;
   }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   ASN1_INTEGER *serial = ASN1_INTEGER_new();
#else
   ASN1_INTEGER *serial = M_ASN1_INTEGER_new();
#endif
   ASN1_INTEGER_set(serial, 0);
   int rc = X509_set_serialNumber(cert, serial);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   ASN1_INTEGER_free(serial);
#else
   M_ASN1_INTEGER_free(serial);
#endif
   if (rc != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot set certificate serial number"));
      X509_free(cert);
      return NULL;
   }

   X509_NAME *subject;
   if ((cn != NULL) || (ou != NULL))
   {
      subject = X509_NAME_dup(requestSubject);
      if (subject != NULL)
      {
         if (ou != NULL)
         {
            int idx = X509_NAME_get_index_by_NID(subject, NID_organizationalUnitName, -1);
            if (idx != -1)
               X509_NAME_delete_entry(subject, idx);
            if (!X509_NAME_add_entry_by_txt(subject, "OU", MBSTRING_UTF8, (const BYTE *)ou, -1, -1, 0))
               nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: X509_NAME_add_entry_by_txt failed for OU=%hs"), ou);
         }
         if (cn != NULL)
         {
            int idx = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
            if (idx != -1)
               X509_NAME_delete_entry(subject, idx);
            if (!X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_UTF8, (const BYTE *)cn, -1, -1, 0))
               nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: X509_NAME_add_entry_by_txt failed for CN=%hs"), cn);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_NAME_dup() failed"));
      }
   }
   else
   {
      subject = requestSubject;
   }

   if (subject == NULL)
   {
      X509_free(cert);
      return NULL;
   }

   rc = X509_set_subject_name(cert, subject);
   if (subject != requestSubject)
      X509_NAME_free(subject);
   if (rc != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_subject_name() failed"));
      X509_free(cert);
      return NULL;
   }

   X509_NAME *issuerName = X509_get_subject_name(s_serverCertificate);
   if (issuerName == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot get CA subject name"));
      X509_free(cert);
      return NULL;
   }

   if (X509_set_issuer_name(cert, issuerName) != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_issuer_name() failed"));
      X509_free(cert);
      return NULL;
   }

   EVP_PKEY *pubkey = X509_REQ_get_pubkey(request);
   if (pubkey == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_REQ_get_pubkey() failed"));
      X509_free(cert);
      return NULL;
   }

   if (X509_REQ_verify(request, pubkey) != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: certificate request verification failed"));
      EVP_PKEY_free(pubkey);
      X509_free(cert);
      return NULL;
   }

   rc = X509_set_pubkey(cert, pubkey);
   EVP_PKEY_free(pubkey);
   if (rc != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_pubkey() failed"));
      X509_free(cert);
      return NULL;
   }

   if (X509_gmtime_adj(X509_getm_notBefore(cert), 0) == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot set start time"));
      X509_free(cert);
      return NULL;
   }

   if (X509_gmtime_adj(X509_getm_notAfter(cert), days * 86400) == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot set end time"));
      X509_free(cert);
      return NULL;
   }

   if (X509_sign(cert, s_serverCertificateKey, EVP_sha256()) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_sign() failed"));
      X509_free(cert);
      return NULL;
   }

   char subjectName[1024];
   X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 1024);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: new certificate with subject \"%hs\" issued successfully"), subjectName);
   return cert;
}

/**
 * Get CN from certificate
 */
bool GetCertificateSubjectField(X509 *cert, int nid, TCHAR *buffer, size_t size)
{
   X509_NAME *subject = X509_get_subject_name(cert);
   if (subject == NULL)
      return false;

   int idx = X509_NAME_get_index_by_NID(subject, nid, -1);
   if (idx == -1)
      return false;

   X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, idx);
   if (entry == NULL)
      return false;

   ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
   if (data == NULL)
      return false;

   unsigned char *text;
   ASN1_STRING_to_UTF8(&text, data);
#ifdef UNICODE
   MultiByteToWideChar(CP_UTF8, 0, (char *)text, -1, buffer, (int)size);
#else
   utf8_to_mb((char *)text, -1, buffer, (int)size);
#endif
   buffer[size - 1] = 0;
   OPENSSL_free(text);
   return true;
}

/**
 * Get CN from certificate
 */
bool GetCertificateCN(X509 *cert, TCHAR *buffer, size_t size)
{
   return GetCertificateSubjectField(cert, NID_commonName, buffer, size);
}

/**
 * Get OU from certificate
 */
bool GetCertificateOU(X509 *cert, TCHAR *buffer, size_t size)
{
   return GetCertificateSubjectField(cert, NID_organizationalUnitName, buffer, size);
}

/**
 * Get subject string (C=XX,O=org,OU=unit,CN=cn) from certificate
 */
String GetCertificateSubjectString(X509 *cert)
{
   String text;
   TCHAR buffer[256];
   if (GetCertificateSubjectField(cert, NID_countryName, buffer, 256))
   {
      text.append(_T("C="));
      text.append(buffer);
   }
   if (GetCertificateSubjectField(cert, NID_organizationName, buffer, 256))
   {
      if (!text.isEmpty())
         text.append(_T(','));
      text.append(_T("O="));
      text.append(buffer);
   }
   if (GetCertificateSubjectField(cert, NID_organizationalUnitName, buffer, 256))
   {
      if (!text.isEmpty())
         text.append(_T(','));
      text.append(_T("OU="));
      text.append(buffer);
   }
   if (GetCertificateSubjectField(cert, NID_commonName, buffer, 256))
   {
      if (!text.isEmpty())
         text.append(_T(','));
      text.append(_T("CN="));
      text.append(buffer);
   }
   return text;
}

/**
 * Get country name from server certificate
 */
bool GetServerCertificateCountry(TCHAR *buffer, size_t size)
{
   if (s_serverCertificate == NULL)
      return false;
   return GetCertificateSubjectField(s_serverCertificate, NID_countryName, buffer, size);
}

/**
 * Get organization name from server certificate
 */
bool GetServerCertificateOrganization(TCHAR *buffer, size_t size)
{
   if (s_serverCertificate == NULL)
      return false;
   return GetCertificateSubjectField(s_serverCertificate, NID_organizationName, buffer, size);
}

/**
 * Create X509 certificate structure from login message
 */
X509 *CertificateFromLoginMessage(NXCPMessage *msg)
{
	X509 *cert = NULL;
   size_t len;
   const BYTE *data = msg->getBinaryFieldPtr(VID_CERTIFICATE, &len);
	if ((data != NULL) && (len > 0))
	{
      OPENSSL_CONST BYTE *p = (OPENSSL_CONST BYTE *)data;
		cert = d2i_X509(NULL, &p, (long)len);
	}
	return cert;
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
   TCHAR certCn[256];
   if (!GetCertificateCN(cert, certCn, 256))
      return FALSE;

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Certificate CN=\"%s\", user CN=\"%s\""), certCn, cn);
   return !_tcsicmp(certCn, cn);
}

/**
 * Validate user's certificate
 */
bool ValidateUserCertificate(X509 *cert, const TCHAR *login, const BYTE *challenge, const BYTE *signature,
									  size_t sigLen, int mappingMethod, const TCHAR *mappingData)
{
   BOOL bValid = FALSE;

   char subjectName[1024];
   X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 1024);
#ifdef UNICODE
   WCHAR certSubject[1024];
   MultiByteToWideChar(CP_UTF8, 0, subjectName, -1, certSubject, 1024);
#else
   const char *certSubject = subjectName;
#endif

	DbgPrintf(3, _T("Validating certificate \"%s\" for user %s"), certSubject, login);
	s_certificateStoreLock.lock();

	if (s_trustedCertificateStore == NULL)
	{
		DbgPrintf(3, _T("Cannot validate user certificate because certificate store is not initialized"));
		s_certificateStoreLock.unlock();
		return FALSE;
	}

	// Validate signature
	EVP_PKEY *pKey = X509_get_pubkey(cert);
	if (pKey != NULL)
	{
      BYTE hash[SHA1_DIGEST_SIZE];
		CalculateSHA1Hash(challenge, CLIENT_CHALLENGE_SIZE, hash);
		switch(EVP_PKEY_id(pKey))
		{
			case EVP_PKEY_RSA:
				bValid = RSA_verify(NID_sha1, hash, SHA1_DIGEST_SIZE, const_cast<unsigned char*>(signature), static_cast<unsigned int>(sigLen), EVP_PKEY_get1_RSA(pKey));
				break;
			default:
				DbgPrintf(3, _T("Unknown key type %d in certificate \"%s\" for user %s"), EVP_PKEY_id(pKey), certSubject, login);
				break;
		}
	}

	// Validate certificate
	if (bValid)
	{
		X509_STORE_CTX *pStore = X509_STORE_CTX_new();
		if (pStore != NULL)
		{
			X509_STORE_CTX_init(pStore, s_trustedCertificateStore, cert, NULL);
			bValid = X509_verify_cert(pStore);
			X509_STORE_CTX_free(pStore);
			DbgPrintf(3, _T("Certificate \"%s\" for user %s - validation %s"),
			          certSubject, login, bValid ? _T("successful") : _T("failed"));
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
		switch(mappingMethod)
		{
			case USER_MAP_CERT_BY_SUBJECT:
				bValid = !_tcsicmp(certSubject, CHECK_NULL_EX(mappingData));
				break;
			case USER_MAP_CERT_BY_PUBKEY:
				bValid = CheckPublicKey(pKey, CHECK_NULL_EX(mappingData));
				break;
			case USER_MAP_CERT_BY_CN:
            bValid = CheckCommonName(cert, ((mappingData != NULL) && (*mappingData != 0)) ? mappingData : login);
				break;
			default:
				DbgPrintf(3, _T("Invalid certificate mapping method %d for user %s"), mappingMethod, login);
				bValid = FALSE;
				break;
		}
	}

	s_certificateStoreLock.unlock();
	return bValid;
}

/**
 * Validate agent certificate
 */
bool ValidateAgentCertificate(X509 *cert)
{
   X509_STORE *store = X509_STORE_new();
   if (store == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("ValidateAgentCertificate: cannot create certificate store"));
   }

   for(int i = 0; i < s_serverCACertificates.size(); i++)
      X509_STORE_add_cert(store, s_serverCACertificates.get(i));
   X509_STORE_add_cert(store, s_serverCertificate);
   bool valid = false;

   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   if (ctx != NULL)
   {
      X509_STORE_CTX_init(ctx, store, cert, NULL);
      valid = (X509_verify_cert(ctx) == 1);
      X509_STORE_CTX_free(ctx);
   }
   else
   {
      TCHAR buffer[256];
      nxlog_debug_tag(DEBUG_TAG, 3, _T("ValidateAgentCertificate: X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), buffer));
   }

   X509_STORE_free(store);
   return valid;
}

/**
 * Reload certificates from database
 */
void ReloadCertificates()
{
	s_certificateStoreLock.lock();

	if (s_trustedCertificateStore != NULL)
		X509_STORE_free(s_trustedCertificateStore);

	s_trustedCertificateStore = X509_STORE_new();
	if (s_trustedCertificateStore != NULL)
	{
	   // Add server's certificate as trusted
	   if (s_serverCertificate != NULL)
	      X509_STORE_add_cert(s_trustedCertificateStore, s_serverCertificate);

      // Add server's CA certificate as trusted
	   for(int i = 0; i < s_serverCACertificates.size(); i++)
	      X509_STORE_add_cert(s_trustedCertificateStore, s_serverCACertificates.get(i));

	   TCHAR szBuffer[256];
		_sntprintf(szBuffer, 256, _T("SELECT cert_data,subject FROM certificates WHERE cert_type=%d"), CERT_TYPE_TRUSTED_CA);
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		DB_RESULT hResult = DBSelect(hdb, szBuffer);
		if (hResult != NULL)
		{
		   int loaded = 0;
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				TCHAR *certData = DBGetField(hResult, i, 0, NULL, 0);
				if (certData != NULL)
				{
					size_t len = _tcslen(certData);
					BYTE *binCert = (BYTE *)malloc(len);
					StrToBin(certData, binCert, len);
					free(certData);
               OPENSSL_CONST BYTE *p = binCert;
					X509 *cert = d2i_X509(NULL, &p, (long)len);
					free(binCert);
					if (cert != NULL)
					{
						if (X509_STORE_add_cert(s_trustedCertificateStore, cert))
						{
							loaded++;
						}
						else
						{
						   TCHAR subject[256];
							nxlog_write(MSG_CANNOT_ADD_CERT, EVENTLOG_ERROR_TYPE,
										"ss", DBGetField(hResult, i, 1, subject, 256),
										_ERR_error_tstring(ERR_get_error(), szBuffer));
						}
						X509_free(cert); // X509_STORE_add_cert increments reference count
					}
					else
					{
                  TCHAR subject[256];
						nxlog_write(MSG_CANNOT_LOAD_CERT, EVENTLOG_ERROR_TYPE,
									"ss", DBGetField(hResult, i, 1, subject, 256),
									_ERR_error_tstring(ERR_get_error(), szBuffer));
					}
				}
			}
			DBFreeResult(hResult);

			if (loaded > 0)
				nxlog_write(MSG_CA_CERTIFICATES_LOADED, EVENTLOG_INFORMATION_TYPE, "d", loaded);
		}
		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
	   TCHAR buffer[256];
		nxlog_write(MSG_CANNOT_INIT_CERT_STORE, EVENTLOG_ERROR_TYPE, "s", _ERR_error_tstring(ERR_get_error(), buffer));
	}

	s_certificateStoreLock.unlock();
}

/**
 * Certificate stuff initialization
 */
void InitCertificates()
{
   ReloadCertificates();
}

/**
 * Load server certificate
 */
bool LoadServerCertificate(RSA **serverKey)
{
   if (g_serverCertificatePath[0] == 0)
   {
      nxlog_write(MSG_SERVER_CERT_NOT_SET, NXLOG_INFO, NULL);
      return false;
   }

   // Load server CA certificates
   TCHAR *curr = g_serverCACertificatesPath;
   TCHAR *next = _tcschr(curr, _T('\n'));
   while(next != NULL)
   {
      *next = 0;

      nxlog_debug_tag(DEBUG_TAG, 5, _T("Loading CA certificate from %s"), curr);
      FILE *f = _tfopen(curr, _T("r"));
      if (f == NULL)
      {
         nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", curr, _tcserror(errno));
         return false;
      }
      X509 *cert = PEM_read_X509(f, NULL, NULL, NULL);
      fclose(f);
      if (cert == NULL)
      {
         TCHAR buffer[1024];
         nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", curr, _ERR_error_tstring(ERR_get_error(), buffer));
         return false;
      }

      nxlog_debug_tag(DEBUG_TAG, 3, _T("Adding CA certificate %s"), static_cast<const TCHAR*>(GetCertificateSubjectString(cert)));
      s_serverCACertificates.add(cert);

      curr = next + 1;
      next = _tcschr(curr, _T('\n'));
   }

   // Load server certificate and private key
   FILE *f = _tfopen(g_serverCertificatePath, _T("r"));
   if (f == NULL)
   {
      nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", g_serverCertificatePath, _tcserror(errno));
      return false;
   }

   DecryptPasswordA("system", g_serverCertificatePassword, g_serverCertificatePassword, MAX_PASSWORD);
   s_serverCertificate = PEM_read_X509(f, NULL, NULL, g_serverCertificatePassword);
   if (g_serverCertificateKeyPath[0] != 0)
   {
      // Server key is in separate file
      fclose(f);
      f = _tfopen(g_serverCertificateKeyPath, _T("r"));
      if (f == NULL)
      {
         nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", g_serverCertificateKeyPath, _tcserror(errno));
         return false;
      }
   }
   s_serverCertificateKey = PEM_read_PrivateKey(f, NULL, NULL, g_serverCertificatePassword);
   fclose(f);

   if ((s_serverCertificate == NULL) || (s_serverCertificateKey == NULL))
   {
      TCHAR buffer[1024];
      nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", g_serverCertificatePath, _ERR_error_tstring(ERR_get_error(), buffer));
      return false;
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Server certificate: %s"), static_cast<const TCHAR*>(GetCertificateSubjectString(s_serverCertificate)));

   RSA *privKey = EVP_PKEY_get1_RSA(s_serverCertificateKey);
   RSA *pubKey = EVP_PKEY_get1_RSA(X509_get_pubkey(s_serverCertificate));
   if ((privKey != NULL) && (pubKey != NULL))
   {
      // Combine into one key
      int len = i2d_RSAPublicKey(pubKey, NULL);
      len += i2d_RSAPrivateKey(privKey, NULL);
      BYTE *buffer = (BYTE *)malloc(len);

      BYTE *pos = buffer;
      i2d_RSAPublicKey(pubKey, &pos);
      i2d_RSAPrivateKey(privKey, &pos);

      *serverKey = RSAKeyFromData(buffer, len, true);
      free(buffer);
   }

   return true;
}

#if HAVE_X509_STORE_SET_VERIFY_CB

/**
 * Certificate verification callback
 */
static int CertVerifyCallback(int success, X509_STORE_CTX *ctx)
{
   if (!success)
   {
      X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
      int error = X509_STORE_CTX_get_error(ctx);
      int depth = X509_STORE_CTX_get_error_depth(ctx);
      char subjectName[1024];
      X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 1024);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Certificate \"%hs\" verification error %d (%hs) at depth %d"),
             subjectName, error, X509_verify_cert_error_string(error), depth);
   }
   return success;
}

#endif /* HAVE_X509_STORE_SET_VERIFY_CB */

/**
 * Setup server-side TLS context
 */
bool SetupServerTlsContext(SSL_CTX *context)
{
   if ((s_serverCertificate == NULL) || (s_serverCertificateKey == NULL))
      return false;

   X509_STORE *store = X509_STORE_new();
   if (store == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("SetupServerTlsContext: cannot create certificate store"));
      return false;
   }
#if HAVE_X509_STORE_SET_VERIFY_CB
   X509_STORE_set_verify_cb(store, CertVerifyCallback);
#endif
   X509_STORE_add_cert(store, s_serverCertificate);
   for(int i = 0; i < s_serverCACertificates.size(); i++)
      X509_STORE_add_cert(store, s_serverCACertificates.get(i));
   SSL_CTX_set_cert_store(context, store);
   SSL_CTX_use_certificate(context, s_serverCertificate);
   SSL_CTX_use_PrivateKey(context, s_serverCertificateKey);
   SSL_CTX_set_verify(context, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, NULL);
   return true;
}

#else		/* _WITH_ENCRYPTION */

/**
 * Stub for certificate initialization
 */
void InitCertificates()
{
}

#endif	/* _WITH_ENCRYPTION */
