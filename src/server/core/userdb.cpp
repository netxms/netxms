/*
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
static int m_userCount = 0;
static UserDatabaseObject **m_users = NULL;
static MUTEX m_mutexUserDatabaseAccess = INVALID_MUTEX_HANDLE;
static THREAD m_statusUpdateThread = INVALID_THREAD_HANDLE;

/**
 * Upgrade user accounts status in background
 */
static THREAD_RESULT THREAD_CALL AccountStatusUpdater(void *arg)
{
	DbgPrintf(2, _T("User account status update thread started"));
	while(!SleepAndCheckForShutdown(60))
	{
		DbgPrintf(8, _T("AccountStatusUpdater: wakeup"));

		time_t blockInactiveAccounts = (time_t)ConfigReadInt(_T("BlockInactiveUserAccounts"), 0) * 86400;

		MutexLock(m_mutexUserDatabaseAccess);
		time_t now = time(NULL);
		for(int i = 0; i < m_userCount; i++)
		{
			if (m_users[i]->isDeleted() || (m_users[i]->getId() & GROUP_FLAG))
				continue;

			User *user = (User *)m_users[i];

			if (user->isDisabled() && (user->getReEnableTime() > 0) && (user->getReEnableTime() <= now))
			{
				// Re-enable temporary disabled user
				user->enable();
            WriteAuditLog(AUDIT_SECURITY, TRUE, user->getId(), _T(""), 0, _T("Temporary disabled user account \"%s\" re-enabled by system"), user->getName());
				DbgPrintf(3, _T("Temporary disabled user account \"%s\" re-enabled"), user->getName());
			}

			if (!user->isDisabled() && (blockInactiveAccounts > 0) && (user->getLastLoginTime() > 0) && (user->getLastLoginTime() + blockInactiveAccounts < now))
			{
				user->disable();
            WriteAuditLog(AUDIT_SECURITY, TRUE, user->getId(), _T(""), 0, _T("User account \"%s\" disabled by system due to inactivity"), user->getName());
				DbgPrintf(3, _T("User account \"%s\" disabled due to inactivity"), user->getName());
			}
		}
		MutexUnlock(m_mutexUserDatabaseAccess);
	}

	DbgPrintf(2, _T("User account status update thread stopped"));
	return THREAD_OK;
}

/**
 * Initialize user handling subsystem
 */
void InitUsers()
{
   m_mutexUserDatabaseAccess = MutexCreate();
	m_statusUpdateThread = ThreadCreateEx(AccountStatusUpdater, 0, NULL);
}

/**
 * Cleanup user handling subsystem
 */
void CleanupUsers()
{
	ThreadJoin(m_statusUpdateThread);
}

/**
 * Load user list from database
 */
BOOL LoadUsers()
{
   int i;
   DB_RESULT hResult;

   // Load users
   hResult = DBSelect(g_hCoreDB,
	                   _T("SELECT id,name,system_access,flags,description,guid,ldap_dn,")
							 _T("password,full_name,grace_logins,auth_method,")
							 _T("cert_mapping_method,cert_mapping_data,auth_failures,")
							 _T("last_passwd_change,min_passwd_length,disabled_until,")
							 _T("last_login,xmpp_id FROM users"));
   if (hResult == NULL)
      return FALSE;

   m_userCount = DBGetNumRows(hResult);
   m_users = (UserDatabaseObject **)malloc(sizeof(UserDatabaseObject *) * m_userCount);
   for(i = 0; i < m_userCount; i++)
		m_users[i] = new User(hResult, i);

   DBFreeResult(hResult);

   // Check if user with UID 0 was loaded
   for(i = 0; i < m_userCount; i++)
		if (m_users[i]->getId() == 0)
         break;

   // Create superuser account if it doesn't exist
   if (i == m_userCount)
   {
      m_userCount++;
      m_users = (UserDatabaseObject **)realloc(m_users, sizeof(UserDatabaseObject *) * m_userCount);
		m_users[i] = new User();
      nxlog_write(MSG_SUPERUSER_CREATED, EVENTLOG_WARNING_TYPE, NULL);
   }

   // Load groups
   hResult = DBSelect(g_hCoreDB, _T("SELECT id,name,system_access,flags,description,guid,ldap_dn FROM user_groups"));
   if (hResult == NULL)
      return FALSE;

	int mark = m_userCount;
   m_userCount += DBGetNumRows(hResult);
   m_users = (UserDatabaseObject **)realloc(m_users, sizeof(UserDatabaseObject *) * m_userCount);
   for(i = mark; i < m_userCount; i++)
		m_users[i] = new Group(hResult, i - mark);

   DBFreeResult(hResult);

   // Check if everyone group was loaded
   for(i = mark; i < m_userCount; i++)
		if (m_users[i]->getId() == GROUP_EVERYONE)
         break;

   // Create everyone group if it doesn't exist
   if (i == m_userCount)
   {
      m_userCount++;
      m_users = (UserDatabaseObject **)realloc(m_users, sizeof(UserDatabaseObject *) * m_userCount);
		m_users[i] = new Group();
      nxlog_write(MSG_EVERYONE_GROUP_CREATED, EVENTLOG_WARNING_TYPE, NULL);
   }

   return TRUE;
}

