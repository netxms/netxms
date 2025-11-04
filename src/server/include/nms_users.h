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
** File: nms_users.h
**
**/

#ifndef _nms_users_h_
#define _nms_users_h_

#include <nms_util.h>
#include <nxcldefs.h>

#if WITH_LDAP

#if !defined(__hpux)
#define LDAP_DEPRECATED 1
#endif

#if defined(_WIN32)

#include <winldap.h>
#include <winber.h>

#elif SUNOS_OPENLDAP

#include <openldap/ldap.h>

#else /* !SUNOS_OPENLDAP && !_WIN32 */

#include <ldap.h>
#if HAVE_LDAP_SSL_H
#include <ldap_ssl.h>
#endif

#if !HAVE_BER_INT_T
typedef int ber_int_t;
#endif

#endif /* SUNOS_OPENLDAP || _WIN32 */

#endif /* WITH_LDAP */

#define MAX_LDAP_ATTR_NAME_LEN

/**
 * LDAP object type
 */
enum class LDAP_ObjectType
{
   OTHER = 0,
   USER = 1,
   GROUP = 2
};

/**
 * LDAP entry (object)
 */
struct LDAP_Object
{
   LDAP_ObjectType m_type;
   TCHAR *m_id;
   TCHAR *m_loginName;
   TCHAR *m_fullName;
   TCHAR *m_description;
   TCHAR *m_email;
   TCHAR *m_phoneNumber;
   StringSet m_memberList;

   LDAP_Object();
   ~LDAP_Object();
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
   LDAP_CHAR m_connList[MAX_CONFIG_VALUE_LENGTH];
   LDAP_CHAR m_searchBase[MAX_CONFIG_VALUE_LENGTH];
   LDAP_CHAR m_searchFilter[MAX_CONFIG_VALUE_LENGTH];
   LDAP_CHAR m_userDN[MAX_CONFIG_VALUE_LENGTH];
   LDAP_CHAR m_userPassword[MAX_PASSWORD];
   char *m_ldapFullNameAttr;
   char *m_ldapUserLoginNameAttr;
   char *m_ldapGroupLoginNameAttr;
   char *m_ldapDescriptionAttr;
   char *m_ldapEmailAttr;
   char *m_ldapPhoneNumberAttr;
   char *m_ldapUserIdAttr;
   char *m_ldapGroupIdAttr;
   TCHAR *m_userClass;
   TCHAR *m_groupClass;
   int m_action;
   int m_secure;
   int m_pageSize;
   StringObjectMap<LDAP_Object> *m_userIdEntryList;
   StringObjectMap<LDAP_Object> *m_userDnEntryList;
   StringObjectMap<LDAP_Object> *m_groupIdEntryList;
   StringObjectMap<LDAP_Object> *m_groupDnEntryList;

   void open();
   void close();
   uint32_t login(bool isSync);
   void compareGroupList();
   void compareUserLists();
   wchar_t *getAttrValue(LDAPMessage *entry, const char *attr, int index = 0);
   wchar_t *getIdAttrValue(LDAPMessage *entry, const char *attr);
   void prepareStringForInit(LDAP_CHAR *connectionLine);
   int readInPages(LDAP_CHAR *base);
   void fillLists(LDAPMessage *searchResult);
   wchar_t *dnFromMessage(LDAPMessage *entry);
   void updateMembers(StringSet *memberList, const char *firstAttr, LDAPMessage *firstEntry, const LDAP_CHAR *dn);

   static String getErrorString(int code);
#endif // WITH_LDAP

public:
#if WITH_LDAP
   LDAPConnection();
   ~LDAPConnection();
#endif // WITH_LDAP

   void syncUsers();
   uint32_t ldapUserLogin(const wchar_t *name, const wchar_t *password);
};

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
enum class UserAuthenticationMethod
{
   LOCAL                 = 0,
   RADIUS                = 1,
   CERTIFICATE           = 2,
   CERTIFICATE_OR_LOCAL  = 3,
   CERTIFICATE_OR_RADIUS = 4,
   LDAP                  = 5
};

/**
 * Authentication method from integer
 */
