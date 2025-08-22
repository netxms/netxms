/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: vault.cpp
**
**/

#include "libnetxms.h"
#include <nxvault.h>
#include <nxlibcurl.h>

#define DEBUG_TAG _T("vault")

/**
 * Constructor
 */
VaultClient::VaultClient()
{
   m_url[0] = 0;
   m_token[0] = 0;
   m_timeout = 5000;
   m_verifyPeer = true;
   m_tokenExpiration = 0;
}

/**
 * Constructor with URL
 */
VaultClient::VaultClient(const char *url, uint32_t timeout)
{
   strlcpy(m_url, url, sizeof(m_url));
   size_t urlLen = strlen(m_url);
   if (urlLen > 0 && m_url[urlLen - 1] == '/')
      m_url[urlLen - 1] = 0;

   m_token[0] = 0;
   m_timeout = timeout;
   m_verifyPeer = true;
   m_tokenExpiration = 0;
}

/**
 * Set Vault URL
 */
void VaultClient::setUrl(const char *url)
{
   strlcpy(m_url, url, sizeof(m_url));
   size_t urlLen = strlen(m_url);
   if (urlLen > 0 && m_url[urlLen - 1] == '/')
      m_url[urlLen - 1] = 0;
}

/**
 * Parse JSON response
 */
bool VaultClient::parseJsonResponse(const char *response, json_t **json)
{
   json_error_t error;
   *json = json_loads(response, 0, &error);
   if (*json == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to parse JSON response: %hs (line %d)"), error.text, error.line);
      return false;
   }
   return true;
}

/**
 * Read field from JSON object using path notation (e.g., "data/data/password")
 */
bool VaultClient::readJsonField(json_t *json, const char *path, char *buffer, size_t bufferSize)
{
   json_t *value = json_object_get_by_path_a(json, path);
   if (value != nullptr && json_is_string(value))
   {
      strlcpy(buffer, json_string_value(value), bufferSize);
      return true;
   }
   return false;
}

/**
 * Make API call to Vault
 */
bool VaultClient::apiCall(const char *method, const char *path, const char *payload, char *responseBuffer, size_t responseBufferSize)
{
   if (m_url[0] == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Vault URL not configured"));
      return false;
   }

   // Build full URL
   char url[1024];
   strcpy(url, m_url);
   strlcat(url, path, sizeof(url));

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Vault API call: %hs %hs"), method, url);

   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to initialize CURL"));
      return false;
   }

   // Response data
   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);

   // Set CURL options
   curl_easy_setopt(curl, CURLOPT_URL, url);
   curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, (long)m_timeout);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   // SSL options
   if (!m_verifyPeer)
   {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
   }

   // Headers
   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Content-Type: application/json");
   
   // Add token if authenticated
   if (m_token[0] != 0)
   {
      char authHeader[384];
      snprintf(authHeader, sizeof(authHeader), "X-Vault-Token: %s", m_token);
      headers = curl_slist_append(headers, authHeader);
   }
   
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   // Request body
   if (payload != nullptr)
   {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
   }

   // Perform request
   CURLcode res = curl_easy_perform(curl);
   
   // Copy response data to output buffer
   responseData.write('\0');
   const char* data = reinterpret_cast<const char*>(responseData.buffer());
   size_t dataLen = strlen(data);
   if (dataLen >= responseBufferSize)
      dataLen = responseBufferSize - 1;
   memcpy(responseBuffer, data, dataLen);
   responseBuffer[dataLen] = 0;

   bool success = false;
   if (res == CURLE_OK)
   {
      long responseCode;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
      if (responseCode >= 200 && responseCode < 300)
      {
         success = true;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Vault API call successful (HTTP %ld)"), responseCode);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Vault API call failed with HTTP %ld: %hs"), responseCode, responseBuffer);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Vault API call failed: %hs"), curl_easy_strerror(res));
   }

   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);
   return success;
}

/**
 * Check if client is authenticated
 */
bool VaultClient::isAuthenticated() const
{
   return m_token[0] != 0 && (m_tokenExpiration == 0 || m_tokenExpiration > time(nullptr));
}

/**
 * Authenticate with token
 */
bool VaultClient::authenticateWithToken(const char *token)
{
   strlcpy(m_token, token, sizeof(m_token));
   m_tokenExpiration = 0;  // Token auth doesn't expire
   
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Authenticated with Vault using token"));
   return true;
}

/**
 * Authenticate with AppRole
 */
