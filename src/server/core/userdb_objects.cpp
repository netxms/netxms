/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: userdb_objects.cpp
**/

#include "nxcore.h"
#include <nxcore_2fa.h>
#include <nms_users.h>

#define DEBUG_TAG _T("userdb")

bool CheckUserMembershipInternal(uint32_t userId, uint32_t groupId, GroupSearchPath *searchPath);

/**
 * Compare user IDs (for qsort)
 */
static int CompareUserId(const void *e1, const void *e2)
{
   uint32_t id1 = *static_cast<const uint32_t*>(e1);
   uint32_t id2 = *static_cast<const uint32_t*>(e2);
   return (id1 < id2) ? -1 : ((id1 > id2) ? 1 : 0);
}

/**
 * Generate hash for password
 */
static void CalculatePasswordHash(const wchar_t *password, PasswordHashType type, PasswordHash *ph, const BYTE *salt = nullptr)
{
	char mbPassword[1024];
	wchar_to_utf8(password, -1, mbPassword, 1024);
	mbPassword[1023] = 0;

   BYTE buffer[1024];

   memset(ph, 0, sizeof(PasswordHash));
   ph->hashType = type;
   switch(type)
   {
      case PWD_HASH_SHA1:
      	CalculateSHA1Hash(mbPassword, strlen(mbPassword), ph->hash);
         break;
      case PWD_HASH_SHA256:
         if (salt != nullptr)
            memcpy(buffer, salt, PASSWORD_SALT_LENGTH);
         else
            GenerateRandomBytes(buffer, PASSWORD_SALT_LENGTH);
         strcpy(reinterpret_cast<char*>(&buffer[PASSWORD_SALT_LENGTH]), mbPassword);
         CalculateSHA256Hash(buffer, strlen(mbPassword) + PASSWORD_SALT_LENGTH, ph->hash);
         memcpy(ph->salt, buffer, PASSWORD_SALT_LENGTH);
         break;
      default:
         break;
   }
}

/*****************************************************************************
 **  UserDatabaseObject
 ****************************************************************************/

/**
 * Constructor for generic user database object - create from database
 * Expects fields in the following order:
 *    id,name,system_access,flags,description,guid,ldap_dn,ui_access_rules
 */
UserDatabaseObject::UserDatabaseObject(DB_HANDLE hdb, DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	DBGetField(hResult, row, 1, m_name, MAX_USER_NAME);
	m_systemRights = DBGetFieldUInt64(hResult, row, 2);
	m_flags = DBGetFieldULong(hResult, row, 3);
	DBGetField(hResult, row, 4, m_description, MAX_USER_DESCR);
	m_guid = DBGetFieldGUID(hResult, row, 5);
	m_ldapDn = DBGetField(hResult, row, 6, nullptr, 0);
	m_ldapId = DBGetField(hResult, row, 7, nullptr, 0);
	m_created = static_cast<time_t>(DBGetFieldInt64(hResult, row, 8));
	m_uiAccessRules = DBGetField(hResult, row, 9, nullptr, 0);
}

/**
 * Default constructor for generic object
 */
UserDatabaseObject::UserDatabaseObject()
{
   m_id = 0;
   m_guid = uuid::generate();
   m_name[0] = 0;
   m_ldapDn = nullptr;
   m_ldapId = nullptr;
	m_systemRights = 0;
   m_uiAccessRules = nullptr;
	m_description[0] = 0;
	m_flags = 0;
	m_created = time(nullptr);
}

/**
 * Constructor for generic object - create new object with given id and name
 */
UserDatabaseObject::UserDatabaseObject(uint32_t id, const TCHAR *name)
{
	m_id = id;
   m_guid = uuid::generate();
	_tcslcpy(m_name, name, MAX_USER_NAME);
	m_systemRights = 0;
	m_uiAccessRules = nullptr;
	m_description[0] = 0;
	m_flags = UF_MODIFIED;
	m_ldapDn = nullptr;
	m_ldapId = nullptr;
   m_created = time(nullptr);
}

/**
 * Copy constructor for generic object
 */
UserDatabaseObject::UserDatabaseObject(const UserDatabaseObject *src)
{
   m_id = src->m_id;
   m_guid = src->m_guid;
   _tcslcpy(m_name, src->m_name, MAX_USER_NAME);
   m_systemRights = src->m_systemRights;
   m_uiAccessRules = MemCopyString(src->m_uiAccessRules);
   _tcslcpy(m_description, src->m_description, MAX_USER_DESCR);
   m_flags = src->m_flags;
   m_attributes.addAll(&src->m_attributes);
   m_ldapDn = MemCopyString(src->m_ldapDn);
   m_ldapId = MemCopyString(src->m_ldapId);
   m_created = src->m_created;
}

/**
 * Destructor for generic user database object
 */
UserDatabaseObject::~UserDatabaseObject()
{
   MemFree(m_uiAccessRules);
   MemFree(m_ldapDn);
   MemFree(m_ldapId);
}

/**
 * Save object to database
 */
bool UserDatabaseObject::saveToDatabase(DB_HANDLE hdb)
{
	return false;
}

/**
 * Delete object from database
 */
bool UserDatabaseObject::deleteFromDatabase(DB_HANDLE hdb)
{
	return false;
}

/**
 * Fill NXCP message with object data
 */
void UserDatabaseObject::fillMessage(NXCPMessage *msg)
{
   msg->setField(VID_USER_ID, m_id);
   msg->setField(VID_USER_NAME, m_name);
   msg->setField(VID_USER_FLAGS, static_cast<uint16_t>(m_flags));
   msg->setField(VID_USER_SYS_RIGHTS, m_systemRights);
   msg->setField(VID_UI_ACCESS_RULES, m_uiAccessRules);
   msg->setField(VID_USER_DESCRIPTION, m_description);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_LDAP_DN, m_ldapDn);
   msg->setField(VID_LDAP_ID, m_ldapId);
   msg->setFieldFromTime(VID_CREATION_TIME, m_created);
   m_attributes.fillMessage(msg, VID_CUSTOM_ATTRIBUTES_BASE, VID_NUM_CUSTOM_ATTRIBUTES);
}

