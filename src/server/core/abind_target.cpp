/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Raden Solutions
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
** File: abind_target.cpp
**
**/

#include "nxcore.h"

/**
 * Auto bind object constructor
 */
AutoBindTarget::AutoBindTarget(NetObj *_this)
{
   m_this = _this;
   m_bindFilter = NULL;
   m_bindFilterSource = NULL;
   m_mutexProperties = MutexCreate();
   m_autoBindFlag = false;
   m_autoUnbindFlag = false;
}

/**
 * Auto bind object constructor
 */
AutoBindTarget::AutoBindTarget(NetObj *_this, ConfigEntry *config)
{
   m_this = _this;
   m_bindFilter = NULL;
   m_bindFilterSource = NULL;
   m_mutexProperties = MutexCreate();

   ObjectArray<ConfigEntry> *tmp = config->getSubEntries(_T("filter"));
   if(tmp->size() > 0)
   {
     m_autoBindFlag = tmp->get(0)->getAttributeAsBoolean(_T("autoBind"));
     m_autoUnbindFlag = tmp->get(0)->getAttributeAsBoolean(_T("autoUnbind"));
   }
   delete tmp;
   setAutoBindFilter(config->getSubEntryValue(_T("filter")));
}

/**
 * Auto bind object destructor
 */
AutoBindTarget::~AutoBindTarget()
{
   delete m_bindFilter;
   MemFree(m_bindFilterSource);
   MutexDestroy(m_mutexProperties);
}

/**
 * Set auto bind mode for container
 */
void AutoBindTarget::setAutoBindMode(bool doBind, bool doUnbind)
{
   internalLock();
   m_autoBindFlag = doBind;
   m_autoUnbindFlag = doUnbind;
   internalUnlock();
   m_this->markAsModified(MODIFY_OTHER);
}

/**
 * Set auto bind filter.
 *
 * @param filter new filter script code or NULL to clear filter
 */
void AutoBindTarget::setAutoBindFilter(const TCHAR *filter)
{
   internalLock();
   MemFree(m_bindFilterSource);
   delete m_bindFilter;
   if (filter != NULL)
   {
      TCHAR error[256];
      m_bindFilterSource = MemCopyString(filter);
      m_bindFilter = NXSLCompile(m_bindFilterSource, error, 256, NULL);
      if (m_bindFilter == NULL)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), m_this->getId());
         PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_this->getId());
         nxlog_write(NXLOG_WARNING, _T("Failed to compile autobind script for object %s [%u] (%s)"), m_this->getName(), m_this->getId(), error);
      }
   }
   else
   {
      m_bindFilterSource = NULL;
      m_bindFilter = NULL;
   }
   internalUnlock();
}

/**
 * Modify object from NXCP message
 */
void AutoBindTarget::modifyFromMessage(NXCPMessage *request)
{
   internalLock();
   if (request->isFieldExist(VID_AUTOBIND_FLAG))
   {
      m_autoBindFlag = request->getFieldAsBoolean(VID_AUTOBIND_FLAG);
   }
   if (request->isFieldExist(VID_AUTOUNBIND_FLAG))
   {
      m_autoUnbindFlag = request->getFieldAsBoolean(VID_AUTOUNBIND_FLAG);
   }
   internalUnlock();

   // Change apply filter
   if (request->isFieldExist(VID_AUTOBIND_FILTER))
   {
      TCHAR *filter = request->getFieldAsString(VID_AUTOBIND_FILTER);
      setAutoBindFilter(filter);
      MemFree(filter);
   }
}

/**
 * Create NXCP message with object's data
 */
void AutoBindTarget::fillMessage(NXCPMessage *msg)
{
   internalLock();
   msg->setField(VID_AUTOBIND_FLAG, m_autoBindFlag);
   msg->setField(VID_AUTOUNBIND_FLAG, m_autoUnbindFlag);
   msg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_bindFilterSource));
   internalUnlock();
}

/**
 * Load object from database
 */
bool AutoBindTarget::loadFromDatabase(DB_HANDLE hdb, UINT32 objectId)
{
   TCHAR szQuery[256];

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT bind_filter,bind_flag,unbind_flag FROM auto_bind_target WHERE object_id=%d"), objectId);
   DB_RESULT hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;

   TCHAR *filter = DBGetField(hResult, 0, 0, NULL, 0);
   setAutoBindFilter(filter);
   MemFree(filter);

   m_autoBindFlag = DBGetFieldLong(hResult, 0, 1) ? true : false;
   m_autoUnbindFlag = DBGetFieldLong(hResult, 0, 2) ? true : false;
   DBFreeResult(hResult);
   return true;
}

