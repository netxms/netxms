/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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

/**
 * Maximum number of grace logins allowed for user
 */
#define MAX_GRACE_LOGINS      5

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
   uuid_t m_guid;
	TCHAR m_name[MAX_USER_NAME];
	TCHAR m_description[MAX_USER_DESCR];
	UINT32 m_systemRights;
	UINT32 m_flags;
	StringMap m_attributes;		// Custom attributes

	bool loadCustomAttributes(DB_HANDLE hdb);
	bool saveCustomAttributes(DB_HANDLE hdb);

public:
	UserDatabaseObject();
	UserDatabaseObject(DB_RESULT hResult, int row);
	UserDatabaseObject(UINT32 id, const TCHAR *name);
	virtual ~UserDatabaseObject();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(CSCPMessage *msg);
	virtual void modifyFromMessage(CSCPMessage *msg);

	UINT32 getId() { return m_id; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getDescription() { return m_description; }
	UINT32 getSystemRights() { return m_systemRights; }
	UINT32 getFlags() { return m_flags; }
	TCHAR *getGuidAsText(TCHAR *buffer) { return uuid_to_string(m_guid, buffer); }

	bool isDeleted() { return (m_flags & UF_DELETED) ? true : false; }
	bool isDisabled() { return (m_flags & UF_DISABLED) ? true : false; }
	bool isModified() { return (m_flags & UF_MODIFIED) ? true : false; }

	void setDeleted() { m_flags |= UF_DELETED; }

	const TCHAR *getAttribute(const TCHAR *name) { return m_attributes.get(name); }
	UINT32 getAttributeAsULong(const TCHAR *name);
	void setAttribute(const TCHAR *name, const TCHAR *value) { m_attributes.set(name, value); m_flags |= UF_MODIFIED; }
};

/**
 * User object
 */
class NXCORE_EXPORTABLE User : public UserDatabaseObject
{
protected:
	TCHAR m_fullName[MAX_USER_FULLNAME];
   BYTE m_passwordHash[SHA1_DIGEST_SIZE];
   int m_graceLogins;
   int m_authMethod;
	int m_certMappingMethod;
	TCHAR *m_certMappingData;
	time_t m_disabledUntil;
	time_t m_lastPasswordChange;
	time_t m_lastLogin;
	int m_minPasswordLength;
	int m_authFailures;

public:
	User();
	User(DB_RESULT hResult, int row);
	User(UINT32 id, const TCHAR *name);
	virtual ~User();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(CSCPMessage *msg);
	virtual void modifyFromMessage(CSCPMessage *msg);

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

	bool validatePassword(const TCHAR *password);
	bool validateHashedPassword(const BYTE *password);
	void decreaseGraceLogins() { if (m_graceLogins > 0) m_graceLogins--; m_flags |= UF_MODIFIED; }
	void setPassword(const TCHAR *password, bool clearChangePasswdFlag);
	void increaseAuthFailures();
	void resetAuthFailures() { m_authFailures = 0; m_flags |= UF_MODIFIED; }
	void updateLastLogin() { m_lastLogin = time(NULL); m_flags |= UF_MODIFIED; }
	void updatePasswordChangeTime() { m_lastPasswordChange = time(NULL); m_flags |= UF_MODIFIED; }
	void enable();
	void disable() { m_flags |= UF_DISABLED | UF_MODIFIED; }
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

	virtual void fillMessage(CSCPMessage *msg);
	virtual void modifyFromMessage(CSCPMessage *msg);

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	void addUser(UINT32 userId);
	void deleteUser(UINT32 userId);
	bool isMember(UINT32 userId);
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

   void fillMessage(CSCPMessage *pMsg);
};

/**
 * Functions
 */
BOOL LoadUsers();
void SaveUsers(DB_HANDLE hdb);
void SendUserDBUpdate(int code, UINT32 id, UserDatabaseObject *object);
UINT32 AuthenticateUser(TCHAR *pszName, TCHAR *pszPassword,
							  UINT32 dwSigLen, void *pCert, BYTE *pChallenge,
							  UINT32 *pdwId, UINT32 *pdwSystemRights,
							  bool *pbChangePasswd, bool *pbIntruderLockout);

UINT32 NXCORE_EXPORTABLE SetUserPassword(UINT32 id, const TCHAR *newPassword, const TCHAR *oldPassword, bool changeOwnPassword);
bool NXCORE_EXPORTABLE CheckUserMembership(UINT32 dwUserId, UINT32 dwGroupId);
UINT32 NXCORE_EXPORTABLE DeleteUserDatabaseObject(UINT32 id);
UINT32 NXCORE_EXPORTABLE CreateNewUser(TCHAR *pszName, BOOL bIsGroup, UINT32 *pdwId);
UINT32 NXCORE_EXPORTABLE ModifyUserDatabaseObject(CSCPMessage *msg);
UserDatabaseObject NXCORE_EXPORTABLE **OpenUserDatabase(int *count);
void NXCORE_EXPORTABLE CloseUserDatabase();
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(UINT32 id, const TCHAR *name);
UINT32 NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(UINT32 id, const TCHAR *name);
void NXCORE_EXPORTABLE SetUserDbObjectAttr(UINT32 id, const TCHAR *name, const TCHAR *value);
bool NXCORE_EXPORTABLE ResolveUserId(UINT32 id, TCHAR *buffer, int bufSize);
void DumpUsers(CONSOLE_CTX pCtx);

#endif
