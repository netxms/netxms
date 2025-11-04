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
** File: userdb.cpp
**
**/

#include "nxcore.h"
#include <nms_users.h>

#define DEBUG_TAG _T("userdb")

/**
 * Password complexity options
 */
#define PSWD_MUST_CONTAIN_DIGITS          0x0001
#define PSWD_MUST_CONTAIN_UPPERCASE       0x0002
#define PSWD_MUST_CONTAIN_LOWERCASE       0x0004
#define PSWD_MUST_CONTAIN_SPECIAL_CHARS   0x0008
#define PSWD_FORBID_ALPHABETICAL_SEQUENCE 0x0010
#define PSWD_FORBID_KEYBOARD_SEQUENCE     0x0020

/**
 * Action done on deleted user/group
 */
 #define USER_DELETE   0
 #define USER_DISABLE  1

/**
 * Externals
 */
bool RadiusAuth(const wchar_t *login, const wchar_t *passwd);

/**
 * Static data
 */
static HashMap<uint32_t, UserDatabaseObject> s_userDatabase(Ownership::True);
static StringObjectMap<UserDatabaseObject> s_ldapNames(Ownership::False);
static StringObjectMap<User> s_ldapUserId(Ownership::False);
static StringObjectMap<Group> s_ldapGroupId(Ownership::False);
static StringObjectMap<User> s_users(Ownership::False);
static StringObjectMap<Group> s_groups(Ownership::False);
static RWLock s_userDatabaseLock;
static THREAD s_statusUpdateThread = INVALID_THREAD_HANDLE;

/**
 * Add user database object
 */
static inline void AddDatabaseObject(UserDatabaseObject *object)
{
   s_userDatabase.set(object->getId(), object);
   if (object->isGroup())
      s_groups.set(object->getName(), static_cast<Group*>(object));
   else
      s_users.set(object->getName(), static_cast<User*>(object));
   if (object->isLDAPUser())
   {
      s_ldapNames.set(object->getDN(), object);
      if(object->getLdapId() != nullptr)
      {
         if (object->isGroup())
            s_ldapGroupId.set(object->getLdapId(), static_cast<Group*>(object));
         else
            s_ldapUserId.set(object->getLdapId(), static_cast<User*>(object));
      }
   }
}

/**
 * Remove user database object
 */
static inline void RemoveDatabaseObject(UserDatabaseObject *object)
{
   if (object->isGroup())
      s_groups.remove(object->getName());
   else
      s_users.remove(object->getName());
   if (object->isLDAPUser())
   {
      s_ldapNames.remove(object->getDN());
      if(object->getLdapId() != nullptr)
      {
         if (object->isGroup())
            s_ldapGroupId.remove(object->getLdapId());
         else
            s_ldapUserId.remove(object->getLdapId());
      }
   }
}

/**
 * Get effective system rights for user
 */
static uint64_t GetEffectiveSystemRights(User *user)
{
   uint64_t systemRights = user->getSystemRights();
   GroupSearchPath searchPath;
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
      if (object->isDeleted() || object->isDisabled() || !object->isGroup())
         continue;

      // The previous search path is checked to avoid performing a deep membership search again
      if (searchPath.contains(object->getId()))
      {
         systemRights |= object->getSystemRights();
         continue;
      }

      searchPath.clear();
      if (static_cast<Group*>(object)->isMember(user->getId(), &searchPath))
      {
         systemRights |= object->getSystemRights();
      }
      else
      {
         searchPath.clear();
      }
   }
   return systemRights;
}

/**
 * Get effective system rights for user
 */
uint64_t NXCORE_EXPORTABLE GetEffectiveSystemRights(uint32_t userId)
{
   uint64_t systemRights = 0;
   s_userDatabaseLock.readLock();
   UserDatabaseObject *user = s_userDatabase.get(userId);
   if ((user != nullptr) && !user->isGroup())
   {
      systemRights = GetEffectiveSystemRights(static_cast<User*>(user));
   }
   s_userDatabaseLock.unlock();
   return systemRights;
}

/**
 * Get effective UI access rules for user
 */
static String GetEffectiveUIAccessRules(User *user)
{
   StringBuffer uiAccessRules;

   const TCHAR *r = user->getUIAccessRules();
   if ((r != nullptr) && (*r != 0))
      uiAccessRules.append(r);

   GroupSearchPath searchPath;
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
      if (object->isDeleted() || object->isDisabled() || !object->isGroup())
         continue;

      // The previous search path is checked to avoid performing a deep membership search again
      if (searchPath.contains(object->getId()))
      {
         r = object->getUIAccessRules();
         if ((r != nullptr) && (*r != 0))
         {
            if (!uiAccessRules.isEmpty())
               uiAccessRules.append(_T(';'));
            uiAccessRules.append(r);
         }
         continue;
      }

      searchPath.clear();
      if (static_cast<Group*>(object)->isMember(user->getId(), &searchPath))
      {
         r = object->getUIAccessRules();
         if ((r != nullptr) && (*r != 0))
         {
            if (!uiAccessRules.isEmpty())
               uiAccessRules.append(_T(';'));
            uiAccessRules.append(r);
         }
      }
      else
      {
         searchPath.clear();
      }
   }
   return uiAccessRules;
}

/**
 * Get effective UI access rules for user
 */
String NXCORE_EXPORTABLE GetEffectiveUIAccessRules(uint32_t userId)
{
   if (userId == 0)
      return String(_T("*"));

   MutableString uiAccessRules;
   s_userDatabaseLock.readLock();
   UserDatabaseObject *user = s_userDatabase.get(userId);
   if ((user != nullptr) && !user->isGroup())
   {
      uiAccessRules = GetEffectiveUIAccessRules(static_cast<User*>(user));
   }
   s_userDatabaseLock.unlock();
   return uiAccessRules;
}

/**
 * Upgrade user accounts status in background
 */
static void AccountStatusUpdater()
{
   ThreadSetName("AccountUpdate");
	nxlog_debug_tag(DEBUG_TAG, 2, _T("User account status update thread started"));
	while(!SleepAndCheckForShutdown(60))
	{
		nxlog_debug_tag(DEBUG_TAG, 8, _T("AccountStatusUpdater: wakeup"));

		time_t blockInactiveAccounts = (time_t)ConfigReadInt(_T("BlockInactiveUserAccounts"), 0) * 86400;

		s_userDatabaseLock.writeLock();
		time_t now = time(NULL);
		Iterator<UserDatabaseObject> it = s_userDatabase.begin();
		while(it.hasNext())
		{
		   UserDatabaseObject *object = it.next();
			if (object->isDeleted() || object->isGroup())
				continue;

			User *user = static_cast<User*>(object);

			if (user->isDisabled() && (user->getReEnableTime() > 0) && (user->getReEnableTime() <= now))
			{
				// Re-enable temporary disabled user
				user->enable();
            nxlog_debug_tag(DEBUG_TAG, 3, _T("Temporary disabled user account \"%s\" re-enabled"), user->getName());

            // Execute audit log in separate thread to avoid deadlock
				uint32_t userId = user->getId();
				String userName(user->getName());
				ThreadPoolExecute(g_mainThreadPool,
				   [userId, userName] () -> void
				   {
				      WriteAuditLog(AUDIT_SECURITY, true, userId, nullptr, AUDIT_SYSTEM_SID, 0, _T("Temporary disabled user account \"%s\" re-enabled by system"), userName.cstr());
				   });
			}

			if (!user->isDisabled() && (blockInactiveAccounts > 0) && (user->getLastLoginTime() > 0) && (user->getLastLoginTime() + blockInactiveAccounts < now) &&
			    ((user->getEnableTime() == 0) || (user->getEnableTime() + blockInactiveAccounts < now)))
			{
				user->disable();
            nxlog_debug_tag(DEBUG_TAG, 3, _T("User account \"%s\" disabled due to inactivity"), user->getName());

            // Execute audit log in separate thread to avoid deadlock
            uint32_t userId = user->getId();
            String userName(user->getName());
            ThreadPoolExecute(g_mainThreadPool,
               [userId, userName] () -> void
               {
                  WriteAuditLog(AUDIT_SECURITY, true, userId, nullptr, AUDIT_SYSTEM_SID, 0, _T("User account \"%s\" disabled by system due to inactivity"), userName.cstr());
               });
			}
		}
		s_userDatabaseLock.unlock();
	}

	nxlog_debug_tag(DEBUG_TAG, 2, _T("User account status update thread stopped"));
}

