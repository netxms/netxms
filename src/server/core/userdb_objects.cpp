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


/*****************************************************************************
 **  UserDatabaseObject
 ****************************************************************************/


//
// Constructor for generic user database object - create from database
// Expects fields in the following order:
//    id,name,system_access,flags,description,guid
//

UserDatabaseObject::UserDatabaseObject(DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	DBGetField(hResult, row, 1, m_name, MAX_USER_NAME);
	m_systemRights = DBGetFieldULong(hResult, row, 2);
	m_flags = DBGetFieldULong(hResult, row, 3);
	DBGetField(hResult, row, 4, m_description, MAX_USER_DESCR);
	DecodeSQLString(m_description);
	DBGetFieldGUID(hResult, row, 5, m_guid);
}


//
// Default constructor for generic object
//

UserDatabaseObject::UserDatabaseObject()
{
}


//
// Condtructor for generic object - create new object with given id and name
//

UserDatabaseObject::UserDatabaseObject(UINT32 id, const TCHAR *name)
{
	m_id = id;
	uuid_generate(m_guid);
	nx_strncpy(m_name, name, MAX_USER_NAME);
	m_systemRights = 0;
	m_description[0] = 0;
	m_flags = UF_MODIFIED;
}


//
// Destructor for generic user database object
//

UserDatabaseObject::~UserDatabaseObject()
{
}


//
// Save object to database
//

bool UserDatabaseObject::saveToDatabase(DB_HANDLE hdb)
{
	return false;
}


//
// Delete object from database
//

bool UserDatabaseObject::deleteFromDatabase(DB_HANDLE hdb)
{
	return false;
}


//
// Fill NXCP message with object data
//

void UserDatabaseObject::fillMessage(CSCPMessage *msg)
{
	UINT32 i, varId;

   msg->SetVariable(VID_USER_ID, m_id);
   msg->SetVariable(VID_USER_NAME, m_name);
   msg->SetVariable(VID_USER_FLAGS, (WORD)m_flags);
   msg->SetVariable(VID_USER_SYS_RIGHTS, (UINT64)m_systemRights);
   msg->SetVariable(VID_USER_DESCRIPTION, m_description);
   msg->SetVariable(VID_GUID, m_guid, UUID_LENGTH);
	msg->SetVariable(VID_NUM_CUSTOM_ATTRIBUTES, m_attributes.getSize());
	for(i = 0, varId = VID_CUSTOM_ATTRIBUTES_BASE; i < m_attributes.getSize(); i++)
	{
		msg->SetVariable(varId++, m_attributes.getKeyByIndex(i));
		msg->SetVariable(varId++, m_attributes.getValueByIndex(i));
	}
}


//
// Modify object from NXCP message
//

void UserDatabaseObject::modifyFromMessage(CSCPMessage *msg)
{
	UINT32 flags, fields;

	fields = msg->GetVariableLong(VID_FIELDS);
	DbgPrintf(5, _T("UserDatabaseObject::modifyFromMessage(): id=%d fields=%08X"), m_id, fields);

	if (fields & USER_MODIFY_DESCRIPTION)
	   msg->GetVariableStr(VID_USER_DESCRIPTION, m_description, MAX_USER_DESCR);
	if (fields & USER_MODIFY_LOGIN_NAME)
	   msg->GetVariableStr(VID_USER_NAME, m_name, MAX_USER_NAME);

	// Update custom attributes only if VID_NUM_CUSTOM_ATTRIBUTES exist -
	// older client versions may not be aware of custom attributes
	if ((fields & USER_MODIFY_CUSTOM_ATTRIBUTES) || msg->IsVariableExist(VID_NUM_CUSTOM_ATTRIBUTES))
	{
		UINT32 i, varId, count;
		TCHAR *name, *value;

		count = msg->GetVariableLong(VID_NUM_CUSTOM_ATTRIBUTES);
		m_attributes.clear();
		for(i = 0, varId = VID_CUSTOM_ATTRIBUTES_BASE; i < count; i++)
		{
			name = msg->GetVariableStr(varId++);
			value = msg->GetVariableStr(varId++);
			m_attributes.setPreallocated((name != NULL) ? name : _tcsdup(_T("")), (value != NULL) ? value : _tcsdup(_T("")));
		}
	}

	if ((m_id != 0) && (fields & USER_MODIFY_ACCESS_RIGHTS))
		m_systemRights = (UINT32)msg->GetVariableInt64(VID_USER_SYS_RIGHTS);

	if (fields & USER_MODIFY_FLAGS)
	{
	   flags = msg->GetVariableShort(VID_USER_FLAGS);
		// Modify only UF_DISABLED, UF_CHANGE_PASSWORD, and UF_CANNOT_CHANGE_PASSWORD flags from message
		// Ignore all but CHANGE_PASSWORD flag for superuser and "everyone" group
		m_flags &= ~(UF_DISABLED | UF_CHANGE_PASSWORD | UF_CANNOT_CHANGE_PASSWORD);
		if ((m_id == 0) || (m_id == GROUP_EVERYONE))
			m_flags |= flags & UF_CHANGE_PASSWORD;
		else
			m_flags |= flags & (UF_DISABLED | UF_CHANGE_PASSWORD | UF_CANNOT_CHANGE_PASSWORD);
	}

	m_flags |= UF_MODIFIED;
}