/**
 * Modify object from NXCP message
 */
void UserDatabaseObject::modifyFromMessage(const NXCPMessage& msg)
{
	uint32_t fields = msg.getFieldAsUInt32(VID_FIELDS);
	nxlog_debug_tag(DEBUG_TAG, 5, _T("UserDatabaseObject::modifyFromMessage(): id=%d fields=%08X"), m_id, fields);

	if (fields & USER_MODIFY_DESCRIPTION)
	   msg.getFieldAsString(VID_USER_DESCRIPTION, m_description, MAX_USER_DESCR);
	if (fields & USER_MODIFY_LOGIN_NAME)
	   msg.getFieldAsString(VID_USER_NAME, m_name, MAX_USER_NAME);

	// Update custom attributes only if VID_NUM_CUSTOM_ATTRIBUTES exist -
	// older client versions may not be aware of custom attributes
	if ((fields & USER_MODIFY_CUSTOM_ATTRIBUTES) || msg.isFieldExist(VID_NUM_CUSTOM_ATTRIBUTES))
	{
		uint32_t count = msg.getFieldAsUInt32(VID_NUM_CUSTOM_ATTRIBUTES);
		m_attributes.clear();
		for(uint32_t i = 0, fieldId = VID_CUSTOM_ATTRIBUTES_BASE; i < count; i++)
		{
			TCHAR *name = msg.getFieldAsString(fieldId++);
			TCHAR *value = msg.getFieldAsString(fieldId++);
			m_attributes.setPreallocated((name != nullptr) ? name : MemCopyString(_T("")), (value != nullptr) ? value : MemCopyString(_T("")));
		}
	}

	if ((m_id != 0) && (fields & USER_MODIFY_ACCESS_RIGHTS))
		m_systemRights = msg.getFieldAsUInt64(VID_USER_SYS_RIGHTS);

   if (fields & USER_MODIFY_UI_ACCESS_RULES)
   {
      MemFree(m_uiAccessRules);
      m_uiAccessRules = msg.getFieldAsString(VID_UI_ACCESS_RULES);
   }

	if (fields & USER_MODIFY_FLAGS)
	{
	   uint32_t flags = msg.getFieldAsUInt16(VID_USER_FLAGS);
		// Modify only UF_DISABLED, UF_CHANGE_PASSWORD, UF_CANNOT_CHANGE_PASSWORD and UF_CLOSE_OTHER_SESSIONS flags from message
		// Ignore all but CHANGE_PASSWORD flag for superuser and "everyone" group
		m_flags &= ~(UF_DISABLED | UF_CHANGE_PASSWORD | UF_CANNOT_CHANGE_PASSWORD | UF_CLOSE_OTHER_SESSIONS);
		if (m_id == 0)
			m_flags |= flags & (UF_DISABLED | UF_CHANGE_PASSWORD);
		else if (m_id == GROUP_EVERYONE)
         m_flags |= flags & UF_CHANGE_PASSWORD;
		else
			m_flags |= flags & (UF_DISABLED | UF_CHANGE_PASSWORD | UF_CANNOT_CHANGE_PASSWORD | UF_CLOSE_OTHER_SESSIONS);
	}

	m_flags |= UF_MODIFIED;
}

/**
 * Detach user from LDAP user
 */
void UserDatabaseObject::detachLdapUser()
{
   m_flags &= ~UF_LDAP_USER;
   setDN(nullptr);
	m_flags |= UF_MODIFIED;
}

/**
 * Load custom attributes from database
 */
bool UserDatabaseObject::loadCustomAttributes(DB_HANDLE hdb)
{
	bool success = false;
	DB_RESULT hResult = ExecuteSelectOnObject(hdb, m_id, _T("SELECT attr_name,attr_value FROM userdb_custom_attributes WHERE object_id={id}"));
	if (hResult != nullptr)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			TCHAR *attrName = DBGetField(hResult, i, 0, nullptr, 0);
			if (attrName == nullptr)
				attrName = MemCopyString(_T(""));

			TCHAR *attrValue = DBGetField(hResult, i, 1, nullptr, 0);
			if (attrValue == nullptr)
				attrValue = MemCopyString(_T(""));

			m_attributes.setPreallocated(attrName, attrValue);
		}
		DBFreeResult(hResult);
		success = true;
	}
	return success;
}

/**
 * Callback for saving custom attribute in database
 */
static EnumerationCallbackResult SaveAttributeCallback(const TCHAR *key, const void *value, void *data)
{
   DB_STATEMENT hStmt = (DB_STATEMENT)data;
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, (const TCHAR *)value, DB_BIND_STATIC);
   return DBExecute(hStmt) ? _CONTINUE : _STOP;
}

/**
 * Save custom attributes to database
 */
bool UserDatabaseObject::saveCustomAttributes(DB_HANDLE hdb)
{
	bool success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM userdb_custom_attributes WHERE object_id=?"));
	if (success && !m_attributes.isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO userdb_custom_attributes (object_id,attr_name,attr_value) VALUES (?,?,?)"), m_attributes.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = (m_attributes.forEach(SaveAttributeCallback, hStmt) == _CONTINUE);
         DBFreeStatement(hStmt);
      }
   }
	return success;
}

/**
 * Get custom attribute as 32 bit unsigned integer
 */
uint32_t UserDatabaseObject::getAttributeAsUInt32(const TCHAR *name) const
{
	const TCHAR *value = getAttribute(name);
	return (value != nullptr) ? _tcstoul(value, nullptr, 0) : 0;
}