/**
 * Initialize user handling subsystem
 */
void InitUsers()
{
   s_users.setIgnoreCase((g_flags & AF_CASE_INSENSITIVE_LOGINS) != 0);
   s_groups.setIgnoreCase((g_flags & AF_CASE_INSENSITIVE_LOGINS) != 0);
	s_statusUpdateThread = ThreadCreateEx(AccountStatusUpdater);
}

/**
 * Cleanup user handling subsystem
 */
void CleanupUsers()
{
	ThreadJoin(s_statusUpdateThread);
}

/**
 * Load user list from database
 */
bool LoadUsers()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load users
   DB_RESULT hResult = DBSelect(hdb,
	                   _T("SELECT id,name,system_access,flags,description,guid,ldap_dn,")
							 _T("ldap_unique_id,created,ui_access_rules,password,full_name,grace_logins,")
							 _T("auth_method,cert_mapping_method,cert_mapping_data,auth_failures,")
							 _T("last_passwd_change,min_passwd_length,disabled_until,")
							 _T("last_login,email,phone_number FROM users"));
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   DB_HANDLE cachedb = (g_flags & AF_CACHE_DB_ON_STARTUP) ? DBOpenInMemoryDatabase() : nullptr;
   if (cachedb != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Caching user configuration tables"));
      if (!DBCacheTable(cachedb, hdb, _T("userdb_custom_attributes"), _T("object_id,attr_name"), _T("*")) ||
          !DBCacheTable(cachedb, hdb, _T("user_group_members"), _T("group_id,user_id"), _T("*")) ||
          !DBCacheTable(cachedb, hdb, _T("two_factor_auth_bindings"), _T("user_id,name"), _T("*")))
      {
         DBCloseInMemoryDatabase(cachedb);
         cachedb = nullptr;
      }
   }

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
		User *user = new User((cachedb != nullptr) ? cachedb : hdb, hResult, i);
		AddDatabaseObject(user);
   }

   DBFreeResult(hResult);

   // Create system account if it doesn't exist
   if (!s_userDatabase.contains(0))
   {
		User *user = new User();
		AddDatabaseObject(user);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("System account was created because it was not presented in database"));
   }

   // Load groups
   hResult = DBSelect(hdb, _T("SELECT id,name,system_access,flags,description,guid,ldap_dn,ldap_unique_id,created,ui_access_rules FROM user_groups"));
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      if (cachedb != nullptr)
         DBCloseInMemoryDatabase(cachedb);
      return false;
   }

   count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
		Group *group = new Group((cachedb != nullptr) ? cachedb : hdb, hResult, i);
		AddDatabaseObject(group);
   }

   DBFreeResult(hResult);

   // Create everyone group if it doesn't exist
   if (!s_userDatabase.contains(GROUP_EVERYONE))
   {
		Group *group = new Group();
		group->saveToDatabase(hdb);
		AddDatabaseObject(group);
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("User group \"Everyone\" was created because it was not presented in database"));
   }

   DBConnectionPoolReleaseConnection(hdb);
   if (cachedb != nullptr)
      DBCloseInMemoryDatabase(cachedb);
   return true;
}

/**
 * Save user list to database
 */
void SaveUsers(DB_HANDLE hdb, uint32_t watchdogId)
{
   // Save users
   s_userDatabaseLock.writeLock();
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      WatchdogNotify(watchdogId);
      UserDatabaseObject *object = it.next();
      if (object->isDeleted())
      {
			object->deleteFromDatabase(hdb);
			RemoveDatabaseObject(object);
			it.remove();
      }
		else if (object->isModified())
      {
			object->saveToDatabase(hdb);
      }
   }
   s_userDatabaseLock.unlock();
}

/**
 * Authenticate user
 * Checks if provided login name and password are correct, and returns RCC_SUCCESS
 * on success and appropriate RCC otherwise. On success authentication, user's ID is stored
 * int pdwId. If password authentication is used, dwSigLen should be set to zero.
 * If user already authenticated by SSO server, ssoAuth must be set to true. Password expiration,
 * change flag and grace count ignored for SSO logins.
 */
