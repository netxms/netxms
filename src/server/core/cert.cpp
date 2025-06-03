/* 
** NetXMS - Network Management System
** Copyright (C) 2007-2025 Raden Solutions
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
#include <nxstat.h>

#define DEBUG_TAG    _T("crypto.cert")

#if OPENSSL_VERSION_NUMBER < 0x10100000L
static inline ASN1_TIME *X509_getm_notBefore(const X509 *x)
{
   return X509_get_notBefore(x);
}

static inline ASN1_TIME *X509_getm_notAfter(const X509 *x)
{
   return X509_get_notAfter(x);
}

static inline const ASN1_TIME *X509_get0_notAfter(const X509 *x)
{
   return X509_get_notAfter(x);
}
#endif

/**
 * Server certificate file information
 */
StringSet g_trustedCertificates;
StringSet g_crlList;
wchar_t g_serverCertificatePath[MAX_PATH] = L"";
wchar_t g_serverCertificateKeyPath[MAX_PATH] = L"";
char g_serverCertificatePassword[MAX_PASSWORD] = "";
wchar_t g_internalCACertificatePath[MAX_PATH] = L"";
wchar_t g_internalCACertificateKeyPath[MAX_PATH] = L"";
char g_internalCACertificatePassword[MAX_PASSWORD] = "";

/**
 * Server certificate
 */
static X509 *s_serverCertificate = nullptr;
static EVP_PKEY *s_serverCertificateKey = nullptr;

/**
 * Server internal certificate
 */
static X509 *s_internalCACertificate = nullptr;
static EVP_PKEY *s_internalCACertificateKey = nullptr;

/**
 * Trusted CA certificate store
 */
static X509_STORE *s_trustedCertificateStore = nullptr;
static Mutex s_certificateStoreLock;

/**
 * Log record ID
 */
static VolatileCounter s_logRecordId = 0;

/**
 * Issue certificate signed with server's certificate
 */
