/* 
** NetXMS - Network Management System
** Copyright (C) 2021 Raden Solutions
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
** File: authentification.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("2fa")

/**
 * Authentification method base class
 */
class AuthentificationMethod
{
protected:
   TCHAR m_methodName[MAX_OBJECT_NAME];
   TCHAR m_description[MAX_2FA_DESCRIPTION];
   bool m_isValid;
   AuthentificationMethod(const TCHAR* name, const TCHAR* description, const Config& config);

public:
   virtual ~AuthentificationMethod() {};
   virtual AuthentificationToken* prepareChallenge(uint32_t userId) = 0;
   virtual bool validateChallenge(AuthentificationToken* token, const TCHAR* response) = 0;
   bool isValid() { return m_isValid; };
   const TCHAR* getName() { return m_methodName; };
   const TCHAR* getDescription() { return m_description; };
   bool saveToDatabase(char* configuration);
};

static StringObjectMap<AuthentificationMethod> s_authMethodList(Ownership::True);
static Mutex s_authMethodListLock;

/**
 * Authentification method base constructor
 */
AuthentificationMethod::AuthentificationMethod(const TCHAR* name, const TCHAR* description, const Config& config)
{
   m_isValid = false;
   _tcslcpy(m_methodName, name, MAX_OBJECT_NAME);
   _tcslcpy(m_description, description, MAX_OBJECT_NAME);
}

/**
 * Save 2FA method to database
 */
