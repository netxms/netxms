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
** File: situation.cpp
**
**/

#include "nxcore.h"


//
// Static data
//

static INDEX *m_pSituationIndex = NULL;
static DWORD m_dwSituationIndexSize = 0;
static RWLOCK m_rwlockSituationIndex;


//
// SituationInstance constructor
//

SituationInstance::SituationInstance(const TCHAR *name)
{
	m_name = _tcsdup(name);
}


//
// SituationInstance destructor
//

SituationInstance::~SituationInstance()
{
	safe_free(m_name);
}


//
// Update atribute
//

void SituationInstance::UpdateAttribute(const TCHAR *attribute, const TCHAR *value)
{
	m_attributes.Set(attribute, value);
}


//
// Create NXCP message
//

DWORD SituationInstance::CreateMessage(CSCPMessage *msg, DWORD baseId)
{
	DWORD i, id = baseId;
	
	msg->SetVariable(id++, m_name);
	msg->SetVariable(id++, m_attributes.Size());
	for(i = 0; i < m_attributes.Size(); i++)
	{
		msg->SetVariable(id++, m_attributes.GetKeyByIndex(i));
		msg->SetVariable(id++, m_attributes.GetValueByIndex(i));
	}
	return id;
}


//
// Situation constructor
//

Situation::Situation(const TCHAR *name)
{
	m_id = CreateUniqueId(IDG_SITUATION);
	m_name = _tcsdup(name);
	m_comments = NULL;
	m_numInstances = 0;
	m_instanceList = NULL;
	m_accessMutex = MutexCreate();
}


//
// Situation constructor for loading from database
// Expected field order: id,name,comments
//

Situation::Situation(DB_RESULT handle, int row)
{
	m_id = DBGetFieldULong(handle, row, 0);
	m_name = DBGetField(handle, row, 1, NULL, 0);
	m_comments = DBGetField(handle, row, 2, NULL, 0);
	m_accessMutex = MutexCreate();
}


//
// Situation destructor
//

Situation::~Situation()
{
	int i;

	safe_free(m_name);
	safe_free(m_comments);
	for(i = 0; i < m_numInstances; i++)
		delete m_instanceList[i];
	safe_free(m_instanceList);
	MutexDestroy(m_accessMutex);
}


//
// Update situation
//

void Situation::UpdateSituation(const TCHAR *instance, const TCHAR *attribute, const TCHAR *value)
{
	int i;

	Lock();
	
	for(i = 0; i < m_numInstances; i++)
	{
		if (!_tcsicmp(m_instanceList[i]->GetName(), instance))
		{
			m_instanceList[i]->UpdateAttribute(attribute, value);
			break;
		}
	}
	
	// Create new instance if needed
	if (i == m_numInstances)
	{
		m_numInstances++;
		m_instanceList = (SituationInstance **)realloc(m_instanceList, sizeof(SituationInstance *) * m_numInstances);
		m_instanceList[i] = new SituationInstance(instance);
		m_instanceList[i]->UpdateAttribute(attribute, value);
	}
	
	Unlock();
}


//
// Delete instance
//

BOOL Situation::DeleteInstance(const TCHAR *instance)
{
	int i;
	BOOL success = FALSE;

	Lock();
	
	for(i = 0; i < m_numInstances; i++)
	{
		if (!_tcsicmp(m_instanceList[i]->GetName(), instance))
		{
			delete m_instanceList[i];
			m_numInstances--;
			memmove(&m_instanceList[i], &m_instanceList[i + 1], sizeof(SituationInstance *) * (m_numInstances - i));
			success = TRUE;
			break;
		}
	}
	
	Unlock();
	return success;
}


//
// Create NXCP message
//

void Situation::CreateMessage(CSCPMessage *msg)
{
	int i;
	DWORD id;
	
	Lock();
	
	msg->SetVariable(VID_SITUATION_ID, m_id);
	msg->SetVariable(VID_NAME, CHECK_NULL_EX(m_name));
	msg->SetVariable(VID_COMMENTS, CHECK_NULL_EX(m_comments));
	msg->SetVariable(VID_INSTANCE_COUNT, (DWORD)m_numInstances);
	
	for(i = 0, id = VID_INSTANCE_LIST_BASE; i < m_numInstances; i++)
	{
		id = m_instanceList[i]->CreateMessage(msg, id);
	}
	
	Unlock();
}