//
// Load custom attributes from database
//

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
			if (attrName != NULL)
				DecodeSQLString(attrName);
			else
				attrName = _tcsdup(_T(""));

			attrValue = DBGetField(hResult, i, 1, NULL, 0);
			if (attrValue != NULL)
				DecodeSQLString(attrValue);
			else
				attrValue = _tcsdup(_T(""));

			m_attributes.setPreallocated(attrName, attrValue);
		}
		DBFreeResult(hResult);
		success = true;
	}
	return success;
}


//
// Save custom attributes to database
//

bool UserDatabaseObject::saveCustomAttributes(DB_HANDLE hdb)
{
	TCHAR query[8192], *escName, *escValue;
	UINT32 i;
	bool success = false;

	_sntprintf(query, 256, _T("DELETE FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
	if (DBQuery(hdb, query))
	{
		for(i = 0; i < m_attributes.getSize(); i++)
		{
			escName = EncodeSQLString(m_attributes.getKeyByIndex(i));
			escValue = EncodeSQLString(m_attributes.getValueByIndex(i));
			_sntprintf(query, 8192, _T("INSERT INTO userdb_custom_attributes (object_id,attr_name,attr_value) VALUES (%d,'%s','%s')"),
			           m_id, escName, escValue);
			free(escName);
			free(escValue);
			if (!DBQuery(hdb, query))
				break;
		}
		success = (i == m_attributes.getSize());
	}
	return success;
}


//
// Get custom attribute as UINT32
//

UINT32 UserDatabaseObject::getAttributeAsULong(const TCHAR *name)
{
	const TCHAR *value = getAttribute(name);
	return (value != NULL) ? _tcstoul(value, NULL, 0) : 0;
}


/*****************************************************************************
 **  User
 ****************************************************************************/


//
// Constructor for user object - create from database
// Expects fields in the following order:
//    id,name,system_access,flags,description,guid,password,full_name,
//    grace_logins,auth_method,cert_mapping_method,cert_mapping_data,
//    auth_failures,last_passwd_change,min_passwd_length,disabled_until,
//    last_login
//

User::User(DB_RESULT hResult, int row)
     :UserDatabaseObject(hResult, row)
{
	TCHAR buffer[256];

	if (StrToBin(DBGetField(hResult, row, 6, buffer, 256), m_passwordHash, SHA1_DIGEST_SIZE) != SHA1_DIGEST_SIZE)
	{
		nxlog_write(MSG_INVALID_SHA1_HASH, EVENTLOG_WARNING_TYPE, "s", m_name);
		CalculateSHA1Hash((BYTE *)"netxms", 6, m_passwordHash);
	}

	DBGetField(hResult, row, 7, m_fullName, MAX_USER_FULLNAME);
	DecodeSQLString(m_fullName);

	m_graceLogins = DBGetFieldLong(hResult, row, 8);
	m_authMethod = DBGetFieldLong(hResult, row, 9);
	m_certMappingMethod = DBGetFieldLong(hResult, row, 10);
	
	m_certMappingData = DBGetField(hResult, row, 11, NULL, 0);
	if (m_certMappingData != NULL)
		DecodeSQLString(m_certMappingData);

	m_authFailures = DBGetFieldLong(hResult, row, 12);
	m_lastPasswordChange = (time_t)DBGetFieldLong(hResult, row, 13);
	m_minPasswordLength = DBGetFieldLong(hResult, row, 14);
	m_disabledUntil = (time_t)DBGetFieldLong(hResult, row, 15);
	m_lastLogin = (time_t)DBGetFieldLong(hResult, row, 16);

	// Set full system access for superuser
	if (m_id == 0)
		m_systemRights = SYSTEM_ACCESS_FULL;

	loadCustomAttributes(g_hCoreDB);
}


//
// Constructor for user object - create default superuser
//

User::User()
{
	m_id = 0;
	_tcscpy(m_name, _T("admin"));
	m_flags = UF_MODIFIED | UF_CHANGE_PASSWORD;
	m_systemRights = SYSTEM_ACCESS_FULL;
	m_fullName[0] = 0;
	_tcscpy(m_description, _T("Built-in system administrator account"));
	CalculateSHA1Hash((BYTE *)"netxms", 6, m_passwordHash);
	m_graceLogins = MAX_GRACE_LOGINS;
	m_authMethod = AUTH_NETXMS_PASSWORD;
	uuid_generate(m_guid);
	m_certMappingMethod = 0;
	m_certMappingData = NULL;
	m_authFailures = 0;
	m_lastPasswordChange = 0;
	m_minPasswordLength = -1;	// Use system-wide default
	m_disabledUntil = 0;
	m_lastLogin = 0;
}


//
// Constructor for user object - new user
//

User::User(UINT32 id, const TCHAR *name)
     : UserDatabaseObject(id, name)
{
	m_fullName[0] = 0;
	m_graceLogins = MAX_GRACE_LOGINS;
	m_authMethod = AUTH_NETXMS_PASSWORD;
	m_certMappingMethod = USER_MAP_CERT_BY_SUBJECT;
	m_certMappingData = NULL;
	CalculateSHA1Hash((BYTE *)"", 0, m_passwordHash);
	m_authFailures = 0;
	m_lastPasswordChange = 0;
	m_minPasswordLength = -1;	// Use system-wide default
	m_disabledUntil = 0;
	m_lastLogin = 0;
}


//
// Destructor for user object
//

User::~User()
{
	safe_free(m_certMappingData);
}


//
// Save object to database
//

bool User::saveToDatabase(DB_HANDLE hdb)
{
   DB_RESULT hResult;
	TCHAR query[4096], password[SHA1_DIGEST_SIZE * 2 + 1], guidText[64], *escData, *escFullName, *escDescr;
   bool userExists = false;

   // Clear modification flag
   m_flags &= ~UF_MODIFIED;

   // Check if user record exists in database
   _sntprintf(query, 4096, _T("SELECT id FROM users WHERE id=%d"), m_id);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) != 0)
         userExists = true;
      DBFreeResult(hResult);
   }

   // Create or update record in database
   BinToStr(m_passwordHash, SHA1_DIGEST_SIZE, password);
	escData = EncodeSQLString(CHECK_NULL_EX(m_certMappingData));
	escFullName = EncodeSQLString(m_fullName);
	escDescr = EncodeSQLString(m_description);
   if (userExists)
      _sntprintf(query, 4096, _T("UPDATE users SET name='%s',password='%s',system_access=%d,flags=%d,")
                              _T("full_name='%s',description='%s',grace_logins=%d,guid='%s',")
							         _T("auth_method=%d,cert_mapping_method=%d,cert_mapping_data='%s',")
										_T("auth_failures=%d,last_passwd_change=%d,min_passwd_length=%d,")
										_T("disabled_until=%d,last_login=%d WHERE id=%d"),
              m_name, password, m_systemRights, m_flags, escFullName,
              escDescr, m_graceLogins, uuid_to_string(m_guid, guidText),
              m_authMethod, m_certMappingMethod, escData, m_authFailures, (int)m_lastPasswordChange,
				  m_minPasswordLength, (int)m_disabledUntil, (int)m_lastLogin, m_id);
   else
      _sntprintf(query, 4096, _T("INSERT INTO users (id,name,password,system_access,flags,full_name,")
		                        _T("description,grace_logins,guid,auth_method,cert_mapping_method,")
                              _T("cert_mapping_data,password_history,auth_failures,last_passwd_change,")
										_T("min_passwd_length,disabled_until,last_login) VALUES ")
										_T("(%d,'%s','%s',%d,%d,'%s','%s',%d,'%s',%d,%d,'%s','%s',%d,%d,%d,%d,%d)"),
              m_id, m_name, password, m_systemRights, m_flags,
              escFullName, escDescr, m_graceLogins, uuid_to_string(m_guid, guidText),
              m_authMethod, m_certMappingMethod, escData, password,
				  m_authFailures, (int)m_lastPasswordChange, m_minPasswordLength,
				  (int)m_disabledUntil, (int)m_lastLogin);
	free(escData);
	free(escFullName);
	free(escDescr);

	BOOL rc = DBBegin(hdb);
	if (rc)
	{
		rc = DBQuery(hdb, query);
		if (rc)
		{
			rc = saveCustomAttributes(hdb);
		}
		if (rc)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}
	return rc ? true : false;
}


