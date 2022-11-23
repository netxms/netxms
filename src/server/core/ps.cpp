/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: ps.cpp
**
**/

#include "nxcore.h"
#include <nxcore_ps.h>

/**
 * Storage
 */
static StringMap s_persistentStorage;
static Mutex s_lockPStorage(MutexType::FAST);

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

/**
 * Destroy NXSL persistent storage
 */
void PersistentStorageDestroy()
{
   delete s_valueDeleteList;
   delete s_valueSetList;
   s_persistentStorage.clear();
}

/**
 * Update persistent storage value by key or add it if not existing
 */
void SetPersistentStorageValue(const TCHAR *key, const TCHAR *value)
{
   if (key == NULL)
      return;

   TCHAR tempKey[128];
   if (_tcslen(key) > 127)
   {
      _tcslcpy(tempKey, key, 128);
      key = tempKey;
   }

   s_lockPStorage.lock();
   s_persistentStorage.set(key, CHECK_NULL_EX(value));
   s_valueSetList->set(key, CHECK_NULL_EX(value));
   s_valueDeleteList->remove(key);
   s_lockPStorage.unlock();
}

/**
 * Delete persistent storage value by key
 */
bool DeletePersistentStorageValue(const TCHAR *key)
{
   if (key == NULL)
      return false;

   TCHAR tempKey[128];
   if (_tcslen(key) > 127)
   {
      _tcslcpy(tempKey, key, 128);
      key = tempKey;
   }

   s_lockPStorage.lock();
   bool success = s_persistentStorage.contains(key);
   if (success)
   {
      s_persistentStorage.remove(key);
      s_valueSetList->remove(key);
      s_valueDeleteList->set(key, _T(""));
   }
   s_lockPStorage.unlock();
   return success;
}

/**
 * Get persistent storage value by key
 */
SharedString GetPersistentStorageValue(const TCHAR *key)
{
   if (key == nullptr)
      return nullptr;

   TCHAR tempKey[128];
   if (_tcslen(key) > 127)
   {
      _tcslcpy(tempKey, key, 128);
      key = tempKey;
   }

   s_lockPStorage.lock();
   SharedString value(s_persistentStorage.get(key));
   s_lockPStorage.unlock();
   return value;
}

/**
 * Set all persistent storage info to message
 */
void GetPersistentStorageList(NXCPMessage *msg)
{
   s_lockPStorage.lock();
   s_persistentStorage.fillMessage(msg, VID_PSTORAGE_LIST_BASE, VID_NUM_PSTORAGE);
   s_lockPStorage.unlock();
}

/**
 * Callback for persistent storage value delete form database
 */
static EnumerationCallbackResult DeletePSValueCB(const TCHAR *key, const TCHAR *value, std::pair<DB_STATEMENT, uint32_t> *context)
{
   WatchdogNotify(context->second);
   DBBind(context->first, 1, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
   return DBExecute(context->first) ? _CONTINUE : _STOP;
}

/**
 * Callback for persistent storage value set to database
 */
static EnumerationCallbackResult SetPSValueCB(const TCHAR *key, const TCHAR *value, std::pair<DB_HANDLE, uint32_t> *context)
{
   WatchdogNotify(context->second);

   DB_STATEMENT hStmt =
      IsDatabaseRecordExist(context->first, _T("persistent_storage"), _T("entry_key"), key) ?
         DBPrepare(context->first, _T("UPDATE persistent_storage SET value=? WHERE entry_key=?")) :
         DBPrepare(context->first, _T("INSERT INTO persistent_storage (value,entry_key) VALUES (?,?)"));

   bool success;
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   else
   {
      success = false;
   }

   return success ? _CONTINUE : _STOP;
}

/**
 * Callback for persistent storage value delete form database
 */
static EnumerationCallbackResult MoveToPreviousListCB(const TCHAR *key, const TCHAR *value, StringMap *moveMap)
{
   if (!s_valueDeleteList->contains(key) && !s_valueSetList->contains(key))
      moveMap->set(key, value);
   return _CONTINUE;
}

/**
 * Update persistent storage in database
 */
void UpdatePStorageDatabase(DB_HANDLE hdb, uint32_t watchdogId)
{
   if (s_valueDeleteList->isEmpty() && s_valueSetList->isEmpty()) //do nothing if there are no updates
      return;

   DBBegin(hdb);
   bool success = false;

   StringMap *tmpDeleteList;
   StringMap *tmpSetList;
   s_lockPStorage.lock();
   tmpDeleteList = s_valueDeleteList;
   s_valueDeleteList = new StringMap();
   tmpSetList = s_valueSetList;
   s_valueSetList = new StringMap();
   s_lockPStorage.unlock();

   if (!tmpDeleteList->isEmpty())
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM persistent_storage WHERE entry_key=?"));
      if (hStmt != nullptr)
      {
         std::pair<DB_STATEMENT, uint32_t> context(hStmt, watchdogId);
         success = _CONTINUE == tmpDeleteList->forEach(DeletePSValueCB, &context);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (!tmpSetList->isEmpty())
   {
      std::pair<DB_HANDLE, uint32_t> context(hdb, watchdogId);
      success = _CONTINUE == tmpSetList->forEach(SetPSValueCB, &context);
   }

   if (success)
   {
      DBCommit(hdb);
   }
   else
   {
      DBRollback(hdb);
      s_lockPStorage.lock();
      tmpDeleteList->forEach(MoveToPreviousListCB, s_valueDeleteList);
      tmpSetList->forEach(MoveToPreviousListCB, s_valueSetList);
      //move all updates to previous list if this item is not yet updated
      s_lockPStorage.unlock();
   }

   delete tmpDeleteList;
   delete tmpSetList;
}

/*****************************************
 * NXSL persistent storage implementation
 *****************************************/

/**
 * Write value to storage
 */
void NXSL_PersistentStorage::write(const TCHAR *name, NXSL_Value *value)
{
   if (!value->isNull())
   {
      SetPersistentStorageValue(name, value->getValueAsCString());
   }
   else
   {
      DeletePersistentStorageValue(name);
   }
}

/**
 * Read from persistent storage
 */
NXSL_Value *NXSL_PersistentStorage::read(const TCHAR *name, NXSL_ValueManager *vm)
{
   SharedString value = GetPersistentStorageValue(name);
   return !value.isNull() ? vm->createValue(value) : vm->createValue();
}

/**
 * Remove persistent storage entry
 */
void NXSL_PersistentStorage::remove(const TCHAR *name)
{
   DeletePersistentStorageValue(name);
}