//
// Update situation configuration from NXCP message
//

void Situation::UpdateFromMessage(CSCPMessage *msg)
{
	Lock();
	if (msg->IsVariableExist(VID_NAME))
	{
		safe_free(m_name);
		m_name = msg->GetVariableStr(VID_NAME);
	}
	if (msg->IsVariableExist(VID_COMMENTS))
	{
		safe_free(m_comments);
		m_comments = msg->GetVariableStr(VID_COMMENTS);
	}
	Unlock();
}


//
// Initialize situations
//

BOOL SituationsInit(void)
{
	DWORD i;
	DB_RESULT result;
	BOOL success = TRUE;
	
   m_rwlockSituationIndex = RWLockCreate();
   
   // Load situations from database
   result = DBSelect(g_hCoreDB, _T("SELECT id,name,comments FROM situations ORDER BY id"));
   if (result != NULL)
   {
   	m_dwSituationIndexSize = DBGetNumRows(result);
   	m_pSituationIndex = (INDEX *)realloc(m_pSituationIndex, sizeof(INDEX) * m_dwSituationIndexSize);
   	for(i = 0; i < m_dwSituationIndexSize; i++)
   	{
   		m_pSituationIndex[i].dwKey = DBGetFieldULong(result, i, 0);
   		m_pSituationIndex[i].pObject = (NetObj *)(new Situation(result, i));
		}
   	DBFreeResult(result);
	}
	else
	{
		DbgPrintf(3, _T("Cannot load situations from database due to DBSelect() failure"));
		success = FALSE;
	}
	return success;
}


//
// Find situation by ID
//

Situation *FindSituationById(DWORD id)
{
   DWORD dwPos;
   Situation *st;

   if (m_pSituationIndex == NULL)
      return NULL;

   RWLockReadLock(m_rwlockSituationIndex, INFINITE);
   dwPos = SearchIndex(m_pSituationIndex, m_dwSituationIndexSize, id);
   st = (dwPos == INVALID_INDEX) ? NULL : (Situation *)m_pSituationIndex[dwPos].pObject;
   RWLockUnlock(m_rwlockSituationIndex);
   return st;
}


//
// Create new situation
//

Situation *CreateSituation(const TCHAR *name)
{
	Situation *st;
	
	st = new Situation(name);
   RWLockWriteLock(m_rwlockSituationIndex, INFINITE);
   AddObjectToIndex(&m_pSituationIndex, &m_dwSituationIndexSize, st->GetId(), st);
   RWLockUnlock(m_rwlockSituationIndex);
	return st;
}


//
// Delete situation
//

DWORD DeleteSituation(DWORD id)
{
   DWORD dwPos, rcc;

   if (m_pSituationIndex == NULL)
      return RCC_INVALID_SITUATION_ID;

   RWLockReadLock(m_rwlockSituationIndex, INFINITE);
   dwPos = SearchIndex(m_pSituationIndex, m_dwSituationIndexSize, id);
   if (dwPos != INVALID_INDEX)
   {
   	delete (Situation *)m_pSituationIndex[dwPos].pObject;
   	DeleteObjectFromIndex(&m_pSituationIndex, &m_dwSituationIndexSize, id);
   	rcc = RCC_SUCCESS;
   }
   else
   {
   	rcc = RCC_INVALID_SITUATION_ID;
   }
   RWLockUnlock(m_rwlockSituationIndex);
   return rcc;
}


//
// Send all situations to client
//

void SendSituationListToClient(ClientSession *session, CSCPMessage *msg)
{
	DWORD i;
	
   RWLockReadLock(m_rwlockSituationIndex, INFINITE);
   
	msg->SetVariable(VID_SITUATION_COUNT, m_dwSituationIndexSize);
	session->SendMessage(msg);
	
	msg->SetCode(CMD_SITUATION_DATA);
	for(i = 0; i < m_dwSituationIndexSize; i++)
	{
		msg->DeleteAllVariables();
		((Situation *)m_pSituationIndex[i].pObject)->CreateMessage(msg);
		session->SendMessage(msg);
	}
	
   RWLockUnlock(m_rwlockSituationIndex);
}

