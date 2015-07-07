/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: nms_users.h
**
**/

#if WITH_LDAP

#if !defined(__hpux)
#define LDAP_DEPRECATED 1
#endif

#ifdef _WIN32

#include <winldap.h>
#include <winber.h>

#else /* _WIN32 */

#include <ldap.h>
#if HAVE_LDAP_SSL_H
#include <ldap_ssl.h>
#endif

#if !HAVE_BER_INT_T
typedef int ber_int_t;
#endif

#endif /* _WIN32 */

#endif /* WITH_LDAP */

/**
 * LDAP entry (object)
 */
class Entry
{
public:
   UINT32 m_type;
   TCHAR* m_loginName;
   TCHAR* m_fullName;
   TCHAR* m_description;
   StringList *m_memberList;

   Entry();
   ~Entry();
};

/**
 * LDAP connector
 */
class LDAPConnection
{
private:
#if WITH_LDAP
   LDAP *m_ldapConn;
#ifdef _WIN32
   TCHAR m_connList[MAX_DB_STRING];
   TCHAR m_searchBase[MAX_DB_STRING];
   TCHAR m_searchFilter[MAX_DB_STRING];
   TCHAR m_userDN[MAX_DB_STRING];
   TCHAR m_userPassword[MAX_DB_STRING];
#else
   char m_connList[MAX_DB_STRING];
   char m_searchBase[MAX_DB_STRING];
   char m_searchFilter[MAX_DB_STRING];
   char m_userDN[MAX_DB_STRING];
   char m_userPassword[MAX_DB_STRING];
#endif
   char m_ldapFullNameAttr[MAX_DB_STRING];
   char m_ldapLoginNameAttr[MAX_DB_STRING];
   char m_ldapDescriptionAttr[MAX_DB_STRING];
   TCHAR m_userClass[MAX_DB_STRING];
   TCHAR m_groupClass[MAX_DB_STRING];
   int m_action;
   int m_secure;
   int m_pageSize;

   void closeLDAPConnection();
   void initLDAP();
   UINT32 loginLDAP();
   TCHAR *getErrorString(int code);
   void getAllSyncParameters();
   void compareGroupList(StringObjectMap<Entry>* groupEntryList);
   void compareUserLists(StringObjectMap<Entry>* userEntryList);
   TCHAR *getAttrValue(LDAPMessage *entry, const char *attr, UINT32 i = 0);
#ifdef _WIN32
   void prepareStringForInit(TCHAR *connectionLine);
#else
   void prepareStringForInit(char *connectionLine);
#endif // _WIN32
   int readInPages(StringObjectMap<Entry> *userEntryList, StringObjectMap<Entry> *groupEntryList);
   void fillLists(LDAPMessage *searchResult, StringObjectMap<Entry> *userEntryList, StringObjectMap<Entry> *groupEntryList);
#endif // WITH_LDAP

public:
#if WITH_LDAP
   LDAPConnection();
   ~LDAPConnection();
#endif // WITH_LDAP

   void syncUsers();
   UINT32 ldapUserLogin(const TCHAR *name, const TCHAR *password);
};

#ifndef _nms_users_h_
#define _nms_users_h_

/**
 * Maximum number of grace logins allowed for user
 */
#define MAX_GRACE_LOGINS      5

/**
 * Maximum length of XMPP ID
 */
#define MAX_XMPP_ID_LEN          128

/**
 * Authentication methods
 */
enum UserAuthMethod
{
   AUTH_NETXMS_PASSWORD = 0,
   AUTH_RADIUS          = 1,
   AUTH_CERTIFICATE     = 2,
   AUTH_CERT_OR_PASSWD  = 3,
   AUTH_CERT_OR_RADIUS  = 4
};

/**
 * Generic user database object
 */
class NXCORE_EXPORTABLE UserDatabaseObject
{
protected:
	UINT32 m_id;
   uuid m_guid;
	TCHAR m_name[MAX_USER_NAME];
	TCHAR m_description[MAX_USER_DESCR];
	UINT64 m_systemRights;
	UINT32 m_flags;
	StringMap m_attributes;		// Custom attributes
   TCHAR *m_userDn;

	bool loadCustomAttributes(DB_HANDLE hdb);
	bool saveCustomAttributes(DB_HANDLE hdb);

public:
	UserDatabaseObject();
	UserDatabaseObject(DB_RESULT hResult, int row);
	UserDatabaseObject(UINT32 id, const TCHAR *name);
	virtual ~UserDatabaseObject();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(NXCPMessage *msg);
	virtual void modifyFromMessage(NXCPMessage *msg);
	void detachLdapUser();

