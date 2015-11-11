/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

static ObjectIndex s_idxSituations;


//
// Callback for client session enumeration
//

static void SendSituationNotification(ClientSession *pSession, void *pArg)
{
   pSession->onSituationChange((NXCPMessage *)pArg);
}


//
// Notify clients about change in situations
//

static void NotifyClientsOnSituationChange(int code, Situation *st)
{
	NXCPMessage msg;

	msg.setCode(CMD_SITUATION_CHANGE);
	msg.setField(VID_NOTIFICATION_CODE, (WORD)code);
	st->fillMessage(&msg);
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

/**
 * Get atribute's value
 */
const TCHAR *SituationInstance::GetAttribute(const TCHAR *attribute)
{
	return m_attributes.get(attribute);
}

/**
 * Create NXCP message
 */
UINT32 SituationInstance::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	msg->setField(baseId, m_name);
   m_attributes.fillMessage(msg, baseId + 1, baseId + 2);
   return baseId + m_attributes.size() * 2 + 2;
}

/**
 * Situation constructor
 */
Situation::Situation(const TCHAR *name)
{
	m_id = CreateUniqueId(IDG_SITUATION);
	m_name = _tcsdup(name);
	m_comments = NULL;
	m_numInstances = 0;
	m_instanceList = NULL;
	m_accessMutex = MutexCreate();
}

/**
 * Situation constructor for loading from database
 * Expected field order: id,name,comments
 */
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
	size_t qlen;
	BOOL isUpdate;
	
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	escName = EncodeSQLString(CHECK_NULL_EX(m_name));
	escComments = EncodeSQLString(CHECK_NULL_EX(m_comments));
	qlen = _tcslen(escName) + _tcslen(escComments) + 256;
	query = (TCHAR *)malloc(qlen * sizeof(TCHAR));
	_sntprintf(query, qlen, _T("SELECT id FROM situations WHERE id=%d"), m_id);
	result = DBSelect(hdb, query);
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
		_sntprintf(query, qlen, _T("UPDATE situations SET name='%s',comments='%s' WHERE id=%d"),
		           escName, escComments, m_id);
	else
		_sntprintf(query, qlen, _T("INSERT INTO situations (id,name,comments) VALUES (%d,'%s','%s')"),
		           m_id, escName, escComments);
	free(escName);
	free(escComments);
	DBQuery(hdb, query);
	free(query);

	DBConnectionPoolReleaseConnection(hdb);
}


//
// Delete from database
//

void Situation::DeleteFromDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	TCHAR query[256];
	_sntprintf(query, 256, _T("DELETE FROM situations WHERE id=%d"), m_id);
	DBQuery(hdb, query);
	DBConnectionPoolReleaseConnection(hdb);
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

/**
 * Create NXCP message
 */
void Situation::fillMessage(NXCPMessage *msg)
{
	int i;
	UINT32 id;
	
	Lock();
	
	msg->setField(VID_SITUATION_ID, m_id);
	msg->setField(VID_NAME, CHECK_NULL_EX(m_name));
	msg->setField(VID_COMMENTS, CHECK_NULL_EX(m_comments));
	msg->setField(VID_INSTANCE_COUNT, (UINT32)m_numInstances);
	
	for(i = 0, id = VID_INSTANCE_LIST_BASE; i < m_numInstances; i++)
	{
		id = m_instanceList[i]->fillMessage(msg, id);
	}
	
	Unlock();
}

/**
 * Update situation configuration from NXCP message
 */
void Situation::UpdateFromMessage(NXCPMessage *msg)
{
	Lock();
	if (msg->isFieldExist(VID_NAME))
	{
		safe_free(m_name);
		m_name = msg->getFieldAsString(VID_NAME);
	}
	if (msg->isFieldExist(VID_COMMENTS))
	{
		safe_free(m_comments);
		m_comments = msg->getFieldAsString(VID_COMMENTS);
	}
	SaveToDatabase();
	Unlock();
	NotifyClientsOnSituationChange(SITUATION_UPDATE, this);
}


//
// Initialize situations
//

