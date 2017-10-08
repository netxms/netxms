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
   TCHAR* m_id;
   StringSet *m_memberList;

   Entry();
   ~Entry();
};

#if WITH_LDAP

// Defines to handle string encoding difference between Windows and other systems
#ifdef _WIN32

#define LDAP_CHAR TCHAR
#define ldap_strchr _tcschr
#define ldap_strstr _tcsstr
#define ldap_strrchr _tcsrchr
#define ldap_strcpy _tcscpy
#define ldap_strcat _tcscat
#define ldap_strlen _tcslen
#define ldap_timeval l_timeval
#define ldap_strdup _tcsdup
#define ldap_snprintf _sntprintf
#define LdapConfigRead ConfigReadStr
#define _TLDAP(x) _T(x)
#define LDAP_TFMT _T("%s")

#else

#define LDAP_CHAR char
#define ldap_strchr strchr
#define ldap_strstr strstr
#define ldap_strrchr strrchr
#define ldap_strcpy strcpy
#define ldap_strcat strcat
#define ldap_strlen strlen
#define ldap_timeval timeval
#define ldap_strdup strdup
#define ldap_snprintf snprintf
#define LdapConfigRead ConfigReadStrUTF8
#define _TLDAP(x) x
#define LDAP_TFMT _T("%hs")

#endif // _WIN32
#endif // WITH_LDAP

/**
 * LDAP connector
 */
class LDAPConnection
{
private:
#if WITH_LDAP
   LDAP *m_ldapConn;
   LDAP_CHAR m_connList[MAX_CONFIG_VALUE];
   LDAP_CHAR m_searchBase[MAX_CONFIG_VALUE];
   LDAP_CHAR m_searchFilter[MAX_CONFIG_VALUE];
   LDAP_CHAR m_userDN[MAX_CONFIG_VALUE];
   LDAP_CHAR m_userPassword[MAX_PASSWORD];
   char m_ldapFullNameAttr[MAX_CONFIG_VALUE];
   char m_ldapLoginNameAttr[MAX_CONFIG_VALUE];
   char m_ldapDescriptionAttr[MAX_CONFIG_VALUE];
   char m_ldapUsreIdAttr[MAX_CONFIG_VALUE];
   char m_ldapGroupIdAttr[MAX_CONFIG_VALUE];
   TCHAR m_userClass[MAX_CONFIG_VALUE];
   TCHAR m_groupClass[MAX_CONFIG_VALUE];
   int m_action;
   int m_secure;
   int m_pageSize;

   void closeLDAPConnection();
   void initLDAP();
   UINT32 loginLDAP();
   TCHAR *getErrorString(int code);
   void getAllSyncParameters();
   void compareGroupList();
   void compareUserLists();
   TCHAR *getAttrValue(LDAPMessage *entry, const char *attr, UINT32 i = 0);
   TCHAR *getIdAttrValue(LDAPMessage *entry, const char *attr);
   void prepareStringForInit(LDAP_CHAR *connectionLine);
   int readInPages(LDAP_CHAR *base);
   void fillLists(LDAPMessage *searchResult);
   TCHAR *ldap_internal_get_dn(LDAP *conn, LDAPMessage *entry);
   void updateMembers(StringSet *memberList, const char *firstAttr, LDAPMessage *firstEntry, const LDAP_CHAR *dn);
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
   TCHAR *m_ldapDn;
   TCHAR *m_ldapId;

	bool loadCustomAttributes(DB_HANDLE hdb);
	bool saveCustomAttributes(DB_HANDLE hdb);

public:
	UserDatabaseObject();
	UserDatabaseObject(DB_HANDLE hdb, DB_RESULT hResult, int row);
	UserDatabaseObject(UINT32 id, const TCHAR *name);
	virtual ~UserDatabaseObject();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(NXCPMessage *msg);
	virtual void modifyFromMessage(NXCPMessage *msg);

	virtual json_t *toJson() const;

	UINT32 getId() const { return m_id; }
	const TCHAR *getName() const { return m_name; }
	const TCHAR *getDescription() const { return m_description; }
	UINT64 getSystemRights() const { return m_systemRights; }
	UINT32 getFlags() const { return m_flags; }
   TCHAR *getGuidAsText(TCHAR *buffer) const { return m_guid.toString(buffer); }
   const TCHAR *getDn() const { return m_ldapDn; }
   const TCHAR *getLdapId() const { return m_ldapId; }

   bool isGroup() const { return (m_id & GROUP_FLAG) != 0; }
	bool isDeleted() const { return (m_flags & UF_DELETED) ? true : false; }
	bool isDisabled() const { return (m_flags & UF_DISABLED) ? true : false; }
	bool isModified() const { return (m_flags & UF_MODIFIED) ? true : false; }
	bool isLDAPUser() const { return (m_flags & UF_LDAP_USER) ? true : false; }

	void setDeleted() { m_flags |= UF_DELETED; }
	void enable();
	void disable();
	void setFlags(UINT32 flags) { m_flags = flags; }
	void removeSyncException();