static inline UserAuthenticationMethod UserAuthenticationMethodFromInt(int value)
{
   switch(value)
   {
      case 1:
         return UserAuthenticationMethod::RADIUS;
      case 2:
         return UserAuthenticationMethod::CERTIFICATE;
      case 3:
         return UserAuthenticationMethod::CERTIFICATE_OR_LOCAL;
      case 4:
         return UserAuthenticationMethod::CERTIFICATE_OR_RADIUS;
      case 5:
         return UserAuthenticationMethod::LDAP;
      default:
         return UserAuthenticationMethod::LOCAL;
   }
}

/**
 * Generic user database object
 */
class NXCORE_EXPORTABLE UserDatabaseObject
{
protected:
   uint32_t m_id;
   uuid m_guid;
   TCHAR m_name[MAX_USER_NAME];
   TCHAR m_description[MAX_USER_DESCR];
   uint64_t m_systemRights;
   TCHAR *m_uiAccessRules;
   uint32_t m_flags;
   StringMap m_attributes;		// Custom attributes
   TCHAR *m_ldapDn;
   TCHAR *m_ldapId;
   time_t m_created;

   bool loadCustomAttributes(DB_HANDLE hdb);
   bool saveCustomAttributes(DB_HANDLE hdb);

public:
   UserDatabaseObject();
   UserDatabaseObject(const UserDatabaseObject *src);
   UserDatabaseObject(DB_HANDLE hdb, DB_RESULT hResult, int row);
   UserDatabaseObject(uint32_t id, const TCHAR *name);
   virtual ~UserDatabaseObject();

   virtual bool saveToDatabase(DB_HANDLE hdb);
   virtual bool deleteFromDatabase(DB_HANDLE hdb);

   virtual void fillMessage(NXCPMessage *msg);
   virtual void modifyFromMessage(const NXCPMessage& msg);

   virtual json_t *toJson() const;

   uint32_t getId() const { return m_id; }
   const TCHAR *getName() const { return m_name; }
   const TCHAR *getDescription() const { return m_description; }
   uint64_t getSystemRights() const { return m_systemRights; }
   const TCHAR *getUIAccessRules() const { return m_uiAccessRules; }
   uint32_t getFlags() const { return m_flags; }
   uuid getGuid() const { return m_guid; }
   TCHAR *getGuidAsText(TCHAR *buffer) const { return m_guid.toString(buffer); }
   const TCHAR *getDN() const { return m_ldapDn; }
   const TCHAR *getLdapId() const { return m_ldapId; }

   bool isGroup() const { return (m_id & GROUP_FLAG) != 0; }
   bool isDeleted() const { return (m_flags & UF_DELETED) != 0; }
   bool isDisabled() const { return (m_flags & UF_DISABLED) != 0; }
   bool isModified() const { return (m_flags & UF_MODIFIED) != 0; }
   bool isLDAPUser() const { return (m_flags & UF_LDAP_USER) != 0; }

   void setModified() { m_flags |= UF_MODIFIED; }
   void setDeleted() { m_flags |= UF_DELETED; }
   void enable();
   void disable();
   void setFlags(uint32_t flags) { m_flags = flags; }
   void removeSyncException();

   const TCHAR *getAttribute(const TCHAR *name) const { return m_attributes.get(name); }
   uint32_t getAttributeAsUInt32(const TCHAR *name) const;
   void enumerateCustomAttributes(EnumerationCallbackResult (*callback)(const TCHAR*, const TCHAR*, void*), void *context) const
   {
      m_attributes.forEach(callback, context);
   }
   void setAttribute(const TCHAR *name, const TCHAR *value)
   {
      m_attributes.set(name, value);
      m_flags |= UF_MODIFIED;
   }

   void setName(const TCHAR *name);
   void setDescription(const TCHAR *description);

   void setDN(const TCHAR *dn);
   void setLdapId(const TCHAR *id);
   void detachLdapUser();

   NXSL_Value *getCustomAttributeForNXSL(NXSL_VM *vm, const TCHAR *name) const;
   NXSL_Value *getCustomAttributesForNXSL(NXSL_VM *vm) const { return vm->createValue(new NXSL_HashMap(vm, &m_attributes)); }
   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm);
};

/**
 * Hash types
 */