bool VaultClient::authenticateWithAppRole(const char *roleId, const char *secretId)
{
   // Create JSON payload with proper escaping
   json_t *json = json_object();
   json_object_set_new(json, "role_id", json_string(roleId));
   json_object_set_new(json, "secret_id", json_string(secretId));
   
   char *payload = json_dumps(json, JSON_COMPACT);
   json_decref(json);
   
   if (payload == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to create JSON payload for AppRole authentication"));
      return false;
   }

   char response[8192];
   bool apiSuccess = apiCall("POST", "/v1/auth/approle/login", payload, response, sizeof(response));
   free(payload);
   
   if (!apiSuccess)
   {
      return false;
   }

   json_t *responseJson;
   if (!parseJsonResponse(response, &responseJson))
   {
      return false;
   }

   bool success = false;
   char token[256];
   if (readJsonField(responseJson, "auth/client_token", token, sizeof(token)))
   {
      strlcpy(m_token, token, sizeof(m_token));
      
      // Read lease duration
      json_t *auth = json_object_get(responseJson, "auth");
      if (auth != nullptr)
      {
         json_t *leaseDuration = json_object_get(auth, "lease_duration");
         if (leaseDuration != nullptr && json_is_integer(leaseDuration))
         {
            int duration = (int)json_integer_value(leaseDuration);
            m_tokenExpiration = (duration > 0) ? time(nullptr) + duration : 0;
         }
      }
      
      success = true;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Successfully authenticated with Vault using AppRole"));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to extract token from AppRole authentication response"));
   }

   json_decref(responseJson);
   return success;
}

/**
 * Authenticate with username/password
 */
bool VaultClient::authenticateWithUserPass(const char *username, const char *password)
{
   // Create JSON payload with proper escaping
   json_t *json = json_object();
   json_object_set_new(json, "password", json_string(password));
   
   char *payload = json_dumps(json, JSON_COMPACT);
   json_decref(json);
   
   if (payload == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to create JSON payload for username/password authentication"));
      return false;
   }

   char path[256];
   snprintf(path, sizeof(path), "/v1/auth/userpass/login/%s", username);

   char response[8192];
   bool apiSuccess = apiCall("POST", path, payload, response, sizeof(response));
   free(payload);
   
   if (!apiSuccess)
   {
      return false;
   }

   json_t *responseJson;
   if (!parseJsonResponse(response, &responseJson))
   {
      return false;
   }

   bool success = false;
   char token[256];
   if (readJsonField(responseJson, "auth/client_token", token, sizeof(token)))
   {
      strlcpy(m_token, token, sizeof(m_token));
      
      // Read lease duration
      json_t *auth = json_object_get(responseJson, "auth");
      if (auth != nullptr)
      {
         json_t *leaseDuration = json_object_get(auth, "lease_duration");
         if (leaseDuration != nullptr && json_is_integer(leaseDuration))
         {
            int duration = (int)json_integer_value(leaseDuration);
            m_tokenExpiration = (duration > 0) ? time(nullptr) + duration : 0;
         }
      }
      
      success = true;
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Successfully authenticated with Vault using username/password"));
   }

   json_decref(responseJson);
   return success;
}

/**
 * General authentication method
 */
bool VaultClient::authenticate(VaultAuthMethod method, const char *param1, const char *param2)
{
   switch (method)
   {
      case VaultAuthMethod::TOKEN:
         return authenticateWithToken(param1);
      case VaultAuthMethod::APPROLE:
         return authenticateWithAppRole(param1, param2);
      case VaultAuthMethod::USERPASS:
         return authenticateWithUserPass(param1, param2);
      default:
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Unsupported Vault authentication method: %d"), (int)method);
         return false;
   }
}

/**
 * Read secret from Vault
 */
bool VaultClient::readSecret(const char *path, const char *field, char *buffer, size_t bufferSize)
{
   if (!isAuthenticated())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot read secret: not authenticated"));
      return false;
   }

   // For KV v2, we need to modify the path
   char apiPath[512];
   if (strstr(path, "/data/") == nullptr && strncmp(path, "data/", 5) != 0)
   {
      // Assume KV v2 and insert /data/ after the mount point
      char *pathCopy = strdup(path);
      char *firstSlash = strchr(pathCopy, '/');
      if (firstSlash != nullptr)
      {
         *firstSlash = 0;
         snprintf(apiPath, sizeof(apiPath), "/v1/%s/data/%s", pathCopy, firstSlash + 1);
      }
      else
      {
         snprintf(apiPath, sizeof(apiPath), "/v1/%s", path);
      }
      free(pathCopy);
   }
   else
   {
      snprintf(apiPath, sizeof(apiPath), "/v1/%s", path);
   }

   char response[65536];
   if (!apiCall("GET", apiPath, nullptr, response, sizeof(response)))
   {
      return false;
   }

   json_t *json;
   if (!parseJsonResponse(response, &json))
   {
      return false;
   }

   // Try different paths for the field
   bool success = false;
   char fieldPath[256];
   
   // Try data.data.field for KV v2
   snprintf(fieldPath, sizeof(fieldPath), "data/data/%s", field);
   if (readJsonField(json, fieldPath, buffer, bufferSize))
   {
      success = true;
   }
   else
   {
      // Try data.field for KV v1
      snprintf(fieldPath, sizeof(fieldPath), "data/%s", field);
      if (readJsonField(json, fieldPath, buffer, bufferSize))
      {
         success = true;
      }
   }

   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Successfully read secret from %hs"), path);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to read field '%hs' from secret at %hs"), field, path);
   }

   json_decref(json);
   return success;
}

/**
 * Read secret from Vault (wide char version)
 */