	const TCHAR *getAttribute(const TCHAR *name) { return m_attributes.get(name); }
	UINT32 getAttributeAsULong(const TCHAR *name);
	void setAttribute(const TCHAR *name, const TCHAR *value) { m_attributes.set(name, value); m_flags |= UF_MODIFIED; }
	void setName(const TCHAR *name);
	void setDescription(const TCHAR *description);

	void setDn(const TCHAR *dn);
	void setLdapId(const TCHAR *id);
   void detachLdapUser();
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
	User(DB_HANDLE hdb, DB_RESULT hResult, int row);
	User(UINT32 id, const TCHAR *name);
	virtual ~User();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(NXCPMessage *msg);
	virtual void modifyFromMessage(NXCPMessage *msg);

   virtual json_t *toJson() const;

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
	Group(DB_HANDLE hdb, DB_RESULT hResult, int row);
	Group(UINT32 id, const TCHAR *name);
	virtual ~Group();

	virtual void fillMessage(NXCPMessage *msg);
	virtual void modifyFromMessage(NXCPMessage *msg);

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual json_t *toJson() const;

	void addUser(UINT32 userId);
	void deleteUser(UINT32 userId);
	bool isMember(UINT32 userId, IntegerArray<UINT32> *searchPath = NULL);
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
   int m_size;
   int m_allocated;
   ACL_ELEMENT *m_elements;

public:
   AccessList();
   ~AccessList();

   bool getUserRights(UINT32 dwUserId, UINT32 *pdwAccessRights);
   void addElement(UINT32 dwUserId, UINT32 dwAccessRights);
   bool deleteElement(UINT32 dwUserId);
   void deleteAll();

   void enumerateElements(void (* pHandler)(UINT32, UINT32, void *), void *pArg);

   void fillMessage(NXCPMessage *pMsg);

   json_t *toJson();
};

/**
 * Functions
 */
BOOL LoadUsers();
void SaveUsers(DB_HANDLE hdb, UINT32 watchdogId);
void SendUserDBUpdate(int code, UINT32 id, UserDatabaseObject *object);
void SendUserDBUpdate(int code, UINT32 id);
UINT32 AuthenticateUser(const TCHAR *login, const TCHAR *password, size_t sigLen, void *pCert,
                        BYTE *pChallenge, UINT32 *pdwId, UINT64 *pdwSystemRights,
							   bool *pbChangePasswd, bool *pbIntruderLockout, bool *closeOtherSessions,
							   bool ssoAuth, UINT32 *graceLogins);
bool AuthenticateUserForXMPPCommands(const char *xmppId);
bool AuthenticateUserForXMPPSubscription(const char *xmppId);

UINT32 NXCORE_EXPORTABLE ValidateUserPassword(UINT32 userId, const TCHAR *login, const TCHAR *password, bool *isValid);
UINT32 NXCORE_EXPORTABLE SetUserPassword(UINT32 id, const TCHAR *newPassword, const TCHAR *oldPassword, bool changeOwnPassword);
bool CheckUserMembershipInternal(UINT32 userId, UINT32 groupId, IntegerArray<UINT32> *searchPath);
bool NXCORE_EXPORTABLE CheckUserMembership(UINT32 userId, UINT32 groupId);
UINT32 NXCORE_EXPORTABLE DeleteUserDatabaseObject(UINT32 id, bool alreadyLocked = false);
UINT32 NXCORE_EXPORTABLE CreateNewUser(const TCHAR *name, bool isGroup, UINT32 *id);
UINT32 NXCORE_EXPORTABLE ModifyUserDatabaseObject(NXCPMessage *msg, json_t **oldData, json_t **newData);
UINT32 NXCORE_EXPORTABLE DetachLdapUser(UINT32 id);
Iterator<UserDatabaseObject> NXCORE_EXPORTABLE *OpenUserDatabase();
void NXCORE_EXPORTABLE CloseUserDatabase(Iterator<UserDatabaseObject> *it);
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(UINT32 id, const TCHAR *name);
UINT32 NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(UINT32 id, const TCHAR *name);
void NXCORE_EXPORTABLE SetUserDbObjectAttr(UINT32 id, const TCHAR *name, const TCHAR *value);
bool NXCORE_EXPORTABLE ResolveUserId(UINT32 id, TCHAR *buffer, int bufSize);
void UpdateLDAPUser(const TCHAR* dn, Entry *obj);
void RemoveDeletedLDAPEntries(StringObjectMap<Entry> *entryListDn, StringObjectMap<Entry> *entryListId, UINT32 m_action, bool isUser);
void UpdateLDAPGroup(const TCHAR* dn, Entry *obj);
THREAD_RESULT THREAD_CALL SyncLDAPUsers(void *arg);
void FillGroupMembershipInfo(NXCPMessage *msg, UINT32 userId);
void UpdateGroupMembership(UINT32 userId, int numGroups, UINT32 *groups);
void DumpUsers(CONSOLE_CTX pCtx);

/**
 * CAS API
 */
void CASReadSettings();
bool CASAuthenticate(const char *ticket, TCHAR *loginName);

#endif
