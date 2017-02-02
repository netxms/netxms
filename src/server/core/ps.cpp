/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Raden Solutions
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
** File: script.cpp
**
**/

#include "nxcore.h"

/**
 * Storage
 */
static StringMap s_persistentStorage;
static MUTEX s_lockPStorage;

/**
 * Lists for database update
 */
static StringMap *s_valueDeleteList;
static StringMap *s_valueSetList;

/**
 * NXSL persistent storage
 */
NXSL_PersistentStorage g_nxslPstorage;

/**
 * Load storage
 */
void PersistentStorageInit()
{
   s_lockPStorage = MutexCreate();
   s_valueDeleteList = new StringMap();
   s_valueSetList = new StringMap();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelect(hdb, _T("SELECT entry_key,value FROM persistent_storage"));
   if (result != NULL)
   {
      int count = DBGetNumRows(result);
      for(int i = 0; i < count; i++)
      {
         s_persistentStorage.setPreallocated(DBGetField(result, i, 0, NULL, 0), DBGetField(result, i, 1, NULL, 0));
      }
      DBFreeResult(result);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

void PersistentStorageDestroy()
{
   MutexDestroy(s_lockPStorage);
   delete s_valueDeleteList;
   delete s_valueSetList;
   s_persistentStorage.clear();
}

/**
 * Update persistent storage value by key or add it if not existing
 */
void SetPersistentStorageValue(const TCHAR *key, const TCHAR *value)
{
   if(key == NULL)
      return;
   MutexLock(s_lockPStorage);
   s_persistentStorage.set(key, value == NULL ? _T("") : value);
   s_valueSetList->set(key, value == NULL ? _T("") : value);
   s_valueDeleteList->remove(key);
   MutexUnlock(s_lockPStorage);
}

/**
 * Delete persistent storage value by key
 */
bool DeletePersistentStorageValue(const TCHAR *key)
{
   if(key == NULL)
      return false;
   MutexLock(s_lockPStorage);
   bool success = s_persistentStorage.contains(key);
   if(success)
   {
      s_persistentStorage.remove(key);
      s_valueSetList->remove(key);
      s_valueDeleteList->set(key, _T(""));

   }
   MutexUnlock(s_lockPStorage);
   return success;
}

/**
 * Get persistent storage value by key
 */
const TCHAR *GetPersistentStorageValue(const TCHAR *key)
{
   if(key == NULL)
      return NULL;
   MutexLock(s_lockPStorage);
   const TCHAR *value = s_persistentStorage.get(key);
   MutexUnlock(s_lockPStorage);
   return value;
}

/**
 * Set all persistent storage info to message
 */
void GetPersistentStorageList(NXCPMessage *msg)
{
   MutexLock(s_lockPStorage);
   s_persistentStorage.fillMessage(msg, VID_NUM_PSTORAGE, VID_PSTORAGE_LIST_BASE);
   MutexUnlock(s_lockPStorage);
}

struct PsCbContainer
{
   void *ptr;
   UINT32 watchdogId;
};

/**
 * Callback for persistent storage value delete form database
 */
static EnumerationCallbackResult DeletePSValueCB(const TCHAR *key, const void *value, void *data)
{
   WatchdogNotify(((PsCbContainer *)data)->watchdogId);
   DB_STATEMENT hStmt = (DB_STATEMENT)((PsCbContainer *)data)->ptr;
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
   return DBExecute(hStmt) ? _CONTINUE : _STOP;
}

/**
 * Callback for persistent storage value set to database
 */
static EnumerationCallbackResult SetPSValueCB(const TCHAR *key, const void *value, void *data)
{
   WatchdogNotify(((PsCbContainer *)data)->watchdogId);
   DB_HANDLE hdb = (DB_HANDLE)((PsCbContainer *)data)->ptr;
   bool success = true;
   bool isNew = true;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT value FROM persistent_storage WHERE entry_key=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
      DB_RESULT result = DBSelectPrepared(hStmt);
      if(result != NULL)
      {
         isNew = DBGetNumRows(result) == 0;
         DBFreeResult(result);
      }
      else
      {
         success = false;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      success = false;
   }

   if (success)
   {
      if (isNew)
         hStmt = DBPrepare(hdb, _T("INSERT INTO persistent_storage (value,entry_key) VALUES (?,?)"));
      else
         hStmt = DBPrepare(hdb, _T("UPDATE persistent_storage SET value=? WHERE entry_key=?"));

      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, (TCHAR *)value, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   return success ? _CONTINUE : _STOP;
}

/**
 * Callback for persistent storage value delete form database
 */
static EnumerationCallbackResult MoveToPreviousListCB(const TCHAR *key, const void *value, void *data)
{
   StringMap *mapToMove = (StringMap*)data;
   if (!s_valueDeleteList->contains(key) && !s_valueSetList->contains(key))
      mapToMove->set(key, (TCHAR *)value);
   return _CONTINUE;
}

/**
 * Update persistent storage in database
 */
void UpdatePStorageDatabase(DB_HANDLE hdb, UINT32 watchdogId)
{
   if (s_valueDeleteList->size() == 0 && s_valueSetList->size() == 0) //do nothing if there are no updates
      return;

   DBBegin(hdb);
   bool success = false;

   StringMap *tmpDeleteList;
   StringMap *tmpSetList;
   MutexLock(s_lockPStorage);
   tmpDeleteList = s_valueDeleteList;
   s_valueDeleteList = new StringMap();
   tmpSetList = s_valueSetList;
   s_valueSetList = new StringMap();
   MutexUnlock(s_lockPStorage);

   if (tmpDeleteList->size()> 0)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM persistent_storage WHERE entry_key=?"));
      if (hStmt != NULL)
      {
         PsCbContainer container;
         container.watchdogId = watchdogId;
         container.ptr = hStmt;
         success = _CONTINUE == tmpDeleteList->forEach(DeletePSValueCB, &container);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (tmpSetList->size() > 0)
   {
      PsCbContainer container;
      container.watchdogId = watchdogId;
      container.ptr = hdb;
      success = _CONTINUE == tmpSetList->forEach(SetPSValueCB, &container);
   }

   if (success)
   {
      DBCommit(hdb);
   }
   else
   {
      DBRollback(hdb);
      MutexLock(s_lockPStorage);
      tmpDeleteList->forEach(MoveToPreviousListCB, s_valueDeleteList);
      tmpSetList->forEach(MoveToPreviousListCB, s_valueSetList);
      //move all updates to previous list if this item is not yet updated
      MutexUnlock(s_lockPStorage);
   }

   delete tmpDeleteList;
   delete tmpSetList;
}

/*****************************************
 * NXSL persistent storage implementation
 *****************************************/


void NXSL_PersistentStorage::write(const TCHAR *name, NXSL_Value *value)
{
   if(!value->isNull())
   {
      SetPersistentStorageValue(name, value->getValueAsCString());
   }
   else
   {
      DeletePersistentStorageValue(name);
   }
   delete value;
}

NXSL_Value *NXSL_PersistentStorage::read(const TCHAR *name)
{
   return new NXSL_Value(GetPersistentStorageValue(name));
}

void NXSL_PersistentStorage::remove(const TCHAR *name)
{
   DeletePersistentStorageValue(name);
}

/*********************************
 * Backported code for situations
 *********************************/

/**
 * Situation info
 */
class SistuationInfo
{
private:
   TCHAR *situationName;
   TCHAR *instanceName;
public:
   ~SistuationInfo() { free(situationName);  free(instanceName); }
   SistuationInfo(const TCHAR *situation, const TCHAR *instance) { situationName  = _tcsdup(situation); instanceName = _tcsdup(instance); }
   const TCHAR *getSituationName() { return situationName; }
   const TCHAR *getInstanceName() { return instanceName; }
};

/**
 * NXSL "Situation" class
 */
class NXSL_SituationClass : public NXSL_Class
{
public:
   NXSL_SituationClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
	virtual void onObjectDelete(NXSL_Object *object);
};


/**
 * Implementation of "Situation" class
 */
NXSL_SituationClass::NXSL_SituationClass()
                    :NXSL_Class()
{
   setName(_T("Situation"));
}

void NXSL_SituationClass::onObjectDelete(NXSL_Object *object)
{
   delete (SistuationInfo *)object->getData();
}

NXSL_Value *NXSL_SituationClass::getAttr(NXSL_Object *pObject, const TCHAR *pszAttr)
{
   SistuationInfo *info;
   NXSL_Value *value = NULL;

   info = (SistuationInfo *)pObject->getData();
   if (!_tcscmp(pszAttr, _T("name")))
   {
      value = new NXSL_Value(info->getSituationName());
   }
   else if (!_tcscmp(pszAttr, _T("instance")))
   {
      value = new NXSL_Value(info->getInstanceName());
   }
	else
	{
	   String key;
      key.append(info->getSituationName());
      key.append(_T("."));
      key.append(info->getInstanceName());
      key.append(_T("."));
      key.append(pszAttr);

      const TCHAR *attrValue = GetPersistentStorageValue((const TCHAR *)key);
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

/**
 * NXSL "Situation" class object
 */
static NXSL_SituationClass m_nxslSituationClass;


/**
 * NXSL function for finding situation
 */
static int F_FindSituation(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
   if (!argv[0]->isString() || !argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

   const TCHAR *situationName = argv[0]->getValueAsCString();
	const TCHAR *instanceName = argv[1]->getValueAsCString();

   if (situationName != NULL && instanceName != NULL)
   {
      *ppResult = new NXSL_Value(new NXSL_Object(&m_nxslSituationClass, new SistuationInfo(situationName, instanceName)));
   }
   else
   {
      *ppResult = new NXSL_Value;
   }

   return 0;
}


/**
 * NXSL function: get situation instance attribute
 */
static int F_GetSituationAttribute(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm)
{
	NXSL_Object *object;

	if (!argv[0]->isObject())
		return NXSL_ERR_NOT_OBJECT;

	if (!argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	object = argv[0]->getValueAsObject();
	if (_tcscmp(object->getClass()->getName(), m_nxslSituationClass.getName()))
		return NXSL_ERR_BAD_CLASS;

	SistuationInfo *info = ((SistuationInfo *)object->getData());
	String key;
	key.append(info->getSituationName());
	key.append(_T("."));
	key.append(info->getInstanceName());
	key.append(_T("."));
	key.append(argv[1]->getValueAsCString());

	const TCHAR *attrValue = GetPersistentStorageValue((const TCHAR *)key);
	*ppResult = (attrValue != NULL) ? new NXSL_Value(attrValue) : new NXSL_Value;
	return 0;
}


/**
 * NXSL function set
 */
NXSL_ExtFunction g_nxslSituationFunctions[] =
{
   { _T("FindSituation"), F_FindSituation, 2 },
   { _T("GetSituationAttribute"), F_GetSituationAttribute, 2 }
};
UINT32 g_nxslNumSituationFunctions = sizeof(g_nxslSituationFunctions) / sizeof(NXSL_ExtFunction);