//
// Delete object from database
//

bool User::deleteFromDatabase(DB_HANDLE hdb)
{
	TCHAR query[256];
	BOOL rc;

	rc = DBBegin(hdb);
	if (rc)
	{
		_sntprintf(query, 256, _T("DELETE FROM users WHERE id=%d"), m_id);
		rc = DBQuery(hdb, query);
		if (rc)
		{
			_sntprintf(query, 256, _T("DELETE FROM user_profiles WHERE user_id=%d"), m_id);
			rc = DBQuery(hdb, query);
			if (rc)
			{
				_sntprintf(query, 256, _T("DELETE FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
				rc = DBQuery(hdb, query);
			}
		}
		if (rc)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}
	return rc ? true : false;
}


//
// Validate user's password
// For non-UNICODE build, password must be UTF-8 encoded
//

bool User::validatePassword(const TCHAR *password)
{
   BYTE hash[SHA1_DIGEST_SIZE];

#ifdef UNICODE
	char mbPassword[1024];
	WideCharToMultiByte(CP_UTF8, 0, password, -1, mbPassword, 1024, NULL, NULL);
	mbPassword[1023] = 0;
	CalculateSHA1Hash((BYTE *)mbPassword, strlen(mbPassword), hash);
#else
	CalculateSHA1Hash((BYTE *)password, strlen(password), hash);
#endif
	return !memcmp(hash, m_passwordHash, SHA1_DIGEST_SIZE);
}

bool User::validateHashedPassword(const BYTE *password)
{
	return !memcmp(password, m_passwordHash, SHA1_DIGEST_SIZE);
}

/**
 * Set user's password
 * For non-UNICODE build, password must be UTF-8 encoded
 */
void User::setPassword(const TCHAR *password, bool clearChangePasswdFlag)
{
#ifdef UNICODE
	char mbPassword[1024];
	WideCharToMultiByte(CP_UTF8, 0, password, -1, mbPassword, 1024, NULL, NULL);
	mbPassword[1023] = 0;
	CalculateSHA1Hash((BYTE *)mbPassword, strlen(mbPassword), m_passwordHash);
#else
	CalculateSHA1Hash((BYTE *)password, strlen(password), m_passwordHash);
#endif
	m_graceLogins = MAX_GRACE_LOGINS;
	m_flags |= UF_MODIFIED;
	if (clearChangePasswdFlag)
		m_flags &= ~UF_CHANGE_PASSWORD;
}

/**
 * Fill NXCP message with user data
 */
void User::fillMessage(CSCPMessage *msg)
{
	UserDatabaseObject::fillMessage(msg);

   msg->SetVariable(VID_USER_FULL_NAME, m_fullName);
   msg->SetVariable(VID_AUTH_METHOD, (WORD)m_authMethod);
	msg->SetVariable(VID_CERT_MAPPING_METHOD, (WORD)m_certMappingMethod);
	msg->SetVariable(VID_CERT_MAPPING_DATA, CHECK_NULL_EX(m_certMappingData));
	msg->SetVariable(VID_LAST_LOGIN, (UINT32)m_lastLogin);
	msg->SetVariable(VID_LAST_PASSWORD_CHANGE, (UINT32)m_lastPasswordChange);
	msg->SetVariable(VID_MIN_PASSWORD_LENGTH, (WORD)m_minPasswordLength);
	msg->SetVariable(VID_DISABLED_UNTIL, (UINT32)m_disabledUntil);
	msg->SetVariable(VID_AUTH_FAILURES, (UINT32)m_authFailures);
}

/**
 * Modify user object from NXCP message
 */
void User::modifyFromMessage(CSCPMessage *msg)
{
	UserDatabaseObject::modifyFromMessage(msg);

	UINT32 fields = msg->GetVariableLong(VID_FIELDS);

	if (fields & USER_MODIFY_FULL_NAME)
		msg->GetVariableStr(VID_USER_FULL_NAME, m_fullName, MAX_USER_FULLNAME);
	if (fields & USER_MODIFY_AUTH_METHOD)
	   m_authMethod = msg->GetVariableShort(VID_AUTH_METHOD);
	if (fields & USER_MODIFY_PASSWD_LENGTH)
	   m_minPasswordLength = msg->GetVariableShort(VID_MIN_PASSWORD_LENGTH);
	if (fields & USER_MODIFY_TEMP_DISABLE)
	   m_disabledUntil = (time_t)msg->GetVariableLong(VID_DISABLED_UNTIL);
	if (fields & USER_MODIFY_CERT_MAPPING)
	{
		m_certMappingMethod = msg->GetVariableShort(VID_CERT_MAPPING_METHOD);
		safe_free(m_certMappingData);
		m_certMappingData = msg->GetVariableStr(VID_CERT_MAPPING_DATA);
	}
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
}


/*****************************************************************************
 **  Group
 ****************************************************************************/


//
// Constructor for group object - create from database
// Expects fields in the following order:
//    id,name,system_access,flags,description,guid
//

Group::Group(DB_RESULT hr, int row)
      :UserDatabaseObject(hr, row)
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


//
// Constructor for group object - create "Everyone" group
//

Group::Group()
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


//
// Constructor for group object - create new group
//

Group::Group(UINT32 id, const TCHAR *name)
      :UserDatabaseObject(id, name)
{
	m_memberCount = 0;
	m_members = NULL;
}


//
// Destructor for group object
//

Group::~Group()
{
	safe_free(m_members);
}


//
// Save object to database
//

bool Group::saveToDatabase(DB_HANDLE hdb)
{
   DB_RESULT hResult;
	TCHAR query[4096], guidText[64], *escDescr;
   bool groupExists = false;
	BOOL rc;

   // Clear modification flag
   m_flags &= ~UF_MODIFIED;

   // Check if group record exists in database
   _sntprintf(query, 4096, _T("SELECT name FROM user_groups WHERE id=%d"), m_id);
   hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) != 0)
         groupExists = true;
      DBFreeResult(hResult);
   }

   // Create or update record in database
   escDescr = EncodeSQLString(m_description);
   if (groupExists)
      _sntprintf(query, 4096, _T("UPDATE user_groups SET name='%s',system_access=%d,flags=%d,")
                              _T("description='%s',guid='%s' WHERE id=%d"),
              m_name, m_systemRights, m_flags, escDescr,
              uuid_to_string(m_guid, guidText), m_id);
   else
      _sntprintf(query, 4096, _T("INSERT INTO user_groups (id,name,system_access,flags,description,guid) ")
                              _T("VALUES (%d,'%s',%d,%d,'%s','%s')"),
              m_id, m_name, m_systemRights, m_flags, escDescr,
              uuid_to_string(m_guid, guidText));
   free(escDescr);

   rc = DBBegin(hdb);
	if (rc)
	{
		rc = DBQuery(hdb,query);
		if (rc)
		{
			if (groupExists)
			{
				_sntprintf(query, 4096, _T("DELETE FROM user_group_members WHERE group_id=%d"), m_id);
				rc = DBQuery(hdb, query);
			}

			if (rc)
			{
				for(int i = 0; (i < m_memberCount) && rc; i++)
				{
					_sntprintf(query, 4096, _T("INSERT INTO user_group_members (group_id, user_id) VALUES (%d, %d)"),
							     m_id, m_members[i]);
					rc = DBQuery(hdb, query);
					if (rc)
					{
						rc = saveCustomAttributes(hdb);
					}
				}
			}
		}
		if (rc)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}

	return rc ? true : false;
}