X509 *IssueCertificate(X509_REQ *request, const char *ou, const char *cn, int days)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: new certificate request (CN override: %hs, OU override: %hs)"),
            (cn != nullptr) ? cn : "<not set>", (ou != nullptr) ? ou : "<not set>");


   if (s_internalCACertificate == nullptr || s_internalCACertificateKey == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: CA certificate not set"));
      return nullptr;
   }

   X509_NAME *requestSubject = X509_REQ_get_subject_name(request);
   if (requestSubject == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot get subject from certificate request"));
      return nullptr;
   }

   X509 *cert = X509_new();
   if (cert == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_new() failed"));
      return nullptr;
   }

   if (X509_set_version(cert, 2) != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_version() failed"));
      X509_free(cert);
      return nullptr;
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
      return nullptr;
   }

   X509_NAME *subject;
   if ((cn != nullptr) || (ou != nullptr))
   {
      subject = X509_NAME_dup(requestSubject);
      if (subject != nullptr)
      {
         if (ou != nullptr)
         {
            int idx = X509_NAME_get_index_by_NID(subject, NID_organizationalUnitName, -1);
            if (idx != -1)
               X509_NAME_delete_entry(subject, idx);
            if (!X509_NAME_add_entry_by_txt(subject, "OU", MBSTRING_UTF8, (const BYTE *)ou, -1, -1, 0))
               nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: X509_NAME_add_entry_by_txt failed for OU=%hs"), ou);
         }
         if (cn != nullptr)
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

   if (subject == nullptr)
   {
      X509_free(cert);
      return nullptr;
   }

   rc = X509_set_subject_name(cert, subject);
   if (subject != requestSubject)
      X509_NAME_free(subject);
   if (rc != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_subject_name() failed"));
      X509_free(cert);
      return nullptr;
   }

   X509_NAME *issuerName = X509_get_subject_name(s_internalCACertificate);
   if (issuerName == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot get CA subject name"));
      X509_free(cert);
      return nullptr;
   }

   if (X509_set_issuer_name(cert, issuerName) != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_issuer_name() failed"));
      X509_free(cert);
      return nullptr;
   }

   EVP_PKEY *pubkey = X509_REQ_get_pubkey(request);
   if (pubkey == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_REQ_get_pubkey() failed"));
      X509_free(cert);
      return nullptr;
   }

   if (X509_REQ_verify(request, pubkey) != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: certificate request verification failed"));
      EVP_PKEY_free(pubkey);
      X509_free(cert);
      return nullptr;
   }

   rc = X509_set_pubkey(cert, pubkey);
   EVP_PKEY_free(pubkey);
   if (rc != 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_set_pubkey() failed"));
      X509_free(cert);
      return nullptr;
   }

   if (X509_gmtime_adj(X509_getm_notBefore(cert), 0) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot set start time"));
      X509_free(cert);
      return nullptr;
   }

   if (X509_gmtime_adj(X509_getm_notAfter(cert), days * 86400) == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: cannot set end time"));
      X509_free(cert);
      return nullptr;
   }

   if (X509_sign(cert, s_internalCACertificateKey, EVP_sha256()) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: call to X509_sign() failed"));
      X509_free(cert);
      return nullptr;
   }

   char subjectName[1024];
   X509_NAME_oneline(X509_get_subject_name(cert), subjectName, 1024);
   nxlog_debug_tag(DEBUG_TAG, 4, _T("IssueCertificate: new certificate with subject \"%hs\" issued successfully"), subjectName);
   return cert;
}

/**
 * Get country name from server certificate
 */
bool GetServerCertificateCountry(TCHAR *buffer, size_t size)
{
   if (s_serverCertificate == nullptr)
      return false;
   return GetCertificateSubjectField(s_serverCertificate, NID_countryName, buffer, size);
}

/**
 * Get organization name from server certificate
 */
bool GetServerCertificateOrganization(TCHAR *buffer, size_t size)
{
   if (s_serverCertificate == nullptr)
      return false;
   return GetCertificateSubjectField(s_serverCertificate, NID_organizationName, buffer, size);
}

/**
 * Create X509 certificate structure from login message
 */
X509 *CertificateFromLoginMessage(const NXCPMessage& msg)
{
	X509 *cert = nullptr;
   size_t len;
   const BYTE *data = msg.getBinaryFieldPtr(VID_CERTIFICATE, &len);
	if ((data != nullptr) && (len > 0))
	{
      const BYTE *p = data;
		cert = d2i_X509(nullptr, &p, (long)len);
	}
	return cert;
}

/**
 * Check public key
 */
static bool CheckPublicKey(EVP_PKEY *key, const TCHAR *mappingData)
{
	int pkeyLen = i2d_PublicKey(key, nullptr);
	auto ucBuf = MemAllocArray<unsigned char>(pkeyLen + 1);
	auto uctempBuf = ucBuf;
	i2d_PublicKey(key, &uctempBuf);
	
	TCHAR *pkeyText = MemAllocString(pkeyLen * 2 + 1);
	BinToStr(ucBuf, pkeyLen, pkeyText);

	bool valid = (_tcscmp(pkeyText, mappingData) == 0);

	MemFree(ucBuf);
	MemFree(pkeyText);

	return valid;
}

/**
 * Check certificate's CN
 */
static bool CheckCommonName(X509 *cert, const TCHAR *cn)
{
   TCHAR certCn[256];
   if (!GetCertificateCN(cert, certCn, 256))
      return false;

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Certificate CN=\"%s\", user CN=\"%s\""), certCn, cn);
   return _tcsicmp(certCn, cn) == 0;
}

/**
 * Check certificate's template ID
 */
static bool CheckTemplateId(X509 *cert, const TCHAR *userTemplateId)
{
   String certTemplateId = GetCertificateTemplateId(cert);
   if (certTemplateId.isEmpty())
      return false;
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Certificate templateId=\"%s\", user templateId=\"%s\""), certTemplateId.cstr(), userTemplateId);
   return _tcscmp(certTemplateId, userTemplateId) == 0;
}

/**
 * Validate user's certificate
 */
bool ValidateUserCertificate(X509 *cert, const TCHAR *login, const BYTE *challenge, const BYTE *signature, size_t sigLen, CertificateMappingMethod mappingMethod, const TCHAR *mappingData)
{
   String certSubject = GetCertificateSubjectString(cert);

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Validating certificate \"%s\" for user %s"), certSubject.cstr(), login);
	s_certificateStoreLock.lock();

	if (s_trustedCertificateStore == nullptr)
	{
	   nxlog_debug_tag(DEBUG_TAG, 3, _T("Cannot validate user certificate because certificate store is not initialized"));
		s_certificateStoreLock.unlock();
		return false;
	}

	// Validate signature
   bool isValid = false;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_PKEY *pKey = X509_get0_pubkey(cert);
