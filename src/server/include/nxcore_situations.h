/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: nxcore_situations.h
**
**/

#ifndef _nxcore_situations_h_
#define _nxcore_situations_h_


//
// Situation instance object
//

class Situation;

class SituationInstance
{
private:
	Situation *m_parent;
	TCHAR *m_name;
	StringMap m_attributes;

public:
	SituationInstance(const TCHAR *name, Situation *parent);
	~SituationInstance();

	const TCHAR *GetName() { return m_name; }
	Situation *GetParent() { return m_parent; }
	
	DWORD CreateMessage(CSCPMessage *msg, DWORD baseId);

	const TCHAR *GetAttribute(const TCHAR *attribute);
	void UpdateAttribute(const TCHAR *attribute, const TCHAR *value);
};


//
// Situation object
//

class Situation
{
private:
	DWORD m_id;
	TCHAR *m_name;
	TCHAR *m_comments;
	int m_numInstances;
	SituationInstance **m_instanceList;
	MUTEX m_accessMutex;
	
	void Lock() { MutexLock(m_accessMutex); }
	void Unlock() { MutexUnlock(m_accessMutex); }

public:
	Situation(const TCHAR *name);
	Situation(DB_RESULT handle, int row);
	~Situation();
	
	void SaveToDatabase();
	void DeleteFromDatabase();
	
	DWORD GetId() { return m_id; }
	const TCHAR *GetName() { return m_name; }
	
	void CreateMessage(CSCPMessage *msg);
	void UpdateFromMessage(CSCPMessage *msg);

	void UpdateSituation(const TCHAR *instance, const TCHAR *attribute, const TCHAR *value);
	BOOL DeleteInstance(const TCHAR *instance);
	SituationInstance *FindInstance(const TCHAR *name);
};


//
// Functions
//

BOOL SituationsInit(void);
Situation *FindSituationById(DWORD id);
Situation *FindSituationByName(const TCHAR *name);
Situation *CreateSituation(const TCHAR *name);
DWORD DeleteSituation(DWORD id);
void SendSituationListToClient(ClientSession *session, CSCPMessage *msg);


#endif
