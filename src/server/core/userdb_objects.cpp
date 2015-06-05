/*
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
UserDatabaseObject::UserDatabaseObject(DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	DBGetField(hResult, row, 1, m_name, MAX_USER_NAME);
	m_systemRights = DBGetFieldUInt64(hResult, row, 2);
	m_flags = DBGetFieldULong(hResult, row, 3);
	DBGetField(hResult, row, 4, m_description, MAX_USER_DESCR);
	DBGetFieldGUID(hResult, row, 5, m_guid);
	m_userDn = DBGetField(hResult, row, 6, NULL, 0);
}

/**
 * Default constructor for generic object
 */
UserDatabaseObject::UserDatabaseObject()
{
   m_userDn = NULL;
}

/**
 * Constructor for generic object - create new object with given id and name
 */
UserDatabaseObject::UserDatabaseObject(UINT32 id, const TCHAR *name)
{
	m_id = id;
	uuid_generate(m_guid);
	nx_strncpy(m_name, name, MAX_USER_NAME);
	m_systemRights = 0;
	m_description[0] = 0;
	m_flags = UF_MODIFIED;
	m_userDn = NULL;
}

/**
 * Destructor for generic user database object
 */
UserDatabaseObject::~UserDatabaseObject()
{
   safe_free(m_userDn);
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
   msg->setField(VID_GUID, m_guid, UUID_LENGTH);
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
	   //Null dn if user is detached from LDAP
      if((m_flags & UF_LDAP_USER) && !(flags & UF_LDAP_USER))
         setDn(NULL);
		// Modify only UF_DISABLED, UF_CHANGE_PASSWORD, UF_CANNOT_CHANGE_PASSWORD and UF_LDAP_USER flags from message
		// Ignore all but CHANGE_PASSWORD flag for superuser and "everyone" group
		m_flags &= ~(UF_DISABLED | UF_CHANGE_PASSWORD | UF_CANNOT_CHANGE_PASSWORD | UF_LDAP_USER);
		if ((m_id == 0) || (m_id == GROUP_EVERYONE))
			m_flags |= flags & UF_CHANGE_PASSWORD;
		else
			m_flags |= flags & (UF_DISABLED | UF_CHANGE_PASSWORD | UF_CANNOT_CHANGE_PASSWORD | UF_LDAP_USER);

	}

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
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO userdb_custom_attributes (object_id,attr_name,attr_value) VALUES (?,?,?)"));
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
   safe_free(m_userDn);
   m_userDn = (dn != NULL) ? _tcsdup(dn) : NULL;
   m_flags |= UF_MODIFIED;
}

/**
 * Disable user account because of sync exception
 */
void UserDatabaseObject::setSyncException()
{
   m_flags |= UF_SYNC_EXCEPTION | UF_MODIFIED;
   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
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
User::User(DB_RESULT hResult, int row) : UserDatabaseObject(hResult, row)
{
	TCHAR buffer[256];

   bool validHash = false;
   DBGetField(hResult, row, 7, buffer, 256);
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
	   nxlog_write(MSG_INVALID_PASSWORD_HASH, NXLOG_WARNING, "s", m_name);
	   CalculatePasswordHash(_T("netxms"), PWD_HASH_SHA256, &m_password);
      m_flags |= UF_MODIFIED | UF_CHANGE_PASSWORD;
   }

	DBGetField(hResult, row, 8, m_fullName, MAX_USER_FULLNAME);
	m_graceLogins = DBGetFieldLong(hResult, row, 9);
	m_authMethod = DBGetFieldLong(hResult, row, 10);
	m_certMappingMethod = DBGetFieldLong(hResult, row, 11);
	m_certMappingData = DBGetField(hResult, row, 12, NULL, 0);
	m_authFailures = DBGetFieldLong(hResult, row, 13);
	m_lastPasswordChange = (time_t)DBGetFieldLong(hResult, row, 14);
	m_minPasswordLength = DBGetFieldLong(hResult, row, 15);
	m_disabledUntil = (time_t)DBGetFieldLong(hResult, row, 16);
	m_lastLogin = (time_t)DBGetFieldLong(hResult, row, 17);
   DBGetField(hResult, row, 18, m_xmppId, MAX_XMPP_ID_LEN);

	// Set full system access for superuser
	if (m_id == 0)
		m_systemRights = SYSTEM_ACCESS_FULL;

	loadCustomAttributes(g_hCoreDB);
}