/**
 * Set description
 */
void UserDatabaseObject::setDescription(const TCHAR *description)
{
   if (_tcscmp(m_description, CHECK_NULL_EX(description)))
   {
      _tcslcpy(m_description, CHECK_NULL_EX(description), MAX_USER_DESCR);
      m_flags |= UF_MODIFIED;
   }
}

/**
 * Set name
 */
void UserDatabaseObject::setName(const TCHAR *name)
{
   if (_tcscmp(m_name, CHECK_NULL_EX(name)))
   {
      _tcslcpy(m_name, CHECK_NULL_EX(name), MAX_USER_NAME);
      m_flags |= UF_MODIFIED;
   }
}

/**
 * Set object's DN
 */
void UserDatabaseObject::setDN(const TCHAR *dn)
{
   if ((dn == nullptr) || ((m_ldapDn != nullptr) && !_tcscmp(m_ldapDn, dn)))
      return;
   MemFree(m_ldapDn);
   m_ldapDn = MemCopyString(dn);
   m_flags |= UF_MODIFIED;
}

/**
 * Set object's LDAP ID
 */
void UserDatabaseObject::setLdapId(const TCHAR *id)
{
   if ((m_ldapId != nullptr) && !_tcscmp(m_ldapId, id))
      return;
   MemFree(m_ldapId);
   m_ldapId = MemCopyString(id);
   m_flags |= UF_MODIFIED;
}

/**
 * Remove LDAP synchronization exception
 */
void UserDatabaseObject::removeSyncException()
{
   if ((m_flags & UF_SYNC_EXCEPTION)> 0)
   {
      m_flags &= ~UF_SYNC_EXCEPTION;
      m_flags |= UF_MODIFIED;
   }
}

/**
 * Enable user account
 */
void UserDatabaseObject::enable()
{
	m_flags &= ~(UF_DISABLED);
	m_flags |= UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Disable user account
 */
void UserDatabaseObject::disable()
{
   m_flags |= UF_DISABLED | UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Serialize object to JSON
 */
json_t *UserDatabaseObject::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "systemRights", json_integer(m_systemRights));
   json_object_set_new(root, "uiAccessRules", json_string_t(m_uiAccessRules));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "attributes", m_attributes.toJson());
   json_object_set_new(root, "ldapDn", json_string_t(m_ldapDn));
   json_object_set_new(root, "ldapId", json_string_t(m_ldapId));
   json_object_set_new(root, "created", json_integer(m_created));
   return root;
}

/**
 * Get custom attribute as NXSL value
 */
NXSL_Value *UserDatabaseObject::getCustomAttributeForNXSL(NXSL_VM *vm, const TCHAR *name) const
{
   NXSL_Value *value = nullptr;
   const TCHAR *av = m_attributes.get(name);
   if (av != nullptr)
      value = vm->createValue(av);
   return value;
}

/**
 * Create NXSL object
 */
NXSL_Value *UserDatabaseObject::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslUserDBObjectClass, this));
}

/*****************************************************************************
 **  User
 ****************************************************************************/

/**
 * Constructor for user object - create from database
 * Expects fields in the following order:
 *    id,name,system_access,flags,description,guid,ldap_dn,ui_access_rules,password,full_name,
 *    grace_logins,auth_method,cert_mapping_method,cert_mapping_data,
 *    auth_failures,last_passwd_change,min_passwd_length,disabled_until,
 *    last_login,email,phone_number
 */
User::User(DB_HANDLE hdb, DB_RESULT hResult, int row) : UserDatabaseObject(hdb, hResult, row)
{
   bool validHash = false;
   bool noPassword = false;
   TCHAR buffer[256];
   DBGetField(hResult, row, 10, buffer, 256);
   if (buffer[0] == _T('$'))
   {
      // new format - with hash type indicator
      if (buffer[1] == 'A')
      {
         m_password.hashType = PWD_HASH_SHA256;
         if (_tcslen(buffer) >= 82)
         {
            if ((StrToBin(&buffer[2], m_password.salt, PASSWORD_SALT_LENGTH) == PASSWORD_SALT_LENGTH) &&
                (StrToBin(&buffer[18], m_password.hash, SHA256_DIGEST_SIZE) == SHA256_DIGEST_SIZE))
               validHash = true;
         }
      }
      else if (buffer[1] == '$')
      {
         noPassword = true;
         m_password.hashType = PWD_HASH_DISABLED;
      }
   }
   else
   {
      // old format - SHA1 hash without salt
      m_password.hashType = PWD_HASH_SHA1;
      if (StrToBin(buffer, m_password.hash, SHA1_DIGEST_SIZE) == SHA1_DIGEST_SIZE)
         validHash = true;
   }
   if (!validHash && !noPassword)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid password hash for user \"%s\"; password reset to default"), m_name);
      CalculatePasswordHash(_T("netxms"), PWD_HASH_SHA256, &m_password);
      m_flags |= UF_MODIFIED | UF_CHANGE_PASSWORD;
   }

   DBGetField(hResult, row, 11, m_fullName, MAX_USER_FULLNAME);
   m_graceLogins = DBGetFieldLong(hResult, row, 12);
   m_authMethod = UserAuthenticationMethodFromInt(DBGetFieldLong(hResult, row, 13));
   m_certMappingMethod = static_cast<CertificateMappingMethod>(DBGetFieldLong(hResult, row, 14));
   m_certMappingData = DBGetField(hResult, row, 15, nullptr, 0);
   m_authFailures = DBGetFieldLong(hResult, row, 16);
   m_lastPasswordChange = (time_t)DBGetFieldLong(hResult, row, 17);
   m_minPasswordLength = DBGetFieldLong(hResult, row, 18);
   m_disabledUntil = (time_t)DBGetFieldLong(hResult, row, 19);
   m_lastLogin = (time_t)DBGetFieldLong(hResult, row, 20);
   m_email = DBGetField(hResult, row, 21, nullptr, 0);
   m_phoneNumber = DBGetField(hResult, row, 22, nullptr, 0);

   m_enableTime = 0;

   // Set full system access for superuser
   if (m_id == 0)
      m_systemRights = SYSTEM_ACCESS_FULL;

   load2FABindings(hdb);
   loadCustomAttributes(hdb);
}