uint32_t AuthenticateUser(const TCHAR *login, const TCHAR *password, size_t sigLen, void *pCert,
         BYTE *pChallenge, uint32_t *pdwId, uint64_t *pdwSystemRights, bool *pbChangePasswd, bool *pbIntruderLockout,
         bool *closeOtherSessions, bool ssoAuth, uint32_t *graceLogins)
{
   s_userDatabaseLock.readLock();
   User *user = s_users.get(login);
   if ((user == nullptr) || user->isDeleted())
   {
      // no such user
      s_userDatabaseLock.unlock();
      return RCC_ACCESS_DENIED;
   }
   user = new User(user);  // create copy for authentication
   s_userDatabaseLock.unlock();

   *closeOtherSessions = false;
   *pdwId = user->getId(); // always set user ID for caller so audit log will contain correct user ID on failures as well

   if (!ssoAuth && (user->getFlags() & UF_TOKEN_AUTH_ONLY))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User \"%s\" can be authenticated only by token"), user->getName());
      delete user;
      return RCC_ACCESS_DENIED;
   }

   uint32_t rcc = RCC_ACCESS_DENIED;
   bool passwordValid = false;

   // Determine authentication method to use
   UserAuthenticationMethod authMethod = user->getAuthMethod();
   if (!ssoAuth)
   {
      if ((authMethod == UserAuthenticationMethod::CERTIFICATE_OR_LOCAL) || (authMethod == UserAuthenticationMethod::CERTIFICATE_OR_RADIUS))
      {
         if (sigLen > 0)
         {
            // certificate auth
            authMethod = UserAuthenticationMethod::CERTIFICATE;
         }
         else
         {
            authMethod = (authMethod == UserAuthenticationMethod::CERTIFICATE_OR_LOCAL) ? UserAuthenticationMethod::LOCAL : UserAuthenticationMethod::RADIUS;
         }
      }

      switch(authMethod)
      {
         case UserAuthenticationMethod::LOCAL:
            if (sigLen == 0)
            {
               passwordValid = user->validatePassword(password);
            }
            else
            {
               // We got certificate instead of password
               passwordValid = false;
            }
            break;
         case UserAuthenticationMethod::RADIUS:
            if (sigLen == 0)
            {
               passwordValid = RadiusAuth(login, password);
            }
            else
            {
               // We got certificate instead of password
               passwordValid = false;
            }
            break;
         case UserAuthenticationMethod::CERTIFICATE:
            if ((sigLen != 0) && (pCert != nullptr))
            {
               passwordValid = ValidateUserCertificate(static_cast<X509*>(pCert), login, pChallenge,
                     reinterpret_cast<const BYTE*>(password), sigLen, user->getCertMappingMethod(),
                     user->getCertMappingData());
            }
            else
            {
               // We got password instead of certificate
               passwordValid = false;
            }
            break;
         case UserAuthenticationMethod::LDAP:
            if (user->isLDAPUser())
            {
               rcc = LDAPConnection().ldapUserLogin(user->getDN(), password);
               if (rcc == RCC_SUCCESS)
                  passwordValid = true;
            }
            else
            {
               // Attempt for LDAP authentication for non-LDAP user
               nxlog_debug_tag(DEBUG_TAG, 4, _T("LDAP authentication request for local user \"%s\""), user->getName());
               passwordValid = false;
            }
            break;
         default:
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Unsupported authentication method %d requested for user %s"), user->getAuthMethod(), login);
            passwordValid = false;
            break;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("User \"%s\" already authenticated by SSO server"), user->getName());
      passwordValid = true;
   }

   delete user;   // delete copy
   s_userDatabaseLock.writeLock();
   user = s_users.get(login);
   if ((user == nullptr) || user->isDeleted() || (user->getId() != *pdwId))
   {
      // User was deleted while authenticating
      s_userDatabaseLock.unlock();
      return RCC_ACCESS_DENIED;
   }
   if (passwordValid)
   {
      if (!user->isDisabled())
      {
         user->resetAuthFailures();
         if (!ssoAuth)
         {
            if ((authMethod == UserAuthenticationMethod::LOCAL) && (user->getFlags() & UF_CHANGE_PASSWORD))
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Password for user \"%s\" need to be changed"), user->getName());
               if (user->getId() != 0)	// Do not check grace logins for built-in system user
               {
                  if (user->getGraceLogins() <= 0)
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, _T("User \"%s\" has no grace logins left"), user->getName());
                     rcc = RCC_NO_GRACE_LOGINS;
                  }
                  else
                  {
                     user->decreaseGraceLogins();
                  }
               }
               *pbChangePasswd = true;
            }
            else
            {
               // Check if password was expired
               int passwordExpirationTime = ConfigReadInt(_T("Server.Security.PasswordExpiration"), 0);
               if ((authMethod == UserAuthenticationMethod::LOCAL) &&
                   (passwordExpirationTime > 0) &&
                   ((user->getFlags() & UF_PASSWORD_NEVER_EXPIRES) == 0) &&
                   (time(nullptr) > user->getPasswordChangeTime() + passwordExpirationTime * 86400))
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, _T("Password for user \"%s\" has expired"), user->getName());
                  if (user->getId() != 0)	// Do not check grace logins for built-in system user
                  {
                     if (user->getGraceLogins() <= 0)
                     {
                        nxlog_debug_tag(DEBUG_TAG, 4, _T("User \"%s\" has no grace logins left"), user->getName());
                        rcc = RCC_NO_GRACE_LOGINS;
                     }
                     else
                     {
                        user->decreaseGraceLogins();
                     }
                  }
                  *pbChangePasswd = true;
               }
               else
               {
                  *pbChangePasswd = false;
               }
            }
         }
         else
         {
            *pbChangePasswd = false;
         }

         if (rcc != RCC_NO_GRACE_LOGINS)
         {
            *pdwSystemRights = GetEffectiveSystemRights(user);
            *closeOtherSessions = (user->getFlags() & UF_CLOSE_OTHER_SESSIONS) != 0;
            *graceLogins = user->getGraceLogins();
            user->updateLastLogin();
            rcc = RCC_SUCCESS;
         }
      }
      else
      {
         rcc = RCC_ACCOUNT_DISABLED;
      }
      *pbIntruderLockout = false;
   }
   else
   {
      user->increaseAuthFailures();
      *pbIntruderLockout = user->isIntruderLockoutActive();
   }
   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Validate user account and retrieve necessary information. Primarily intended for use by token authentication.
 * Fills in login name and system access rights on success, and RCC on failure.
 */
bool NXCORE_EXPORTABLE ValidateUserId(uint32_t id, TCHAR *loginName, uint64_t *systemAccess, uint32_t *rcc)
{
   bool success = false;
   s_userDatabaseLock.readLock();
   UserDatabaseObject *object = s_userDatabase.get(id);
   if ((object != nullptr) && !object->isGroup())
   {
      if (!object->isDisabled())
      {
         _tcslcpy(loginName, object->getName(), MAX_USER_NAME);
         *systemAccess = GetEffectiveSystemRights(static_cast<User*>(object));
         success = true;
      }
      else
      {
         *rcc = RCC_ACCOUNT_DISABLED;
      }
   }
   else
   {
      *rcc = RCC_INVALID_USER_ID;
   }
   s_userDatabaseLock.unlock();
   return success;
}

/**
 * Check if user is a member of specific child group
 */
bool CheckUserMembershipInternal(uint32_t userId, uint32_t groupId, GroupSearchPath *searchPath)
{
   Group *group = static_cast<Group*>(s_userDatabase.get(groupId));
   return (group != nullptr) ? group->isMember(userId, searchPath) : false;
}

/**
 * Check if user is a member of specific group
 */
bool NXCORE_EXPORTABLE CheckUserMembership(uint32_t userId, uint32_t groupId)
{
	if (!(groupId & GROUP_FLAG))
		return false;

   if (groupId == GROUP_EVERYONE)
		return true;

   bool result = false;
   GroupSearchPath searchPath;

   s_userDatabaseLock.readLock();

   result = CheckUserMembershipInternal(userId, groupId, &searchPath);

   s_userDatabaseLock.unlock();

   return result;
}

/**
 * Fill message with group membership information for given user.
 * Access to user database must be locked.
 */
void FillGroupMembershipInfo(NXCPMessage *msg, uint32_t userId)
{
   IntegerArray<uint32_t> list;
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
		if (object->isGroup() && (object->getId() != GROUP_EVERYONE) && ((Group *)object)->isMember(userId))
		{
         list.add(object->getId());
		}
   }
   msg->setField(VID_NUM_GROUPS, list.size());
   if (list.size() > 0)
      msg->setFieldFromInt32Array(VID_GROUPS, &list);
}

/**
 * Update group membership for user
 */
void UpdateGroupMembership(uint32_t userId, size_t numGroups, uint32_t *groups)
{
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
		if (object->isGroup() && (object->getId() != GROUP_EVERYONE))
		{
         bool found = false;
         for(size_t j = 0; j < numGroups; j++)
         {
            if (object->getId() == groups[j])
            {
               found = true;
               break;
            }
         }
         if (found)
         {
            static_cast<Group*>(object)->addUser(userId);
         }
         else
         {
            static_cast<Group*>(object)->deleteUser(userId);
         }
		}
   }
}

/**
 * Resolve user's ID to login name. Returns pointer to buffer on success and NULL on failure.
 * Buffer must be at least MAX_USER_NAME characters long.
 * If "noFail" flag is set, return string in form {id} if user cannot be found.
 */
TCHAR NXCORE_EXPORTABLE *ResolveUserId(uint32_t id, TCHAR *buffer, bool noFail)
{
   s_userDatabaseLock.readLock();
   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
      _tcslcpy(buffer, object->getName(), MAX_USER_NAME);
   else if (noFail)
      _sntprintf(buffer, MAX_USER_NAME, _T("{%u}"), id);
   s_userDatabaseLock.unlock();
	return ((object != nullptr) || noFail) ? buffer : nullptr;
}

/**
 * Resolve user name to ID. Returns user ID or INVALID_UID if login name is not known.
 */
uint32_t NXCORE_EXPORTABLE ResolveUserName(const TCHAR *loginName)
{
   s_userDatabaseLock.readLock();
   User *user = s_users.get(loginName);
   uint32_t id = (user != nullptr) ? user->getId() : INVALID_UID;
   s_userDatabaseLock.unlock();
   return id;
}