//
// Delete object from database
//

bool Group::deleteFromDatabase(DB_HANDLE hdb)
{
	TCHAR query[256];
	BOOL rc;

	rc = DBBegin(hdb);
	if (rc)
	{
		_sntprintf(query, 256, _T("DELETE FROM user_groups WHERE id=%d"), m_id);
		rc = DBQuery(hdb, query);
		if (rc)
		{
			_sntprintf(query, 256, _T("DELETE FROM user_group_members WHERE group_id=%d"), m_id);
			rc = DBQuery(hdb, query);
			if (rc)
			{
				_sntprintf(query, 256, _T("DELETE FROM userdb_custom_attributes WHERE object_id=%d"), m_id);
				rc = DBQuery(hdb, query);
			}
		}
		if (rc)
			DBCommit(hdb);
		else
			DBRollback(hdb);
	}
	return rc ? true : false;
}


//
// Check if given user is a member
//

bool Group::isMember(UINT32 userId)
{
	int i;

	if (m_id == GROUP_EVERYONE)
		return true;

	for(i = 0; i < m_memberCount; i++)
		if (m_members[i] == userId)
			return true;
	return false;
}


//
// Add user to group
//

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
}


//
// Delete user from group
//

void Group::deleteUser(UINT32 userId)
{
   int i;

   for(i = 0; i < m_memberCount; i++)
      if (m_members[i] == userId)
      {
         m_memberCount--;
         memmove(&m_members[i], &m_members[i + 1], sizeof(UINT32) * (m_memberCount - i));
      }
	m_flags |= UF_MODIFIED;
}