/**
 * Constructor for user object - create default system user
 */
User::User() : UserDatabaseObject()
{
	m_id = 0;
	_tcscpy(m_name, _T("system"));
	m_flags = UF_MODIFIED | UF_CHANGE_PASSWORD;
	m_systemRights = SYSTEM_ACCESS_FULL;
	m_fullName[0] = 0;
	_tcscpy(m_description, _T("Built-in system account"));
	CalculatePasswordHash(_T("netxms"), PWD_HASH_SHA256, &m_password);
	m_graceLogins = ConfigReadInt(_T("Server.Security.GraceLoginCount"), 5);
	m_authMethod = UserAuthenticationMethod::LOCAL;
	m_certMappingMethod = MAP_CERTIFICATE_BY_CN;
	m_certMappingData = nullptr;
	m_authFailures = 0;
	m_lastPasswordChange = 0;
	m_minPasswordLength = -1;	// Use system-wide default
	m_disabledUntil = 0;
	m_lastLogin = 0;
	m_enableTime = 0;
   m_email = nullptr;
   m_phoneNumber = nullptr;
}

/**
 * Constructor for user object - new user
 */
User::User(uint32_t id, const TCHAR *name, UserAuthenticationMethod authMethod) : UserDatabaseObject(id, name)
{
	m_fullName[0] = 0;
	m_graceLogins = ConfigReadInt(_T("Server.Security.GraceLoginCount"), 5);
	m_authMethod = authMethod;
	m_certMappingMethod = MAP_CERTIFICATE_BY_CN;
	m_certMappingData = nullptr;
	CalculatePasswordHash(_T(""), PWD_HASH_SHA256, &m_password);
	m_authFailures = 0;
	m_lastPasswordChange = 0;
	m_minPasswordLength = -1;	// Use system-wide default
	m_disabledUntil = 0;
	m_lastLogin = 0;
   m_enableTime = 0;
   m_email = nullptr;
   m_phoneNumber = nullptr;
}

/**
 * Copy callback for user 2FA configs
 */
static EnumerationCallbackResult User2FAMethodsCopyCallback(const TCHAR* name, const shared_ptr<Config>* value, SharedStringObjectMap<Config>* config)
{
   config->set(name, *value);
   return _CONTINUE;
}

/**
 * Copy constructor for user object
 */
User::User(const User *src) : UserDatabaseObject(src)
{
   m_password.hashType = src->m_password.hashType;
   memcpy(m_password.hash, src->m_password.hash, SHA256_DIGEST_SIZE);
   memcpy(m_password.salt, src->m_password.salt, PASSWORD_SALT_LENGTH);

   _tcslcpy(m_fullName, src->m_fullName, MAX_USER_FULLNAME);
   m_graceLogins = src->m_graceLogins;
   m_authMethod = src->m_authMethod;
   m_certMappingMethod = src->m_certMappingMethod;
   m_certMappingData = MemCopyString(src->m_certMappingData);
   m_disabledUntil = src->m_disabledUntil;
   m_lastPasswordChange = src->m_lastPasswordChange;
   m_lastLogin = src->m_lastLogin;
   m_enableTime = src->m_enableTime;
   m_minPasswordLength = src->m_minPasswordLength;
   m_authFailures = src->m_authFailures;
   m_email = MemCopyString(src->m_email);
   m_phoneNumber = MemCopyString(src->m_phoneNumber);

   src->m_2FABindings.forEach(User2FAMethodsCopyCallback, &m_2FABindings);
}

/**
 * Destructor for user object
 */
User::~User()
{
   MemFree(m_certMappingData);
   MemFree(m_email);
   MemFree(m_phoneNumber);
}

/**
 * Save object to database
 */
