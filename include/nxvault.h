/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: nxvault.h
**
**/

#ifndef _nxvault_h_
#define _nxvault_h_

#include <nms_common.h>
#include <nms_util.h>

/**
 * Vault authentication methods
 */
enum class VaultAuthMethod
{
   NONE = 0,
   TOKEN = 1,
   APPROLE = 2,
   USERPASS = 3,
   LDAP = 4,
   AWS = 5
};

/**
 * Vault client class for HashiCorp Vault integration
 */
class LIBNETXMS_EXPORTABLE VaultClient
{
private:
   char m_url[512];
   char m_token[256];
   uint32_t m_timeout;
   bool m_verifyPeer;
   time_t m_tokenExpiration;

   bool parseJsonResponse(const char *response, json_t **json);
   bool readJsonField(json_t *json, const char *path, char *buffer, size_t bufferSize);
   bool renewToken();

protected:
   bool apiCall(const char *method, const char *path, const char *payload, char *responseBuffer, size_t responseBufferSize);

public:
   VaultClient();
   VaultClient(const char *url, uint32_t timeout = 5000);
   ~VaultClient()
   {
      // Clear sensitive data
      SecureZeroMemory(m_token, sizeof(m_token));
   }

   void setUrl(const char *url);
   void setTimeout(uint32_t timeout)
   {
      m_timeout = timeout;
   }
   void setVerifyPeer(bool verify)
   {
      m_verifyPeer = verify;
   }

   bool authenticate(VaultAuthMethod method, const char *param1, const char *param2 = nullptr);
   bool authenticateWithToken(const char *token);
   bool authenticateWithAppRole(const char *roleId, const char *secretId);
   bool authenticateWithUserPass(const char *username, const char *password);

   bool readSecret(const char *path, const char *field, char *buffer, size_t bufferSize);
   bool readSecret(const char *path, const char *field, wchar_t *buffer, size_t bufferSize);
   bool writeSecret(const char *path, const char *field, const char *value);

   bool isAuthenticated() const;
};

/**
 * Vault database credential configuration
 */
struct VaultDatabaseCredentialConfig
{
   const char *url;
   const char *appRoleId;
   const char *appRoleSecretId;
   const char *dbCredentialPath;
   uint32_t timeout;
   bool tlsVerify;
};

/**
 * Retrieve database credentials from Vault
 * @param config Vault configuration parameters
 * @param dbLogin Buffer to store database login (will be modified if successful)
 * @param dbLoginSize Size of dbLogin buffer
 * @param dbPassword Buffer to store database password (will be modified if successful) 
 * @param dbPasswordSize Size of dbPassword buffer
 * @param debugTag Tag to use for debug logging (if NULL, uses "vault")
 * @param consoleOutput If true, print messages to console instead of using nxlog
 * @return true if credentials were successfully retrieved
 */
LIBNETXMS_EXPORTABLE bool RetrieveDatabaseCredentialsFromVault(const VaultDatabaseCredentialConfig *config, 
      TCHAR *dbLogin, size_t dbLoginSize, TCHAR *dbPassword, size_t dbPasswordSize, 
      const TCHAR *debugTag = nullptr, bool consoleOutput = false);

#endif   /* _nxvault_h_ */