/**
 * Update system-wide access rights in given session
 */
static void UpdateGlobalAccessRightsCallback(ClientSession *session)
{
   session->updateSystemAccessRights();
}

/**
 * Update system-wide access rights in all user sessions
 */
static void UpdateGlobalAccessRights()
{
   EnumerateClientSessions(UpdateGlobalAccessRightsCallback);
}

/**
 * Thread pool wrapper for DeleteUserFromAllObjects
 */
static void DeleteUserFromAllObjectsWrapper(void *uid)
{
   DeleteUserFromAllObjects(CAST_FROM_POINTER(uid, uint32_t));
}

/**
 * Delete user or group
 *
 * @param id user database object ID
 * @param alreadyLocked true if user database lock already acquired
 * @return RCC ready to be sent to client
 */
static uint32_t DeleteUserDatabaseObjectInternal(uint32_t id, bool alreadyLocked)
{
   if (alreadyLocked)
   {
      // Running DeleteUserFromAllObjects with write lock set on user database
      // can cause deadlock if access right check is running in parallel
      ThreadPoolExecute(g_mainThreadPool, DeleteUserFromAllObjectsWrapper, CAST_TO_POINTER(id, void*));
   }
   else
   {
      DeleteUserFromAllObjects(id);
   }

   if (!alreadyLocked)
      s_userDatabaseLock.writeLock();

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
   {
      object->setDeleted();
      if (!(id & GROUP_FLAG))
      {
         Iterator<UserDatabaseObject> it = s_userDatabase.begin();
         while(it.hasNext())
         {
            UserDatabaseObject *group = it.next();
            if (group->getId() & GROUP_FLAG)
            {
               static_cast<Group*>(group)->deleteUser(id);
            }
         }
      }
   }

   if (!alreadyLocked)
      s_userDatabaseLock.unlock();

   // Update system access rights in all connected sessions
   // Use separate thread to avoid deadlocks
   if (id & GROUP_FLAG)
      ThreadPoolExecute(g_mainThreadPool, UpdateGlobalAccessRights);

   SendUserDBUpdate(USER_DB_DELETE, id, nullptr);
   return RCC_SUCCESS;
}

/**
 * Delete user or group
 *
 * @param id user database object ID
 * @return RCC ready to be sent to client
 */
uint32_t NXCORE_EXPORTABLE DeleteUserDatabaseObject(uint32_t id)
{
   return DeleteUserDatabaseObjectInternal(id, false);
}

/**
 * Create new user or group
 */
uint32_t NXCORE_EXPORTABLE CreateNewUser(const TCHAR *name, bool isGroup, uint32_t *id)
{
   uint32_t rcc = RCC_SUCCESS;

   s_userDatabaseLock.writeLock();

   // Check for duplicate name
   UserDatabaseObject *object = isGroup ? (UserDatabaseObject *)s_groups.get(name) : (UserDatabaseObject *)s_users.get(name);
   if (object != nullptr)
   {
      rcc = RCC_OBJECT_ALREADY_EXISTS;
   }

   if (rcc == RCC_SUCCESS)
   {
      if (isGroup)
      {
         object = new Group(CreateUniqueId(IDG_USER_GROUP), name);
      }
      else
      {
         object = new User(CreateUniqueId(IDG_USER), name);
      }
      AddDatabaseObject(object);
      SendUserDBUpdate(USER_DB_CREATE, object->getId(), object);
      *id = object->getId();
   }

   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Modify user database object
 */
uint32_t NXCORE_EXPORTABLE ModifyUserDatabaseObject(const NXCPMessage& msg, json_t **oldData, json_t **newData)
{
   uint32_t rcc = RCC_INVALID_USER_ID;
   bool updateAccessRights = false;

   uint32_t id = msg.getFieldAsUInt32(VID_USER_ID);

   s_userDatabaseLock.writeLock();

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
   {
      TCHAR name[MAX_USER_NAME], prevName[MAX_USER_NAME];

      uint32_t fields = msg.getFieldAsUInt32(VID_FIELDS);
      if (fields & USER_MODIFY_LOGIN_NAME)
      {
         msg.getFieldAsString(VID_USER_NAME, name, MAX_USER_NAME);
         if (IsValidObjectName(name))
         {
            _tcslcpy(prevName, object->getName(), MAX_USER_NAME);
         }
         else
         {
            rcc = RCC_INVALID_OBJECT_NAME;
         }
      }

      if (rcc != RCC_INVALID_OBJECT_NAME)
      {
         *oldData = object->toJson();
         object->modifyFromMessage(msg);
         *newData = object->toJson();
         SendUserDBUpdate(USER_DB_MODIFY, id, object);
         rcc = RCC_SUCCESS;
         updateAccessRights = ((msg.getFieldAsUInt32(VID_FIELDS) & (USER_MODIFY_ACCESS_RIGHTS | USER_MODIFY_GROUP_MEMBERSHIP | USER_MODIFY_MEMBERS)) != 0);
      }

      if ((rcc == RCC_SUCCESS) && (fields & USER_MODIFY_LOGIN_NAME))
      {
         // update login names hash map if login name was modified
         if (_tcscmp(prevName, object->getName()))
         {
            if (object->isGroup())
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("Group rename: %s -> %s"), prevName, object->getName());
               s_groups.remove(prevName);
               s_groups.set(object->getName(), (Group *)object);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("User rename: %s -> %s"), prevName, object->getName());
               s_users.remove(prevName);
               s_users.set(object->getName(), (User *)object);
            }
         }
      }
   }

   s_userDatabaseLock.unlock();

   // Use separate thread to avoid deadlocks
   if (updateAccessRights)
      ThreadPoolExecute(g_mainThreadPool, UpdateGlobalAccessRights);

   return rcc;
}

/**
 * Mark user database object as modified.
 * Executed in background thread to avoid possible deadlocks if called from code that is within low-level lock on user data.
 */
void MarkUserDatabaseObjectAsModified(uint32_t id)
{
   ThreadPoolExecute(g_mainThreadPool, [id]() -> void {
      s_userDatabaseLock.writeLock();
      UserDatabaseObject *object = s_userDatabase.get(id);
      if (object != nullptr)
      {
         object->setModified();
         SendUserDBUpdate(USER_DB_MODIFY, id, object);
      }
      s_userDatabaseLock.unlock();
   });
}

/**
 * Check if provided user name is not used or belongs to given user.
 * Access to user DB must be locked when this function is called
 */
static inline bool UserNameIsUnique(const TCHAR *name, User *user)
{
   User *u = s_users.get(name);
   return (u == nullptr) || ((user != nullptr) && (user->getId() == u->getId()));
}

/**
 * Check if provided group name is not used or belongs to given group.
 * Access to user DB must be locked when this function is called
 */
static inline bool GroupNameIsUnique(const TCHAR *name, Group *group)
{
   Group *g = s_groups.get(name);
   return (g == nullptr) || ((group != nullptr) && (group->getId() == g->getId()));
}

/**
 * Generates unique name for LDAP user
 */
static inline TCHAR *GenerateUniqueName(const TCHAR *ldapName, uint32_t userId, TCHAR *buffer)
{
   _sntprintf(buffer, MAX_USER_NAME, _T("%s_LDAP%u"), ldapName, userId);
   return buffer;
}

/**
 * Update/Add LDAP user
 */