bool User::saveToDatabase(DB_HANDLE hdb)
{
   // Clear modification flag
   m_flags &= ~UF_MODIFIED;

   // Create or update record in database
   TCHAR password[128];
   switch(m_password.hashType)
   {
      case PWD_HASH_SHA1:
         BinToStr(m_password.hash, SHA1_DIGEST_SIZE, password);
         break;
      case PWD_HASH_SHA256:
         _tcscpy(password, _T("$A"));
         BinToStr(m_password.salt, PASSWORD_SALT_LENGTH, &password[2]);
         BinToStr(m_password.hash, SHA256_DIGEST_SIZE, &password[18]);
         break;
      default:
         _tcscpy(password, _T("$$"));
         break;
   }

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("users"), _T("id"), m_id))
   {
      hStmt = DBPrepare(hdb,
         _T("UPDATE users SET name=?,password=?,system_access=?,flags=?,full_name=?,description=?,grace_logins=?,guid=?,")
			_T("  auth_method=?,cert_mapping_method=?,cert_mapping_data=?,auth_failures=?,last_passwd_change=?,")
         _T("  min_passwd_length=?,disabled_until=?,last_login=?,ldap_dn=?,ldap_unique_id=?,created=?,")
         _T("  email=?,phone_number=?,ui_access_rules=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb,
         _T("INSERT INTO users (name,password,system_access,flags,full_name,description,grace_logins,guid,auth_method,")
         _T("  cert_mapping_method,cert_mapping_data,password_history,auth_failures,last_passwd_change,min_passwd_length,")
         _T("  disabled_until,last_login,ldap_dn,ldap_unique_id,created,email,phone_number,ui_access_rules,id) ")
         _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,?,?,?,?,?,?,?)"));
   }
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, password, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, m_systemRights);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_fullName, DB_BIND_STATIC);
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_graceLogins);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_authMethod));
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_certMappingMethod);
   DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_certMappingData, DB_BIND_STATIC);
   DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_authFailures);
   DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastPasswordChange));
   DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_minPasswordLength);
   DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_disabledUntil));
   DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastLogin));
   DBBind(hStmt, 17, DB_SQLTYPE_TEXT, m_ldapDn, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_VARCHAR, m_ldapId, DB_BIND_STATIC);
   DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_created));
   DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_email, DB_BIND_STATIC);
   DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, m_phoneNumber, DB_BIND_STATIC);
   DBBind(hStmt, 22, DB_SQLTYPE_VARCHAR, m_uiAccessRules, DB_BIND_STATIC);
   DBBind(hStmt, 23, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBBegin(hdb);
   if (success)
      success = DBExecute(hStmt);

   if (success)
      success = saveCustomAttributes(hdb);

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM two_factor_auth_bindings WHERE user_id=?"));

   if (success && !m_2FABindings.isEmpty())
   {
      DBFreeStatement(hStmt);
      hStmt = DBPrepare(hdb, _T("INSERT INTO two_factor_auth_bindings (user_id,name,configuration) VALUES (?,?,?)"), m_2FABindings.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for (const KeyValuePair<shared_ptr<Config>>* binding : m_2FABindings)
         {
            String xml = binding->value->get()->createXml();
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, binding->key, DB_BIND_STATIC);
            DBBind(hStmt, 3, DB_SQLTYPE_TEXT, xml, DB_BIND_STATIC);
            success = DBExecute(hStmt);
            if (!success)
               break;
         }
      }
      else
      {
         success = false;
      }
   }

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);

   DBFreeStatement(hStmt);
   return success;
}

/**
 * Delete object from database
 */
bool User::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = DBBegin(hdb);
	if (success)
	   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM users WHERE id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_profiles WHERE user_id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM userdb_custom_attributes WHERE object_id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM two_factor_auth_bindings WHERE user_id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_access WHERE user_id=?"));

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
	return success;
}

/**
 * Validate user's password
 */
bool User::validatePassword(const wchar_t *password)
{
   if (m_password.hashType == PWD_HASH_DISABLED)
      return false;

   PasswordHash ph;
   CalculatePasswordHash(password, m_password.hashType, &ph, m_password.salt);
   bool success = !memcmp(ph.hash, m_password.hash, PWD_HASH_SIZE(m_password.hashType));
   if (success && m_password.hashType == PWD_HASH_SHA1)
   {
      // regenerate password hash if old format is used
      CalculatePasswordHash(password, PWD_HASH_SHA256, &m_password);
         m_flags |= UF_MODIFIED;
   }
   return success;
}

/**
 * Set user's password
 */
