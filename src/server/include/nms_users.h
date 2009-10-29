/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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


//
// Maximum number of grace logins allowed for user
//

#define MAX_GRACE_LOGINS      5


//
// Authentication methods
//

#define AUTH_NETXMS_PASSWORD  0
#define AUTH_RADIUS           1
#define AUTH_CERTIFICATE      2


//
// Generic user database object
//

class NXCORE_EXPORTABLE UserDatabaseObject
{
protected:
	DWORD m_id;
   uuid_t m_guid;
	TCHAR m_name[MAX_USER_NAME];
	TCHAR m_description[MAX_USER_DESCR];
	DWORD m_systemRights;
	DWORD m_flags;
	StringMap m_attributes;		// Custom attributes

	bool loadCustomAttributes(DB_HANDLE hdb);
	bool saveCustomAttributes(DB_HANDLE hdb);

public:
	UserDatabaseObject();
	UserDatabaseObject(DB_RESULT hResult, int row);
	UserDatabaseObject(DWORD id, const TCHAR *name);
	virtual ~UserDatabaseObject();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(CSCPMessage *msg);
	virtual void modifyFromMessage(CSCPMessage *msg);

	DWORD getId() { return m_id; }
	const TCHAR *getName() { return m_name; }
	const TCHAR *getDescription() { return m_description; }
	DWORD getSystemRights() { return m_systemRights; }
	DWORD getFlags() { return m_flags; }
	TCHAR *getGuidAsText(TCHAR *buffer) { return uuid_to_string(m_guid, buffer); }

	bool isDeleted() { return (m_flags & UF_DELETED) ? true : false; }
	bool isDisabled() { return (m_flags & UF_DISABLED) ? true : false; }
	bool isModified() { return (m_flags & UF_MODIFIED) ? true : false; }

	void setDeleted() { m_flags |= UF_DELETED; }

	const TCHAR *getAttribute(const TCHAR *name) { return m_attributes.get(name); }
	DWORD getAttributeAsULong(const TCHAR *name);
	void setAttribute(const TCHAR *name, const TCHAR *value) { m_attributes.set(name, value); m_flags |= UF_MODIFIED; }
};


//
// User object
//

class NXCORE_EXPORTABLE User : public UserDatabaseObject
{
protected:
	TCHAR m_fullName[MAX_USER_FULLNAME];
   BYTE m_passwordHash[SHA1_DIGEST_SIZE];
   int m_graceLogins;
   int m_authMethod;
	int m_certMappingMethod;
	TCHAR *m_certMappingData;

public:
	User();
	User(DB_RESULT hResult, int row);
	User(DWORD id, const TCHAR *name);
	virtual ~User();

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	virtual void fillMessage(CSCPMessage *msg);
	virtual void modifyFromMessage(CSCPMessage *msg);

	const TCHAR *getFullName() { return m_fullName; }
	int getGraceLogins() { return m_graceLogins; }
	int getAuthMethod() { return m_authMethod; }
	int getCertMappingMethod() { return m_certMappingMethod; }
	const TCHAR *getCertMappingData() { return m_certMappingData; }

	bool validatePassword(const TCHAR *password);
	void decreaseGraceLogins() { if (m_graceLogins > 0) m_graceLogins--; }
	void setPassword(BYTE *passwordHash, bool clearChangePasswdFlag);
};


//
// Group object
//

class NXCORE_EXPORTABLE Group : public UserDatabaseObject
{
protected:
	int m_memberCount;
	DWORD *m_members;

public:
	Group();
	Group(DB_RESULT hResult, int row);
	Group(DWORD id, const TCHAR *name);
	virtual ~Group();

	virtual void fillMessage(CSCPMessage *msg);
	virtual void modifyFromMessage(CSCPMessage *msg);

	virtual bool saveToDatabase(DB_HANDLE hdb);
	virtual bool deleteFromDatabase(DB_HANDLE hdb);

	void addUser(DWORD userId);
	void deleteUser(DWORD userId);
	bool isMember(DWORD userId);
};


//
// Access list element structure
//

typedef struct
{
   DWORD dwUserId;
   DWORD dwAccessRights;
} ACL_ELEMENT;


//
// Access list class
//

class AccessList
{
private:
   DWORD m_dwNumElements;
   ACL_ELEMENT *m_pElements;
   MUTEX m_hMutex;

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

public:
   AccessList();
   ~AccessList();

   BOOL GetUserRights(DWORD dwUserId, DWORD *pdwAccessRights);
   void AddElement(DWORD dwUserId, DWORD dwAccessRights);
   BOOL DeleteElement(DWORD dwUserId);
   void DeleteAll(void);

   void EnumerateElements(void (* pHandler)(DWORD, DWORD, void *), void *pArg);

   void CreateMessage(CSCPMessage *pMsg);
};


//
// Functions
//

BOOL LoadUsers(void);
void SaveUsers(DB_HANDLE hdb);
void SendUserDBUpdate(int code, DWORD id, UserDatabaseObject *object);
DWORD AuthenticateUser(TCHAR *pszName, TCHAR *pszPassword,
							  DWORD dwSigLen, void *pCert, BYTE *pChallenge,
							  DWORD *pdwId, DWORD *pdwSystemRights,
							  BOOL *pbChangePasswd);

DWORD NXCORE_EXPORTABLE SetUserPassword(DWORD dwId, BYTE *pszPassword, BOOL bResetChPasswd);
bool NXCORE_EXPORTABLE CheckUserMembership(DWORD dwUserId, DWORD dwGroupId);
DWORD NXCORE_EXPORTABLE DeleteUserDatabaseObject(DWORD id);
DWORD NXCORE_EXPORTABLE CreateNewUser(TCHAR *pszName, BOOL bIsGroup, DWORD *pdwId);
DWORD NXCORE_EXPORTABLE ModifyUserDatabaseObject(CSCPMessage *msg);
UserDatabaseObject NXCORE_EXPORTABLE **OpenUserDatabase(int *count);
void NXCORE_EXPORTABLE CloseUserDatabase();
const TCHAR NXCORE_EXPORTABLE *GetUserDbObjectAttr(DWORD id, const TCHAR *name);
DWORD NXCORE_EXPORTABLE GetUserDbObjectAttrAsULong(DWORD id, const TCHAR *name);
void NXCORE_EXPORTABLE SetUserDbObjectAttr(DWORD id, const TCHAR *name, const TCHAR *value);
bool GetUserName(DWORD id, TCHAR *buffer, int bufSize);
void DumpUsers(CONSOLE_CTX pCtx);

#endif