/**
 * Save user list to database
 */
void SaveUsers(DB_HANDLE hdb)
{
   int i;

   // Save users
   MutexLock(m_mutexUserDatabaseAccess);
   for(i = 0; i < m_userCount; i++)
   {
      if (m_users[i]->isDeleted())
      {
			m_users[i]->deleteFromDatabase(hdb);
			delete m_users[i];
         m_userCount--;
         memmove(&m_users[i], &m_users[i + 1], sizeof(UserDatabaseObject *) * (m_userCount - i));
         i--;
      }
		else if (m_users[i]->isModified())
      {
			m_users[i]->saveToDatabase(hdb);
      }
   }
   MutexUnlock(m_mutexUserDatabaseAccess);
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
UINT32 AuthenticateUser(const TCHAR *login, const TCHAR *password, UINT32 dwSigLen, void *pCert,
                        BYTE *pChallenge, UINT32 *pdwId, UINT64 *pdwSystemRights,
							   bool *pbChangePasswd, bool *pbIntruderLockout, bool ssoAuth)
{
   int i, j;
   UINT32 dwResult = RCC_ACCESS_DENIED;
   BOOL bPasswordValid;

   MutexLock(m_mutexUserDatabaseAccess);
   for(i = 0; i < m_userCount; i++)
   {
		if ((!(m_users[i]->getId() & GROUP_FLAG)) &&
			 (!_tcscmp(login, m_users[i]->getName())) &&
			 (!m_users[i]->isDeleted()))
      {
			User *user = (User *)m_users[i];
         *pdwId = user->getId(); // always set user ID for caller so audit log will contain correct user ID on failures as well


         if(user->isLDAPUser())
         {
            if(user->isDisabled() || user->hasSyncException())
            {
               dwResult = RCC_ACCOUNT_DISABLED;
               goto result;
            }
            LDAPConnection conn;
            dwResult = conn.ldapUserLogin(user->getDn(), password);
            goto result;
         }

			// Determine authentication method to use
         if (!ssoAuth)
         {
			   int method = user->getAuthMethod();
			   if ((method == AUTH_CERT_OR_PASSWD) || (method == AUTH_CERT_OR_RADIUS))
			   {
				   if (dwSigLen > 0)
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
					   if (dwSigLen == 0)
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
					   if (dwSigLen == 0)
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
					   if ((dwSigLen != 0) && (pCert != NULL))
					   {
#ifdef _WITH_ENCRYPTION
						   bPasswordValid = ValidateUserCertificate((X509 *)pCert, login, pChallenge,
						                                            (BYTE *)password, dwSigLen,
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
                  nxlog_write(MSG_UNKNOWN_AUTH_METHOD, EVENTLOG_WARNING_TYPE, "ds", user->getAuthMethod(), login);
                  bPasswordValid = FALSE;
                  break;
            }
         }
         else
         {
				DbgPrintf(4, _T("User \"%s\" already authenticated by SSO server"), user->getName());
            bPasswordValid = TRUE;
         }

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
						   if (user->getId() != 0)	// Do not check grace logins for built-in admin user
						   {
							   if (user->getGraceLogins() <= 0)
							   {
								   DbgPrintf(4, _T("User \"%s\" has no grace logins left"), user->getName());
								   dwResult = RCC_NO_GRACE_LOGINS;
								   break;
							   }
							   user->decreaseGraceLogins();
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
							   if (user->getId() != 0)	// Do not check grace logins for built-in admin user
							   {
								   if (user->getGraceLogins() <= 0)
								   {
									   DbgPrintf(4, _T("User \"%s\" has no grace logins left"), user->getName());
									   dwResult = RCC_NO_GRACE_LOGINS;
									   break;
								   }
								   user->decreaseGraceLogins();
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
               *pdwSystemRights = user->getSystemRights();
					user->updateLastLogin();
               dwResult = RCC_SUCCESS;

               // Collect system rights from groups this user belongs to
               for(j = 0; j < m_userCount; j++)
						if ((m_users[j]->getId() & GROUP_FLAG) &&
							 (((Group *)m_users[j])->isMember(user->getId())))
							*pdwSystemRights |= ((Group *)m_users[j])->getSystemRights();
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
         break;
      }
   }
result:
   MutexUnlock(m_mutexUserDatabaseAccess);
   return dwResult;
}

/**
 * Check if user is a member of specific group
 */
bool NXCORE_EXPORTABLE CheckUserMembership(UINT32 dwUserId, UINT32 dwGroupId)
{
   bool result = false;

	if (!(dwGroupId & GROUP_FLAG))
		return false;

   if (dwGroupId == GROUP_EVERYONE)
		return true;

   MutexLock(m_mutexUserDatabaseAccess);
   for(int i = 0; i < m_userCount; i++)
   {
		if (m_users[i]->getId() == dwGroupId)
		{
			result = ((Group *)m_users[i])->isMember(dwUserId);
			break;
		}
   }
   MutexUnlock(m_mutexUserDatabaseAccess);
   return result;
}

/**
 * Fill message with group membership information for given user.
 * Access to user database must be locked.
 */
void FillGroupMembershipInfo(CSCPMessage *msg, UINT32 userId)
{
   UINT32 *list = (UINT32 *)malloc(sizeof(UINT32) * m_userCount);
   UINT32 count = 0;
   for(int i = 0; i < m_userCount; i++)
   {
		if ((m_users[i]->getId() & GROUP_FLAG) && (m_users[i]->getId() != GROUP_EVERYONE) && ((Group *)m_users[i])->isMember(userId))
		{
         list[count++] = m_users[i]->getId();
		}
   }
   msg->SetVariable(VID_NUM_GROUPS, count);
   if (count > 0)
      msg->setFieldInt32Array(VID_GROUPS, count, list);
   free(list);
}

/**
 * Update group membership for user
 */
void UpdateGroupMembership(UINT32 userId, int numGroups, UINT32 *groups)
{
   for(int i = 0; i < m_userCount; i++)
   {
		if ((m_users[i]->getId() & GROUP_FLAG) && (m_users[i]->getId() != GROUP_EVERYONE))
		{
         bool found = false;
         for(int j = 0; j < numGroups; j++)
         {
            if (m_users[i]->getId() == groups[j])
            {
               found = true;
               break;
            }
         }
         if (found)
         {
            ((Group *)m_users[i])->addUser(userId);
         }
         else
         {
            ((Group *)m_users[i])->deleteUser(userId);
         }
		}
   }
}

/**
 * Resolve user's ID to login name
 */
bool NXCORE_EXPORTABLE ResolveUserId(UINT32 id, TCHAR *buffer, int bufSize)
{
	bool found = false;

   MutexLock(m_mutexUserDatabaseAccess);
   for(int i = 0; i < m_userCount; i++)
		if (m_users[i]->getId() == id)
		{
			nx_strncpy(buffer, m_users[i]->getName(), bufSize);
			found = true;
			break;
		}
   MutexUnlock(m_mutexUserDatabaseAccess);

	return found;
}

/**
 * Update/Add LDAP user
 */
void NXCORE_EXPORTABLE UpdateLDAPUsers(const TCHAR* dn, Entry *obj)
{
   MutexLock(m_mutexUserDatabaseAccess);
   bool userModified = false;
   for(int i = 0; i < m_userCount; i++)
   {
		if (!(m_users[i]->getId() & GROUP_FLAG) && m_users[i]->isLDAPUser() && !_tcscmp(CHECK_NULL_EX(m_users[i]->getDn()), dn))
		{
         User* user = (User *)m_users[i];
         if (!user->isDeleted())
         {
            user->removeSyncException();
            if (!UserNameIsUnique(obj->m_loginName, m_users[i]->getId()))
            {
               user->setSyncException();
               TCHAR mistakeDescription[MAX_USER_DESCR];
               _sntprintf(mistakeDescription, MAX_USER_DESCR, _T("UpdateLDAPUsers(): Ldap sync error. User with name \"%s\" already exists."), obj->m_loginName);
               user->setDescription(mistakeDescription);
               DbgPrintf(4, mistakeDescription);
            }
            else
            {
               user->setName(obj->m_loginName);
               user->setFullName(obj->m_fullName);
               user->setDescription(obj->m_description);
               DbgPrintf(4, _T("UpdateLDAPUsers(): User updated: dn: %s, login name: %s, full name: %s, description: %s"), dn, obj->m_loginName, CHECK_NULL(obj->m_fullName), CHECK_NULL(obj->m_description));
            }
            if(user->isModified())
            {
               SendUserDBUpdate(USER_DB_MODIFY, user->getId(), m_users[i]);
            }
         }
         userModified = true;
         break;
		}
   }

   if (!userModified)
   {
      if (UserNameIsUnique(obj->m_loginName, INVALID_INDEX))
      {
         User* user = new User(CreateUniqueId(IDG_USER), obj->m_loginName);
         user->setFullName(obj->m_fullName);
         user->setDescription(obj->m_description);
         user->setFlags(UF_MODIFIED | UF_LDAP_USER);
         user->setDn(dn);
         m_users = (UserDatabaseObject **)realloc(m_users, sizeof(UserDatabaseObject *) * (m_userCount + 1));
         m_users[m_userCount] = user;
         m_userCount++;
         SendUserDBUpdate(USER_DB_CREATE, user->getId(), user);
         DbgPrintf(4, _T("UpdateLDAPUsers(): User added: dn: %s, login name: %s, full name: %s, description: %s"), dn, obj->m_loginName, CHECK_NULL(obj->m_fullName), CHECK_NULL(obj->m_description));
      }
      else
      {
         DbgPrintf(4, _T("UpdateLDAPUsers(): User with name %s already exists, but is not LDAP user. LDAP user won`t be creadted."), obj->m_loginName);
      }
   }
   MutexUnlock(m_mutexUserDatabaseAccess);
}

/**
 * Goes throught all existing ldap entries and check that in newly gotten list they also exist.
 * If ldap entries does not exists in new list - it will be disabled or removed depending on action parameter.
 */
void RemoveDeletedLDAPEntry(StringObjectMap<Entry>* entryList, UINT32 m_action, bool isUser)
{
   MutexLock(m_mutexUserDatabaseAccess);
   for(int i = 0; i < m_userCount; i++)
   {
      bool validType = isUser ? !(m_users[i]->getId() & GROUP_FLAG) : ((m_users[i]->getId() & GROUP_FLAG) ? true : false);
      if (validType && m_users[i]->isLDAPUser() && !m_users[i]->isDeleted())
		{
         if (entryList->get(m_users[i]->getDn()) == NULL)
         {
            if (m_action == USER_DELETE)
               DeleteUserDatabaseObject(m_users[i]->getId());
            if (m_action == USER_DISABLE)
            {
               m_users[i]->disable();
               m_users[i]->setDescription(_T("LDAP entry was deleted."));
            }
            DbgPrintf(4, _T("RemoveDeletedLDAPEntry(): Ldap %s entry was removed form DB."), m_users[i]->getDn());
         }
		}
   }
   MutexUnlock(m_mutexUserDatabaseAccess);
}

/**
 * Update/Add LDAP group
 */
void NXCORE_EXPORTABLE UpdateLDAPGroups(const TCHAR* dn, Entry *obj) //no full name, add users inside group, and delete removed from the group
{
   MutexLock(m_mutexUserDatabaseAccess);
   bool userModified = false;
   for(int i = 0; i < m_userCount; i++)
   {
		if ((m_users[i]->getId() & GROUP_FLAG) && m_users[i]->isLDAPUser() && !_tcscmp(CHECK_NULL_EX(m_users[i]->getDn()), dn))
		{
         Group* group = (Group *)m_users[i];
         if(!group->isDeleted())
         {
            group->removeSyncException();
            if(!GroupNameIsUnique(obj->m_loginName, m_users[i]->getId()))
            {
               group->setSyncException();
               TCHAR mistakeDescription[MAX_USER_DESCR];
               _sntprintf(mistakeDescription, MAX_USER_DESCR, _T("UpdateLDAPGroups(): LDAP sync error. Group with \"%s\" name already exists."), obj->m_loginName);
               group->setDescription(mistakeDescription);
               DbgPrintf(4, mistakeDescription);
            }
            else
            {
               group->setName(obj->m_loginName);
               group->setDescription(obj->m_description);
               DbgPrintf(4, _T("UpdateLDAPGroups(): Group updated: dn: %s, login name: %s, description: %s"), dn, obj->m_loginName, CHECK_NULL(obj->m_description));
            }
            if(group->isModified())
            {
               SendUserDBUpdate(USER_DB_MODIFY, group->getId(), m_users[i]);
            }
            SyncGroupMembers(group , obj);
         }
         userModified = true;
         break;
		}
   }
   if(!userModified)
   {
      if(GroupNameIsUnique(obj->m_loginName, INVALID_INDEX))
      {
         Group* group = new Group(CreateUniqueId(IDG_USER_GROUP), obj->m_loginName);
         group->setDescription(obj->m_description);
         group->setFlags(UF_MODIFIED | UF_LDAP_USER);
         group->setDn(dn);
         SendUserDBUpdate(USER_DB_CREATE, group->getId(), group);
         m_users = (UserDatabaseObject **)realloc(m_users, sizeof(UserDatabaseObject *) * (m_userCount + 1));
         m_users[m_userCount] = group;
         m_userCount++;
         SyncGroupMembers(group , obj);
         DbgPrintf(4, _T("UpdateLDAPGroups(): Group added: dn: %s, login name: %s, description: %s"), dn, obj->m_loginName, CHECK_NULL(obj->m_description));
      }
      else
      {
         DbgPrintf(4, _T("UpdateLDAPGroups(): Group with %s name already exists, but is not LDAP user. LDAP user won't be creadted."), obj->m_loginName);
      }
   }
   MutexUnlock(m_mutexUserDatabaseAccess);
}

/**
 * Synchronize new user list with old user list of given group. Not LDAP usera will not be changed.
 */
void SyncGroupMembers(Group* group, Entry *obj)
{
   StringList* newMembers = obj->m_memberList;
   UINT32 *oldMembers = NULL;
   int count = group->getMembers(&oldMembers);
   int i, j;
   DbgPrintf(4, _T("SyncGroupMembers(): Sync for group: %s."), group->getDn());

   /**
    * Go throught existing group member list checking each ldap user by dn
    * with new gotten group member list and removing ldap users not existing in last list.
    */
   for(i = 0; i <count; i++)
   {
      UserDatabaseObject * user = GetUser(oldMembers[i]);
      if(user != NULL && user->isLDAPUser())
      {
         bool found = false;
         for(j = 0; j < newMembers->getSize(); j++)
         {
            if(!_tcscmp(newMembers->getValue(j), CHECK_NULL_EX(user->getDn())))
            {
               found = true;
               break;
            }
         }
         if(!found)
         {
            DbgPrintf(4, _T("SyncGroupMembers: Remove from %s group deleted user: %s"), group->getDn(), user->getDn());
            group->deleteUser(user->getId());
            int count = group->getMembers(&oldMembers);
         }
      }
   }

   /**
    * Go throught new gotten group member list checking each ldap user by dn
    * with existing group member list and adding users not existing in last list.
    */
   for(i = 0; i< newMembers->getSize(); i++)
   {
      UserDatabaseObject * userNew = GetUser(newMembers->getValue(i));
      if(userNew != NULL && userNew->isLDAPUser())
      {
         bool found = false;
         for(j = 0; j <count; j++)
         {
            UINT32 member = oldMembers[j];
            UserDatabaseObject * userOld = GetUser(member);
            if(!_tcscmp(CHECK_NULL_EX(userOld->getDn()), userNew->getDn()) && userOld->isLDAPUser()) //check that is ldap user  ->  if not - continue to check (it may happend that we have 2 users with 1 ldap - 1 ldapUser, other is not ldap?)
            {
               found = true;
               break;
            }
         }
         if(!found)
         {
            DbgPrintf(4, _T("SyncGroupMembers: %s user added to %s group"), newMembers->getValue(i), group->getDn());
            group->addUser(userNew->getId());
            int count = group->getMembers(&oldMembers);
         }
      }
   }
}

/**
 * Gets user object from it's ID
 * DB should be locked outside this function
 */
UserDatabaseObject* GetUser(UINT32 userID)
{
   for(int i = 0; i < m_userCount; i++)
   {
      if(m_users[i]->getId() == userID)
         return m_users[i];
   }
   return NULL;
}

/**
 * Gets user object from it's ID
 * DB should be locked outside this function
 */
UserDatabaseObject* GetUser(const TCHAR* dn)
{
   for(int i = 0; i < m_userCount; i++)
   {
      if(!_tcscmp(CHECK_NULL_EX(m_users[i]->getDn()), CHECK_NULL_EX(dn)))
         return m_users[i];
   }
   return NULL;
}

/**
 * Acess to user DB must be loked when this function is called
 */
bool UserNameIsUnique(TCHAR* name, UINT32 id)
{
   for(int i = 0; i < m_userCount; i++)
		if (!(m_users[i]->getId() & GROUP_FLAG) && !_tcscmp(m_users[i]->getName(), name) && m_users[i]->getId() != id)
         return false;
   return true;
}

/**
 * Acess to user DB must be loked when this function is called
 */
bool GroupNameIsUnique(TCHAR* name, UINT32 id)
{
   for(int i = 0; i < m_userCount; i++)
		if ((m_users[i]->getId() & GROUP_FLAG) && !_tcscmp(m_users[i]->getName(), name) && m_users[i]->getId() != id)
         return false;
   return true;
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
   MutexLock(m_mutexUserDatabaseAccess);

   for(int i = 0; i < m_userCount; i++)
   {
		if (!(m_users[i]->getId() & GROUP_FLAG))
      {
			UINT64 systemRights = m_users[i]->getSystemRights();
			for(int j = 0; j < m_userCount; j++)
         {
				if ((m_users[j]->getId() & GROUP_FLAG) && (((Group *) m_users[j])->isMember(m_users[i]->getId())))
					systemRights |= ((Group *)m_users[j])->getSystemRights();
			}

         TCHAR szGUID[64];
         ConsolePrintf(pCtx, _T("%-20s %-36s 0x") UINT64X_FMT(_T("016")) _T("\n"), m_users[i]->getName(), m_users[i]->getGuidAsText(szGUID), systemRights);
		}
	}
   MutexUnlock(m_mutexUserDatabaseAccess);
   ConsolePrintf(pCtx, _T("\n"));
}

/**
 * Delete user or group
 *
 * @param id user database object ID
 * @return RCC ready to be sent to client
 */
UINT32 NXCORE_EXPORTABLE DeleteUserDatabaseObject(UINT32 id)
{
   int i, j;

   DeleteUserFromAllObjects(id);

   MutexLock(m_mutexUserDatabaseAccess);

   for(i = 0; i < m_userCount; i++)
	{
		if (m_users[i]->getId() == id)
		{
			m_users[i]->setDeleted();
			if (!(id & GROUP_FLAG))
			{
				// Remove user from all groups
				for(j = 0; j < m_userCount; j++)
				{
					if (m_users[j]->getId() & GROUP_FLAG)
					{
						((Group *)m_users[j])->deleteUser(id);
					}
				}
			}
			break;
		}
	}

   MutexUnlock(m_mutexUserDatabaseAccess);

   SendUserDBUpdate(USER_DB_DELETE, id, NULL);
   return RCC_SUCCESS;
}

/**
 * Create new user or group
 */
UINT32 NXCORE_EXPORTABLE CreateNewUser(TCHAR *pszName, BOOL bIsGroup, UINT32 *pdwId)
{
   UINT32 dwResult = RCC_SUCCESS;
	UserDatabaseObject *object;
	int i;

   MutexLock(m_mutexUserDatabaseAccess);

   // Check for duplicate name
   for(i = 0; i < m_userCount; i++)
	{
      if (!_tcscmp(m_users[i]->getName(), pszName))
      {
         dwResult = RCC_ALREADY_EXIST;
         break;
      }
	}

	if (dwResult == RCC_SUCCESS)
	{
		if (bIsGroup)
		{
			object = new Group(CreateUniqueId(IDG_USER_GROUP), pszName);
		}
		else
		{
			object = new User(CreateUniqueId(IDG_USER), pszName);
		}

		m_users = (UserDatabaseObject **)realloc(m_users, sizeof(UserDatabaseObject *) * (m_userCount + 1));
		m_users[m_userCount] = object;
		m_userCount++;

		SendUserDBUpdate(USER_DB_CREATE, object->getId(), object);

		*pdwId = object->getId();
	}

   MutexUnlock(m_mutexUserDatabaseAccess);
	return dwResult;
}

/**
 * Modify user database object
 */
UINT32 NXCORE_EXPORTABLE ModifyUserDatabaseObject(CSCPMessage *msg)
{
   UINT32 id, fields, dwResult = RCC_INVALID_USER_ID;
	int i;

	id = msg->GetVariableLong(VID_USER_ID);

   MutexLock(m_mutexUserDatabaseAccess);

   // Find object to be modified in list
   for(i = 0; i < m_userCount; i++)
		if (m_users[i]->getId() == id)
      {
			TCHAR name[MAX_USER_NAME];

			fields = msg->GetVariableLong(VID_FIELDS);
			if (fields & USER_MODIFY_LOGIN_NAME)
			{
				msg->GetVariableStr(VID_USER_NAME, name, MAX_USER_NAME);
				if (!IsValidObjectName(name))
				{
					dwResult = RCC_INVALID_OBJECT_NAME;
					break;
				}
			}

			m_users[i]->modifyFromMessage(msg);
         SendUserDBUpdate(USER_DB_MODIFY, id, m_users[i]);
         dwResult = RCC_SUCCESS;
         break;
      }

   MutexUnlock(m_mutexUserDatabaseAccess);
   return dwResult;
}

/**
 * Send user DB update for given user ID.
 * Access to suer database must be already locked.
 */
void SendUserDBUpdate(int code, UINT32 id)
{
   for(int i = 0; i < m_userCount; i++)
   {
      if (m_users[i]->getId() == id)
      {
         SendUserDBUpdate(code, id, m_users[i]);
         break;
      }
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
	int i;
   UINT32 dwResult = RCC_INVALID_USER_ID;

	if (id & GROUP_FLAG)
		return RCC_INVALID_USER_ID;

   MutexLock(m_mutexUserDatabaseAccess);

   // Find user
   for(i = 0; i < m_userCount; i++)
		if (m_users[i]->getId() == id)
      {
			User *user = (User *)m_users[i];
			if (changeOwnPassword)
			{
				if (!user->canChangePassword() || !user->validatePassword(oldPassword))
				{
					dwResult = RCC_ACCESS_DENIED;
					break;
				}

				// Check password length
				int minLength = (user->getMinMasswordLength() == -1) ? ConfigReadInt(_T("MinPasswordLength"), 0) : user->getMinMasswordLength();
				if ((int)_tcslen(newPassword) < minLength)
				{
					dwResult = RCC_WEAK_PASSWORD;
					break;
				}

				// Check password complexity
				if (!CheckPasswordComplexity(newPassword))
				{
					dwResult = RCC_WEAK_PASSWORD;
					break;
				}

				// Update password history
				int passwordHistoryLength = ConfigReadInt(_T("PasswordHistoryLength"), 0);
				if (passwordHistoryLength > 0)
				{
					TCHAR query[8192], *ph = NULL;

					_sntprintf(query, 8192, _T("SELECT password_history FROM users WHERE id=%d"), id);
					DB_RESULT hResult = DBSelect(g_hCoreDB, query);
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
								break;
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
							DBQuery(g_hCoreDB, query);
						}

						free(ph);
						if (dwResult == RCC_REUSED_PASSWORD)
							break;
					}
					else
					{
						dwResult = RCC_DB_FAILURE;
						break;
					}
				}

				user->updatePasswordChangeTime();
			}
			user->setPassword(newPassword, changeOwnPassword);
         dwResult = RCC_SUCCESS;
         break;
      }

   MutexUnlock(m_mutexUserDatabaseAccess);
   return dwResult;
}

/**
 * Open user database
 */
UserDatabaseObject NXCORE_EXPORTABLE **OpenUserDatabase(int *count)
{
   MutexLock(m_mutexUserDatabaseAccess);
	*count = m_userCount;
	return m_users;
}

/**
 * Close user database
 */
void NXCORE_EXPORTABLE CloseUserDatabase()
{
   MutexUnlock(m_mutexUserDatabaseAccess);
}

/**
 * Get custom attribute's value
 */
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(UINT32 id, const TCHAR *name)
{
	const TCHAR *value = NULL;

   MutexLock(m_mutexUserDatabaseAccess);

   for(int i = 0; i < m_userCount; i++)
		if (m_users[i]->getId() == id)
      {
			value = m_users[i]->getAttribute(name);
         break;
      }

   MutexUnlock(m_mutexUserDatabaseAccess);
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
   MutexLock(m_mutexUserDatabaseAccess);

   for(int i = 0; i < m_userCount; i++)
		if (m_users[i]->getId() == id)
      {
			m_users[i]->setAttribute(name, value);
         break;
      }

   MutexUnlock(m_mutexUserDatabaseAccess);
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

   MutexLock(m_mutexUserDatabaseAccess);
   for(int i = 0; i < m_userCount; i++)
   {
		if (!(m_users[i]->getId() & GROUP_FLAG) &&
          !m_users[i]->isDisabled() && !m_users[i]->isDeleted() &&
			 !_tcsicmp(_xmppId, ((User *)m_users[i])->getXmppId()))
      {
         DbgPrintf(4, _T("User %s authenticated for XMPP subscription"), m_users[i]->getName());
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_users[i]->getId(), NULL, 0, _T("User authenticated for XMPP subscription"));
         success = true;
         break;
      }
   }
   MutexUnlock(m_mutexUserDatabaseAccess);

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

   MutexLock(m_mutexUserDatabaseAccess);
   for(int i = 0; i < m_userCount; i++)
   {
		if (!(m_users[i]->getId() & GROUP_FLAG) &&
          !m_users[i]->isDisabled() && !m_users[i]->isDeleted() &&
			 !_tcsicmp(_xmppId, ((User *)m_users[i])->getXmppId()))
      {
         UINT64 systemRights = m_users[i]->getSystemRights();

         // Collect system rights from groups this user belongs to
         for(int j = 0; j < m_userCount; j++)
				if ((m_users[j]->getId() & GROUP_FLAG) &&
					 (((Group *)m_users[j])->isMember(m_users[i]->getId())))
					systemRights |= ((Group *)m_users[j])->getSystemRights();

         if (systemRights & SYSTEM_ACCESS_XMPP_COMMANDS)
         {
            DbgPrintf(4, _T("User %s authenticated for XMPP commands"), m_users[i]->getName());
            WriteAuditLog(AUDIT_SECURITY, TRUE, m_users[i]->getId(), NULL, 0, _T("User authenticated for XMPP commands"));
            success = true;
         }
         else
         {
            DbgPrintf(4, _T("Access to XMPP commands denied for user %s"), m_users[i]->getName());
            WriteAuditLog(AUDIT_SECURITY, FALSE, m_users[i]->getId(), NULL, 0, _T("Access to XMPP commands denied"));
         }
         break;
      }
   }
   MutexUnlock(m_mutexUserDatabaseAccess);

   free(_xmppId);
   return success;
}