	UINT32 getId() { return m_id; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getDescription() { return m_description; }
	UINT64 getSystemRights() { return m_systemRights; }
	UINT32 getFlags() { return m_flags; }
   TCHAR *getGuidAsText(TCHAR *buffer) { return m_guid.toString(buffer); }
   const TCHAR *getDn() { return m_userDn; }

	bool isDeleted() { return (m_flags & UF_DELETED) ? true : false; }
	bool isDisabled() { return (m_flags & UF_DISABLED) ? true : false; }
	bool isModified() { return (m_flags & UF_MODIFIED) ? true : false; }
	bool isLDAPUser() { return (m_flags & UF_LDAP_USER) ? true : false; }
	bool hasSyncException() { return (m_flags & UF_SYNC_EXCEPTION) ? true : false; }

	void setDeleted() { m_flags |= UF_DELETED; }
	void enable();
	void disable();
	void setFlags(UINT32 flags) { m_flags = flags; }
	void removeSyncException();
	void setSyncException();

	const TCHAR *getAttribute(const TCHAR *name) { return m_attributes.get(name); }
	UINT32 getAttributeAsULong(const TCHAR *name);
	void setAttribute(const TCHAR *name, const TCHAR *value) { m_attributes.set(name, value); m_flags |= UF_MODIFIED; }
	void setName(const TCHAR *name);
	void setDescription(const TCHAR *description);
	void setDn(const TCHAR *dn);
};

/**
 * Hash types
 */
enum PasswordHashType
{
   PWD_HASH_SHA1 = 0,
   PWD_HASH_SHA256 = 1
};

/**
 * Password salt length
 */
#define PASSWORD_SALT_LENGTH  8

/**
 * Password hash size
 */
#define PWD_HASH_SIZE(t) ((t == PWD_HASH_SHA256) ? SHA256_DIGEST_SIZE : ((t == PWD_HASH_SHA1) ? SHA1_DIGEST_SIZE : 0))

/**
 * Hashed password
 */
struct PasswordHash
{
   PasswordHashType hashType;
   BYTE hash[SHA256_DIGEST_SIZE];
   BYTE salt[PASSWORD_SALT_LENGTH];
};

/**
 * User object
 */
class NXCORE_EXPORTABLE User : public UserDatabaseObject
{
protected:
	TCHAR m_fullName[MAX_USER_FULLNAME];
   PasswordHash m_password;
   int m_graceLogins;
   int m_authMethod;
	int m_certMappingMethod;
	TCHAR *m_certMappingData;
	time_t m_disabledUntil;
	time_t m_lastPasswordChange;
	time_t m_lastLogin;
	int m_minPasswordLength;
	int m_authFailures;
   TCHAR m_xmppId[MAX_XMPP_ID_LEN];

public:
	User();
	User(DB_RESULT hResult, int row);
	User(UINT32 id, const TCHAR *name);
	virtual ~User();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(NXCPMessage *msg);
	virtual void modifyFromMessage(NXCPMessage *msg);

	const TCHAR *getFullName() { return m_fullName; }
	int getGraceLogins() { return m_graceLogins; }
	int getAuthMethod() { return m_authMethod; }
	int getCertMappingMethod() { return m_certMappingMethod; }
	time_t getLastLoginTime() { return m_lastLogin; }
	time_t getPasswordChangeTime() { return m_lastPasswordChange; }
	const TCHAR *getCertMappingData() { return m_certMappingData; }
	bool isIntruderLockoutActive() { return (m_flags & UF_INTRUDER_LOCKOUT) != 0; }
	bool canChangePassword() { return (m_flags & UF_CANNOT_CHANGE_PASSWORD) == 0; }
	int getMinMasswordLength() { return m_minPasswordLength; }
	time_t getReEnableTime() { return m_disabledUntil; }
   const TCHAR *getXmppId() { return m_xmppId; }

	bool validatePassword(const TCHAR *password);
	void decreaseGraceLogins() { if (m_graceLogins > 0) m_graceLogins--; m_flags |= UF_MODIFIED; }
	void setPassword(const TCHAR *password, bool clearChangePasswdFlag);
	void increaseAuthFailures();
	void resetAuthFailures() { m_authFailures = 0; m_flags |= UF_MODIFIED; }
	void updateLastLogin() { m_lastLogin = time(NULL); m_flags |= UF_MODIFIED; }
	void updatePasswordChangeTime() { m_lastPasswordChange = time(NULL); m_flags |= UF_MODIFIED; }
	void setFullName(const TCHAR *fullName);
	void enable();
};

/**
 * Group object
 */
class NXCORE_EXPORTABLE Group : public UserDatabaseObject
{
protected:
	int m_memberCount;
	UINT32 *m_members;

public:
	Group();
	Group(DB_RESULT hResult, int row);
	Group(UINT32 id, const TCHAR *name);
	virtual ~Group();

