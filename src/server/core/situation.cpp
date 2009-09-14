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
// Callback for client session enumeration
//

static void SendSituationNotification(ClientSession *pSession, void *pArg)
{
   pSession->onSituationChange((CSCPMessage *)pArg);
}


//
// Notify clients about change in situations
//

static void NotifyClientsOnSituationChange(int code, Situation *st)
{
	CSCPMessage msg;

	msg.SetCode(CMD_SITUATION_CHANGE);
	msg.SetVariable(VID_NOTIFICATION_CODE, (WORD)code);
	st->CreateMessage(&msg);
   EnumerateClientSessions(SendSituationNotification, &msg);
}

	
//
// SituationInstance constructor
//

SituationInstance::SituationInstance(const TCHAR *name, Situation *parent)
{
	m_name = _tcsdup(name);
	m_parent = parent;
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
	m_attributes.set(attribute, value);
}


//
// Get atribute's value
//

const TCHAR *SituationInstance::GetAttribute(const TCHAR *attribute)
{
	return m_attributes.get(attribute);
}


//
// Create NXCP message
//

DWORD SituationInstance::CreateMessage(CSCPMessage *msg, DWORD baseId)
{
	DWORD i, id = baseId;
	
	msg->SetVariable(id++, m_name);
	msg->SetVariable(id++, m_attributes.getSize());
	for(i = 0; i < m_attributes.getSize(); i++)
	{
		msg->SetVariable(id++, m_attributes.getKeyByIndex(i));
		msg->SetVariable(id++, m_attributes.getValueByIndex(i));
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
	DecodeSQLString(m_name);
	m_comments = DBGetField(handle, row, 2, NULL, 0);
	DecodeSQLString(m_comments);
	m_numInstances = 0;
	m_instanceList = NULL;
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
// Save to database
//

void Situation::SaveToDatabase()
{
	DB_RESULT result;
	TCHAR *query, *escName, *escComments;
	BOOL isUpdate;
	
	escName = EncodeSQLString(CHECK_NULL_EX(m_name));
	escComments = EncodeSQLString(CHECK_NULL_EX(m_comments));
	query = (TCHAR *)malloc((_tcslen(escName) + _tcslen(escComments) + 256) * sizeof(TCHAR));
	_stprintf(query, _T("SELECT id FROM situations WHERE id=%d"), m_id);
	result = DBSelect(g_hCoreDB, query);
	if (result != NULL)
	{
		isUpdate = DBGetNumRows(result) > 0;
		DBFreeResult(result);
	}
	else
	{
		isUpdate = FALSE;
	}
	if (isUpdate)
		_stprintf(query, _T("UPDATE situations SET name='%s',comments='%s' WHERE id=%d"),
		          escName, escComments, m_id);
	else
		_stprintf(query, _T("INSERT INTO situations (id,name,comments) VALUES (%d,'%s','%s')"),
		          m_id, escName, escComments);
	free(escName);
	free(escComments);
	DBQuery(g_hCoreDB, query);
	free(query);
}


//
// Delete from database
//

void Situation::DeleteFromDatabase()
{
	TCHAR query[256];
	
	_sntprintf(query, 256, _T("DELETE FROM situations WHERE id=%d"), m_id);
	DBQuery(g_hCoreDB, query);
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
		m_instanceList[i] = new SituationInstance(instance, this);
		m_instanceList[i]->UpdateAttribute(attribute, value);
	}
	
	Unlock();

	NotifyClientsOnSituationChange(SITUATION_UPDATE, this);
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

	if (success)
		NotifyClientsOnSituationChange(SITUATION_UPDATE, this);
	return success;
}


//
// Find situation instance by name
//

SituationInstance *Situation::FindInstance(const TCHAR *name)
{
	int i;
	SituationInstance *instance = NULL;

	Lock();
	
	for(i = 0; i < m_numInstances; i++)
	{
		if (!_tcsicmp(m_instanceList[i]->GetName(), name))
		{
			instance = m_instanceList[i];
			break;
		}
	}
	
	Unlock();
	return instance;
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
	SaveToDatabase();
	Unlock();
	NotifyClientsOnSituationChange(SITUATION_UPDATE, this);
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
// Find situation by name
//

Situation *FindSituationByName(const TCHAR *name)
{
   DWORD i;
   Situation *st = NULL;

   if (m_pSituationIndex == NULL)
      return NULL;

   RWLockReadLock(m_rwlockSituationIndex, INFINITE);
	for(i = 0; i < m_dwSituationIndexSize; i++)
	{
		if (!_tcsicmp(name, ((Situation *)m_pSituationIndex[i].pObject)->GetName()))
		{
			st = (Situation *)m_pSituationIndex[i].pObject;
			break;
		}
	}
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
   st->SaveToDatabase();
	NotifyClientsOnSituationChange(SITUATION_CREATE, st);
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

   RWLockWriteLock(m_rwlockSituationIndex, INFINITE);
   dwPos = SearchIndex(m_pSituationIndex, m_dwSituationIndexSize, id);
   if (dwPos != INVALID_INDEX)
   {
		NotifyClientsOnSituationChange(SITUATION_DELETE, (Situation *)m_pSituationIndex[dwPos].pObject);
   	((Situation *)m_pSituationIndex[dwPos].pObject)->DeleteFromDatabase();
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
	session->sendMessage(msg);
	
	msg->SetCode(CMD_SITUATION_DATA);
	for(i = 0; i < m_dwSituationIndexSize; i++)
	{
		msg->DeleteAllVariables();
		((Situation *)m_pSituationIndex[i].pObject)->CreateMessage(msg);
		session->sendMessage(msg);
	}
	
   RWLockUnlock(m_rwlockSituationIndex);
}


//
// NXSL "Situation" class
//

class NXSL_SituationClass : public NXSL_Class
{
public:
   NXSL_SituationClass();

   virtual NXSL_Value *GetAttr(NXSL_Object *pObject, char *pszAttr);
};


//
// Implementation of "Situation" class
//

NXSL_SituationClass::NXSL_SituationClass()
                    :NXSL_Class()
{
   strcpy(m_szName, "Situation");
}

NXSL_Value *NXSL_SituationClass::GetAttr(NXSL_Object *pObject, char *pszAttr)
{
   SituationInstance *instance;
   NXSL_Value *value = NULL;
	const TCHAR *attrValue;

   instance = (SituationInstance *)pObject->Data();
   if (!strcmp(pszAttr, "name"))
   {
      value = new NXSL_Value(instance->GetParent()->GetName());
   }
   else if (!strcmp(pszAttr, "id"))
   {
      value = new NXSL_Value(instance->GetParent()->GetId());
   }
   else if (!strcmp(pszAttr, "instance"))
   {
      value = new NXSL_Value(instance->GetName());
   }
	else
	{
		attrValue = instance->GetAttribute(pszAttr);
		if (attrValue != NULL)
		{
			value = new NXSL_Value(attrValue);
		}
		else
		{
			value = new NXSL_Value;	// return NULL
		}
	}
   return value;
}


//
// NXSL "Situation" class object
//

static NXSL_SituationClass m_nxslSituationClass;


//
// NXSL function for finding situation
//

static int F_FindSituation(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	Situation *situation;
	SituationInstance *instance;

   if (argv[0]->IsInteger())
   {
		situation = FindSituationById(argv[0]->GetValueAsUInt32());
   }
   else	// First parameter is not a number, assume that it's a name
   {
		situation = FindSituationByName(argv[0]->GetValueAsCString());
   }

	if (situation != NULL)
	{
		instance = situation->FindInstance(argv[1]->GetValueAsCString());
		if (instance != NULL)
		{
			*ppResult = new NXSL_Value(new NXSL_Object(&m_nxslSituationClass, instance));
		}
		else
		{
			*ppResult = new NXSL_Value;	// Instance not found, return NULL
		}
	}
	else
	{
		*ppResult = new NXSL_Value;	// Situation not found, return NULL
	}

   return 0;
}


//
// NXSL function: get situation instance attribute
//

static int F_GetSituationAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
	NXSL_Object *object;
	const TCHAR *attrValue;

	if (!argv[0]->IsObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->IsString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->GetValueAsObject();
	if (_tcscmp(object->Class()->Name(), "Situation"))
		return NXSL_ERR_BAD_CLASS;

	attrValue = ((SituationInstance *)object->Data())->GetAttribute(argv[1]->GetValueAsCString());
	*ppResult = (attrValue != NULL) ? new NXSL_Value(attrValue) : new NXSL_Value;
	return 0;
}


//
// NXSL function set
//

NXSL_ExtFunction g_nxslSituationFunctions[] =
{
   { "FindSituation", F_FindSituation, 2 },
   { "GetSituationAttribute", F_GetSituationAttribute, 2 }
};
DWORD g_nxslNumSituationFunctions = sizeof(g_nxslSituationFunctions) / sizeof(NXSL_ExtFunction);