enum PasswordHashType
{
   PWD_HASH_SHA1 = 0,
   PWD_HASH_SHA256 = 1,
   PWD_HASH_DISABLED = 2
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

#ifdef _WIN32
template class NXCORE_TEMPLATE_EXPORTABLE ObjectMemoryPool<shared_ptr<Config>>;
template class NXCORE_TEMPLATE_EXPORTABLE StringObjectMap<shared_ptr<Config>>;
template class NXCORE_TEMPLATE_EXPORTABLE SharedStringObjectMap<Config>;
#endif

/**
 * User object
 */
class NXCORE_EXPORTABLE User : public UserDatabaseObject
{
protected:
   TCHAR m_fullName[MAX_USER_FULLNAME];
   PasswordHash m_password;
   int m_graceLogins;
   UserAuthenticationMethod m_authMethod;
   CertificateMappingMethod m_certMappingMethod;
   TCHAR *m_certMappingData;
   time_t m_disabledUntil;
   time_t m_lastPasswordChange;
   time_t m_lastLogin;
   time_t m_enableTime;
   int m_minPasswordLength;
   int m_authFailures;
   TCHAR *m_phoneNumber;
   TCHAR *m_email;
   SharedStringObjectMap<Config> m_2FABindings;

   void load2FABindings(DB_HANDLE hdb);

public:
	User();
	User(const User *src);
	User(DB_HANDLE hdb, DB_RESULT hResult, int row);
	User(uint32_t id, const TCHAR *name, UserAuthenticationMethod authMethod = UserAuthenticationMethod::LOCAL);
	virtual ~User();

	virtual bool saveToDatabase(DB_HANDLE hdb) override;
	virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

	virtual void fillMessage(NXCPMessage *msg) override;
	virtual void modifyFromMessage(const NXCPMessage& msg) override;

   virtual json_t *toJson() const override;

	const TCHAR *getFullName() const { return m_fullName; }
	int getGraceLogins() const { return m_graceLogins; }
	UserAuthenticationMethod getAuthMethod() const { return m_authMethod; }
	CertificateMappingMethod getCertMappingMethod() const { return m_certMappingMethod; }
	time_t getLastLoginTime() const { return m_lastLogin; }
   time_t getEnableTime() const { return m_enableTime; }
	time_t getPasswordChangeTime() const { return m_lastPasswordChange; }
	const TCHAR *getCertMappingData() const { return m_certMappingData; }
	bool isIntruderLockoutActive() const { return (m_flags & UF_INTRUDER_LOCKOUT) != 0; }
	bool canChangePassword()const  { return (m_flags & UF_CANNOT_CHANGE_PASSWORD) == 0; }
	int getMinMasswordLength() const { return m_minPasswordLength; }
	time_t getReEnableTime() const { return m_disabledUntil; }
   const TCHAR *getPhoneNumber() const { return CHECK_NULL_EX(m_phoneNumber); }
   const TCHAR *getEmail() const { return CHECK_NULL_EX(m_email); }

   bool validatePassword(const wchar_t *password);
   void decreaseGraceLogins() { if (m_graceLogins > 0) m_graceLogins--; m_flags |= UF_MODIFIED; }
   void setPassword(const wchar_t *password, bool clearChangePasswdFlag);
   void increaseAuthFailures();
   void resetAuthFailures() { m_authFailures = 0; m_flags |= UF_MODIFIED; }
   void updateLastLogin() { m_lastLogin = time(nullptr); m_flags |= UF_MODIFIED; }
   void updatePasswordChangeTime() { m_lastPasswordChange = time(nullptr); m_flags |= UF_MODIFIED; }
   void setFullName(const TCHAR *fullName);
   void setEmail(const TCHAR *email);
   void setPhoneNumber(const TCHAR *phoneNumber);
   void enable();

   unique_ptr<StringList> getConfigured2FAMethods() const;
   void fill2FAMethodBindingInfo(NXCPMessage *msg) const;
   shared_ptr<Config> get2FAMethodBinding(const TCHAR* methodName) const { return m_2FABindings.getShared(methodName); }
   bool has2FAMethodBinding(const TCHAR* methodName) const { return m_2FABindings.contains(methodName); }
   uint32_t modify2FAMethodBinding(const TCHAR* methodName, const StringMap& configuration);
   uint32_t delete2FAMethodBinding(const TCHAR* methodName);

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;
};

/**
 * Group search path, used by Group::isMember
 */
class GroupSearchPath
{
private:
   size_t m_size;
   uint32_t m_buffer[256];
   std::vector<uint32_t> *m_extendedBuffer;

public:
   GroupSearchPath()
   {
      m_size = 0;
      m_extendedBuffer = nullptr;
   }
   ~GroupSearchPath()
   {
      delete m_extendedBuffer;
   }