bool AuthentificationMethod::saveToDatabase(char* configuration)
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("two_factor_auth_methods"), _T("name"), m_methodName))
   {
      hStmt = DBPrepare(hdb,
               _T("UPDATE two_factor_auth_methods SET description=?,configuration=? WHERE name=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb,
               _T("INSERT INTO two_factor_auth_methods (description,configuration,name) VALUES (?,?,?)"));
   }
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, configuration, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_methodName, DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * TOTP authentification class
 */
class TOTPAuthMethod : public AuthentificationMethod
{
public:
   TOTPAuthMethod(const TCHAR* name, const TCHAR* description, const Config& config) : AuthentificationMethod(name, description, config) { m_isValid = true; };
   ~TOTPAuthMethod() {};
   AuthentificationToken* prepareChallenge(uint32_t userId) override;
   bool validateChallenge(AuthentificationToken* token, const TCHAR* response) override;
};

/**
 * Preparing challenge for user with TOTP
 */
AuthentificationToken* TOTPAuthMethod::prepareChallenge(uint32_t userId)
{
   shared_ptr<Config> binding = GetUser2FABindingInfo(userId, m_methodName);
   AuthentificationToken* token = nullptr;
   if (binding != nullptr)
   {
      const TCHAR* secret = binding->getValue(_T("/2FA/Secret"));
      if (secret != nullptr)
      {
         uint32_t secretLength = static_cast<uint32_t>(_tcslen(secret) / 2);
         uint8_t *secretData = MemAllocArray<uint8_t>(secretLength);
         StrToBin(secret, secretData, secretLength);
         token = new TOTPToken(m_methodName, secretData, secretLength);
      }
   }
   return token;
}

/**
 * Validate response from user
 */
bool TOTPAuthMethod::validateChallenge(AuthentificationToken* token, const TCHAR* response)
{
   if (!_tcscmp(token->getMethodName(), m_methodName))
   {
      auto pToken = static_cast<TOTPToken*>(token);
      time_t now = time(NULL);
      for (uint64_t i = 0; i < 3; i++) // we will test current time factor, previous and next
      {
         //calculating TOTP challenge
         uint64_t timeFactor = htonq(((now / 30lu) + i) - 2lu);
         uint8_t hash[SHA1_DIGEST_SIZE];
         uint32_t hashLen;
         HMAC(EVP_sha1(), pToken->getSecret(), static_cast<int>(pToken->getSecretLength()),
          reinterpret_cast<const BYTE*>(&timeFactor), sizeof(uint64_t), hash, &hashLen);
         int offset = (int)(hash[SHA1_DIGEST_SIZE - 1] & 0x0F);
         uint32_t challenge;
         memcpy(&challenge, hash+offset, sizeof(uint32_t));
         challenge = (ntohl(challenge) & 0x7FFFFFFF) % 1000000;
         if (challenge == (uint32_t)_tcstol(response, nullptr, 10))
         {
            return true;
         }
      }
   }
   return false;
}

/**
 * Notification channel authentification class
 */
class MessageAuthMethod : public AuthentificationMethod
{
private:
   TCHAR m_channelName[MAX_OBJECT_NAME];

public:
   MessageAuthMethod(const TCHAR* name, const TCHAR* description, const Config& config);
   ~MessageAuthMethod() {};
   AuthentificationToken* prepareChallenge(uint32_t userId) override;
   bool validateChallenge(AuthentificationToken* token, const TCHAR* response) override;
};

/**
 * Notification channel authentification class constructor
 */
MessageAuthMethod::MessageAuthMethod(const TCHAR* name, const TCHAR* description, const Config& config) : AuthentificationMethod(name, description, config)
{
   const TCHAR* channelName = config.getValue(_T("/Message/ChannelName"));
   if (channelName != nullptr)
   {
      _tcslcpy(m_channelName, channelName, MAX_OBJECT_NAME);
      m_isValid = true;
   }
}

/**
 * Preparing challenge for user using notification channel
 */
AuthentificationToken* MessageAuthMethod::prepareChallenge(uint32_t userId)
{
   shared_ptr<Config> binding = GetUser2FABindingInfo(userId, m_methodName);
   if (binding == nullptr || m_channelName == nullptr)
      return nullptr;

   uint32_t challenge;
   GenerateRandomBytes((uint8_t*)&challenge, sizeof(uint32_t));
   challenge = challenge % 1000000; //We will use 6 digits code
   TCHAR* phone = MemCopyString(binding->getValue(_T("/2FA/PhoneNumber")));
   const TCHAR* subject = binding->getValue(_T("/2FA/Subject"));
   TCHAR challengeStr[7];
   _sntprintf(challengeStr, 7, _T("%u"), challenge);
   SendNotification(m_channelName, phone, subject, challengeStr);
   MemFree(phone);
   return new MessageToken(m_methodName, challenge);
}

/**
 * Validate response from user
 */
bool MessageAuthMethod::validateChallenge(AuthentificationToken* token, const TCHAR* response)
{
   return static_cast<MessageToken*>(token)->getSecret() == _tcstol(response, nullptr, 10);
}

/**
 * Create new authentification driver based on configuration data
 */
AuthentificationMethod* CreateAuthentificationMethod(const TCHAR* name, const TCHAR* methodType, const TCHAR* description, const char* configData)
{
   AuthentificationMethod* method = nullptr;
   Config config;
   if (config.loadConfigFromMemory(configData, strlen(configData), _T("2FA"), nullptr, true, false))
   {
      if (methodType != nullptr)
      {
         if (!_tcscmp(methodType, _T("TOTP")))
         {
            method = new TOTPAuthMethod(name, description, config);
         }
         if (!_tcscmp(methodType, _T("Message")))
         {
            method = new MessageAuthMethod(name, description, config);
         }

         if (method != nullptr)
         {
            if (!method->isValid())
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Authentification method %s is not valid"), name);
               delete_and_null(method);
            }
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("No such method type - %s"), methodType);
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Authentification method %s configuration don't have MethodType parameter"), name);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Configuration parsing failed for method %s, configuration: %s"), name, configData);
   }
   return method;
}

/**
 * 2FA method information
 */
class TwoFactorAuthMethodInfo
{
private:
   TCHAR m_name[MAX_OBJECT_NAME];
   TCHAR m_description[MAX_2FA_DESCRIPTION];
   TCHAR m_type[MAX_OBJECT_NAME];
   char* m_configuration;
public:
   TwoFactorAuthMethodInfo(TCHAR* name, TCHAR* type, TCHAR* description, char* configuration)
   {
      _tcslcpy(m_name, name, MAX_OBJECT_NAME);
      _tcslcpy(m_description, description, MAX_2FA_DESCRIPTION);
      _tcslcpy(m_type, m_type, MAX_OBJECT_NAME);
      m_configuration = configuration;
   }

   ~TwoFactorAuthMethodInfo()
   {
      MemFree(m_configuration);
   }

   TCHAR* getName() { return m_name; };
   TCHAR* getDescription() { return m_description; };
   char* getConfiguration() { return m_configuration; };
   TCHAR* getType() { return m_type; };
};

/**
 * Reading 2FA methods info from DB
 */
unique_ptr<ObjectArray<TwoFactorAuthMethodInfo>> Load2FAMethodsInfoFromDB()
{
   auto array = make_unique<ObjectArray<TwoFactorAuthMethodInfo>>();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelect(hdb, _T("SELECT name,driver,description,configuration FROM two_factor_auth_methods"));
   if (result != nullptr)
   {
      int numRows = DBGetNumRows(result);
      for (int i = 0; i < numRows; i++)
      {
         TCHAR name[MAX_OBJECT_NAME];
         DBGetField(result, i, 0, name, MAX_OBJECT_NAME);
         TCHAR type[MAX_OBJECT_NAME];
         DBGetField(result, i, 1, name, MAX_OBJECT_NAME);
         TCHAR description[MAX_2FA_DESCRIPTION];
         DBGetField(result, i, 2, description, MAX_2FA_DESCRIPTION);
         char* configuration = DBGetFieldUTF8(result, i, 3, nullptr, 0);
         auto info = new TwoFactorAuthMethodInfo(name, type, description, configuration);
         array->add(info);
      }
      DBFreeResult(result);
   }
   DBConnectionPoolReleaseConnection(hdb);

   return array;
}

/**
 * Load authentification methods from database
 */
void LoadAuthentificationMethods()
{
   int numberOfAddedMethods = 0;
   unique_ptr<ObjectArray<TwoFactorAuthMethodInfo>> methodsInfo = Load2FAMethodsInfoFromDB();
   for (int i = 0; i < methodsInfo->size(); i++)
   {
      TwoFactorAuthMethodInfo* info = methodsInfo->get(i);
      if (info->getConfiguration() != nullptr)
      {
         AuthentificationMethod *am = CreateAuthentificationMethod(info->getName(), info->getType(), info->getDescription(), info->getConfiguration());
         if (am != nullptr)
         {
            s_authMethodListLock.lock();
            s_authMethodList.set(info->getName(), am);
            s_authMethodListLock.unlock();
            numberOfAddedMethods++;
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Authentification method %s successfully created"), info->getName());
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Authentification method %s creation failed"), info->getName());
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Unable to read configuration for authentification method %s. Method creation failed"), info->getName());
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d authentification methods added"), numberOfAddedMethods);
}

/**
 * Prepare 2FA challenge for user using selected method
 */
AuthentificationToken* Prepare2FAChallenge( const TCHAR* methodName, uint32_t userId)
{
   AuthentificationToken* token = nullptr;
   s_authMethodListLock.lock();
   AuthentificationMethod *am = s_authMethodList.get(methodName);
   if (am != nullptr)
   {
      token = am->prepareChallenge(userId);
   }
   s_authMethodListLock.unlock();
   return token;
}

/**
 * Validate 2FA challenge
 */
bool Validate2FAChallenge(AuthentificationToken* token, TCHAR *challenge)
{
   bool success = false;
   s_authMethodListLock.lock();
   if (token != nullptr)
   {
      AuthentificationMethod *am = s_authMethodList.get(token->getMethodName());
      if (am != nullptr)
      {
         success = am->validateChallenge(token, challenge);
      }
   }
   s_authMethodListLock.unlock();
   return success;
}

/**
 * Adds 2FA method info from DB to message
 */
void Get2FAMethodInfo(const TCHAR* methodInfo, NXCPMessage& msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelectFormatted(hdb, _T("SELECT name,driver,description,configuration FROM two_factor_auth_methods WHERE name=%s"), DBPrepareString(hdb, methodInfo).cstr());
   if (result != nullptr)
   {
      if (DBGetNumRows(result) > 0)
      {
         TCHAR name[MAX_OBJECT_NAME], driver[MAX_OBJECT_NAME], description[MAX_2FA_DESCRIPTION];
         DBGetField(result, 0, 0, name, MAX_OBJECT_NAME);
         DBGetField(result, 0, 1, driver, MAX_OBJECT_NAME);
         DBGetField(result, 0, 2, description, MAX_2FA_DESCRIPTION);
         char* configuration = DBGetFieldUTF8(result, 0, 3, nullptr, 0);
      }
      DBFreeResult(result);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Fills message with 2FA methods list
 */
void Get2FAMethods(NXCPMessage& msg)
{
   unique_ptr<ObjectArray<TwoFactorAuthMethodInfo>> methodsInfo = Load2FAMethodsInfoFromDB();
   s_authMethodListLock.lock();
   StringList* keys = s_authMethodList.keys();
   for (int i = 0; i < methodsInfo->size(); i++)
   {
      msg.setField(VID_2FA_METHODS_LIST_BASE + (i * 10), methodsInfo->get(i)->getName());
      msg.setField(VID_2FA_METHODS_LIST_BASE + (i * 10) + 1, methodsInfo->get(i)->getType());
      msg.setField(VID_2FA_METHODS_LIST_BASE + (i * 10) + 2, methodsInfo->get(i)->getDescription());
      msg.setField(VID_2FA_METHODS_LIST_BASE + (i * 10) + 3, keys->contains(methodsInfo->get(i)->getName()));
   }
   s_authMethodListLock.unlock();
   msg.setField(VID_2FA_METHODS_COUNT, methodsInfo->size());
}

/**
 * Updates or creates new authentification method
 */
uint32_t Modify2FAMethod(const TCHAR* name, const TCHAR* methodType, const TCHAR* description, char* configuration)
{
   uint32_t rcc = RCC_2FA_INVALID_METHOD;
   AuthentificationMethod* am = CreateAuthentificationMethod(name, methodType, description, configuration);
   if (am != nullptr)
   {
      if (am->saveToDatabase(configuration))
      {
         s_authMethodListLock.lock();
         s_authMethodList.set(name, am);
         s_authMethodListLock.unlock();
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Authentification method %s successfully created"), name);
         rcc = RCC_SUCCESS;
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Authentification method %s creation failed"), name);
   }
   return rcc;
}

/**
 * Deletes 2FA method
 */
uint32_t Delete2FAMethod(const TCHAR* name) 
{
   uint32_t rcc = RCC_DB_FAILURE;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM two_factor_auth_methods WHERE name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      if (DBExecute(hStmt))
      {
         s_authMethodListLock.lock();
         s_authMethodList.remove(name);
         s_authMethodListLock.unlock();
         rcc = RCC_SUCCESS;
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}