/**
 * Constructor for user object - create default superuser
 */
User::User()
{
	m_id = 0;
	_tcscpy(m_name, _T("admin"));
	m_flags = UF_MODIFIED | UF_CHANGE_PASSWORD;
	m_systemRights = SYSTEM_ACCESS_FULL;
	m_fullName[0] = 0;
	_tcscpy(m_description, _T("Built-in system administrator account"));
	CalculatePasswordHash(_T("netxms"), PWD_HASH_SHA256, &m_password);
	m_graceLogins = MAX_GRACE_LOGINS;
	m_authMethod = AUTH_NETXMS_PASSWORD;
	uuid_generate(m_guid);
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
	m_graceLogins = MAX_GRACE_LOGINS;
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
 * Destructor for user object
 */
User::~User()
{
	safe_free(m_certMappingData);
}

/**
 * Save object to database
 */
bool User::saveToDatabase(DB_HANDLE hdb)
{
	TCHAR password[128], guidText[64];

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
         _T("  min_passwd_length=?,disabled_until=?,last_login=?,xmpp_id=?,ldap_dn=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb,
         _T("INSERT INTO users (name,password,system_access,flags,full_name,description,grace_logins,guid,auth_method,")
         _T("  cert_mapping_method,cert_mapping_data,password_history,auth_failures,last_passwd_change,min_passwd_length,")
         _T("  disabled_until,last_login,xmpp_id,ldap_dn,id) VALUES (?,?,?,?,?,?,?,?,?,?,?,'',?,?,?,?,?,?,?,?)"));
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
   DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, uuid_to_string(m_guid, guidText), DB_BIND_STATIC);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_authMethod);
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_certMappingMethod);
   DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, m_certMappingData, DB_BIND_STATIC);
   DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_authFailures);
   DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, (UINT32)m_lastPasswordChange);
   DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_minPasswordLength);
   DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (UINT32)m_disabledUntil);
   DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, (UINT32)m_lastLogin);
   DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, m_xmppId, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_TEXT, m_userDn, DB_BIND_STATIC);
   DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_id);

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
	TCHAR query[256];
	bool success;

	success = DBBegin(hdb);
	if (success)
	{
		_sntprintf(query, 256, _T("DELETE FROM users WHERE id=%d"), m_id);
		success = DBQuery(hdb, query);
		if (success)
		{
			_sntprintf(query, 256, _T("DELETE FROM user_profiles WHERE user_id=%d"), m_id);
			success = DBQuery(hdb, query);
			if (success)
			{
				_sntprintf(query, 256, _T("DELETE FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
				success = DBQuery(hdb, query);
			}
		}
		if (success)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}
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
	m_graceLogins = MAX_GRACE_LOGINS;
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
		safe_free(m_certMappingData);
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
      safe_free(groups);
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
	m_graceLogins = MAX_GRACE_LOGINS;
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

/*****************************************************************************
 **  Group
 ****************************************************************************/

/**
 * Constructor for group object - create from database
 * Expects fields in the following order:
 *    id,name,system_access,flags,description,guid,ldap_dn
 */
Group::Group(DB_RESULT hr, int row) : UserDatabaseObject(hr, row)
{
	DB_RESULT hResult;
	TCHAR query[256];

	_sntprintf(query, 256, _T("SELECT user_id FROM user_group_members WHERE group_id=%d"), m_id);
   hResult = DBSelect(g_hCoreDB, query);
   if (hResult != NULL)
	{
		m_memberCount = DBGetNumRows(hResult);
		if (m_memberCount > 0)
		{
			m_members = (UINT32 *)malloc(sizeof(UINT32) * m_memberCount);
			for(int i = 0; i < m_memberCount; i++)
				m_members[i] = DBGetFieldULong(hResult, i, 0);
		}
		else
		{
			m_members = NULL;
		}
		DBFreeResult(hResult);
	}

	loadCustomAttributes(g_hCoreDB);
}

/**
 * Constructor for group object - create "Everyone" group
 */
Group::Group() : UserDatabaseObject()
{
	m_id = GROUP_EVERYONE;
	_tcscpy(m_name, _T("Everyone"));
	m_flags = UF_MODIFIED;
	m_systemRights = 0;
	_tcscpy(m_description, _T("Built-in everyone group"));
	uuid_generate(m_guid);
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
 * Destructor for group object
 */
Group::~Group()
{
	safe_free(m_members);
}

/**
 * Save object to database
 */
bool Group::saveToDatabase(DB_HANDLE hdb)
{
	TCHAR guidText[64];

   // Clear modification flag
   m_flags &= ~UF_MODIFIED;

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("user_groups"), _T("id"), m_id))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE user_groups SET name=?,system_access=?,flags=?,description=?,guid=?,ldap_dn=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO user_groups (name,system_access,flags,description,guid,ldap_dn,id) VALUES (?,?,?,?,?,?,?)"));
   }
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, m_systemRights);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, uuid_to_string(m_guid, guidText), DB_BIND_STATIC);
   DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_userDn, DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_id);

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
            hStmt = DBPrepare(hdb, _T("INSERT INTO user_group_members (group_id,user_id) VALUES (?,?)"));
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
	TCHAR query[256];
	bool success;

	success = DBBegin(hdb);
	if (success)
	{
		_sntprintf(query, 256, _T("DELETE FROM user_groups WHERE id=%d"), m_id);
		success = DBQuery(hdb, query);
		if (success)
		{
			_sntprintf(query, 256, _T("DELETE FROM user_group_members WHERE group_id=%d"), m_id);
			success = DBQuery(hdb, query);
			if (success)
			{
				_sntprintf(query, 256, _T("DELETE FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
				success = DBQuery(hdb, query);
			}
		}
		if (success)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}
	return success;
}

/**
 * Check if given user is a member
 */
bool Group::isMember(UINT32 userId)
{
	int i;

	if (m_id == GROUP_EVERYONE)
		return true;

   //This is done not to assign disabled group rights on a enabled user
   if ((m_flags & UF_SYNC_EXCEPTION) || (m_flags & UF_DISABLED))
      return false;

	for(i = 0; i < m_memberCount; i++)
		if (m_members[i] == userId)
			return true;
	return false;
}

/**
 * Add user to group
 */
void Group::addUser(UINT32 userId)
{
	int i;

   // Check if user already in group
   for(i = 0; i < m_memberCount; i++)
      if (m_members[i] == userId)
         return;

   // Not in group, add it
	m_memberCount++;
   m_members = (UINT32 *)realloc(m_members, sizeof(UINT32) * m_memberCount);
   m_members[i] = userId;

	m_flags |= UF_MODIFIED;

   SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
}

/**
 * Delete user from group
 */
void Group::deleteUser(UINT32 userId)
{
   int i;

   for(i = 0; i < m_memberCount; i++)
      if (m_members[i] == userId)
      {
         m_memberCount--;
         memmove(&m_members[i], &m_members[i + 1], sizeof(UINT32) * (m_memberCount - i));
      	m_flags |= UF_MODIFIED;
         SendUserDBUpdate(USER_DB_MODIFY, m_id, this);
         break;
      }
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
            bool found = false;
   			for(int j = 0; j < count; j++)
            {
               if (members[j] == m_members[i])
               {
                  members[j] = 0xFFFFFFFF;    // mark as found
                  found = true;
                  break;
               }
            }

            if (!found)
               SendUserDBUpdate(USER_DB_MODIFY, m_members[i]);  // new member added
         }
			for(i = 0; i < count; i++)
            if (members[i] != 0xFFFFFFFF)  // not present in new list
               SendUserDBUpdate(USER_DB_MODIFY, members[i]);
		}
		else
		{
         m_members = NULL;

         // notify change for all old members
			for(i = 0; i < count; i++)
            SendUserDBUpdate(USER_DB_MODIFY, members[i]);
		}
		safe_free(members);
	}
}