void User::setPassword(const wchar_t *password, bool clearChangePasswdFlag)
{
   CalculatePasswordHash(password, PWD_HASH_SHA256, &m_password);
   m_graceLogins = ConfigReadInt(_T("Server.Security.GraceLoginCount"), 5);;
   m_flags |= UF_MODIFIED;
   if (clearChangePasswdFlag)
      m_flags &= ~UF_CHANGE_PASSWORD;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Fill NXCP message with user data
 */
void User::fillMessage(NXCPMessage *msg)
{
   UserDatabaseObject::fillMessage(msg);

   msg->setField(VID_USER_FULL_NAME, m_fullName);
   msg->setField(VID_AUTH_METHOD, static_cast<uint16_t>(m_authMethod));
	msg->setField(VID_CERT_MAPPING_METHOD, static_cast<uint16_t>(m_certMappingMethod));
	msg->setField(VID_CERT_MAPPING_DATA, CHECK_NULL_EX(m_certMappingData));
	msg->setFieldFromTime(VID_LAST_LOGIN, m_lastLogin);
	msg->setFieldFromTime(VID_LAST_PASSWORD_CHANGE, m_lastPasswordChange);
	msg->setField(VID_MIN_PASSWORD_LENGTH, static_cast<uint16_t>(m_minPasswordLength));
	msg->setFieldFromTime(VID_DISABLED_UNTIL, m_disabledUntil);
	msg->setField(VID_AUTH_FAILURES, m_authFailures);
   msg->setField(VID_EMAIL, CHECK_NULL_EX(m_email));
   msg->setField(VID_PHONE_NUMBER, CHECK_NULL_EX(m_phoneNumber));

   FillGroupMembershipInfo(msg, m_id);

   fill2FAMethodBindingInfo(msg);
}

/**
 * Modify user object from NXCP message
 */
void User::modifyFromMessage(const NXCPMessage& msg)
{
   uint32_t fields = msg.getFieldAsUInt32(VID_FIELDS);
   if (fields & USER_MODIFY_FLAGS)
   {
      uint32_t flags = msg.getFieldAsUInt16(VID_USER_FLAGS);
      if (((m_flags & UF_DISABLED) != 0) && ((flags & UF_DISABLED) == 0))
      {
         // user is being enabled, update enable time so it will not be disabled again immediately by inactivity timer
         m_enableTime = time(nullptr);
      }
   }

   UserDatabaseObject::modifyFromMessage(msg);

	if (fields & USER_MODIFY_FULL_NAME)
		msg.getFieldAsString(VID_USER_FULL_NAME, m_fullName, MAX_USER_FULLNAME);
	if (fields & USER_MODIFY_AUTH_METHOD)
	   m_authMethod = UserAuthenticationMethodFromInt(msg.getFieldAsUInt16(VID_AUTH_METHOD));
	if (fields & USER_MODIFY_PASSWD_LENGTH)
	   m_minPasswordLength = msg.getFieldAsUInt16(VID_MIN_PASSWORD_LENGTH);
	if (fields & USER_MODIFY_TEMP_DISABLE)
	   m_disabledUntil = (time_t)msg.getFieldAsUInt32(VID_DISABLED_UNTIL);
	if (fields & USER_MODIFY_CERT_MAPPING)
	{
		m_certMappingMethod = static_cast<CertificateMappingMethod>(msg.getFieldAsUInt16(VID_CERT_MAPPING_METHOD));
		MemFree(m_certMappingData);
		m_certMappingData = msg.getFieldAsString(VID_CERT_MAPPING_DATA);
	}
   if (fields & USER_MODIFY_EMAIL)
      msg.getFieldAsString(VID_EMAIL, &m_email);
   if (fields & USER_MODIFY_PHONE_NUMBER)
      msg.getFieldAsString(VID_PHONE_NUMBER, &m_phoneNumber);

   if (fields & USER_MODIFY_GROUP_MEMBERSHIP)
   {
      size_t count = msg.getFieldAsUInt32(VID_NUM_GROUPS);
      uint32_t *groups = nullptr;
      if (count > 0)
      {
         groups = MemAllocArrayNoInit<uint32_t>(count);
         msg.getFieldAsInt32Array(VID_GROUPS, count, groups);
      }
      UpdateGroupMembership(m_id, count, groups);
      MemFree(groups);
   }

   if (fields & USER_MODIFY_2FA_BINDINGS)
   {
      StringSet currentMethods;
      size_t count = msg.getFieldAsUInt32(VID_2FA_METHOD_COUNT);
      uint32_t fieldId = VID_2FA_METHOD_LIST_BASE;
      for(size_t i = 0; i < count; i++)
      {
         TCHAR methodName[MAX_OBJECT_NAME];
         msg.getFieldAsString(fieldId, methodName, MAX_OBJECT_NAME);
         currentMethods.add(methodName);

         StringMap configuration(msg, fieldId + 10, fieldId + 9);
         bool newBinding = false;
         shared_ptr<Config> binding = m_2FABindings.getShared(methodName);
         if (binding == nullptr)
         {
            binding = make_shared<Config>();
            newBinding = true;
         }
         if (Update2FAMethodBindingConfiguration(methodName, binding.get(), configuration))
         {
            if (newBinding)
               m_2FABindings.set(methodName, binding);
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Attempt to configure binding for non-existing 2FA method %s for user %s [%u]"), methodName, m_name, m_id);
         }
         fieldId += 1000;
      }

      // Delete existing methods that are no longer configured
      m_2FABindings.filterElements([](const TCHAR *name, const void *binding, void *context) { return static_cast<StringSet*>(context)->contains(name); }, &currentMethods);
   }

   // Clear intruder lockout flag if user is not disabled anymore
   if (!(m_flags & UF_DISABLED))
      m_flags &= ~UF_INTRUDER_LOCKOUT;
}

/**
 * Increase auth failures and lockout account if threshold reached
 */
void User::increaseAuthFailures()
{
	m_authFailures++;

	int lockoutThreshold = ConfigReadInt(_T("Server.Security.IntruderLockoutThreshold"), 0);
	if ((lockoutThreshold > 0) && (m_authFailures >= lockoutThreshold))
	{
		m_disabledUntil = time(NULL) + ConfigReadInt(_T("Server.Security.IntruderLockoutTime"), 30) * 60;
		m_flags |= UF_DISABLED | UF_INTRUDER_LOCKOUT;
	}

	m_flags |= UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Enable user account
 */
void User::enable()
{
	m_authFailures = 0;
	m_graceLogins = ConfigReadInt(_T("Server.Security.GraceLoginCount"), 5);
	m_disabledUntil = 0;
	m_flags &= ~(UF_DISABLED | UF_INTRUDER_LOCKOUT);
	m_flags |= UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Set user's full name
 */
void User::setFullName(const TCHAR *fullName)
{
   if (_tcscmp(m_fullName, CHECK_NULL_EX(fullName)))
   {
      _tcslcpy(m_fullName, CHECK_NULL_EX(fullName), MAX_USER_FULLNAME);
      m_flags |= UF_MODIFIED;
   }
}

/**
 * Set user's email
 */
void User::setEmail(const TCHAR *email)
{
   if (_tcscmp(CHECK_NULL_EX(m_email), CHECK_NULL_EX(email)))
   {
      MemFree(m_email);
      m_email = MemCopyString(email);
      m_flags |= UF_MODIFIED;
   }
}

/**
 * Set user's phone number
 */
void User::setPhoneNumber(const TCHAR *phoneNumber)
{
   if (_tcscmp(CHECK_NULL_EX(m_phoneNumber), CHECK_NULL_EX(phoneNumber)))
   {
      MemFree(m_phoneNumber);
      m_phoneNumber = MemCopyString(phoneNumber);
      m_flags |= UF_MODIFIED;
   }
}

/**
 * Serialize object to JSON
 */
json_t *User::toJson() const
{
   json_t *root = UserDatabaseObject::toJson();
   json_object_set_new(root, "fullName", json_string_t(m_fullName));
   json_object_set_new(root, "graceLogins", json_integer(m_graceLogins));
   json_object_set_new(root, "authMethod", json_integer(static_cast<int>(m_authMethod)));
   json_object_set_new(root, "certMappingMethod", json_integer(m_certMappingMethod));
   json_object_set_new(root, "certMappingData", json_string_t(m_certMappingData));
   json_object_set_new(root, "disabledUntil", json_integer(m_disabledUntil));
   json_object_set_new(root, "lastPasswordChange", json_integer(m_lastPasswordChange));
   json_object_set_new(root, "lastLogin", json_integer(m_lastLogin));
   json_object_set_new(root, "minPasswordLength", json_integer(m_minPasswordLength));
   json_object_set_new(root, "authFailures", json_integer(m_authFailures));
   return root;
}

/**
 * Create NXSL object
 */
NXSL_Value *User::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslUserClass, this));
}

