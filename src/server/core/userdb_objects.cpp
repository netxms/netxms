/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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

/**
 * Compare user IDs (for qsort)
 */
static int CompareUserId(const void *e1, const void *e2)
{
   UINT32 id1 = *((UINT32 *)e1);
   UINT32 id2 = *((UINT32 *)e2);

   return (id1 < id2) ? -1 : ((id1 > id2) ? 1 : 0);
}

/**
 * Generate hash for password
 */
static void CalculatePasswordHash(const TCHAR *password, PasswordHashType type, PasswordHash *ph, const BYTE *salt = NULL)
{
#ifdef UNICODE
	char mbPassword[1024];
	WideCharToMultiByte(CP_UTF8, 0, password, -1, mbPassword, 1024, NULL, NULL);
	mbPassword[1023] = 0;
#else
   const char *mbPassword = password;
#endif

   BYTE buffer[1024];

   memset(ph, 0, sizeof(PasswordHash));
   ph->hashType = type;
   switch(type)
   {
      case PWD_HASH_SHA1:
      	CalculateSHA1Hash((BYTE *)mbPassword, strlen(mbPassword), ph->hash);
         break;
      case PWD_HASH_SHA256:
         if (salt != NULL)
            memcpy(buffer, salt, PASSWORD_SALT_LENGTH);
         else
            GenerateRandomBytes(buffer, PASSWORD_SALT_LENGTH);
         strcpy((char *)&buffer[PASSWORD_SALT_LENGTH], mbPassword);
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
 *    id,name,system_access,flags,description,guid,ldap_dn
 */
UserDatabaseObject::UserDatabaseObject(DB_HANDLE hdb, DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	DBGetField(hResult, row, 1, m_name, MAX_USER_NAME);
	m_systemRights = DBGetFieldUInt64(hResult, row, 2);
	m_flags = DBGetFieldULong(hResult, row, 3);
	DBGetField(hResult, row, 4, m_description, MAX_USER_DESCR);
	m_guid = DBGetFieldGUID(hResult, row, 5);
	m_ldapDn = DBGetField(hResult, row, 6, NULL, 0);
	m_ldapId = DBGetField(hResult, row, 7, NULL, 0);
	m_created = (time_t)DBGetFieldULong(hResult, row, 8);
}

/**
 * Default constructor for generic object
 */
UserDatabaseObject::UserDatabaseObject()
{
   m_id = 0;
   m_guid = uuid::generate();
   m_name[0] = 0;
   m_ldapDn = NULL;
   m_ldapId = NULL;
	m_systemRights = 0;
	m_description[0] = 0;
	m_flags = 0;
	m_created = time(NULL);
}

/**
 * Constructor for generic object - create new object with given id and name
 */
UserDatabaseObject::UserDatabaseObject(UINT32 id, const TCHAR *name)
{
	m_id = id;
   m_guid = uuid::generate();
	nx_strncpy(m_name, name, MAX_USER_NAME);
	m_systemRights = 0;
	m_description[0] = 0;
	m_flags = UF_MODIFIED;
	m_ldapDn = NULL;
	m_ldapId = NULL;
   m_created = time(NULL);
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
   msg->setField(VID_USER_FLAGS, (WORD)m_flags);
   msg->setField(VID_USER_SYS_RIGHTS, m_systemRights);
   msg->setField(VID_USER_DESCRIPTION, m_description);
   msg->setField(VID_GUID, m_guid);
   msg->setField(VID_LDAP_DN, m_ldapDn);
   msg->setField(VID_LDAP_ID, m_ldapId);
   msg->setField(VID_CREATION_TIME, (UINT32)m_created);
   m_attributes.fillMessage(msg, VID_NUM_CUSTOM_ATTRIBUTES, VID_CUSTOM_ATTRIBUTES_BASE);
}

/**
 * Modify object from NXCP message
 */
void UserDatabaseObject::modifyFromMessage(NXCPMessage *msg)
{
	UINT32 flags, fields;

	fields = msg->getFieldAsUInt32(VID_FIELDS);
	DbgPrintf(5, _T("UserDatabaseObject::modifyFromMessage(): id=%d fields=%08X"), m_id, fields);

	if (fields & USER_MODIFY_DESCRIPTION)
	   msg->getFieldAsString(VID_USER_DESCRIPTION, m_description, MAX_USER_DESCR);
	if (fields & USER_MODIFY_LOGIN_NAME)
	   msg->getFieldAsString(VID_USER_NAME, m_name, MAX_USER_NAME);

	// Update custom attributes only if VID_NUM_CUSTOM_ATTRIBUTES exist -
	// older client versions may not be aware of custom attributes
	if ((fields & USER_MODIFY_CUSTOM_ATTRIBUTES) || msg->isFieldExist(VID_NUM_CUSTOM_ATTRIBUTES))
	{
		UINT32 i, varId, count;
		TCHAR *name, *value;

		count = msg->getFieldAsUInt32(VID_NUM_CUSTOM_ATTRIBUTES);
		m_attributes.clear();
		for(i = 0, varId = VID_CUSTOM_ATTRIBUTES_BASE; i < count; i++)
		{
			name = msg->getFieldAsString(varId++);
			value = msg->getFieldAsString(varId++);
			m_attributes.setPreallocated((name != NULL) ? name : _tcsdup(_T("")), (value != NULL) ? value : _tcsdup(_T("")));
		}
	}

	if ((m_id != 0) && (fields & USER_MODIFY_ACCESS_RIGHTS))
		m_systemRights = msg->getFieldAsUInt64(VID_USER_SYS_RIGHTS);

	if (fields & USER_MODIFY_FLAGS)
	{
	   flags = msg->getFieldAsUInt16(VID_USER_FLAGS);
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
   setDn(NULL);
	m_flags |= UF_MODIFIED;
}

/**
 * Load custom attributes from database
 */
bool UserDatabaseObject::loadCustomAttributes(DB_HANDLE hdb)
{
	DB_RESULT hResult;
	TCHAR query[256], *attrName, *attrValue;
	bool success = false;

	_sntprintf(query, 256, _T("SELECT attr_name,attr_value FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
	hResult = DBSelect(hdb, query);
	if (hResult != NULL)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			attrName = DBGetField(hResult, i, 0, NULL, 0);
			if (attrName == NULL)
				attrName = _tcsdup(_T(""));

			attrValue = DBGetField(hResult, i, 1, NULL, 0);
			if (attrValue == NULL)
				attrValue = _tcsdup(_T(""));

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
	TCHAR query[256];
	bool success = false;

	_sntprintf(query, 256, _T("DELETE FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
	if (DBQuery(hdb, query))
	{
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO userdb_custom_attributes (object_id,attr_name,attr_value) VALUES (?,?,?)"), true);
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         success = (m_attributes.forEach(SaveAttributeCallback, hStmt) == _CONTINUE);
         DBFreeStatement(hStmt);
      }
	}
	return success;
}

/**
 * Get custom attribute as UINT32
 */
UINT32 UserDatabaseObject::getAttributeAsULong(const TCHAR *name)
{
	const TCHAR *value = getAttribute(name);
	return (value != NULL) ? _tcstoul(value, NULL, 0) : 0;
}

void UserDatabaseObject::setDescription(const TCHAR *description)
{
   if (_tcscmp(CHECK_NULL_EX(m_description), CHECK_NULL_EX(description)))
   {
      nx_strncpy(m_description, CHECK_NULL_EX(description), MAX_USER_DESCR);
      m_flags |= UF_MODIFIED;
   }
}

void UserDatabaseObject::setName(const TCHAR *name)
{
   if (_tcscmp(CHECK_NULL_EX(m_name), CHECK_NULL_EX(name)))
   {
      nx_strncpy(m_name, CHECK_NULL_EX(name), MAX_USER_NAME);
      m_flags |= UF_MODIFIED;
   }
}

void UserDatabaseObject::setDn(const TCHAR *dn)
{
   if(dn == NULL)
      return;
   if(m_ldapDn != NULL && !_tcscmp(m_ldapDn, dn))
      return;
   free(m_ldapDn);
   m_ldapDn = MemCopyString(dn);
   m_flags |= UF_MODIFIED;
}

void UserDatabaseObject::setLdapId(const TCHAR *id)
{
   if(m_ldapId != NULL && !_tcscmp(m_ldapId, id))
      return;
   free(m_ldapId);
   m_ldapId = MemCopyString(id);
   m_flags |= UF_MODIFIED;
}

void UserDatabaseObject::removeSyncException()
{
   if((m_flags & UF_SYNC_EXCEPTION)> 0)
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
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "attributes", m_attributes.toJson());
   json_object_set_new(root, "ldapDn", json_string_t(m_ldapDn));
   json_object_set_new(root, "ldapId", json_string_t(m_ldapId));
   json_object_set_new(root, "created", json_integer((UINT32)m_created));
   return root;
}

/**
 * Get custom attribute as NXSL value
 */
NXSL_Value *UserDatabaseObject::getCustomAttributeForNXSL(NXSL_VM *vm, const TCHAR *name) const
{
   NXSL_Value *value = NULL;
   const TCHAR *av = m_attributes.get(name);
   if (av != NULL)
      value = vm->createValue(av);
   return value;
}

/**
 * Get all custom attributes as NXSL hash map
 */
NXSL_Value *UserDatabaseObject::getCustomAttributesForNXSL(NXSL_VM *vm) const
{
   NXSL_HashMap *map = new NXSL_HashMap(vm);
   StructArray<KeyValuePair> *attributes = m_attributes.toArray();
   for(int i = 0; i < attributes->size(); i++)
   {
      KeyValuePair *p = attributes->get(i);
      map->set(p->key, vm->createValue(static_cast<const TCHAR*>(p->value)));
   }
   delete attributes;
   return vm->createValue(map);
}

/**
 * Create NXSL object
 */
NXSL_Value *UserDatabaseObject::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslUserDBObjectClass, this));
}

/*****************************************************************************
 **  User
 ****************************************************************************/

/**
 * Constructor for user object - create from database
 * Expects fields in the following order:
 *    id,name,system_access,flags,description,guid,ldap_dn,password,full_name,
 *    grace_logins,auth_method,cert_mapping_method,cert_mapping_data,
 *    auth_failures,last_passwd_change,min_passwd_length,disabled_until,
 *    last_login,xmpp_id
 */
User::User(DB_HANDLE hdb, DB_RESULT hResult, int row) : UserDatabaseObject(hdb, hResult, row)
{
	TCHAR buffer[256];

   bool validHash = false;
   DBGetField(hResult, row, 9, buffer, 256);
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
   }
   else
   {
      // old format - SHA1 hash without salt
      m_password.hashType = PWD_HASH_SHA1;
      if (StrToBin(buffer, m_password.hash, SHA1_DIGEST_SIZE) == SHA1_DIGEST_SIZE)
         validHash = true;
   }
   if (!validHash)
   {
	   nxlog_write(NXLOG_WARNING, _T("Invalid password hash for user %s; password reset to default"), m_name);
	   CalculatePasswordHash(_T("netxms"), PWD_HASH_SHA256, &m_password);
      m_flags |= UF_MODIFIED | UF_CHANGE_PASSWORD;
   }

	DBGetField(hResult, row, 10, m_fullName, MAX_USER_FULLNAME);
	m_graceLogins = DBGetFieldLong(hResult, row, 11);
	m_authMethod = DBGetFieldLong(hResult, row, 12);
	m_certMappingMethod = DBGetFieldLong(hResult, row, 13);
	m_certMappingData = DBGetField(hResult, row, 14, NULL, 0);
	m_authFailures = DBGetFieldLong(hResult, row, 15);
	m_lastPasswordChange = (time_t)DBGetFieldLong(hResult, row, 16);
	m_minPasswordLength = DBGetFieldLong(hResult, row, 17);
	m_disabledUntil = (time_t)DBGetFieldLong(hResult, row, 18);
	m_lastLogin = (time_t)DBGetFieldLong(hResult, row, 19);
   DBGetField(hResult, row, 20, m_xmppId, MAX_XMPP_ID_LEN);

	// Set full system access for superuser
	if (m_id == 0)
		m_systemRights = SYSTEM_ACCESS_FULL;

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
	m_graceLogins = ConfigReadInt(_T("GraceLoginCount"), 5);
	m_authMethod = AUTH_NETXMS_PASSWORD;
	m_certMappingMethod = USER_MAP_CERT_BY_CN;
	m_certMappingData = NULL;
	m_authFailures = 0;
	m_lastPasswordChange = 0;
	m_minPasswordLength = -1;	// Use system-wide default
	m_disabledUntil = 0;
	m_lastLogin = 0;
   m_xmppId[0] = 0;
}

/**
 * Constructor for user object - new user
 */
User::User(UINT32 id, const TCHAR *name) : UserDatabaseObject(id, name)
{
	m_fullName[0] = 0;
	m_graceLogins = ConfigReadInt(_T("GraceLoginCount"), 5);
	m_authMethod = AUTH_NETXMS_PASSWORD;
	m_certMappingMethod = USER_MAP_CERT_BY_CN;
	m_certMappingData = NULL;
	CalculatePasswordHash(_T(""), PWD_HASH_SHA256, &m_password);
	m_authFailures = 0;
	m_lastPasswordChange = 0;
	m_minPasswordLength = -1;	// Use system-wide default
	m_disabledUntil = 0;
	m_lastLogin = 0;
   m_xmppId[0] = 0;
}

/**
 * Copy constructor for user object
 */
User::User(const User *src) : UserDatabaseObject(src)
{
   m_password.hashType = src->m_password.hashType;
   memcpy(m_password.hash, src->m_password.hash, SHA256_DIGEST_SIZE);
   memcpy(m_password.salt, src->m_password.salt, PASSWORD_SALT_LENGTH);

   _tcsncpy(m_fullName, src->m_fullName, MAX_USER_FULLNAME);
   m_graceLogins = src->m_graceLogins;
   m_authMethod = src->m_authMethod;
   m_certMappingMethod = src->m_certMappingMethod;
   m_certMappingData = MemCopyString(src->m_certMappingData);
   m_disabledUntil = src->m_disabledUntil;
   m_lastPasswordChange = src->m_lastPasswordChange;
   m_lastLogin = src->m_lastLogin;
   m_minPasswordLength = src->m_minPasswordLength;
   m_authFailures = src->m_authFailures;
   _tcsncpy(m_xmppId, src->m_xmppId, MAX_XMPP_ID_LEN);
}

/**
 * Destructor for user object
 */
User::~User()
{
	MemFree(m_certMappingData);
}

/**
 * Save object to database
 */
bool User::saveToDatabase(DB_HANDLE hdb)
{
	TCHAR password[128];

   // Clear modification flag
   m_flags &= ~UF_MODIFIED;

   // Create or update record in database
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
         _T("  min_passwd_length=?,disabled_until=?,last_login=?,xmpp_id=?,ldap_dn=?,ldap_unique_id=?,created=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb,
         _T("INSERT INTO users (name,password,system_access,flags,full_name,description,grace_logins,guid,auth_method,")
         _T("  cert_mapping_method,cert_mapping_data,password_history,auth_failures,last_passwd_change,min_passwd_length,")
         _T("  disabled_until,last_login,xmpp_id,ldap_dn,ldap_unique_id,created,id) VALUES (?,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,?,?,?,?,?)"));
   }
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, password, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, m_systemRights);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_fullName, DB_BIND_STATIC);
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_graceLogins);
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_authMethod);
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_certMappingMethod);
   DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_certMappingData, DB_BIND_STATIC);
   DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_authFailures);
   DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, (UINT32)m_lastPasswordChange);
   DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_minPasswordLength);
   DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (UINT32)m_disabledUntil);
   DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, (UINT32)m_lastLogin);
   DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_xmppId, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_TEXT, m_ldapDn, DB_BIND_STATIC);
   DBBind(hStmt, 19, DB_SQLTYPE_VARCHAR, m_ldapId, DB_BIND_STATIC);
   DBBind(hStmt, 20, DB_SQLTYPE_INTEGER, (UINT32)m_created);
   DBBind(hStmt, 21, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBBegin(hdb);
	if (success)
	{
		success = DBExecute(hStmt);
		if (success)
		{
			success = saveCustomAttributes(hdb);
		}
		if (success)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}
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
	{
	   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM users WHERE id=?"));
	}

   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_profiles WHERE user_id=?"));
   }

   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM userdb_custom_attributes WHERE object_id=?"));
   }

   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_access WHERE user_id=?"));
   }

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
	return success;
}