void UpdateLDAPUser(const TCHAR *dn, const LDAP_Object *ldapObject)
{
   s_userDatabaseLock.writeLock();

   bool userModified = false;
   bool conflict = false;
   TCHAR description[1024];

   // Check existing user with same DN
   UserDatabaseObject *object = nullptr;
   if (ldapObject->m_id != nullptr)
   {
      object = s_ldapGroupId.get(ldapObject->m_id);
      if (object == nullptr)
      {
         object = s_ldapUserId.get(ldapObject->m_id);
      }
   }
   else
   {
      object = s_ldapNames.get(dn);
   }

   if ((object != nullptr) && object->isGroup())
   {
      _sntprintf(description, MAX_USER_DESCR, _T("Got user with DN=%s but found existing group %s with same DN"), dn, object->getName());
      EventBuilder(EVENT_LDAP_SYNC_ERROR, g_dwMgmtNode)
         .param(_T("userId"), object->getId(), EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("userGuid"), object->getGuid())
         .param(_T("userLdapDn"), object->getDN())
         .param(_T("userName"), object->getName())
         .param(_T("description"), description)
         .post(); 
      nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateLDAPUser(): %s"), description);
      conflict = true;
   }

   if ((object != nullptr) && !conflict)
   {
      User *user = static_cast<User*>(object);
      if (!user->isDeleted())
      {
         user->removeSyncException();
         if (UserNameIsUnique(ldapObject->m_loginName, user))
         {
            user->setName(ldapObject->m_loginName);
         }
         else
         {
            TCHAR userName[MAX_USER_NAME];
            GenerateUniqueName(ldapObject->m_loginName, user->getId(), userName);
            if (_tcscmp(user->getName(), userName))
            {
               user->setName(userName);
               _sntprintf(description, 1024, _T("User with name \"%s\" already exists. Unique user name have been generated: \"%s\""), ldapObject->m_loginName, userName);
               EventBuilder(EVENT_LDAP_SYNC_ERROR, g_dwMgmtNode)
                  .param(_T("userId"), user->getId(), EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("userGuid"), user->getGuid())
                  .param(_T("userLdapDn"), user->getDN())
                  .param(_T("userName"), user->getName())
                  .param(_T("description"), description)
                  .post(); 
               nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateLDAPUser(): %s"), description);
            }
         }

         user->setFullName(ldapObject->m_fullName);
         user->setDescription(ldapObject->m_description);
         user->setEmail(ldapObject->m_email);
         user->setPhoneNumber(ldapObject->m_phoneNumber);
         if (_tcscmp(user->getDN(), dn))
         {
            s_ldapNames.remove(user->getDN());
            user->setDN(dn);
            s_ldapNames.set(dn, user);
         }

         if (user->isModified())
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateLDAPUser(): User updated: ID: %s DN: %s, login name: %s, full name: %s, description: %s"), CHECK_NULL(ldapObject->m_id), dn, ldapObject->m_loginName, CHECK_NULL(ldapObject->m_fullName), CHECK_NULL(ldapObject->m_description));
            SendUserDBUpdate(USER_DB_MODIFY, user->getId(), user);
         }
      }
      userModified = true;
   }

   if (!userModified && !conflict)
   {
      uint32_t userId = CreateUniqueId(IDG_USER);
      TCHAR userNameBuffer[MAX_USER_NAME];
      const TCHAR *userName;
      bool uniqueName = UserNameIsUnique(ldapObject->m_loginName, nullptr);
      if (uniqueName)
      {
         userName = ldapObject->m_loginName;
      }
      else
      {
         userName = GenerateUniqueName(ldapObject->m_loginName, userId, userNameBuffer);
      }

      int method = ConfigReadInt(_T("LDAP.NewUserAuthMethod"), static_cast<int>(UserAuthenticationMethod::LDAP));
      if ((method < 0) || (method > 5))
         method = static_cast<int>(UserAuthenticationMethod::LDAP);
      User *user = new User(userId, userName, UserAuthenticationMethodFromInt(method));
      user->setFullName(ldapObject->m_fullName);
      user->setDescription(ldapObject->m_description);
      user->setEmail(ldapObject->m_email);
      user->setPhoneNumber(ldapObject->m_phoneNumber);
      if ((method == static_cast<int>(UserAuthenticationMethod::LOCAL)) || (method == static_cast<int>(UserAuthenticationMethod::CERTIFICATE_OR_LOCAL)))
         user->setFlags(UF_MODIFIED | UF_LDAP_USER | UF_CHANGE_PASSWORD);
      else
         user->setFlags(UF_MODIFIED | UF_LDAP_USER);
      user->setDN(dn);
      if (ldapObject->m_id != nullptr)
         user->setLdapId(ldapObject->m_id);

      AddDatabaseObject(user);
      SendUserDBUpdate(USER_DB_CREATE, user->getId(), user);

      if (!uniqueName)
      {
         _sntprintf(description, 1024, _T("User with name \"%s\" already exists. Unique user name have been generated: \"%s\""), ldapObject->m_loginName, userName);
         nxlog_debug_tag(DEBUG_TAG, 4,  _T("UpdateLDAPUser(): %s"), description);
         EventBuilder(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode)
            .param(_T("userId"), user->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("userGuid"), user->getGuid())
            .param(_T("userLdapDn"), user->getDN())
            .param(_T("userName"), user->getName())
            .param(_T("description"), description)
            .post(); 
      }
      nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateLDAPUser(): User added: ID: %s DN: %s, login name: %s, full name: %s, description: %s"), CHECK_NULL(ldapObject->m_id), dn, userName, CHECK_NULL(ldapObject->m_fullName), CHECK_NULL(ldapObject->m_description));
   }
   s_userDatabaseLock.unlock();
}

/**
 * Goes through all existing LDAP entries and check that in newly gotten list they also exist.
 * If LDAP entries does not exists in new list - it will be disabled or removed depending on action parameter.
 */
void RemoveDeletedLDAPEntries(StringObjectMap<LDAP_Object> *entryListDn, StringObjectMap<LDAP_Object> *entryListId, uint32_t m_action, bool isUser)
{
   s_userDatabaseLock.writeLock();
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
      if (!object->isLDAPUser() || object->isDeleted())
         continue;

      if (isUser ? ((object->getId() & GROUP_FLAG) == 0) : ((object->getId() & GROUP_FLAG) != 0))
		{
         if (((object->getLdapId() == nullptr) || !entryListId->contains(object->getLdapId())) && !entryListDn->contains(object->getDN()))
         {
            if (m_action == USER_DELETE)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("RemoveDeletedLDAPEntry(): LDAP %s object %s was removed from user database"), isUser ? _T("user") : _T("group"), object->getDN());
               DeleteUserDatabaseObjectInternal(object->getId(), true);
            }
            else if (m_action == USER_DISABLE)
            {
               nxlog_debug_tag(DEBUG_TAG, 4, _T("RemoveDeletedLDAPEntry(): LDAP %s object %s was disabled"), isUser ? _T("user") : _T("group"), object->getDN());
               object->disable();
               object->setDescription(_T("LDAP entry was deleted."));
            }
         }
		}
   }
   s_userDatabaseLock.unlock();
}

/**
 * Synchronize new user/group list with old user/group list of given group.
 * Note: none LDAP users and groups will not be changed.
 */