   /**
    * Add group ID to the search path
    */
   void add(uint32_t id)
   {
      if (m_size < sizeof(m_buffer) / sizeof(uint32_t))
      {
         m_buffer[m_size++] = id;
      }
      else
      {
         if (m_extendedBuffer == nullptr)
            m_extendedBuffer = new std::vector<uint32_t>(m_buffer, m_buffer + m_size);
         m_extendedBuffer->push_back(id);
         m_size++;
      }
   }

   /**
    * Clear search path
    */
   void clear()
   {
      m_size = 0;
      if (m_extendedBuffer != nullptr)
         m_extendedBuffer->clear();
   }

   /**
    * Check if given element is part of search path
    */
   bool contains(uint32_t id)
   {
      if (m_extendedBuffer != nullptr)
      {
         for(size_t i = 0; i < m_size; i++)
            if (m_extendedBuffer->at(i) == id)
               return true;
      }
      else
      {
         for(size_t i = 0; i < m_size; i++)
            if (m_buffer[i] == id)
               return true;
      }
      return false;
   }
};

/**
 * Group object
 */
class NXCORE_EXPORTABLE Group : public UserDatabaseObject
{
protected:
   IntegerArray<uint32_t> m_members;

public:
   Group();
   Group(const Group *src);
   Group(DB_HANDLE hdb, DB_RESULT hResult, int row);
   Group(uint32_t id, const TCHAR *name);
   virtual ~Group();

   virtual void fillMessage(NXCPMessage *msg) override;
   virtual void modifyFromMessage(const NXCPMessage& msg) override;

   virtual bool saveToDatabase(DB_HANDLE hdb) override;
   virtual bool deleteFromDatabase(DB_HANDLE hdb) override;

   virtual json_t *toJson() const override;

   void addUser(uint32_t userId);
   void deleteUser(uint32_t userId);
   bool isMember(uint32_t userId, GroupSearchPath *searchPath = nullptr) const;
   const IntegerArray<uint32_t>& getMembers() const { return m_members; }
   int getMemberCount() const { return m_members.size(); }