/**
 * Load 2FA methods bindings for user from DB
 */
void User::load2FABindings(DB_HANDLE hdb)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT name,configuration FROM two_factor_auth_bindings WHERE user_id=?"));
   if (hStmt == nullptr)
      return;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int bindingCount = 0;
      int rowCount = DBGetNumRows(hResult);
      for (int i = 0; i < rowCount; i++)
      {
         TCHAR methodName[MAX_OBJECT_NAME];
         DBGetField(hResult, i, 0, methodName, MAX_OBJECT_NAME);
         char* configuration = DBGetFieldUTF8(hResult, i, 1, nullptr, 0);
         auto binding = make_shared<Config>();
         if (binding->loadConfigFromMemory(configuration, strlen(configuration), _T("MethodBinding"), nullptr, true, false))
         {
            m_2FABindings.set(methodName, binding);
            bindingCount++;
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Loaded binding for two-factor authentication method %s for user %s"), methodName, m_name);
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Binding for two-factor authentication method %s for user %s failed to load"), methodName, m_name);
         }
         MemFree(configuration);
      }
      DBFreeResult(hResult);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("%d two-factor authentication method binding%s added for user %s"), bindingCount, (bindingCount == 1) ? _T("") : _T("s"), m_name);
   }
   DBFreeStatement(hStmt);
}

/**
 * Get method names
 */
unique_ptr<StringList> User::getConfigured2FAMethods() const
{
   auto methods = make_unique<StringList>();
   for (const KeyValuePair<shared_ptr<Config>>* binding : m_2FABindings)
      methods->add(binding->key);
   return methods;
}

/**
 * Fill NXCP message with 2FA method bindings
 */
void User::fill2FAMethodBindingInfo(NXCPMessage *msg) const
{
   uint32_t count = 0;
   uint32_t fieldId = VID_2FA_METHOD_LIST_BASE;
   for (const KeyValuePair<shared_ptr<Config>>* binding : m_2FABindings)
   {
      msg->setField(fieldId, binding->key);
      unique_ptr<StringMap> configuration = Extract2FAMethodBindingConfiguration(binding->key, **binding->value);
      if (configuration != nullptr)
      {
         configuration->fillMessage(msg, fieldId + 10, fieldId + 9);
      }
      else
      {
         msg->setField(fieldId + 1, static_cast<uint32_t>(0));
      }
      fieldId += 1000;
      count++;
   }
   msg->setField(VID_2FA_METHOD_COUNT, count);
}

/**
 * Create/modify 2FA method binding for user
 */
uint32_t User::modify2FAMethodBinding(const TCHAR* methodName, const StringMap& configuration)
{
   uint32_t rcc;
   bool newBinding = false;
   shared_ptr<Config> binding = m_2FABindings.getShared(methodName);
   if (binding == nullptr)
   {
      binding = make_shared<Config>();
      newBinding = true;
   }
   if (Update2FAMethodBindingConfiguration(methodName, binding.get(), configuration))
   {
      if (newBinding)
         m_2FABindings.set(methodName, binding);
      m_flags |= UF_MODIFIED;
      rcc = RCC_SUCCESS;
   }
   else
   {
      rcc = RCC_NO_SUCH_2FA_METHOD;
   }
   return rcc;
}

/**
 * Delete 2FA binding for user
 */
uint32_t User::delete2FAMethodBinding(const TCHAR* methodName)
{
   if (!m_2FABindings.contains(methodName))
      return RCC_NO_SUCH_2FA_BINDING;

   m_2FABindings.remove(methodName);
   m_flags |= UF_MODIFIED;
   return RCC_SUCCESS;
}

/*****************************************************************************
 **  Group
 ****************************************************************************/

/**
 * Constructor for group object - create from database
 * Expects fields in the following order:
 *    id,name,system_access,flags,description,guid,ldap_dn,ui_access_rules
 */
Group::Group(DB_HANDLE hdb, DB_RESULT hr, int row) : UserDatabaseObject(hdb, hr, row)
{
   DB_RESULT hResult = ExecuteSelectOnObject(hdb, m_id, _T("SELECT user_id FROM user_group_members WHERE group_id={id}"));
   if (hResult != nullptr)
	{
		int count = DBGetNumRows(hResult);
		if (count > 0)
		{
			for(int i = 0; i < count; i++)
				m_members.add(DBGetFieldULong(hResult, i, 0));
			m_members.sort(CompareUserId);
		}
		DBFreeResult(hResult);
	}

	loadCustomAttributes(hdb);
}

/**
 * Constructor for group object - create "Everyone" group
 */
Group::Group() : UserDatabaseObject()
{
	m_id = GROUP_EVERYONE;
	_tcscpy(m_name, _T("Everyone"));
	m_flags = UF_MODIFIED;
	m_systemRights = (SYSTEM_ACCESS_VIEW_EVENT_DB | SYSTEM_ACCESS_VIEW_ALL_ALARMS);
	_tcscpy(m_description, _T("Built-in everyone group"));
}

/**
 * Constructor for group object - create new group
 */
Group::Group(uint32_t id, const TCHAR *name) : UserDatabaseObject(id, name)
{
}

/**
 * Copy constructor for group object
 */
Group::Group(const Group *src) : UserDatabaseObject(src), m_members(src->m_members)
{
}

/**
 * Destructor for group object
 */
Group::~Group()
{
}

/**
 * Save object to database
 */