//
// Fill NXCP message with user group data
//

void Group::fillMessage(CSCPMessage *msg)
{
   UINT32 varId;
	int i;

	UserDatabaseObject::fillMessage(msg);

   msg->SetVariable(VID_NUM_MEMBERS, (UINT32)m_memberCount);
   for(i = 0, varId = VID_GROUP_MEMBER_BASE; i < m_memberCount; i++, varId++)
      msg->SetVariable(varId, m_members[i]);
}


//
// Modify group object from NXCP message
//

void Group::modifyFromMessage(CSCPMessage *msg)
{
	int i;
	UINT32 varId, fields;

	UserDatabaseObject::modifyFromMessage(msg);

	fields = msg->GetVariableLong(VID_FIELDS);
	if (fields & USER_MODIFY_MEMBERS)
	{
		m_memberCount = msg->GetVariableLong(VID_NUM_MEMBERS);
		if (m_memberCount > 0)
		{
			m_members = (UINT32 *)realloc(m_members, sizeof(UINT32) * m_memberCount);
			for(i = 0, varId = VID_GROUP_MEMBER_BASE; i < m_memberCount; i++, varId++)
				m_members[i] = msg->GetVariableLong(varId);
		}
		else
		{
			safe_free_and_null(m_members);
		}
	}
}
