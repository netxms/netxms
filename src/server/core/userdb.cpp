/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
bool RadiusAuth(const TCHAR *pszLogin, const TCHAR *pszPasswd);

/**
 * Static data
 */
static HashMap<UINT32, UserDatabaseObject> s_userDatabase(true);
static StringObjectMap<UserDatabaseObject> s_ldapNames(false);
static StringObjectMap<User> s_ldapUserId(false);
static StringObjectMap<Group> s_ldapGroupId(false);
static StringObjectMap<User> s_users(false);
static StringObjectMap<Group> s_groups(false);
static RWLOCK s_userDatabaseLock = RWLockCreate();
static THREAD s_statusUpdateThread = INVALID_THREAD_HANDLE;

/**
 * Compare user names
 */
inline bool UserNameEquals(const TCHAR *n1, const TCHAR *n2)
{
   return (g_flags & AF_CASE_INSENSITIVE_LOGINS) ? (_tcsicmp(n1, n2) == 0) : (_tcscmp(n1, n2) == 0);
}

/**
 * Add user database object
 */
inline void AddDatabaseObject(UserDatabaseObject *object)
{
   s_userDatabase.set(object->getId(), object);
   if (object->isGroup())
      s_groups.set(object->getName(), (Group *)object);
   else
      s_users.set(object->getName(), (User *)object);
   if (object->isLDAPUser())
   {
      s_ldapNames.set(object->getDn(), object);
      if(object->getLdapId() != NULL)
      {
         if (object->isGroup())
            s_ldapGroupId.set(object->getLdapId(), (Group *)object);
         else
            s_ldapUserId.set(object->getLdapId(), (User *)object);
      }
   }
}

/**
 * Remove user database object
 */