/**
 * Save object to database
 */
bool AutoBindTarget::saveToDatabase(DB_HANDLE hdb)
{
   bool success = false;

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("auto_bind_target"), _T("object_id"), m_this->getId()))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE auto_bind_target SET bind_filter=?,bind_flag=?,unbind_flag=? WHERE object_id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO auto_bind_target (bind_filter,bind_flag,unbind_flag,object_id) VALUES (?,?,?,?)"));
   }
   if (hStmt != NULL)
   {
      internalLock();
      DBBind(hStmt, 1, DB_SQLTYPE_TEXT, m_bindFilterSource, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, (m_autoBindFlag?_T("1"):_T("0")), DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, (m_autoUnbindFlag?_T("1"):_T("0")), DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_this->getId());
      success = DBExecute(hStmt);
      internalUnlock();
      DBFreeStatement(hStmt);
   }

   return success;
}

/**
 * Delete object from database
 */
bool AutoBindTarget::deleteFromDatabase(DB_HANDLE hdb)
{
   return ExecuteQueryOnObject(hdb, m_this->getId(), _T("DELETE FROM auto_bind_target WHERE object_id=?"));
}

/**
 * Check if object should be automatically applied to given data collection target
 * Returns AutoBindDecision_Bind if applicable, AutoBindDecision_Unbind if not,
 * AutoBindDecision_Ignore if no change required (script error or no auto apply)
 */
AutoBindDecision AutoBindTarget::isApplicable(DataCollectionTarget *target)
{
   AutoBindDecision result = AutoBindDecision_Ignore;

   NXSL_VM *filter = NULL;
   internalLock();
   if (m_autoBindFlag && (m_bindFilter != NULL))
   {
      filter = CreateServerScriptVM(m_bindFilter, target);
      if (filter == NULL)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), m_this->getId());
         PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, _T("Script load error"), m_this->getId());
         nxlog_write(NXLOG_WARNING, _T("Failed to load autobind script for object %s [%u]"), m_this->getName(), m_this->getId());
      }
   }
   internalUnlock();

   if (filter == NULL)
      return result;

   if (filter->run())
   {
      NXSL_Value *value = filter->getResult();
      if (!value->isNull())
         result = value->isTrue() ? AutoBindDecision_Bind : AutoBindDecision_Unbind;
   }
   else
   {
      internalLock();
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), m_this->getId());
      PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_this->getId());
      nxlog_write(NXLOG_WARNING, _T("Failed to execute autobind script for object %s [%u] (%s)"), m_this->getName(), m_this->getId(), filter->getErrorText());
      internalUnlock();
   }
   delete filter;
   return result;
}

/**
 * Serialize object to JSON
 */
void AutoBindTarget::toJson(json_t *root)
{
   internalLock();
   json_object_set_new(root, "autoBind", json_boolean(m_autoBindFlag));
   json_object_set_new(root, "autoUnbind", json_boolean(m_autoUnbindFlag));
   json_object_set_new(root, "applyFilter", json_string_t(m_bindFilterSource));
   internalUnlock();
}

/**
 * Create export record
 */
void AutoBindTarget::createExportRecord(String &str)
{
   internalLock();
   if (m_bindFilterSource != NULL)
   {
      str.append(_T("\t\t\t<filter autoBind=\""));
      str.append(m_autoBindFlag);
      str.append(_T("\" autoUnbind=\""));
      str.append(m_autoUnbindFlag);
      str.append(_T("\">"));
      str.appendPreallocated(EscapeStringForXML(m_bindFilterSource, -1));
      str.append(_T("</filter>\n"));
   }
   internalUnlock();
}

/**
 * Update from import record
 */
void AutoBindTarget::updateFromImport(ConfigEntry *config)
{
   setAutoBindFilter(config->getSubEntryValue(_T("filter")));
   ObjectArray<ConfigEntry> *tmp = config->getSubEntries(_T("filter"));
   if(tmp->size() > 0)
   {
      internalLock();
      m_autoBindFlag = tmp->get(0)->getAttributeAsBoolean(_T("autoBind"));
      m_autoUnbindFlag = tmp->get(0)->getAttributeAsBoolean(_T("autoUnbind"));
      internalUnlock();
   }
   delete tmp;
}