bool Group::saveToDatabase(DB_HANDLE hdb)
{
   // Clear modification flag
   m_flags &= ~UF_MODIFIED;

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("user_groups"), _T("id"), m_id))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE user_groups SET name=?,system_access=?,flags=?,description=?,guid=?,ldap_dn=?,ldap_unique_id=?,created=?,ui_access_rules=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO user_groups (name,system_access,flags,description,guid,ldap_dn,ldap_unique_id,created,ui_access_rules,id) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   }
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, m_systemRights);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_ldapDn, DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_ldapId, DB_BIND_STATIC);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_created));
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_uiAccessRules, DB_BIND_STATIC);
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBBegin(hdb);
	if (success)
      success = DBExecute(hStmt);

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_group_members WHERE group_id=?"));

   if (success && !m_members.isEmpty())
   {
      DBFreeStatement(hStmt);
      hStmt = DBPrepare(hdb, _T("INSERT INTO user_group_members (group_id,user_id) VALUES (?,?)"), m_members.size() > 1);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; (i < m_members.size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_members.get(i));
            success = DBExecute(hStmt);
         }
      }
      else
      {
         success = false;
      }
   }

   if (success)
      success = saveCustomAttributes(hdb);

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);

   DBFreeStatement(hStmt);
   return success;
}

/**
 * Delete object from database
 */
bool Group::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = DBBegin(hdb);
	if (success)
	   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_groups WHERE id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_group_members WHERE group_id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM userdb_custom_attributes WHERE object_id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_access WHERE user_id=?"));

   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM two_factor_auth_bindings WHERE user_id=?"));

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);

	return success;
}

/**
 * Check if given user is a member
 */
bool Group::isMember(uint32_t userId, GroupSearchPath *searchPath) const
{
   if (m_id == GROUP_EVERYONE)
      return true;

   // This is to avoid applying disabled group rights on enabled user
   if ((m_flags & UF_DISABLED))
      return false;

   if (bsearch(&userId, m_members.getBuffer(), m_members.size(), sizeof(uint32_t), CompareUserId) != nullptr)
      return true;

   if (searchPath != nullptr)
   {
      // To avoid going into a recursive loop (e.g. A->B->C->A)
      if (searchPath->contains(m_id))
         return false;

      searchPath->add(m_id);

      // Loops backwards because groups will be at the end of the list, if userId is encountered, normal bsearch takes place
      for(int i = m_members.size() - 1; i >= 0; i--)
      {
         uint32_t gid = m_members.get(i);
         if (!(gid & GROUP_FLAG))
            break;
         if (CheckUserMembershipInternal(userId, gid, searchPath))
            return true;
      }
   }

   return false;
}

/**
 * Add user to group
 */
void Group::addUser(uint32_t userId)
{
   if (bsearch(&userId, m_members.getBuffer(), m_members.size(), sizeof(uint32_t), CompareUserId) != nullptr)
      return;  // already added

   // Not in group, add it
   m_members.add(userId);
   m_members.sort(CompareUserId);

	m_flags |= UF_MODIFIED;

   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Delete user from group
 */
void Group::deleteUser(uint32_t userId)
{
   uint32_t *e = static_cast<uint32_t*>(bsearch(&userId, m_members.getBuffer(), m_members.size(), sizeof(uint32_t), CompareUserId));
   if (e == nullptr)
      return;  // not a member

   int index = (int)((char *)e - (char *)m_members.getBuffer()) / sizeof(uint32_t);
   m_members.remove(index);
   m_flags |= UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Fill NXCP message with user group data
 */
void Group::fillMessage(NXCPMessage *msg)
{
	UserDatabaseObject::fillMessage(msg);

   msg->setField(VID_NUM_MEMBERS, static_cast<uint32_t>(m_members.size()));
   uint32_t fieldId = VID_GROUP_MEMBER_BASE;
   for(int i = 0; i < m_members.size(); i++)
      msg->setField(fieldId++, m_members.get(i));
}

/**
 * Modify group object from NXCP message
 */
void Group::modifyFromMessage(const NXCPMessage& msg)
{
	UserDatabaseObject::modifyFromMessage(msg);

	uint32_t fields = msg.getFieldAsUInt32(VID_FIELDS);
	if (fields & USER_MODIFY_MEMBERS)
	{
      IntegerArray<uint32_t> members = std::move(m_members);
		int count = msg.getFieldAsInt32(VID_NUM_MEMBERS);
		if (count > 0)
		{
			uint32_t fieldId = VID_GROUP_MEMBER_BASE;
			for(int i = 0; i < count; i++, fieldId++)
         {
				uint32_t userId = msg.getFieldAsUInt32(fieldId);
				m_members.add(userId);

            // check if new member
			   uint32_t *e = static_cast<uint32_t*>(bsearch(&userId, members.getBuffer(), members.size(), sizeof(uint32_t), CompareUserId));
			   if (e != nullptr)
			   {
			      *e = 0xFFFFFFFF;    // mark as found
			   }
			   else
			   {
               SendUserDBUpdate(USER_DB_MODIFY, userId);  // new member added
			   }
         }
			for(int i = 0; i < members.size(); i++)
            if (members.get(i) != 0xFFFFFFFF)  // not present in new list
               SendUserDBUpdate(USER_DB_MODIFY, members.get(i));
		   m_members.sort(CompareUserId);
		}
		else
		{
         // notify change for all old members
			for(int i = 0; i < members.size(); i++)
            SendUserDBUpdate(USER_DB_MODIFY, members.get(i));
		}
	}
}

/**
 * Serialize object to JSON
 */
json_t *Group::toJson() const
{
   json_t *root = UserDatabaseObject::toJson();
   json_object_set_new(root, "members", json_integer_array(m_members));
   return root;
}

/**
 * Create NXSL object
 */
NXSL_Value *Group::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslUserGroupClass, this));
}