#else
	EVP_PKEY *pKey = X509_get_pubkey(cert);
#endif
	if (pKey != nullptr)
	{
      BYTE hash[SHA1_DIGEST_SIZE];
		CalculateSHA1Hash(challenge, CLIENT_CHALLENGE_SIZE, hash);
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
		EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pKey, nullptr);
		if (ctx != nullptr)
		{
		   EVP_PKEY_verify_init(ctx);
		   EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PADDING);
		   EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha1());
		   isValid = (EVP_PKEY_verify(ctx, hash, SHA1_DIGEST_SIZE, signature, sigLen) != 0);
		   EVP_PKEY_CTX_free(ctx);
		}
#else
		if (EVP_PKEY_id(pKey) == EVP_PKEY_RSA)
         isValid = (RSA_verify(NID_sha1, hash, SHA1_DIGEST_SIZE, const_cast<unsigned char*>(signature), static_cast<unsigned int>(sigLen), EVP_PKEY_get1_RSA(pKey)) != 0);
		else
         nxlog_debug_tag(DEBUG_TAG, 3, _T("Unsupported key type %d in certificate \"%s\" for user %s"), EVP_PKEY_id(pKey), certSubject.cstr(), login);
#endif
	}

	// Validate certificate
	if (isValid)
	{
		X509_STORE_CTX *pStore = X509_STORE_CTX_new();
		if (pStore != nullptr)
		{
			X509_STORE_CTX_init(pStore, s_trustedCertificateStore, cert, nullptr);
			isValid = (X509_verify_cert(pStore) != 0);
			X509_STORE_CTX_free(pStore);
			nxlog_debug_tag(DEBUG_TAG, 3, _T("Certificate \"%s\" for user %s - validation %s"), certSubject.cstr(), login, isValid ? _T("successful") : _T("failed"));
		}
		else
		{
			TCHAR szBuffer[256];
			nxlog_debug_tag(DEBUG_TAG, 3, _T("X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), szBuffer));
			isValid = false;
		}
	}

	// Check user mapping
	if (isValid)
	{
		switch(mappingMethod)
		{
			case MAP_CERTIFICATE_BY_SUBJECT:
				isValid = (_tcsicmp(certSubject, CHECK_NULL_EX(mappingData)) == 0);
				break;
			case MAP_CERTIFICATE_BY_PUBKEY:
				isValid = CheckPublicKey(pKey, CHECK_NULL_EX(mappingData));
				break;
			case MAP_CERTIFICATE_BY_CN:
            isValid = CheckCommonName(cert, ((mappingData != nullptr) && (*mappingData != 0)) ? mappingData : login);
				break;
         case MAP_CERTIFICATE_BY_TEMPLATE_ID:
            isValid = CheckTemplateId(cert, CHECK_NULL_EX(mappingData));;
            break;
			default:
			   nxlog_debug_tag(DEBUG_TAG, 3, _T("Invalid certificate mapping method %d for user %s"), mappingMethod, login);
				isValid = false;
				break;
		}
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	if (pKey != nullptr)
	   EVP_PKEY_free(pKey);
#endif

	s_certificateStoreLock.unlock();
	return isValid;
}

/**
 * Reload certificates from database
 */
void ReloadCertificates()
{
   auto it = g_crlList.begin();
   while(it.hasNext())
   {
      const wchar_t *location = it.next();
      if (!wcsncmp(location, L"http://", 7) || !wcsncmp(location, L"https://", 8))
      {
         char *url = UTF8StringFromWideString(location);
         AddRemoteCRL(url, true);
         MemFree(url);
      }
      else
      {
         AddLocalCRL(location);
      }
   }

	s_certificateStoreLock.lock();

	if (s_trustedCertificateStore != nullptr)
		X509_STORE_free(s_trustedCertificateStore);

	s_trustedCertificateStore = CreateTrustedCertificatesStore(g_trustedCertificates, true);
	if (s_trustedCertificateStore != nullptr)
	{
	   // Add internal CA certificate as trusted
	   if (s_internalCACertificate != nullptr)
         X509_STORE_add_cert(s_trustedCertificateStore, s_internalCACertificate);
	}
	else
	{
	   wchar_t buffer[256];
		nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Cannot initialize certificate store (%s)", ERR_error_string_W(ERR_get_error(), buffer));
	}

	s_certificateStoreLock.unlock();
}

/**
 * Certificate stuff initialization
 */
void InitCertificates()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM certificate_action_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_logRecordId = DBGetFieldLong(hResult, 0, 0);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);

   ReloadCertificates();
}