   virtual NXSL_Value *createNXSLObject(NXSL_VM *vm) override;
};

/**
 * Functions
 */
bool LoadUsers();
void SaveUsers(DB_HANDLE hdb, uint32_t watchdogId);
void SendUserDBUpdate(uint16_t code, uint32_t id, UserDatabaseObject *object);
void SendUserDBUpdate(uint16_t code, uint32_t id);
uint32_t NXCORE_EXPORTABLE AuthenticateUser(const TCHAR *login, const TCHAR *password, size_t sigLen, void *pCert,
         BYTE *pChallenge, uint32_t *pdwId, uint64_t *pdwSystemRights, bool *pbChangePasswd, bool *pbIntruderLockout,
         bool *closeOtherSessions, bool ssoAuth, uint32_t *graceLogins);

uint32_t NXCORE_EXPORTABLE ValidateUserPassword(uint32_t userId, const wchar_t *login, const wchar_t *password, bool *isValid);
uint32_t NXCORE_EXPORTABLE SetUserPassword(uint32_t id, const wchar_t *newPassword, const wchar_t *oldPassword, bool changeOwnPassword);
uint64_t NXCORE_EXPORTABLE GetEffectiveSystemRights(uint32_t userId);
String NXCORE_EXPORTABLE GetEffectiveUIAccessRules(uint32_t userId);
bool NXCORE_EXPORTABLE CheckUserMembership(uint32_t userId, uint32_t groupId);
uint32_t NXCORE_EXPORTABLE DeleteUserDatabaseObject(uint32_t id);
uint32_t NXCORE_EXPORTABLE CreateNewUser(const TCHAR *name, bool isGroup, uint32_t *id);
uint32_t NXCORE_EXPORTABLE ModifyUserDatabaseObject(const NXCPMessage& msg, json_t **oldData, json_t **newData);
void MarkUserDatabaseObjectAsModified(uint32_t id);
uint32_t NXCORE_EXPORTABLE DetachLDAPUser(uint32_t id);
Iterator<UserDatabaseObject> NXCORE_EXPORTABLE OpenUserDatabase();
void NXCORE_EXPORTABLE CloseUserDatabase();
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(uint32_t id, const TCHAR *name);
uint32_t NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(uint32_t id, const TCHAR *name);
void NXCORE_EXPORTABLE EnumerateUserDbObjectAttributes(uint32_t id, EnumerationCallbackResult (*callback)(const TCHAR*, const TCHAR*, void*), void *context);
void NXCORE_EXPORTABLE SetUserDbObjectAttr(uint32_t id, const TCHAR *name, const TCHAR *value);
uint32_t NXCORE_EXPORTABLE ResolveUserName(const TCHAR *loginName);
TCHAR NXCORE_EXPORTABLE *ResolveUserId(uint32_t id, TCHAR *buffer, bool noFail = false);
bool NXCORE_EXPORTABLE ValidateUserId(uint32_t id, TCHAR *loginName, uint64_t *systemAccess, uint32_t *rcc);
void UpdateLDAPUser(const TCHAR *dn, const LDAP_Object *ldapObject);
void RemoveDeletedLDAPEntries(StringObjectMap<LDAP_Object> *entryListDn, StringObjectMap<LDAP_Object> *entryListId, uint32_t m_action, bool isUser);
void UpdateLDAPGroup(const TCHAR* dn, const LDAP_Object *ldapObject);
void SyncLDAPGroupMembers(const TCHAR *dn, const LDAP_Object *ldapObject);
void FillGroupMembershipInfo(NXCPMessage *msg, uint32_t userId);
void UpdateGroupMembership(uint32_t userId, size_t numGroups, uint32_t *groups);
void DumpUsers(CONSOLE_CTX pCtx);
unique_ptr<ObjectArray<UserDatabaseObject>> NXCORE_EXPORTABLE FindUserDBObjects(const IntegerArray<uint32_t>& ids);
unique_ptr<ObjectArray<UserDatabaseObject>> NXCORE_EXPORTABLE FindUserDBObjects(const StructArray<ResponsibleUser>& ids);
NXSL_Value *GetUserDBObjectForNXSL(uint32_t id, NXSL_VM *vm);

shared_ptr<AuthenticationTokenDescriptor> NXCORE_EXPORTABLE IssueAuthenticationToken(uint32_t userId, uint32_t validFor,
   AuthenticationTokenType type = AuthenticationTokenType::EPHEMERAL, const wchar_t *description = nullptr);
void NXCORE_EXPORTABLE RevokeAuthenticationToken(const UserAuthenticationToken& token);
uint32_t NXCORE_EXPORTABLE RevokeAuthenticationToken(uint32_t tokenId, uint32_t userId = 0);
bool NXCORE_EXPORTABLE ValidateAuthenticationToken(const UserAuthenticationToken& token, uint32_t *userId, bool *serviceToken = nullptr, uint32_t validFor = 0);
void AuthenticationTokensToMessage(uint32_t userId, NXCPMessage *msg);

unique_ptr<StringList> NXCORE_EXPORTABLE GetUserConfigured2FAMethods(uint32_t userId);
shared_ptr<Config> GetUser2FAMethodBinding(int userId, const TCHAR *method);
void FillUser2FAMethodBindingInfo(uint32_t userId, NXCPMessage *msg);
uint32_t ModifyUser2FAMethodBinding(uint32_t userId, const TCHAR* methodName, const StringMap& configuration);
uint32_t DeleteUser2FAMethodBinding(uint32_t userId, const TCHAR* methodName);

/**
 * Template wrapper for EnumerateUserDbObjectAttributes
 */
template<typename C> static inline void EnumerateUserDbObjectAttributes(uint32_t id, EnumerationCallbackResult (*callback)(const TCHAR*, const TCHAR*, C*), C *context)
{
   EnumerateUserDbObjectAttributes(id, reinterpret_cast<EnumerationCallbackResult (*)(const TCHAR*, const TCHAR*, void*)>(callback), context);
}

/**
 * CAS API
 */
void CASReadSettings();
bool CASAuthenticate(const char *ticket, wchar_t *loginName);

#endif   /* _nms_users_h_ */