/**
 * Validate user's password
 */
bool User::validatePassword(const TCHAR *password)
{
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
 * For non-UNICODE build, password must be UTF-8 encoded
 */
void User::setPassword(const TCHAR *password, bool clearChangePasswdFlag)
{
   CalculatePasswordHash(password, PWD_HASH_SHA256, &m_password);
	m_graceLogins = ConfigReadInt(_T("GraceLoginCount"), 5);;
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
   msg->setField(VID_AUTH_METHOD, (WORD)m_authMethod);
	msg->setField(VID_CERT_MAPPING_METHOD, (WORD)m_certMappingMethod);
	msg->setField(VID_CERT_MAPPING_DATA, CHECK_NULL_EX(m_certMappingData));
	msg->setField(VID_LAST_LOGIN, (UINT32)m_lastLogin);
	msg->setField(VID_LAST_PASSWORD_CHANGE, (UINT32)m_lastPasswordChange);
	msg->setField(VID_MIN_PASSWORD_LENGTH, (WORD)m_minPasswordLength);
	msg->setField(VID_DISABLED_UNTIL, (UINT32)m_disabledUntil);
	msg->setField(VID_AUTH_FAILURES, (UINT32)m_authFailures);
   msg->setField(VID_XMPP_ID, m_xmppId);

   FillGroupMembershipInfo(msg, m_id);
}

/**
 * Modify user object from NXCP message
 */
void User::modifyFromMessage(NXCPMessage *msg)
{
	UserDatabaseObject::modifyFromMessage(msg);

	UINT32 fields = msg->getFieldAsUInt32(VID_FIELDS);

	if (fields & USER_MODIFY_FULL_NAME)
		msg->getFieldAsString(VID_USER_FULL_NAME, m_fullName, MAX_USER_FULLNAME);
	if (fields & USER_MODIFY_AUTH_METHOD)
	   m_authMethod = msg->getFieldAsUInt16(VID_AUTH_METHOD);
	if (fields & USER_MODIFY_PASSWD_LENGTH)
	   m_minPasswordLength = msg->getFieldAsUInt16(VID_MIN_PASSWORD_LENGTH);
	if (fields & USER_MODIFY_TEMP_DISABLE)
	   m_disabledUntil = (time_t)msg->getFieldAsUInt32(VID_DISABLED_UNTIL);
	if (fields & USER_MODIFY_CERT_MAPPING)
	{
		m_certMappingMethod = msg->getFieldAsUInt16(VID_CERT_MAPPING_METHOD);
		MemFree(m_certMappingData);
		m_certMappingData = msg->getFieldAsString(VID_CERT_MAPPING_DATA);
	}
	if (fields & USER_MODIFY_XMPP_ID)
		msg->getFieldAsString(VID_XMPP_ID, m_xmppId, MAX_XMPP_ID_LEN);
   if (fields & USER_MODIFY_GROUP_MEMBERSHIP)
   {
      int count = (int)msg->getFieldAsUInt32(VID_NUM_GROUPS);
      UINT32 *groups = NULL;
      if (count > 0)
      {
         groups = (UINT32 *)malloc(sizeof(UINT32) * count);
         msg->getFieldAsInt32Array(VID_GROUPS, (UINT32)count, groups);
      }
      UpdateGroupMembership(m_id, count, groups);
      MemFree(groups);
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

	int lockoutThreshold = ConfigReadInt(_T("IntruderLockoutThreshold"), 0);
	if ((lockoutThreshold > 0) && (m_authFailures >= lockoutThreshold))
	{
		m_disabledUntil = time(NULL) + ConfigReadInt(_T("IntruderLockoutTime"), 30) * 60;
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
	m_graceLogins = ConfigReadInt(_T("GraceLoginCount"), 5);
	m_disabledUntil = 0;
	m_flags &= ~(UF_DISABLED | UF_INTRUDER_LOCKOUT);
	m_flags |= UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

void User::setFullName(const TCHAR *fullName)
{
   if (_tcscmp(CHECK_NULL_EX(m_fullName), CHECK_NULL_EX(fullName)))
   {
      nx_strncpy(m_fullName, CHECK_NULL_EX(fullName), MAX_USER_FULLNAME);
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
   json_object_set_new(root, "authMethod", json_integer(m_authMethod));
   json_object_set_new(root, "certMappingMethod", json_integer(m_certMappingMethod));
   json_object_set_new(root, "certMappingData", json_string_t(m_certMappingData));
   json_object_set_new(root, "disabledUntil", json_integer(m_disabledUntil));
   json_object_set_new(root, "lastPasswordChange", json_integer(m_lastPasswordChange));
   json_object_set_new(root, "lastLogin", json_integer(m_lastLogin));
   json_object_set_new(root, "minPasswordLength", json_integer(m_minPasswordLength));
   json_object_set_new(root, "authFailures", json_integer(m_authFailures));
   json_object_set_new(root, "xmppId", json_string_t(m_xmppId));
   return root;
}

/**
 * Create NXSL object
 */
NXSL_Value *User::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslUserClass, this));
}

/*****************************************************************************
 **  Group
 ****************************************************************************/

/**
 * Constructor for group object - create from database
 * Expects fields in the following order:
 *    id,name,system_access,flags,description,guid,ldap_dn
 */
Group::Group(DB_HANDLE hdb, DB_RESULT hr, int row) : UserDatabaseObject(hdb, hr, row)
{
	DB_RESULT hResult;
	TCHAR query[256];

	_sntprintf(query, 256, _T("SELECT user_id FROM user_group_members WHERE group_id=%d"), m_id);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
	{
		m_memberCount = DBGetNumRows(hResult);
		if (m_memberCount > 0)
		{
			m_members = (UINT32 *)malloc(sizeof(UINT32) * m_memberCount);
			for(int i = 0; i < m_memberCount; i++)
				m_members[i] = DBGetFieldULong(hResult, i, 0);
			qsort(m_members, m_memberCount, sizeof(UINT32), CompareUserId);
		}
		else
		{
			m_members = NULL;
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
	m_memberCount = 0;
	m_members = NULL;
}

/**
 * Constructor for group object - create new group
 */
Group::Group(UINT32 id, const TCHAR *name) : UserDatabaseObject(id, name)
{
	m_memberCount = 0;
	m_members = NULL;
}

/**
 * Copy constructor for group object
 */
Group::Group(const Group *src) : UserDatabaseObject(src)
{
   m_memberCount = src->m_memberCount;
   if (m_memberCount > 0)
   {
      m_members = static_cast<UINT32 *>(malloc(src->m_memberCount * sizeof(UINT32)));
      memcpy(m_members, src->m_members, src->m_memberCount * sizeof(UINT32));
   }
   else
   {
      m_members = NULL;
   }
}

/**
 * Destructor for group object
 */
Group::~Group()
{
	free(m_members);
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
      hStmt = DBPrepare(hdb, _T("UPDATE user_groups SET name=?,system_access=?,flags=?,description=?,guid=?,ldap_dn=?,ldap_unique_id=?,created=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO user_groups (name,system_access,flags,description,guid,ldap_dn,ldap_unique_id,created,id) VALUES (?,?,?,?,?,?,?,?,?)"));
   }
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, m_systemRights);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_ldapDn, DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, m_ldapId, DB_BIND_STATIC);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (UINT32)m_created);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);

   bool success = DBBegin(hdb);
	if (success)
	{
      success = DBExecute(hStmt);
		if (success)
		{
         DBFreeStatement(hStmt);
         hStmt = DBPrepare(hdb, _T("DELETE FROM user_group_members WHERE group_id=?"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            success = DBExecute(hStmt);
         }
         else
         {
            success = false;
         }

			if (success && (m_memberCount > 0))
			{
            DBFreeStatement(hStmt);
            hStmt = DBPrepare(hdb, _T("INSERT INTO user_group_members (group_id,user_id) VALUES (?,?)"), m_memberCount > 1);
            if (hStmt != NULL)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
				   for(int i = 0; (i < m_memberCount) && success; i++)
				   {
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_members[i]);
                  success = DBExecute(hStmt);
				   }
            }
            else
            {
               success = false;
            }
			}

		   if (success)
		   {
			   success = saveCustomAttributes(hdb);
		   }
		}
		if (success)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}

   if (hStmt != NULL)
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
	{
	   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_groups WHERE id=?"));
	}

   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM user_group_members WHERE group_id=?"));
   }

   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM userdb_custom_attributes WHERE object_id=?"));
   }

   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dci_access WHERE user_id=?"));
   }

   if (success)
      DBCommit(hdb);
   else
      DBRollback(hdb);
	return success;
}