/**
 * Load certificate
 */
static bool LoadCertificate(RSA_KEY *serverKey, const TCHAR *certificatePath, const TCHAR *certificateKeyPath, char *certificatePassword, X509 **certificate, EVP_PKEY **certificateKey, const TCHAR *certType)
{
   if (certificatePath[0] == 0)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("%s certificate not set"), certType);
      return false;
   }

   // Load server certificate and private key
   FILE *f = _tfopen(certificatePath, _T("r"));
   if (f == nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot load %s certificate from %s (%s)"), certType, certificatePath, _tcserror(errno));
      return false;
   }

   DecryptPasswordA("system", certificatePassword, certificatePassword, MAX_PASSWORD);
   *certificate = PEM_read_X509(f, nullptr, nullptr, certificatePassword);
   if (certificateKeyPath[0] != 0)
   {
      // Server key is in separate file
      fclose(f);
      f = _tfopen(certificateKeyPath, _T("r"));
      if (f == nullptr)
      {
         nxlog_write(NXLOG_ERROR, _T("Cannot load %s certificate key from %s (%s)"), certType, certificateKeyPath, _tcserror(errno));
         return false;
      }
   }
   *certificateKey = PEM_read_PrivateKey(f, nullptr, nullptr, certificatePassword);
   fclose(f);

   if ((*certificate == nullptr) || (*certificateKey == nullptr))
   {
      TCHAR buffer[1024];
      nxlog_write(NXLOG_ERROR, _T("Cannot load %s certificate from %s (%s)"), certType, certificatePath, _ERR_error_tstring(ERR_get_error(), buffer));
      return false;
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("%s certificate: %s"), certType, static_cast<const TCHAR*>(GetCertificateSubjectString(*certificate)));

   if (serverKey != nullptr)
   {
#if OPENSSL_VERSION_NUMBER >= 0x30000000
      EVP_PKEY *publicKey = X509_get0_pubkey(*certificate);
      if (publicKey != nullptr)
      {
         // Combine into one key
         uint32_t keyLen = i2d_PublicKey(publicKey, nullptr);
         keyLen += i2d_PrivateKey(*certificateKey, nullptr);
         BYTE *keyBuffer = MemAllocArrayNoInit<BYTE>(keyLen);

         BYTE *p = keyBuffer;
         i2d_PublicKey(publicKey, &p);
         i2d_PrivateKey(*certificateKey, &p);

         *serverKey = RSAKeyFromData(keyBuffer, keyLen, true);
         MemFree(keyBuffer);
      }
#else
      RSA *privKey = EVP_PKEY_get1_RSA(*certificateKey);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
      EVP_PKEY *certPubKey = X509_get0_pubkey(*certificate);
#else
      EVP_PKEY *certPubKey = X509_get_pubkey(*certificate);
#endif
      RSA *pubKey = EVP_PKEY_get1_RSA(certPubKey);
      if ((privKey != nullptr) && (pubKey != nullptr))
      {
         // Combine into one key
         int keyLen = i2d_RSAPublicKey(pubKey, nullptr);
         keyLen += i2d_RSAPrivateKey(privKey, nullptr);
         BYTE *keyBuffer = MemAllocArray<BYTE>(keyLen);

         BYTE *pos = keyBuffer;
         i2d_RSAPublicKey(pubKey, &pos);
         i2d_RSAPrivateKey(privKey, &pos);

         *serverKey = RSAKeyFromData(keyBuffer, keyLen, true);
         MemFree(keyBuffer);
      }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
      if (certPubKey != nullptr)
         EVP_PKEY_free(certPubKey);
#endif
#endif
   }

   return true;
}

/**
 * Load server certificate
 */