void SyncLDAPGroupMembers(const TCHAR *dn, const LDAP_Object *ldapObject)
{
   s_userDatabaseLock.writeLock();

   // Check existing user/group with same DN
   UserDatabaseObject *object;
   if (ldapObject->m_id != nullptr)
      object = s_ldapGroupId.get(ldapObject->m_id);
   else
      object = s_ldapNames.get(dn);

   if ((object != nullptr) && !object->isGroup())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SyncGroupMembers(): Ignore LDAP object %s"), dn);
      s_userDatabaseLock.unlock();
      return;
   }

   if (object == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("SyncGroupMembers(): Unable to find LDAP object %s"), dn);
      s_userDatabaseLock.unlock();
      return;
   }

   Group *group = static_cast<Group*>(object);

   nxlog_debug_tag(DEBUG_TAG, 4, _T("SyncGroupMembers(): Sync LDAP group %s"), dn);

   const StringSet& newMembers = ldapObject->m_memberList;
   const IntegerArray<uint32_t>& oldMembers = group->getMembers();

   /*
    * Go through existing group member list checking each LDAP user/group by DN
    * with new gotten group member list and removing LDAP users/groups not existing in last list.
    */
   for(int i = 0; i < oldMembers.size(); i++)
   {
      UserDatabaseObject *user = s_userDatabase.get(oldMembers.get(i));
      if ((user == nullptr) || !user->isLDAPUser())
         continue;

      if (!newMembers.contains(user->getDN()))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SyncGroupMembers: Remove LDAP user %s from LDAP group %s"), user->getDN(), group->getDN());
         group->deleteUser(user->getId());   // oldMembers is a reference to actual member list, Group::deleteUser will update it
         i--;
      }
   }

   /*
    * Go through newly received group member list checking each LDAP user/group by DN
    * with existing group member list and adding users/groups not existing in last list.
    */
   auto it = newMembers.begin();
   while(it.hasNext())
   {
      const TCHAR *dn = it.next();
      UserDatabaseObject *user = s_ldapNames.get(dn);
      if (user == nullptr)
         continue;

      if (!group->isMember(user->getId()))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("SyncGroupMembers: LDAP user %s added to LDAP group %s"), user->getDN(), group->getDN());
         group->addUser(user->getId());
      }
   }

   s_userDatabaseLock.unlock();
}

/**
 * Update/Add LDAP group
 */
void UpdateLDAPGroup(const TCHAR *dn, const LDAP_Object *ldapObject) //no full name, add users inside group, and delete removed from the group
{
   s_userDatabaseLock.writeLock();

   bool groupModified = false;
   bool conflict = false;
   TCHAR description[1024];

   // Check existing user with same DN
   UserDatabaseObject *object = nullptr;

   if (ldapObject->m_id != nullptr)
   {
      object = s_ldapUserId.get(ldapObject->m_id);
      if (object == nullptr)
      {
         object = s_ldapGroupId.get(ldapObject->m_id);
      }
   }
   else
   {
      object = s_ldapNames.get(dn);
   }

   if ((object != nullptr) && !object->isGroup())
   {
      _sntprintf(description, MAX_USER_DESCR, _T("Got group with DN=%s but found existing user %s with same DN"), dn, object->getName());
      EventBuilder(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode)
         .param(_T("userId"), object->getId(), EventBuilder::OBJECT_ID_FORMAT)
         .param(_T("userGuid"), object->getGuid())
         .param(_T("userLdapDn"), object->getDN())
         .param(_T("userName"), object->getName())
         .param(_T("description"), description)
         .post(); 
      nxlog_debug_tag(DEBUG_TAG, 4,  _T("UpdateLDAPGroup(): %s"), description);
      conflict = true;
   }

   if ((object != nullptr) && !conflict)
   {
      Group *group = static_cast<Group*>(object);
      if (!group->isDeleted())
      {
         group->removeSyncException();
         if (GroupNameIsUnique(ldapObject->m_loginName, group))
         {
            group->setName(ldapObject->m_loginName);
         }
         else
         {
            TCHAR groupName[MAX_USER_NAME];
            GenerateUniqueName(ldapObject->m_loginName, group->getId(), groupName);
            if (_tcscmp(group->getName(), groupName))
            {
               group->setName(groupName);
               _sntprintf(description, MAX_USER_DESCR, _T("Group with name \"%s\" already exists. Unique group name have been generated: \"%s\""), ldapObject->m_loginName, groupName);
               EventBuilder(EVENT_LDAP_SYNC_ERROR, g_dwMgmtNode)
                  .param(_T("userId"), group->getId(), EventBuilder::OBJECT_ID_FORMAT)
                  .param(_T("userGuid"), group->getGuid())
                  .param(_T("userLdapDn"), group->getDN())
                  .param(_T("userName"), group->getName())
                  .param(_T("description"), description)
                  .post(); 
               nxlog_debug_tag(DEBUG_TAG, 4,  _T("UpdateLDAPGroup(): %s"),description);
            }
         }

         group->setDescription(ldapObject->m_description);
         if (_tcscmp(group->getDN(), dn))
         {
            s_ldapNames.remove(group->getDN());
            group->setDN(dn);
            s_ldapNames.set(dn, group);
         }

         if (group->isModified())
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateLDAPGroup(): Group updated: ID: %s DN: %s, login name: %s, description: %s"), CHECK_NULL(ldapObject->m_id), dn, ldapObject->m_loginName, CHECK_NULL(ldapObject->m_description));
            SendUserDBUpdate(USER_DB_MODIFY, group->getId(), group);
         }
      }
      groupModified = true;
   }

   if (!groupModified)
   {
      uint32_t groupId = CreateUniqueId(IDG_USER_GROUP);
      TCHAR groupNameBuffer[MAX_USER_NAME];
      const TCHAR *groupName;
      bool uniqueName = GroupNameIsUnique(ldapObject->m_loginName, nullptr);
      if (uniqueName)
      {
         groupName = ldapObject->m_loginName;
      }
      else
      {
         groupName = GenerateUniqueName(ldapObject->m_loginName, groupId, groupNameBuffer);
      }

      Group *group = new Group(groupId, groupName);
      group->setDescription(ldapObject->m_description);
      group->setFlags(UF_MODIFIED | UF_LDAP_USER);
      group->setDN(dn);
      if (ldapObject->m_id != nullptr)
         group->setLdapId(ldapObject->m_id);

      if (!uniqueName)
      {
         _sntprintf(description, MAX_USER_DESCR, _T("Group with name \"%s\" already exists. Unique group name have been generated: \"%s\""), ldapObject->m_loginName, groupName);
         nxlog_debug_tag(DEBUG_TAG, 4,  _T("UpdateLDAPGroup(): %s"),description);
         EventBuilder(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode)
            .param(_T("userId"), group->getId(), EventBuilder::OBJECT_ID_FORMAT)
            .param(_T("userGuid"), group->getGuid())
            .param(_T("userLdapDn"), group->getDN())
            .param(_T("userName"), group->getName())
            .param(_T("description"), description)
            .post(); 
      }

      SendUserDBUpdate(USER_DB_CREATE, group->getId(), group);
      AddDatabaseObject(group);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("UpdateLDAPGroup(): Group added: ID: %s DN: %s, login name: %s, description: %s"), CHECK_NULL(ldapObject->m_id), dn, ldapObject->m_loginName, CHECK_NULL(ldapObject->m_description));
   }
   s_userDatabaseLock.unlock();
}

/**
 * Dump user list to console
 *
 * @param pCtx console context
 */
void DumpUsers(CONSOLE_CTX pCtx)
{
   ConsolePrintf(pCtx, _T("Login name           GUID                                 System rights\n")
                       _T("-----------------------------------------------------------------------\n"));

   s_userDatabaseLock.readLock();
   Iterator<UserDatabaseObject> it = s_userDatabase.begin();
   while(it.hasNext())
   {
      UserDatabaseObject *object = it.next();
		if (!object->isGroup())
      {
         TCHAR szGUID[64];
         UINT64 systemRights = GetEffectiveSystemRights((User *)object);
         ConsolePrintf(pCtx, _T("%-20s %-36s 0x") UINT64X_FMT(_T("016")) _T("\n"), object->getName(), object->getGuidAsText(szGUID), systemRights);
		}
	}
   s_userDatabaseLock.unlock();
   ConsolePrintf(pCtx, _T("\n"));
}