bool VaultClient::readSecret(const char *path, const char *field, wchar_t *buffer, size_t bufferSize)
{
   char tempBuffer[8192];
   if (!readSecret(path, field, tempBuffer, sizeof(tempBuffer)))
      return false;

   MultiByteToWideCharSysLocale(tempBuffer, buffer, bufferSize);
   buffer[bufferSize - 1] = 0;
   return true;
}

/**
 * Write secret to Vault
 */
bool VaultClient::writeSecret(const char *path, const char *field, const char *value)
{
   if (!isAuthenticated())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot write secret: not authenticated"));
      return false;
   }

   // Build JSON payload with proper escaping
   json_t *data = json_object();
   json_object_set_new(data, field, json_string(value));
   
   json_t *root = json_object();
   json_object_set_new(root, "data", data);
   
   char *payload = json_dumps(root, JSON_COMPACT);
   json_decref(root);
   
   if (payload == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to create JSON payload for secret write"));
      return false;
   }

   // For KV v2, we need to modify the path
   char apiPath[512];
   if (strstr(path, "/data/") == nullptr && strncmp(path, "data/", 5) != 0)
   {
      // Assume KV v2 and insert /data/ after the mount point
      char *pathCopy = strdup(path);
      char *firstSlash = strchr(pathCopy, '/');
      if (firstSlash != nullptr)
      {
         *firstSlash = 0;
         snprintf(apiPath, sizeof(apiPath), "/v1/%s/data/%s", pathCopy, firstSlash + 1);
      }
      else
      {
         snprintf(apiPath, sizeof(apiPath), "/v1/%s", path);
      }
      free(pathCopy);
   }
   else
   {
      snprintf(apiPath, sizeof(apiPath), "/v1/%s", path);
   }

   char response[8192];
   bool success = apiCall("POST", apiPath, payload, response, sizeof(response));
   free(payload);
   
   if (success)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Successfully wrote secret to %hs"), path);
   }

   return success;
}

/**
 * Retrieve database credentials from Vault
 */
bool LIBNETXMS_EXPORTABLE RetrieveDatabaseCredentialsFromVault(const VaultDatabaseCredentialConfig *config, 
      TCHAR *dbLogin, size_t dbLoginSize, TCHAR *dbPassword, size_t dbPasswordSize, 
      const TCHAR *debugTag, bool consoleOutput)
{
   const TCHAR *tag = (debugTag != nullptr) ? debugTag : DEBUG_TAG;

   if ((config->url == nullptr) || (config->url[0] == 0) || 
       (config->dbCredentialPath == nullptr) || (config->dbCredentialPath[0] == 0))
   {
      if (!consoleOutput)
         nxlog_debug_tag(tag, 5, _T("Vault integration not configured"));
      return false;
   }

   if (consoleOutput)
      _tprintf(_T("Retrieving database credentials from Vault\n"));
   else
      nxlog_debug_tag(tag, 3, _T("Retrieving database credentials from Vault"));

   VaultClient vault(config->url, config->timeout);
   vault.setVerifyPeer(config->tlsVerify);

   // Check AppRole credentials
   if ((config->appRoleId == nullptr) || (config->appRoleId[0] == 0) || 
       (config->appRoleSecretId == nullptr) || (config->appRoleSecretId[0] == 0))
   {
      if (consoleOutput)
         _tprintf(_T("ERROR: Vault AppRole credentials not configured\n"));
      else
         nxlog_write_tag(NXLOG_ERROR, tag, _T("Vault AppRole credentials not configured"));
      return false;
   }

   // Decrypt AppRole secret ID if encrypted
   char secretId[256];
   DecryptPasswordA("netxms", config->appRoleSecretId, secretId, 256);

   bool credentialsUpdated = false;

   if (vault.authenticateWithAppRole(config->appRoleId, secretId))
   {
      // Try to read database login
      if (vault.readSecret(config->dbCredentialPath, "login", dbLogin, dbLoginSize))
      {
         if (consoleOutput)
            _tprintf(_T("Database login retrieved from Vault\n"));
         else
            nxlog_debug_tag(tag, 3, _T("Database login retrieved from Vault"));
         credentialsUpdated = true;
      }

      // Read database password
      if (vault.readSecret(config->dbCredentialPath, "password", dbPassword, dbPasswordSize))
      {
         if (consoleOutput)
            _tprintf(_T("Database password retrieved from Vault\n"));
         else
            nxlog_debug_tag(tag, 3, _T("Database password retrieved from Vault"));
         credentialsUpdated = true;
      }
      else
      {
         if (consoleOutput)
            _tprintf(_T("ERROR: Failed to read database password from Vault path: %hs\n"), config->dbCredentialPath);
         else
            nxlog_write_tag(NXLOG_ERROR, tag, _T("Failed to read database password from Vault path: %hs"), config->dbCredentialPath);
      }
   }
   else
   {
      if (consoleOutput)
         _tprintf(_T("ERROR: Failed to authenticate with Vault using AppRole\n"));
      else
         nxlog_write_tag(NXLOG_ERROR, tag, _T("Failed to authenticate with Vault using AppRole"));
   }

   return credentialsUpdated;
}