/**
 * Check if given user is a member
 */
bool Group::isMember(UINT32 userId, IntegerArray<UINT32> *searchPath)
{
   if (m_id == GROUP_EVERYONE)
      return true;

   // This is to avoid applying disabled group rights on enabled user
   if ((m_flags & UF_DISABLED))
      return false;

   if (bsearch(&userId, m_members, m_memberCount, sizeof(UINT32), CompareUserId) != NULL)
      return true;

   if (searchPath != NULL)
   {
      // To avoid going into a recursive loop (e.g. A->B->C->A)
      if (searchPath->contains(m_id))
         return false;

      searchPath->add(m_id);

      // Loops backwards because groups will be at the end of the list, if userId is encountered, normal bsearch takes place
      for(int i = m_memberCount - 1; i >= 0; i--)
      {
         if (!(m_members[i] & GROUP_FLAG))
            break;
         if (CheckUserMembershipInternal(userId, m_members[i], searchPath))
            return true;
      }
   }

   return false;
}

/**
 * Add user to group
 */
void Group::addUser(UINT32 userId)
{
   if (bsearch(&userId, m_members, m_memberCount, sizeof(UINT32), CompareUserId) != NULL)
      return;  // already added

   // Not in group, add it
	m_memberCount++;
   m_members = (UINT32 *)realloc(m_members, sizeof(UINT32) * m_memberCount);
   m_members[m_memberCount - 1] = userId;
   qsort(m_members, m_memberCount, sizeof(UINT32), CompareUserId);

	m_flags |= UF_MODIFIED;

   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Delete user from group
 */
void Group::deleteUser(UINT32 userId)
{
   UINT32 *e = (UINT32 *)bsearch(&userId, m_members, m_memberCount, sizeof(UINT32), CompareUserId);
   if (e == NULL)
      return;  // not a member

   int index = (int)((char *)e - (char *)m_members) / sizeof(UINT32);
   m_memberCount--;
   memmove(&m_members[index], &m_members[index + 1], sizeof(UINT32) * (m_memberCount - index));
   m_flags |= UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Get group members.
 */
int Group::getMembers(UINT32 **members)
{
	*members = m_members;
   return m_memberCount;
}

/**
 * Fill NXCP message with user group data
 */
void Group::fillMessage(NXCPMessage *msg)
{
   UINT32 varId;
	int i;

	UserDatabaseObject::fillMessage(msg);

   msg->setField(VID_NUM_MEMBERS, (UINT32)m_memberCount);
   for(i = 0, varId = VID_GROUP_MEMBER_BASE; i < m_memberCount; i++, varId++)
      msg->setField(varId, m_members[i]);
}

/**
 * Modify group object from NXCP message
 */
void Group::modifyFromMessage(NXCPMessage *msg)
{
	int i;
	UINT32 varId, fields;

	UserDatabaseObject::modifyFromMessage(msg);

	fields = msg->getFieldAsUInt32(VID_FIELDS);
	if (fields & USER_MODIFY_MEMBERS)
	{
      UINT32 *members = m_members;
      int count = m_memberCount;
		m_memberCount = msg->getFieldAsUInt32(VID_NUM_MEMBERS);
		if (m_memberCount > 0)
		{
			m_members = (UINT32 *)malloc(sizeof(UINT32) * m_memberCount);
			for(i = 0, varId = VID_GROUP_MEMBER_BASE; i < m_memberCount; i++, varId++)
         {
				m_members[i] = msg->getFieldAsUInt32(varId);

            // check if new member
			   UINT32 *e = (UINT32 *)bsearch(&m_members[i], members, count, sizeof(UINT32), CompareUserId);
			   if (e != NULL)
			   {
			      *((UINT32 *)e) = 0xFFFFFFFF;    // mark as found
			   }
			   else
			   {
               SendUserDBUpdate(USER_DB_MODIFY, m_members[i]);  // new member added
			   }
         }
			for(i = 0; i < count; i++)
            if (members[i] != 0xFFFFFFFF)  // not present in new list
               SendUserDBUpdate(USER_DB_MODIFY, members[i]);
		   qsort(m_members, m_memberCount, sizeof(UINT32), CompareUserId);
		}
		else
		{
         m_members = NULL;

         // notify change for all old members
			for(i = 0; i < count; i++)
            SendUserDBUpdate(USER_DB_MODIFY, members[i]);
		}
		free(members);
	}
}

/**
 * Serialize object to JSON
 */
json_t *Group::toJson() const
{
   json_t *root = UserDatabaseObject::toJson();
   json_object_set_new(root, "members", json_integer_array(m_members, m_memberCount));
   return root;
}

/**
 * Create NXSL object
 */
NXSL_Value *Group::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslUserGroupClass, this));
}