/**
 * Modify user database object
 */
uint32_t NXCORE_EXPORTABLE DetachLDAPUser(uint32_t id)
{
   uint32_t rcc = RCC_INVALID_USER_ID;

   s_userDatabaseLock.writeLock();

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
   {
      s_ldapNames.remove(object->getDN());
      object->detachLdapUser();
      SendUserDBUpdate(USER_DB_MODIFY, id, object);
      rcc = RCC_SUCCESS;
   }

   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Send user DB update for given user ID.
 * Access to user database must be already locked.
 */
void SendUserDBUpdate(uint16_t code, uint32_t id)
{
   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
   {
      SendUserDBUpdate(code, id, object);
   }
}

/**
 * Check if string contains subsequence of given sequence
 */
static bool IsStringContainsSubsequence(const TCHAR *str, const TCHAR *sequence, int len)
{
	int sequenceLen = (int)_tcslen(sequence);
	if ((sequenceLen < len) || (len > 255))
		return false;

	TCHAR subseq[256];
	for(int i = 0; i < sequenceLen - len; i++)
	{
		_tcslcpy(subseq, &sequence[i], len + 1);
		if (_tcsstr(str, subseq) != NULL)
			return true;
	}

	return false;
}

/**
 * Check password's complexity
 */
static bool CheckPasswordComplexity(const TCHAR *password)
{
   int flags = ConfigReadInt(_T("Server.Security.PasswordComplexity"), 0);

	if ((flags & PSWD_MUST_CONTAIN_DIGITS) && (_tcspbrk(password, _T("0123456789")) == NULL))
		return false;

	if ((flags & PSWD_MUST_CONTAIN_UPPERCASE) && (_tcspbrk(password, _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ")) == NULL))
		return false;

	if ((flags & PSWD_MUST_CONTAIN_LOWERCASE) && (_tcspbrk(password, _T("abcdefghijklmnopqrstuvwxyz")) == NULL))
		return false;

	if ((flags & PSWD_MUST_CONTAIN_SPECIAL_CHARS) && (_tcspbrk(password, _T("`~!@#$%^&*()_-=+{}[]|\\'\";:,.<>/?")) == NULL))
		return false;

	if (flags & PSWD_FORBID_ALPHABETICAL_SEQUENCE)
	{
		if (IsStringContainsSubsequence(password, _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("abcdefghijklmnopqrstuvwxyz"), 3))
			return false;
	}

	if (flags & PSWD_FORBID_KEYBOARD_SEQUENCE)
	{
		if (IsStringContainsSubsequence(password, _T("~!@#$%^&*()_+"), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("1234567890-="), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("qwertyuiop[]"), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("asdfghjkl;'"), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("zxcvbnm,./"), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("QWERTYUIOP{}"), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("ASDFGHJKL:\""), 3))
			return false;
		if (IsStringContainsSubsequence(password, _T("ZXCVBNM<>?"), 3))
			return false;
	}

	return true;
}

/**
 * Set user's password
 */
uint32_t NXCORE_EXPORTABLE SetUserPassword(uint32_t id, const wchar_t *newPassword, const wchar_t *oldPassword, bool changeOwnPassword)
{
	if (id & GROUP_FLAG)
		return RCC_INVALID_USER_ID;

   s_userDatabaseLock.writeLock();

   // Find user
   User *user = static_cast<User*>(s_userDatabase.get(id));
   if (user == nullptr)
   {
      s_userDatabaseLock.unlock();
      return RCC_INVALID_USER_ID;
   }

   uint32_t rcc = RCC_INVALID_USER_ID;
   if (changeOwnPassword)
   {
      if (!user->canChangePassword() || !user->validatePassword(oldPassword))
      {
         rcc = RCC_ACCESS_DENIED;
         goto finish;
      }

      // Check password length
      int minLength = (user->getMinMasswordLength() == -1) ? ConfigReadInt(L"Server.Security.MinPasswordLength", 0) : user->getMinMasswordLength();
      if ((int)_tcslen(newPassword) < minLength)
      {
         rcc = RCC_WEAK_PASSWORD;
         goto finish;
      }

      // Check password complexity
      if (!CheckPasswordComplexity(newPassword))
      {
         rcc = RCC_WEAK_PASSWORD;
         goto finish;
      }

      // Update password history
      int passwordHistoryLength = ConfigReadInt(_T("Server.Security.PasswordHistoryLength"), 0);
      if (passwordHistoryLength > 0)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         TCHAR query[8192], *ph = nullptr;

         _sntprintf(query, 8192, _T("SELECT password_history FROM users WHERE id=%d"), id);
         DB_RESULT hResult = DBSelect(hdb, query);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               ph = DBGetField(hResult, 0, 0, nullptr, 0);
            }
            DBFreeResult(hResult);
         }

         if (ph != nullptr)
         {
            BYTE newPasswdHash[SHA1_DIGEST_SIZE];
            char *mb = UTF8StringFromWideString(newPassword);
            CalculateSHA1Hash((BYTE *)mb, strlen(mb), newPasswdHash);
            MemFree(mb);

            int phLen = (int)_tcslen(ph) / (SHA1_DIGEST_SIZE * 2);
            if (phLen > passwordHistoryLength)
               phLen = passwordHistoryLength;

            for(int i = 0; i < phLen; i++)
            {
               BYTE hash[SHA1_DIGEST_SIZE];
               StrToBin(&ph[i * SHA1_DIGEST_SIZE * 2], hash, SHA1_DIGEST_SIZE);
               if (!memcmp(hash, newPasswdHash, SHA1_DIGEST_SIZE))
               {
                  rcc = RCC_REUSED_PASSWORD;
                  goto finish;
               }
            }

            if (rcc != RCC_REUSED_PASSWORD)
            {
               if (phLen == passwordHistoryLength)
               {
                  memmove(ph, &ph[SHA1_DIGEST_SIZE * 2], (phLen - 1) * SHA1_DIGEST_SIZE * 2 * sizeof(TCHAR));
               }
               else
               {
                  ph = MemRealloc(ph, (phLen + 1) * SHA1_DIGEST_SIZE * 2 * sizeof(TCHAR) + sizeof(TCHAR));
                  phLen++;
               }
               BinToStr(newPasswdHash, SHA1_DIGEST_SIZE, &ph[(phLen - 1) * SHA1_DIGEST_SIZE * 2]);

               _sntprintf(query, 8192, _T("UPDATE users SET password_history='%s' WHERE id=%d"), ph, id);
               DBQuery(hdb, query);
            }

            MemFree(ph);
            if (rcc == RCC_REUSED_PASSWORD)
               goto finish;
         }
         else
         {
            rcc = RCC_DB_FAILURE;
            goto finish;
         }
         DBConnectionPoolReleaseConnection(hdb);
      }

      user->updatePasswordChangeTime();
   }
   user->setPassword(newPassword, changeOwnPassword);
   rcc = RCC_SUCCESS;

finish:
   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Validate user's password
 */
uint32_t NXCORE_EXPORTABLE ValidateUserPassword(uint32_t userId, const wchar_t *login, const wchar_t *password, bool *isValid)
{
	if (userId & GROUP_FLAG)
		return RCC_INVALID_USER_ID;

	uint32_t rcc = RCC_INVALID_USER_ID;
   s_userDatabaseLock.readLock();

   // Find user
   User *user = static_cast<User*>(s_userDatabase.get(userId));
   if (user != nullptr)
   {
      rcc = RCC_SUCCESS;
      switch(user->getAuthMethod())
      {
         case UserAuthenticationMethod::LOCAL:
         case UserAuthenticationMethod::CERTIFICATE_OR_LOCAL:
            *isValid = user->validatePassword(password);
            break;
         case UserAuthenticationMethod::RADIUS:
         case UserAuthenticationMethod::CERTIFICATE_OR_RADIUS:
            *isValid = RadiusAuth(login, password);
            break;
         case UserAuthenticationMethod::LDAP:
            if (user->isLDAPUser())
            {
               rcc = LDAPConnection().ldapUserLogin(user->getDN(), password);
               if (rcc == RCC_SUCCESS)
               {
                  *isValid = true;
               }
               else if (rcc == RCC_ACCESS_DENIED)
               {
                  *isValid = false;
                  rcc = RCC_SUCCESS;
               }
            }
            else
            {
               *isValid = false;
            }
            break;
         default:
            rcc = RCC_UNSUPPORTED_AUTH_METHOD;
            break;
      }
   }

   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Open user database
 */
Iterator<UserDatabaseObject> NXCORE_EXPORTABLE OpenUserDatabase()
{
   s_userDatabaseLock.readLock();
	return s_userDatabase.begin();
}

/**
 * Close user database
 */
void NXCORE_EXPORTABLE CloseUserDatabase()
{
   s_userDatabaseLock.unlock();
}

/**
 * Find a list of UserDatabaseObjects of specific id`s
 */
unique_ptr<ObjectArray<UserDatabaseObject>> FindUserDBObjects(const IntegerArray<uint32_t>& ids)
{
   auto userDB = make_unique<ObjectArray<UserDatabaseObject>>(ids.size(), 16, Ownership::True);
   s_userDatabaseLock.readLock();
   for(int i = 0; i < ids.size(); i++)
   {
      UserDatabaseObject *object = s_userDatabase.get(ids.get(i));
      if (object != nullptr)
      {
         if (object->isGroup())
            userDB->add(new Group(static_cast<Group*>(object)));
         else
            userDB->add(new User(static_cast<User*>(object)));
      }
   }
   s_userDatabaseLock.unlock();
   return userDB;
}

/**
 * Find a list of UserDatabaseObjects of specific id`s
 */
unique_ptr<ObjectArray<UserDatabaseObject>> FindUserDBObjects(const StructArray<ResponsibleUser>& ids)
{
   auto userDB = make_unique<ObjectArray<UserDatabaseObject>>(ids.size(), 16, Ownership::True);
   s_userDatabaseLock.readLock();
   for(int i = 0; i < ids.size(); i++)
   {
      UserDatabaseObject *object = s_userDatabase.get(ids.get(i)->userId);
      if (object != nullptr)
      {
         if (object->isGroup())
            userDB->add(new Group(static_cast<Group*>(object)));
         else
            userDB->add(new User(static_cast<User*>(object)));
      }
   }
   s_userDatabaseLock.unlock();
   return userDB;
}

/**
 * Get user database object for use in NXSL script
 */
NXSL_Value *GetUserDBObjectForNXSL(uint32_t id, NXSL_VM *vm)
{
   s_userDatabaseLock.readLock();
   UserDatabaseObject *object = s_userDatabase.get(id);
   NXSL_Value *value;
   if (object != nullptr)
   {
      if (object->isGroup())
         object = new Group(static_cast<Group*>(object));
      else
         object = new User(static_cast<User*>(object));
      value = object->createNXSLObject(vm);  // NXSL object will keep pointer to user DB object and destroy it when destroyed by NXSL VM
   }
   else
   {
      value = vm->createValue();
   }
   s_userDatabaseLock.unlock();
   return value;
}

/**
 * Fill NXCP message 2FA method binding info for given user
 */
void FillUser2FAMethodBindingInfo(uint32_t userId, NXCPMessage *msg)
{
   s_userDatabaseLock.readLock();
   UserDatabaseObject *object = s_userDatabase.get(userId);
   if ((object != nullptr) && !object->isGroup())
   {
      static_cast<User*>(object)->fill2FAMethodBindingInfo(msg);
   }
   s_userDatabaseLock.unlock();
}

/**
 * Get 2FA method binding names for specific user
 */
unique_ptr<StringList> NXCORE_EXPORTABLE GetUserConfigured2FAMethods(uint32_t userId)
{
   unique_ptr<StringList> methods;
   s_userDatabaseLock.readLock();
   UserDatabaseObject *object = s_userDatabase.get(userId);
   if ((object != nullptr) && !object->isGroup())
   {
      methods = static_cast<User*>(object)->getConfigured2FAMethods();
   }
   s_userDatabaseLock.unlock();
   return methods;
}

/**
 * Searching for 2FA method binding for selected user and 2FA method
 */
shared_ptr<Config> GetUser2FAMethodBinding(int userId, const TCHAR* methodName)
{
   shared_ptr<Config> binding;
   s_userDatabaseLock.readLock();
   UserDatabaseObject *object = s_userDatabase.get(userId);
   if ((object != nullptr) && !object->isGroup())
   {
      binding = static_cast<User*>(object)->get2FAMethodBinding(methodName);
   }
   s_userDatabaseLock.unlock();
   return binding;
}

/**
 * Create/Modify 2FA method binding for specific user
 */
uint32_t ModifyUser2FAMethodBinding(uint32_t userId, const TCHAR* methodName, const StringMap& configuration)
{
   uint32_t rcc = RCC_INVALID_USER_ID;
   s_userDatabaseLock.writeLock();
   UserDatabaseObject *object = s_userDatabase.get(userId);
   if ((object != nullptr) && !object->isGroup())
   {
      rcc = static_cast<User*>(object)->modify2FAMethodBinding(methodName, configuration);
   }
   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Delete 2FA method binding for specific user
 */
uint32_t DeleteUser2FAMethodBinding(uint32_t userId, const TCHAR* methodName)
{
   uint32_t rcc = RCC_INVALID_USER_ID;
   s_userDatabaseLock.writeLock();
   UserDatabaseObject *object = s_userDatabase.get(userId);
   if ((object != nullptr) && !object->isGroup())
   {
      rcc = static_cast<User*>(object)->delete2FAMethodBinding(methodName);
   }
   s_userDatabaseLock.unlock();
   return rcc;
}

/**
 * Get custom attribute's value
 */
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(uint32_t id, const TCHAR *name)
{
	const TCHAR *value = nullptr;

   s_userDatabaseLock.readLock();

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
      value = object->getAttribute(name);

   s_userDatabaseLock.unlock();
	return value;
}

/**
 * Get custom attribute's value as unsigned integer
 */
uint32_t NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(uint32_t id, const TCHAR *name)
{
	const TCHAR *value = GetUserDbObjectAttr(id, name);
	return (value != nullptr) ? _tcstoul(value, nullptr, 0) : 0;
}

/**
 * Enumerate all custom attributes for given user database object
 */
void NXCORE_EXPORTABLE EnumerateUserDbObjectAttributes(uint32_t id, EnumerationCallbackResult (*callback)(const TCHAR*, const TCHAR*, void*), void *context)
{
   s_userDatabaseLock.readLock();

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
      object->enumerateCustomAttributes(callback, context);

   s_userDatabaseLock.unlock();
}

/**
 * Set custom attribute's value
 */
void NXCORE_EXPORTABLE SetUserDbObjectAttr(uint32_t id, const TCHAR *name, const TCHAR *value)
{
   s_userDatabaseLock.writeLock();
   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != nullptr)
   {
      object->setAttribute(name, value);
   }
   s_userDatabaseLock.unlock();
}