BOOL SituationsInit()
{
	BOOL success = TRUE;
	
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Load situations from database
   DB_RESULT result = DBSelect(hdb, _T("SELECT id,name,comments FROM situations ORDER BY id"));
   if (result != NULL)
   {
   	int count = DBGetNumRows(result);
   	for(int i = 0; i < count; i++)
   	{
   		Situation *s = new Situation(result, i);
			s_idxSituations.put(s->getId(), (NetObj *)s);
		}
   	DBFreeResult(result);
	}
	else
	{
		DbgPrintf(3, _T("Cannot load situations from database due to DBSelect() failure"));
		success = FALSE;
	}
   DBConnectionPoolReleaseConnection(hdb);
	return success;
}


//
// Find situation by ID
//

Situation *FindSituationById(UINT32 id)
{
	return (Situation *)s_idxSituations.get(id);
}


//
// Find situation by name
//

static bool SituationNameComparator(NetObj *object, void *name)
{
	return !_tcscmp((const TCHAR *)name, ((Situation *)object)->GetName());
}

Situation *FindSituationByName(const TCHAR *name)
{
	return (Situation *)s_idxSituations.find(SituationNameComparator, (void *)name);
}


//
// Create new situation
//

Situation *CreateSituation(const TCHAR *name)
{
	Situation *st;
	
	st = new Situation(name);
	s_idxSituations.put(st->getId(), (NetObj *)st);
   st->SaveToDatabase();
	NotifyClientsOnSituationChange(SITUATION_CREATE, st);
	return st;
}


//
// Delete situation
//

UINT32 DeleteSituation(UINT32 id)
{
   UINT32 rcc;

	Situation *s = (Situation *)s_idxSituations.get(id);
   if (s != NULL)
   {
		s_idxSituations.remove(id);
		NotifyClientsOnSituationChange(SITUATION_DELETE, s);
   	s->DeleteFromDatabase();
   	delete s;
   	rcc = RCC_SUCCESS;
   }
   else
   {
   	rcc = RCC_INVALID_SITUATION_ID;
   }
   return rcc;
}

/**
 * Send all situations to client
 */
void SendSituationListToClient(ClientSession *session, NXCPMessage *msg)
{
	ObjectArray<NetObj> *list = s_idxSituations.getObjects(false);

	msg->setField(VID_SITUATION_COUNT, (UINT32)list->size());
	session->sendMessage(msg);
	
	msg->setCode(CMD_SITUATION_DATA);
	for(int i = 0; i < list->size(); i++)
	{
		msg->deleteAllFields();
		((Situation *)list->get(i))->fillMessage(msg);
		session->sendMessage(msg);
	}

	delete list;
}

/**
 * NXSL "Situation" class
 */
class NXSL_SituationClass : public NXSL_Class
{
public:
   NXSL_SituationClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
};


//
// Implementation of "Situation" class
//

NXSL_SituationClass::NXSL_SituationClass()
                    :NXSL_Class()
{
   _tcscpy(m_name, _T("Situation"));
}

NXSL_Value *NXSL_SituationClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   SituationInstance *instance;
   NXSL_Value *value = NULL;
	const TCHAR *attrValue;

   instance = (SituationInstance *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("name")))
   {
      value = new NXSL_Value(instance->GetParent()->GetName());
   }
   else if (!_tcscmp(pszAttr, _T("id")))
   {
      value = new NXSL_Value(instance->GetParent()->getId());
   }
   else if (!_tcscmp(pszAttr, _T("instance")))
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

static int F_FindSituation(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	Situation *situation;
	SituationInstance *instance;

   if (argv[0]->isInteger())
   {
		situation = FindSituationById(argv[0]->getValueAsUInt32());
   }
   else	// First parameter is not a number, assume that it's a name
   {
		situation = FindSituationByName(argv[0]->getValueAsCString());
   }

	if (situation != NULL)
	{
		instance = situation->FindInstance(argv[1]->getValueAsCString());
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

static int F_GetSituationAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	NXSL_Object *object;
	const TCHAR *attrValue;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), m_nxslSituationClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	attrValue = ((SituationInstance *)object->getData())->GetAttribute(argv[1]->getValueAsCString());
	*ppResult = (attrValue != NULL) ? new NXSL_Value(attrValue) : new NXSL_Value;
	return 0;
}


//
// NXSL function set
//

NXSL_ExtFunction g_nxslSituationFunctions[] =
{
   { _T("FindSituation"), F_FindSituation, 2 },
   { _T("GetSituationAttribute"), F_GetSituationAttribute, 2 }
};
UINT32 g_nxslNumSituationFunctions = sizeof(g_nxslSituationFunctions) / sizeof(NXSL_ExtFunction);