inline void RemoveDatabaseObject(UserDatabaseObject *object)
{
   if (object->isGroup())
      s_groups.remove(object->getName());
   else
      s_users.remove(object->getName());
   if (object->isLDAPUser())
   {
      s_ldapNames.remove(object->getDn());
      if(object->getLdapId() != NULL)
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
static UINT64 GetEffectiveSystemRights(User *user)
{
   UINT64 systemRights = user->getSystemRights();
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   IntegerArray<UINT32> *searchPath = new IntegerArray<UINT32>(16, 16);
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();

      // The previous search path is checked to avoid performing a deep membership search again
      if (object->isGroup() && searchPath->contains(((Group *)object)->getId()))
      {
         systemRights |= ((Group *)object)->getSystemRights();
         continue;
      }
      else
      {
         searchPath->clear();
      }

      if (object->isGroup() && (((Group *)object)->isMember(user->getId(), searchPath)))
      {
         systemRights |= ((Group *)object)->getSystemRights();
      }
      else
      {
         searchPath->clear();
      }
   }
   delete searchPath;
   delete it;
   return systemRights;
}

/**
 * Upgrade user accounts status in background
 */
static THREAD_RESULT THREAD_CALL AccountStatusUpdater(void *arg)
{
   ThreadSetName("AccountUpdate");
	DbgPrintf(2, _T("User account status update thread started"));
	while(!SleepAndCheckForShutdown(60))
	{
		DbgPrintf(8, _T("AccountStatusUpdater: wakeup"));

		time_t blockInactiveAccounts = (time_t)ConfigReadInt(_T("BlockInactiveUserAccounts"), 0) * 86400;

		RWLockWriteLock(s_userDatabaseLock, INFINITE);
		time_t now = time(NULL);
		Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
		while(it->hasNext())
		{
		   UserDatabaseObject *object = it->next();
			if (object->isDeleted() || object->isGroup())
				continue;

			User *user = (User *)object;

			if (user->isDisabled() && (user->getReEnableTime() > 0) && (user->getReEnableTime() <= now))
			{
				// Re-enable temporary disabled user
				user->enable();
            WriteAuditLog(AUDIT_SECURITY, true, user->getId(), NULL, AUDIT_SYSTEM_SID, 0, _T("Temporary disabled user account \"%s\" re-enabled by system"), user->getName());
				DbgPrintf(3, _T("Temporary disabled user account \"%s\" re-enabled"), user->getName());
			}

			if (!user->isDisabled() && (blockInactiveAccounts > 0) && (user->getLastLoginTime() > 0) && (user->getLastLoginTime() + blockInactiveAccounts < now))
			{
				user->disable();
            WriteAuditLog(AUDIT_SECURITY, TRUE, user->getId(), NULL, AUDIT_SYSTEM_SID, 0, _T("User account \"%s\" disabled by system due to inactivity"), user->getName());
				DbgPrintf(3, _T("User account \"%s\" disabled due to inactivity"), user->getName());
			}
		}
		delete it;
		RWLockUnlock(s_userDatabaseLock);
	}

	DbgPrintf(2, _T("User account status update thread stopped"));
	return THREAD_OK;
}

/**
 * Initialize user handling subsystem
 */
void InitUsers()
{
   s_users.setIgnoreCase((g_flags & AF_CASE_INSENSITIVE_LOGINS) != 0);
   s_groups.setIgnoreCase((g_flags & AF_CASE_INSENSITIVE_LOGINS) != 0);
	s_statusUpdateThread = ThreadCreateEx(AccountStatusUpdater, 0, NULL);
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
BOOL LoadUsers()
{
   int i;
   DB_RESULT hResult;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load users
   hResult = DBSelect(hdb,
	                   _T("SELECT id,name,system_access,flags,description,guid,ldap_dn,")
							 _T("ldap_unique_id,password,full_name,grace_logins,auth_method,")
							 _T("cert_mapping_method,cert_mapping_data,auth_failures,")
							 _T("last_passwd_change,min_passwd_length,disabled_until,")
							 _T("last_login,xmpp_id FROM users"));
   if (hResult == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   int count = DBGetNumRows(hResult);
   for(i = 0; i < count; i++)
   {
		User *user = new User(hdb, hResult, i);
		AddDatabaseObject(user);
   }

   DBFreeResult(hResult);

   // Create system account if it doesn't exist
   if (!s_userDatabase.contains(0))
   {
		User *user = new User();
		AddDatabaseObject(user);
      nxlog_write(MSG_SUPERUSER_CREATED, EVENTLOG_WARNING_TYPE, NULL);
   }

   // Load groups
   hResult = DBSelect(hdb, _T("SELECT id,name,system_access,flags,description,guid,ldap_dn,ldap_unique_id FROM user_groups"));
   if (hResult == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return FALSE;
   }

   count = DBGetNumRows(hResult);
   for(i = 0; i < count; i++)
   {
		Group *group = new Group(hdb, hResult, i);
		AddDatabaseObject(group);
   }

   DBFreeResult(hResult);

   // Create everyone group if it doesn't exist
   if (!s_userDatabase.contains(GROUP_EVERYONE))
   {
		Group *group = new Group();
		group->saveToDatabase(hdb);
		AddDatabaseObject(group);
      nxlog_write(MSG_EVERYONE_GROUP_CREATED, EVENTLOG_WARNING_TYPE, NULL);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return TRUE;
}

/**
 * Save user list to database
 */
void SaveUsers(DB_HANDLE hdb, UINT32 watchdogId)
{
   // Save users
   RWLockWriteLock(s_userDatabaseLock, INFINITE);
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      WatchdogNotify(watchdogId);
      UserDatabaseObject *object = it->next();
      if (object->isDeleted())
      {
			object->deleteFromDatabase(hdb);
			RemoveDatabaseObject(object);
			it->remove();
      }
		else if (object->isModified())
      {
			object->saveToDatabase(hdb);
      }
   }
   delete it;
   RWLockUnlock(s_userDatabaseLock);
}

/**
 * Authenticate user
 * Checks if provided login name and password are correct, and returns RCC_SUCCESS
 * on success and appropriate RCC otherwise. On success authentication, user's ID is stored
 * int pdwId. If password authentication is used, dwSigLen should be set to zero.
 * For non-UNICODE build, password must be UTF-8 encoded. If user already authenticated by
 * SSO server, ssoAuth must be set to true. Password expiration, change flag and grace
 * count ignored for SSO logins.
 */
UINT32 AuthenticateUser(const TCHAR *login, const TCHAR *password, size_t sigLen, void *pCert,
                        BYTE *pChallenge, UINT32 *pdwId, UINT64 *pdwSystemRights,
							   bool *pbChangePasswd, bool *pbIntruderLockout, bool *closeOtherSessions,
							   bool ssoAuth, UINT32 *graceLogins)
{
   UINT32 dwResult = RCC_ACCESS_DENIED;
   BOOL bPasswordValid = FALSE;

   *closeOtherSessions = false;
   RWLockWriteLock(s_userDatabaseLock, INFINITE);
   User *user = s_users.get(login);
   if ((user != NULL) && !user->isDeleted())
   {
      *pdwId = user->getId(); // always set user ID for caller so audit log will contain correct user ID on failures as well

      if (user->isLDAPUser())
      {
         if (user->isDisabled())
         {
            dwResult = RCC_ACCOUNT_DISABLED;
            goto result;
         }
         LDAPConnection conn;
         dwResult = conn.ldapUserLogin(user->getDn(), password);
         if (dwResult == RCC_SUCCESS)
            bPasswordValid = TRUE;
         goto result;
      }

      // Determine authentication method to use
      if (!ssoAuth)
      {
         int method = user->getAuthMethod();
         if ((method == AUTH_CERT_OR_PASSWD) || (method == AUTH_CERT_OR_RADIUS))
         {
            if (sigLen > 0)
            {
               // certificate auth
               method = AUTH_CERTIFICATE;
            }
            else
            {
               method = (method == AUTH_CERT_OR_PASSWD) ? AUTH_NETXMS_PASSWORD : AUTH_RADIUS;
            }
         }

         switch(method)
         {
            case AUTH_NETXMS_PASSWORD:
               if (sigLen == 0)
               {
                  bPasswordValid = user->validatePassword(password);
               }
               else
               {
                  // We got certificate instead of password
                  bPasswordValid = FALSE;
               }
               break;
            case AUTH_RADIUS:
               if (sigLen == 0)
               {
                  bPasswordValid = RadiusAuth(login, password);
               }
               else
               {
                  // We got certificate instead of password
                  bPasswordValid = FALSE;
               }
               break;
            case AUTH_CERTIFICATE:
               if ((sigLen != 0) && (pCert != NULL))
               {
#ifdef _WITH_ENCRYPTION
                  bPasswordValid = ValidateUserCertificate(static_cast<X509*>(pCert), login, pChallenge,
                                                           reinterpret_cast<const BYTE*>(password), sigLen,
                                                           user->getCertMappingMethod(),
                                                           user->getCertMappingData());
#else
                  bPasswordValid = FALSE;
#endif
               }
               else
               {
                  // We got password instead of certificate
                  bPasswordValid = FALSE;
               }
               break;
            default:
               nxlog_write(MSG_UNKNOWN_AUTH_METHOD, NXLOG_WARNING, "ds", user->getAuthMethod(), login);
               bPasswordValid = FALSE;
               break;
         }
      }
      else
      {
         DbgPrintf(4, _T("User \"%s\" already authenticated by SSO server"), user->getName());
         bPasswordValid = TRUE;
      }

result:
      if (bPasswordValid)
      {
         if (!user->isDisabled())
         {
            user->resetAuthFailures();
            if (!ssoAuth)
            {
               if (user->getFlags() & UF_CHANGE_PASSWORD)
               {
                  DbgPrintf(4, _T("Password for user \"%s\" need to be changed"), user->getName());
                  if (user->getId() != 0)	// Do not check grace logins for built-in system user
                  {
                     if (user->getGraceLogins() <= 0)
                     {
                        DbgPrintf(4, _T("User \"%s\" has no grace logins left"), user->getName());
                        dwResult = RCC_NO_GRACE_LOGINS;
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
                  int passwordExpirationTime = ConfigReadInt(_T("PasswordExpiration"), 0);
                  if ((user->getAuthMethod() == AUTH_NETXMS_PASSWORD) &&
                      (passwordExpirationTime > 0) &&
                      ((user->getFlags() & UF_PASSWORD_NEVER_EXPIRES) == 0) &&
                      (time(NULL) > user->getPasswordChangeTime() + passwordExpirationTime * 86400))
                  {
                     DbgPrintf(4, _T("Password for user \"%s\" has expired"), user->getName());
                     if (user->getId() != 0)	// Do not check grace logins for built-in system user
                     {
                        if (user->getGraceLogins() <= 0)
                        {
                           DbgPrintf(4, _T("User \"%s\" has no grace logins left"), user->getName());
                           dwResult = RCC_NO_GRACE_LOGINS;
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

            if (dwResult != RCC_NO_GRACE_LOGINS)
            {
               *pdwSystemRights = GetEffectiveSystemRights(user);
               *closeOtherSessions = (user->getFlags() & UF_CLOSE_OTHER_SESSIONS) != 0;
               *graceLogins = user->getGraceLogins();
               user->updateLastLogin();
               dwResult = RCC_SUCCESS;
            }
         }
         else
         {
            dwResult = RCC_ACCOUNT_DISABLED;
         }
         *pbIntruderLockout = false;
      }
      else
      {
         user->increaseAuthFailures();
         *pbIntruderLockout = user->isIntruderLockoutActive();
      }
   }
   RWLockUnlock(s_userDatabaseLock);
   return dwResult;
}

/**
 * Check if user is a member of specific child group
 */
bool CheckUserMembershipInternal(UINT32 userId, UINT32 groupId, IntegerArray<UINT32> *searchPath)
{
   Group *group = (Group *)s_userDatabase.get(groupId);
   if (group != NULL)
   {
      return group->isMember(userId, searchPath);
   }
   return false;
}

/**
 * Check if user is a member of specific group
 */
bool NXCORE_EXPORTABLE CheckUserMembership(UINT32 userId, UINT32 groupId)
{
	if (!(groupId & GROUP_FLAG))
		return false;

   if (groupId == GROUP_EVERYONE)
		return true;

   bool result = false;
   IntegerArray<UINT32> searchPath(16, 16);

   RWLockReadLock(s_userDatabaseLock, INFINITE);

   result = CheckUserMembershipInternal(userId, groupId, &searchPath);

   RWLockUnlock(s_userDatabaseLock);

   return result;
}

/**
 * Fill message with group membership information for given user.
 * Access to user database must be locked.
 */
void FillGroupMembershipInfo(NXCPMessage *msg, UINT32 userId)
{
   IntegerArray<UINT32> list;
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();
		if (object->isGroup() && (object->getId() != GROUP_EVERYONE) && ((Group *)object)->isMember(userId))
		{
         list.add(object->getId());
		}
   }
   delete it;
   msg->setField(VID_NUM_GROUPS, (INT32)list.size());
   if (list.size() > 0)
      msg->setFieldFromInt32Array(VID_GROUPS, &list);
}

/**
 * Update group membership for user
 */
void UpdateGroupMembership(UINT32 userId, int numGroups, UINT32 *groups)
{
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();
		if (object->isGroup() && (object->getId() != GROUP_EVERYONE))
		{
         bool found = false;
         for(int j = 0; j < numGroups; j++)
         {
            if (object->getId() == groups[j])
            {
               found = true;
               break;
            }
         }
         if (found)
         {
            ((Group *)object)->addUser(userId);
         }
         else
         {
            ((Group *)object)->deleteUser(userId);
         }
		}
   }
   delete it;
}

/**
 * Resolve user's ID to login name
 */
bool NXCORE_EXPORTABLE ResolveUserId(UINT32 id, TCHAR *buffer, int bufSize)
{
   RWLockReadLock(s_userDatabaseLock, INFINITE);
   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
      nx_strncpy(buffer, object->getName(), bufSize);
   RWLockUnlock(s_userDatabaseLock);
	return object != NULL;
}

/**
 * Check if provided user name is not used or belongs to given user.
 * Access to user DB must be locked when this function is called
 */
inline bool UserNameIsUnique(const TCHAR *name, User *user)
{
   User *u = s_users.get(name);
   return (u == NULL) || ((user != NULL) && (user->getId() == u->getId()));
}

/**
 * Check if provided group name is not used or belongs to given group.
 * Access to user DB must be locked when this function is called
 */
inline bool GroupNameIsUnique(const TCHAR *name, Group *group)
{
   Group *g = s_groups.get(name);
   return (g == NULL) || ((group != NULL) && (group->getId() == g->getId()));
}

/**
 * Generates unique name for LDAP user
 */
static TCHAR *GenerateUniqueName(const TCHAR *oldName, UINT32 id)
{
   TCHAR *name = (TCHAR *)malloc(sizeof(TCHAR) * 256);
   _sntprintf(name, 256, _T("%s_LDAP%d"), oldName, id);
   return name;
}

/**
 * Update/Add LDAP user
 */
void UpdateLDAPUser(const TCHAR *dn, Entry *obj)
{
   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   bool userModified = false;
   bool conflict = false;
   TCHAR description[1024];
   TCHAR guid[64];

   // Check existing user with same DN
   UserDatabaseObject *object = NULL;
   if(obj->m_id != NULL)
      object = s_ldapUserId.get(obj->m_id);
   else
      object = s_ldapNames.get(dn);

   if ((object != NULL) && object->isGroup())
   {
      _sntprintf(description, MAX_USER_DESCR, _T("Got user with DN=%s but found existing group %s with same DN"), dn, object->getName());
      object->getGuidAsText(guid);
      PostEvent(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode, "issss", object->getId(), guid, object->getDn(), object->getName(), description);
      DbgPrintf(4,  _T("UpdateLDAPUser(): %s"), description);
      conflict = true;
   }

   if ((object != NULL) && !conflict)
   {
      User *user = (User *)object;
      if (!user->isDeleted())
      {
         user->removeSyncException();
         if (!UserNameIsUnique(obj->m_loginName, user))
         {
            TCHAR *userName = GenerateUniqueName(obj->m_loginName, user->getId());
            if(_tcscmp(user->getName(), userName))
            {
               user->setName(userName);
               _sntprintf(description, 1024, _T("User with name \"%s\" already exists. Unique user name have been generated: \"%s\""), obj->m_loginName, userName);
               object->getGuidAsText(guid);
               PostEvent(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode, "issss", object->getId(), guid, object->getDn(), object->getName(), description);
               DbgPrintf(4,  _T("UpdateLDAPUser(): %s"), description);
            }
            user->setFullName(obj->m_fullName);
            user->setDescription(obj->m_description);
            if(_tcscmp(user->getDn(), dn))
            {
               s_ldapNames.remove(user->getDn());
               user->setDn(dn);
               s_ldapNames.set(dn, user);
            }
            free(userName);
         }
         else
         {
            user->setName(obj->m_loginName);
            user->setFullName(obj->m_fullName);
            user->setDescription(obj->m_description);
            if(_tcscmp(user->getDn(), dn))
            {
               s_ldapNames.remove(user->getDn());
               user->setDn(dn);
               s_ldapNames.set(dn, user);
            }
            DbgPrintf(4, _T("UpdateLDAPUser(): User updated: ID: %s DN: %s, login name: %s, full name: %s, description: %s"), CHECK_NULL(obj->m_id), dn, obj->m_loginName, CHECK_NULL(obj->m_fullName), CHECK_NULL(obj->m_description));
         }
         if (user->isModified())
         {
            SendUserDBUpdate(USER_DB_MODIFY, user->getId(), user);
         }
      }
      userModified = true;
   }

   if (!userModified && !conflict)
   {
      if (UserNameIsUnique(obj->m_loginName, NULL))
      {
         User *user = new User(CreateUniqueId(IDG_USER), obj->m_loginName);
         user->setFullName(obj->m_fullName);
         user->setDescription(obj->m_description);
         user->setFlags(UF_MODIFIED | UF_LDAP_USER);
         user->setDn(dn);
         if(obj->m_id != NULL)
            user->setLdapId(obj->m_id);
         AddDatabaseObject(user);
         SendUserDBUpdate(USER_DB_CREATE, user->getId(), user);
         DbgPrintf(4, _T("UpdateLDAPUser(): User added: ID: %s DN: %s, login name: %s, full name: %s, description: %s"), CHECK_NULL(obj->m_id), dn, obj->m_loginName, CHECK_NULL(obj->m_fullName), CHECK_NULL(obj->m_description));
      }
      else
      {
         UINT32 userId = CreateUniqueId(IDG_USER);
         TCHAR *userName = GenerateUniqueName(obj->m_loginName, userId);
         _sntprintf(description, MAX_USER_DESCR, _T("User with name \"%s\" already exists. Unique user name have been generated: \"%s\""), obj->m_loginName, userName);
         DbgPrintf(4,  _T("UpdateLDAPUser(): %s"), description);
         User *user = new User(userId, userName);
         user->setFullName(obj->m_fullName);
         user->setDescription(obj->m_description);
         user->setFlags(UF_MODIFIED | UF_LDAP_USER);
         user->setDn(dn);
         if(obj->m_id != NULL)
            user->setLdapId(obj->m_id);
         AddDatabaseObject(user);
         SendUserDBUpdate(USER_DB_CREATE, user->getId(), user);
         user->getGuidAsText(guid);
         PostEvent(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode, "issss", user->getId(), guid, user->getDn(), user->getName(), description);
         DbgPrintf(4, _T("UpdateLDAPUser(): User added: ID: %s DN: %s, login name: %s, full name: %s, description: %s"), CHECK_NULL(obj->m_id), dn, userName, CHECK_NULL(obj->m_fullName), CHECK_NULL(obj->m_description));
         free(userName);
      }
   }
   RWLockUnlock(s_userDatabaseLock);
}

/**
 * Goes through all existing LDAP entries and check that in newly gotten list they also exist.
 * If LDAP entries does not exists in new list - it will be disabled or removed depending on action parameter.
 */
void RemoveDeletedLDAPEntries(StringObjectMap<Entry> *entryListDn, StringObjectMap<Entry> *entryListId, UINT32 m_action, bool isUser)
{
   RWLockWriteLock(s_userDatabaseLock, INFINITE);
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();
      if (!object->isLDAPUser() || object->isDeleted())
         continue;

      if (isUser ? ((object->getId() & GROUP_FLAG) == 0) : ((object->getId() & GROUP_FLAG) != 0))
		{
         if ((object->getLdapId() == NULL || !entryListId->contains(object->getLdapId())) && !entryListDn->contains(object->getDn()))
         {
            if (m_action == USER_DELETE)
            {
               DbgPrintf(4, _T("RemoveDeletedLDAPEntry(): LDAP %s object %s was removed from user database"), isUser ? _T("user") : _T("group"), object->getDn());
               DeleteUserDatabaseObject(object->getId(), true);
            }
            else if (m_action == USER_DISABLE)
            {
               DbgPrintf(4, _T("RemoveDeletedLDAPEntry(): LDAP %s object %s was disabled"), isUser ? _T("user") : _T("group"), object->getDn());
               object->disable();
               object->setDescription(_T("LDAP entry was deleted."));
            }
         }
		}
   }
   RWLockUnlock(s_userDatabaseLock);
}

/**
 * Synchronize new user list with old user list of given group. Note: LDAP user will not be changed.
 */
static void SyncGroupMembers(Group *group, Entry *obj)
{
   DbgPrintf(4, _T("SyncGroupMembers(): Sync for LDAP group: %s"), group->getDn());

   StringSet *newMembers = obj->m_memberList;
   UINT32 *oldMembers = NULL;
   int count = group->getMembers(&oldMembers);

   /**
    * Go through existing group member list checking each LDAP user by DN
    * with new gotten group member list and removing LDAP users not existing in last list.
    */
   for(int i = 0; i < count; i++)
   {
      UserDatabaseObject *user = s_userDatabase.get(oldMembers[i]);
      if ((user == NULL) || !user->isLDAPUser())
         continue;

      if (!newMembers->contains(user->getDn()))
      {
         DbgPrintf(4, _T("SyncGroupMembers: Remove from %s group deleted user: %s"), group->getDn(), user->getDn());
         group->deleteUser(user->getId());
         count = group->getMembers(&oldMembers);
         i--;
      }
   }

   /**
    * Go through new gotten group member list checking each LDAP user by DN
    * with existing group member list and adding users not existing in last list.
    */
   Iterator<const TCHAR> *it = newMembers->iterator();
   while(it->hasNext())
   {
      const TCHAR *dn = it->next();
      UserDatabaseObject *user = s_ldapNames.get(dn);
      if (user == NULL)
         continue;

      if (!group->isMember(user->getId()))
      {
         DbgPrintf(4, _T("SyncGroupMembers: LDAP user %s added to LDAP group %s"), user->getDn(), group->getDn());
         group->addUser(user->getId());
      }
   }
   delete it;
}

/**
 * Update/Add LDAP group
 */
void UpdateLDAPGroup(const TCHAR *dn, Entry *obj) //no full name, add users inside group, and delete removed from the group
{
   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   bool userModified = false;
   bool conflict = false;
   TCHAR description[1024];
   TCHAR guid[64];

   // Check existing user with same DN
   UserDatabaseObject *object = NULL;

   if(obj->m_id != NULL)
      object = s_ldapGroupId.get(obj->m_id);
   else
      object = s_ldapNames.get(dn);
   if ((object != NULL) && !object->isGroup())
   {
      _sntprintf(description, MAX_USER_DESCR, _T("Got group with DN=%s but found existing group %s with same DN"), dn, object->getName());
      object->getGuidAsText(guid);
      PostEvent(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode, "issss", object->getId(), guid, object->getDn(), object->getName(), description);
      DbgPrintf(4,  _T("UpdateLDAPGroup(): %s"), description);
      conflict = true;
   }

   if ((object != NULL) && !conflict)
   {
      Group *group = (Group *)object;
      if (!group->isDeleted())
      {
         group->removeSyncException();
         if (!GroupNameIsUnique(obj->m_loginName, group))
         {
            TCHAR *groupName = GenerateUniqueName(obj->m_loginName, group->getId());
            if(_tcscmp(group->getName(), groupName))
            {
               group->setName(groupName);
               _sntprintf(description, MAX_USER_DESCR, _T("Group with name \"%s\" already exists. Unique group name have been generated: \"%s\""), obj->m_loginName, groupName);
               object->getGuidAsText(guid);
               PostEvent(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode, "issss", object->getId(), guid, object->getDn(), object->getName(), description);
               DbgPrintf(4,  _T("UpdateLDAPGroup(): %s"),description);
            }
            group->setDescription(obj->m_description);
            if(_tcscmp(group->getDn(), dn))
            {
               s_ldapNames.remove(group->getDn());
               group->setDn(dn);
               s_ldapNames.set(dn, group);
            }
            free(groupName);
         }
         else
         {
            group->setName(obj->m_loginName);
            group->setDescription(obj->m_description);
            if(_tcscmp(group->getDn(), dn))
            {
               s_ldapNames.remove(group->getDn());
               group->setDn(dn);
               s_ldapNames.set(dn, group);
            }
            DbgPrintf(4, _T("UpdateLDAPGroup(): Group updated: ID: %s DN: %s, login name: %s, description: %s"), CHECK_NULL(obj->m_id), dn, obj->m_loginName, CHECK_NULL(obj->m_description));
         }
         if (group->isModified())
         {
            SendUserDBUpdate(USER_DB_MODIFY, group->getId(), group);
         }
         SyncGroupMembers(group , obj);
      }
      userModified = true;
   }

   if (!userModified)
   {
      if (GroupNameIsUnique(obj->m_loginName, NULL))
      {
         Group *group = new Group(CreateUniqueId(IDG_USER_GROUP), obj->m_loginName);
         group->setDescription(obj->m_description);
         group->setFlags(UF_MODIFIED | UF_LDAP_USER);
         group->setDn(dn);
         if(obj->m_id != NULL)
            group->setLdapId(obj->m_id);
         SendUserDBUpdate(USER_DB_CREATE, group->getId(), group);
         AddDatabaseObject(group);
         SyncGroupMembers(group , obj);
         DbgPrintf(4, _T("UpdateLDAPGroup(): Group added: ID: %s DN: %s, login name: %s, description: %s"), CHECK_NULL(obj->m_id), dn, obj->m_loginName, CHECK_NULL(obj->m_description));
      }
      else
      {
         UINT32 id = CreateUniqueId(IDG_USER_GROUP);
         TCHAR *groupName = GenerateUniqueName(obj->m_loginName, id);
         _sntprintf(description, MAX_USER_DESCR, _T("Group with name \"%s\" already exists. Unique group name have been generated: \"%s\""), obj->m_loginName, groupName);
         DbgPrintf(4,  _T("UpdateLDAPGroup(): %s"),description);
         Group *group = new Group(id, groupName);
         group->setDescription(obj->m_description);
         group->setFlags(UF_MODIFIED | UF_LDAP_USER);
         group->setDn(dn);
         if(obj->m_id != NULL)
            group->setLdapId(obj->m_id);
         SendUserDBUpdate(USER_DB_CREATE, group->getId(), group);
         AddDatabaseObject(group);
         SyncGroupMembers(group , obj);
         group->getGuidAsText(guid);
         PostEvent(EVENT_LDAP_SYNC_ERROR ,g_dwMgmtNode, "issss", group->getId(), guid, group->getDn(), group->getName(), description);
         DbgPrintf(4, _T("UpdateLDAPGroup(): Group added: ID: %s DN: %s, login name: %s, description: %s"), CHECK_NULL(obj->m_id), dn, obj->m_loginName, CHECK_NULL(obj->m_description));
         free(groupName);
      }
   }
   RWLockUnlock(s_userDatabaseLock);
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

   RWLockReadLock(s_userDatabaseLock, INFINITE);
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();
		if (!object->isGroup())
      {
         TCHAR szGUID[64];
         UINT64 systemRights = GetEffectiveSystemRights((User *)object);
         ConsolePrintf(pCtx, _T("%-20s %-36s 0x") UINT64X_FMT(_T("016")) _T("\n"), object->getName(), object->getGuidAsText(szGUID), systemRights);
		}
	}
   delete it;
   RWLockUnlock(s_userDatabaseLock);
   ConsolePrintf(pCtx, _T("\n"));
}

/**
 * Delete user or group
 *
 * @param id user database object ID
 * @return RCC ready to be sent to client
 */
UINT32 NXCORE_EXPORTABLE DeleteUserDatabaseObject(UINT32 id, bool alreadyLocked)
{
   DeleteUserFromAllObjects(id);

   if (!alreadyLocked)
      RWLockWriteLock(s_userDatabaseLock, INFINITE);

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
   {
      object->setDeleted();
      if (!(id & GROUP_FLAG))
      {
         Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
         while(it->hasNext())
         {
            UserDatabaseObject *group = it->next();
            if (group->getId() & GROUP_FLAG)
            {
               ((Group *)group)->deleteUser(id);
            }
         }
         delete it;
      }
	}

   if (!alreadyLocked)
      RWLockUnlock(s_userDatabaseLock);

   SendUserDBUpdate(USER_DB_DELETE, id, NULL);
   return RCC_SUCCESS;
}

/**
 * Create new user or group
 */
UINT32 NXCORE_EXPORTABLE CreateNewUser(const TCHAR *name, bool isGroup, UINT32 *id)
{
   UINT32 dwResult = RCC_SUCCESS;

   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   // Check for duplicate name
   UserDatabaseObject *object = isGroup ? (UserDatabaseObject *)s_groups.get(name) : (UserDatabaseObject *)s_users.get(name);
   if (object != NULL)
   {
      dwResult = RCC_OBJECT_ALREADY_EXISTS;
	}

	if (dwResult == RCC_SUCCESS)
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

   RWLockUnlock(s_userDatabaseLock);
	return dwResult;
}

/**
 * Modify user database object
 */
UINT32 NXCORE_EXPORTABLE ModifyUserDatabaseObject(NXCPMessage *msg, json_t **oldData, json_t **newData)
{
   UINT32 dwResult = RCC_INVALID_USER_ID;

	UINT32 id = msg->getFieldAsUInt32(VID_USER_ID);

   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
   {
      TCHAR name[MAX_USER_NAME], prevName[MAX_USER_NAME];

      UINT32 fields = msg->getFieldAsUInt32(VID_FIELDS);
      if (fields & USER_MODIFY_LOGIN_NAME)
      {
         msg->getFieldAsString(VID_USER_NAME, name, MAX_USER_NAME);
         if (IsValidObjectName(name))
         {
            nx_strncpy(prevName, object->getName(), MAX_USER_NAME);
         }
         else
         {
            dwResult = RCC_INVALID_OBJECT_NAME;
         }
      }

      if (dwResult != RCC_INVALID_OBJECT_NAME)
      {
         *oldData = object->toJson();
         object->modifyFromMessage(msg);
         *newData = object->toJson();
         SendUserDBUpdate(USER_DB_MODIFY, id, object);
         dwResult = RCC_SUCCESS;
      }

      if ((dwResult == RCC_SUCCESS) && (fields & USER_MODIFY_LOGIN_NAME))
      {
         // update login names hash map if login name was modified
         if (_tcscmp(prevName, object->getName()))
         {
            if (object->isGroup())
            {
               nxlog_debug(4, _T("Group rename: %s -> %s"), prevName, object->getName());
               s_groups.remove(prevName);
               s_groups.set(object->getName(), (Group *)object);
            }
            else
            {
               nxlog_debug(4, _T("User rename: %s -> %s"), prevName, object->getName());
               s_users.remove(prevName);
               s_users.set(object->getName(), (User *)object);
            }
         }
      }
   }

   RWLockUnlock(s_userDatabaseLock);
   return dwResult;
}

/**
 * Modify user database object
 */
UINT32 NXCORE_EXPORTABLE DetachLdapUser(UINT32 id)
{
   UINT32 dwResult = RCC_INVALID_USER_ID;

   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
   {
      s_ldapNames.remove(object->getDn());
      object->detachLdapUser();
      SendUserDBUpdate(USER_DB_MODIFY, id, object);
      dwResult = RCC_SUCCESS;
   }

   RWLockUnlock(s_userDatabaseLock);
   return dwResult;
}

/**
 * Send user DB update for given user ID.
 * Access to user database must be already locked.
 */
void SendUserDBUpdate(int code, UINT32 id)
{
   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
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
		nx_strncpy(subseq, &sequence[i], len + 1);
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
	int flags = ConfigReadInt(_T("PasswordComplexity"), 0);

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
 * For non-UNICODE build, passwords must be UTF-8 encoded
 */
UINT32 NXCORE_EXPORTABLE SetUserPassword(UINT32 id, const TCHAR *newPassword, const TCHAR *oldPassword, bool changeOwnPassword)
{
	if (id & GROUP_FLAG)
		return RCC_INVALID_USER_ID;

   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   // Find user
   User *user = (User *)s_userDatabase.get(id);
   if (user == NULL)
   {
      RWLockUnlock(s_userDatabaseLock);
      return RCC_INVALID_USER_ID;
   }

   UINT32 dwResult = RCC_INVALID_USER_ID;
   if (changeOwnPassword)
   {
      if (!user->canChangePassword() || !user->validatePassword(oldPassword))
      {
         dwResult = RCC_ACCESS_DENIED;
         goto finish;
      }

      // Check password length
      int minLength = (user->getMinMasswordLength() == -1) ? ConfigReadInt(_T("MinPasswordLength"), 0) : user->getMinMasswordLength();
      if ((int)_tcslen(newPassword) < minLength)
      {
         dwResult = RCC_WEAK_PASSWORD;
         goto finish;
      }

      // Check password complexity
      if (!CheckPasswordComplexity(newPassword))
      {
         dwResult = RCC_WEAK_PASSWORD;
         goto finish;
      }

      // Update password history
      int passwordHistoryLength = ConfigReadInt(_T("PasswordHistoryLength"), 0);
      if (passwordHistoryLength > 0)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         TCHAR query[8192], *ph = NULL;

         _sntprintf(query, 8192, _T("SELECT password_history FROM users WHERE id=%d"), id);
         DB_RESULT hResult = DBSelect(hdb, query);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               ph = DBGetField(hResult, 0, 0, NULL, 0);
            }
            DBFreeResult(hResult);
         }

         if (ph != NULL)
         {
            BYTE newPasswdHash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
            char *mb = UTF8StringFromWideString(newPassword);
            CalculateSHA1Hash((BYTE *)mb, strlen(mb), newPasswdHash);
            free(mb);
#else
            CalculateSHA1Hash((BYTE *)newPassword, strlen(newPassword), newPasswdHash);
#endif

            int phLen = (int)_tcslen(ph) / (SHA1_DIGEST_SIZE * 2);
            if (phLen > passwordHistoryLength)
               phLen = passwordHistoryLength;

            for(int i = 0; i < phLen; i++)
            {
               BYTE hash[SHA1_DIGEST_SIZE];
               StrToBin(&ph[i * SHA1_DIGEST_SIZE * 2], hash, SHA1_DIGEST_SIZE);
               if (!memcmp(hash, newPasswdHash, SHA1_DIGEST_SIZE))
               {
                  dwResult = RCC_REUSED_PASSWORD;
                  goto finish;
               }
            }

            if (dwResult != RCC_REUSED_PASSWORD)
            {
               if (phLen == passwordHistoryLength)
               {
                  memmove(ph, &ph[SHA1_DIGEST_SIZE * 2], (phLen - 1) * SHA1_DIGEST_SIZE * 2 * sizeof(TCHAR));
               }
               else
               {
                  ph = (TCHAR *)realloc(ph, (phLen + 1) * SHA1_DIGEST_SIZE * 2 * sizeof(TCHAR) + sizeof(TCHAR));
                  phLen++;
               }
               BinToStr(newPasswdHash, SHA1_DIGEST_SIZE, &ph[(phLen - 1) * SHA1_DIGEST_SIZE * 2]);

               _sntprintf(query, 8192, _T("UPDATE users SET password_history='%s' WHERE id=%d"), ph, id);
               DBQuery(hdb, query);
            }

            free(ph);
            if (dwResult == RCC_REUSED_PASSWORD)
               goto finish;
         }
         else
         {
            dwResult = RCC_DB_FAILURE;
            goto finish;
         }
         DBConnectionPoolReleaseConnection(hdb);
      }

      user->updatePasswordChangeTime();
   }
   user->setPassword(newPassword, changeOwnPassword);
   dwResult = RCC_SUCCESS;

finish:
   RWLockUnlock(s_userDatabaseLock);
   return dwResult;
}

/**
 * Validate user's password
 */
UINT32 NXCORE_EXPORTABLE ValidateUserPassword(UINT32 userId, const TCHAR *login, const TCHAR *password, bool *isValid)
{
	if (userId & GROUP_FLAG)
		return RCC_INVALID_USER_ID;

   UINT32 rcc = RCC_INVALID_USER_ID;
   RWLockReadLock(s_userDatabaseLock, INFINITE);

   // Find user
   User *user = (User *)s_userDatabase.get(userId);
   if (user != NULL)
   {
      rcc = RCC_SUCCESS;
      if (user->isLDAPUser())
      {
         if (user->isDisabled())
         {
            rcc = RCC_ACCOUNT_DISABLED;
         }
         else
         {
            LDAPConnection conn;
            rcc = conn.ldapUserLogin(user->getDn(), password);
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
      }
      else
      {
         switch(user->getAuthMethod())
         {
            case AUTH_NETXMS_PASSWORD:
            case AUTH_CERT_OR_PASSWD:
   			   *isValid = user->validatePassword(password);
               break;
            case AUTH_RADIUS:
            case AUTH_CERT_OR_RADIUS:
               *isValid = RadiusAuth(login, password);
               break;
            default:
               rcc = RCC_UNSUPPORTED_AUTH_METHOD;
               break;
         }
      }
   }

   RWLockUnlock(s_userDatabaseLock);
   return rcc;
}

/**
 * Open user database
 */
Iterator<UserDatabaseObject> NXCORE_EXPORTABLE *OpenUserDatabase()
{
   RWLockReadLock(s_userDatabaseLock, INFINITE);
	return s_userDatabase.iterator();
}

/**
 * Close user database
 */
void NXCORE_EXPORTABLE CloseUserDatabase(Iterator<UserDatabaseObject> *it)
{
   delete it;
   RWLockUnlock(s_userDatabaseLock);
}

/**
 * Get custom attribute's value
 */
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(UINT32 id, const TCHAR *name)
{
	const TCHAR *value = NULL;

   RWLockReadLock(s_userDatabaseLock, INFINITE);

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
      value = object->getAttribute(name);

   RWLockUnlock(s_userDatabaseLock);
	return value;
}

/**
 * Get custom attribute's value as unsigned integer
 */
UINT32 NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(UINT32 id, const TCHAR *name)
{
	const TCHAR *value = GetUserDbObjectAttr(id, name);
	return (value != NULL) ? _tcstoul(value, NULL, 0) : 0;
}

/**
 * Set custom attribute's value
 */
void NXCORE_EXPORTABLE SetUserDbObjectAttr(UINT32 id, const TCHAR *name, const TCHAR *value)
{
   RWLockWriteLock(s_userDatabaseLock, INFINITE);

   UserDatabaseObject *object = s_userDatabase.get(id);
   if (object != NULL)
   {
      object->setAttribute(name, value);
   }

   RWLockUnlock(s_userDatabaseLock);
}

/**
 * Authenticate user for XMPP subscription
 */
bool AuthenticateUserForXMPPSubscription(const char *xmppId)
{
   if (*xmppId == 0)
      return false;

   bool success = false;

#ifdef UNICODE
   WCHAR *_xmppId = WideStringFromUTF8String(xmppId);
#else
   char *_xmppId = strdup(xmppId);
#endif

   // Remove possible resource at the end
   TCHAR *sep = _tcschr(_xmppId, _T('/'));
   if (sep != NULL)
      *sep = 0;

   RWLockReadLock(s_userDatabaseLock, INFINITE);
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();
		if (!object->isGroup() &&
          !object->isDisabled() && !object->isDeleted() &&
			 !_tcsicmp(_xmppId, ((User *)object)->getXmppId()))
      {
         DbgPrintf(4, _T("User %s authenticated for XMPP subscription"), object->getName());

         TCHAR workstation[256];
         _tcscpy(workstation, _T("XMPP:"));
         nx_strncpy(&workstation[5], _xmppId, 251);
         WriteAuditLog(AUDIT_SECURITY, TRUE, object->getId(), workstation, AUDIT_SYSTEM_SID, 0, _T("User authenticated for XMPP subscription"));

         success = true;
         break;
      }
   }
   delete it;
   RWLockUnlock(s_userDatabaseLock);

   free(_xmppId);
   return success;
}

/**
 * Authenticate user for XMPP commands
 */
bool AuthenticateUserForXMPPCommands(const char *xmppId)
{
   if (*xmppId == 0)
      return false;

   bool success = false;

#ifdef UNICODE
   WCHAR *_xmppId = WideStringFromUTF8String(xmppId);
#else
   char *_xmppId = strdup(xmppId);
#endif

   // Remove possible resource at the end
   TCHAR *sep = _tcschr(_xmppId, _T('/'));
   if (sep != NULL)
      *sep = 0;

   RWLockReadLock(s_userDatabaseLock, INFINITE);
   Iterator<UserDatabaseObject> *it = s_userDatabase.iterator();
   while(it->hasNext())
   {
      UserDatabaseObject *object = it->next();
      if (!object->isGroup() &&
          !object->isDisabled() && !object->isDeleted() &&
          !_tcsicmp(_xmppId, ((User *)object)->getXmppId()))
      {
         UINT64 systemRights = GetEffectiveSystemRights((User *)object);

         TCHAR workstation[256];
         _tcscpy(workstation, _T("XMPP:"));
         nx_strncpy(&workstation[5], _xmppId, 251);

         if (systemRights & SYSTEM_ACCESS_XMPP_COMMANDS)
         {
            DbgPrintf(4, _T("User %s authenticated for XMPP commands"), object->getName());
            WriteAuditLog(AUDIT_SECURITY, TRUE, object->getId(), workstation, AUDIT_SYSTEM_SID, 0, _T("User authenticated for XMPP commands"));
            success = true;
         }
         else
         {
            DbgPrintf(4, _T("Access to XMPP commands denied for user %s"), object->getName());
            WriteAuditLog(AUDIT_SECURITY, FALSE, object->getId(), workstation, AUDIT_SYSTEM_SID, 0, _T("Access to XMPP commands denied"));
         }
         break;
      }
   }
   delete it;
   RWLockUnlock(s_userDatabaseLock);

   free(_xmppId);
   return success;
}