	virtual void fillMessage(NXCPMessage *msg);
	virtual void modifyFromMessage(NXCPMessage *msg);

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	void addUser(UINT32 userId);
	void deleteUser(UINT32 userId);
	bool isMember(UINT32 userId);
	int getMembers(UINT32 **members);
};

/**
 * Access list element structure
 */
typedef struct
{
   UINT32 dwUserId;
   UINT32 dwAccessRights;
} ACL_ELEMENT;

/**
 * Access list class
 */
class AccessList
{
private:
   UINT32 m_dwNumElements;
   ACL_ELEMENT *m_pElements;
   MUTEX m_hMutex;

   void lock() { MutexLock(m_hMutex); }
   void unlock() { MutexUnlock(m_hMutex); }

public:
   AccessList();
   ~AccessList();

   bool getUserRights(UINT32 dwUserId, UINT32 *pdwAccessRights);
   void addElement(UINT32 dwUserId, UINT32 dwAccessRights);
   bool deleteElement(UINT32 dwUserId);
   void deleteAll();

   void enumerateElements(void (* pHandler)(UINT32, UINT32, void *), void *pArg);

   void fillMessage(NXCPMessage *pMsg);
};

/**
 * Functions
 */
BOOL LoadUsers();
void SaveUsers(DB_HANDLE hdb);
void SendUserDBUpdate(int code, UINT32 id, UserDatabaseObject *object);
void SendUserDBUpdate(int code, UINT32 id);
UINT32 AuthenticateUser(const TCHAR *login, const TCHAR *password, UINT32 dwSigLen, void *pCert,
                        BYTE *pChallenge, UINT32 *pdwId, UINT64 *pdwSystemRights,
							   bool *pbChangePasswd, bool *pbIntruderLockout, bool ssoAuth);
bool AuthenticateUserForXMPPCommands(const char *xmppId);
bool AuthenticateUserForXMPPSubscription(const char *xmppId);

UINT32 NXCORE_EXPORTABLE SetUserPassword(UINT32 id, const TCHAR *newPassword, const TCHAR *oldPassword, bool changeOwnPassword);
bool NXCORE_EXPORTABLE CheckUserMembership(UINT32 dwUserId, UINT32 dwGroupId);
UINT32 NXCORE_EXPORTABLE DeleteUserDatabaseObject(UINT32 id);
UINT32 NXCORE_EXPORTABLE CreateNewUser(TCHAR *pszName, BOOL bIsGroup, UINT32 *pdwId);
UINT32 NXCORE_EXPORTABLE ModifyUserDatabaseObject(NXCPMessage *msg);
UINT32 NXCORE_EXPORTABLE DetachLdapUser(UINT32 id);
UserDatabaseObject NXCORE_EXPORTABLE **OpenUserDatabase(int *count);
void NXCORE_EXPORTABLE CloseUserDatabase();
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(UINT32 id, const TCHAR *name);
UINT32 NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(UINT32 id, const TCHAR *name);
void NXCORE_EXPORTABLE SetUserDbObjectAttr(UINT32 id, const TCHAR *name, const TCHAR *value);
bool NXCORE_EXPORTABLE ResolveUserId(UINT32 id, TCHAR *buffer, int bufSize);
void NXCORE_EXPORTABLE UpdateLDAPUser(const TCHAR* dn, Entry *obj);
void RemoveDeletedLDAPEntry(StringObjectMap<Entry>* userEntryList, UINT32 m_action, bool isUser);
void NXCORE_EXPORTABLE UpdateLDAPGroup(const TCHAR* dn, Entry *obj);
void SyncGroupMembers(Group* group, Entry *obj);
UserDatabaseObject* GetUser(UINT32 userID);
UserDatabaseObject* GetUser(const TCHAR* dn);
THREAD_RESULT THREAD_CALL SyncLDAPUsers(void *arg);
bool UserNameIsUnique(const TCHAR *name, UINT32 id);
bool GroupNameIsUnique(const TCHAR *name, UINT32 id);
void FillGroupMembershipInfo(NXCPMessage *msg, UINT32 userId);
void UpdateGroupMembership(UINT32 userId, int numGroups, UINT32 *groups);
void DumpUsers(CONSOLE_CTX pCtx);

/**
 * CAS API
 */
void CASReadSettings();
bool CASAuthenticate(const char *ticket, TCHAR *loginName);

#endif