bool LoadServerCertificate(RSA_KEY *serverKey)
{
   return LoadCertificate(serverKey, g_serverCertificatePath, g_serverCertificateKeyPath, g_serverCertificatePassword, &s_serverCertificate, &s_serverCertificateKey, _T("Server"));
}

/**
 * Load CA certificate
 */
bool LoadInternalCACertificate()
{
   return LoadCertificate(nullptr, g_internalCACertificatePath, g_internalCACertificateKeyPath, g_internalCACertificatePassword, &s_internalCACertificate, &s_internalCACertificateKey, _T("Internal CA"));
}

/**
 * Cleanup server certificates
 */
void CleanupServerCertificates()
{
   if (s_serverCertificate != nullptr)
      X509_free(s_serverCertificate);
   if (s_serverCertificateKey != nullptr)
      EVP_PKEY_free(s_serverCertificateKey);
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
   if ((s_serverCertificate == nullptr) || (s_serverCertificateKey == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("SetupServerTlsContext: server certificate not loaded"));
      return false;
   }

   X509_STORE *store = CreateTrustedCertificatesStore(g_trustedCertificates, true);
   if (store == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("SetupServerTlsContext: cannot create certificate store"));
      return false;
   }
#if HAVE_X509_STORE_SET_VERIFY_CB
   X509_STORE_set_verify_cb(store, CertVerifyCallback);
#endif
   if (s_internalCACertificate != nullptr)
      X509_STORE_add_cert(store, s_internalCACertificate);

   SSL_CTX_set_cert_store(context, store);
   SSL_CTX_use_certificate(context, s_serverCertificate);
   SSL_CTX_use_PrivateKey(context, s_serverCertificateKey);
   SSL_CTX_set_verify(context, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, nullptr);
   return true;
}

/**
 * Scheduled task for CRL reloading
 */
void ReloadCRLs(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Reloading all registered CRLs"));
   ReloadAllCRLs();
}

/**
 * Log certificate action
 */
void LogCertificateAction(CertificateOperation operation, uint32_t userId, uint32_t nodeId, const uuid& nodeGuid, CertificateType type, X509 *cert)
{
   ASN1_INTEGER *serial = X509_get_serialNumber(cert);
   LogCertificateAction(operation, userId, nodeId, nodeGuid, type, GetCertificateSubjectString(cert), ASN1_INTEGER_get(serial));
}

/**
 * Get expiration date for server certificate
 */
String GetServerCertificateExpirationDate()
{
   if (s_serverCertificate == nullptr)
      return String();

   time_t e = GetCertificateExpirationTime(s_serverCertificate);
   TCHAR buffer[64];
   _tcsftime(buffer, 64, _T("%Y-%m-%d"), localtime(&e));
   return String(buffer);
}

/**
 * Get number of days until server certificate expiration
 */
int GetServerCertificateDaysUntilExpiration()
{
   if (s_serverCertificate == nullptr)
      return -1;

   time_t e = GetCertificateExpirationTime(s_serverCertificate);
   time_t now = time(nullptr);
   return static_cast<int>((e - now) / 86400);
}

/**
 * Get server certificate expiration time
 */
time_t GetServerCertificateExpirationTime()
{
   if (s_serverCertificate == nullptr)
      return 0;

   return GetCertificateExpirationTime(s_serverCertificate);
}

/**
 * Check if server certificate is loaded
 */
bool IsServerCertificateLoaded()
{
   return (s_serverCertificate != nullptr) && (s_serverCertificateKey != nullptr);
}

/**
 * Log certificate action
 */
void LogCertificateAction(CertificateOperation operation, uint32_t userId, uint32_t nodeId, const uuid& nodeGuid, CertificateType type, const TCHAR *subject, int32_t serial)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
      (g_dbSyntax == DB_SYNTAX_TSDB) ?
         _T("INSERT INTO certificate_action_log (record_id,operation_timestamp,operation,user_id,node_id,node_guid,cert_type,subject,serial) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?)") :
         _T("INSERT INTO certificate_action_log (record_id,operation_timestamp,operation,user_id,node_id,node_guid,cert_type,subject,serial) VALUES (?,?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, InterlockedIncrement(&s_logRecordId));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(time(nullptr)));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<int32_t>(operation));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, userId);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, nodeId);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, nodeGuid);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<int32_t>(type));
      DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, subject, DB_BIND_STATIC);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, serial);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
