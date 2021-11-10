/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
AutoBindTarget::AutoBindTarget(NetObj *_this) : m_mutexProperties(MutexType::FAST)
{
   m_this = _this;
   memset(m_autoBindFilters, 0, sizeof(m_autoBindFilters));
   memset(m_autoBindFilterSources, 0, sizeof(m_autoBindFilterSources));
   m_autoBindFlags = 0;
}

/**
 * Auto bind object destructor
 */
AutoBindTarget::~AutoBindTarget()
{
   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
   {
      delete m_autoBindFilters[i];
      MemFree(m_autoBindFilterSources[i]);
   }
}

/**
 * Set auto bind mode for this auto bind target
 */
void AutoBindTarget::setAutoBindMode(int filterNumber, bool doBind, bool doUnbind)
{
   if ((filterNumber < 0) || (filterNumber >= MAX_AUTOBIND_TARGET_FILTERS))
      return;

   uint32_t flagMask = (AAF_AUTO_APPLY_1 | AAF_AUTO_REMOVE_1) << filterNumber * 2;
   uint32_t flags = (doBind ? AAF_AUTO_APPLY_1 : 0) | (doUnbind ? AAF_AUTO_REMOVE_1 : 0)  << filterNumber * 2;
   internalLock();
   m_autoBindFlags = (m_autoBindFlags & ~flagMask) | flags;
   internalUnlock();
   m_this->markAsModified(MODIFY_OTHER);
}

/**
 * Set auto bind filter.
 *
 * @param filterNumber filter's number (0 based)
 * @param filter new filter script code or NULL to clear filter
 */
void AutoBindTarget::setAutoBindFilter(int filterNumber, const TCHAR *filter)
{
   if ((filterNumber < 0) || (filterNumber >= MAX_AUTOBIND_TARGET_FILTERS))
      return;

   internalLock();
   MemFree(m_autoBindFilterSources[filterNumber]);
   delete m_autoBindFilters[filterNumber];
   if (filter != nullptr)
   {
      TCHAR error[256];
      m_autoBindFilterSources[filterNumber] = MemCopyString(filter);
      m_autoBindFilters[filterNumber] = NXSLCompile(filter, error, 256, nullptr);
      if (m_autoBindFilters[filterNumber] == nullptr)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("AutoBind::%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), filterNumber);
         PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, 0);
         nxlog_write(NXLOG_WARNING, _T("Failed to compile autobind script #%d for object %s [%u] (%s)"), filterNumber, m_this->getName(), m_this->getId(), error);
      }
   }
   else
   {
      m_autoBindFilterSources[filterNumber] = nullptr;
      m_autoBindFilters[filterNumber] = nullptr;
   }
   internalUnlock();
}

/**
 * Modify object from NXCP message
 */
void AutoBindTarget::modifyFromMessage(const NXCPMessage& request)
{
   if (request.isFieldExist(VID_AUTOBIND_FLAGS))
   {
      internalLock();
      m_autoBindFlags = request.getFieldAsUInt32(VID_AUTOBIND_FLAGS);
      internalUnlock();
   }

   // Change apply filter
   if (request.isFieldExist(VID_AUTOBIND_FILTER))
   {
      TCHAR *filter = request.getFieldAsString(VID_AUTOBIND_FILTER);
      setAutoBindFilter(0, filter);
      MemFree(filter);
   }

   // Change apply filter
   if (request.isFieldExist(VID_AUTOBIND_FILTER_2))
   {
      TCHAR *filter = request.getFieldAsString(VID_AUTOBIND_FILTER_2);
      setAutoBindFilter(1, filter);
      MemFree(filter);
   }
}

/**
 * Create NXCP message with object's data
 */
void AutoBindTarget::fillMessage(NXCPMessage *msg) const
{
   internalLock();
   msg->setField(VID_AUTOBIND_FLAGS, m_autoBindFlags);
   msg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_autoBindFilterSources[0]));
   msg->setField(VID_AUTOBIND_FILTER_2, CHECK_NULL_EX(m_autoBindFilterSources[1]));
   internalUnlock();
}

/**
 * Load object from database
 */
bool AutoBindTarget::loadFromDatabase(DB_HANDLE hdb, uint32_t objectId)
{
   bool success = false;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT bind_filter_1,bind_filter_2,flags FROM auto_bind_target WHERE object_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         TCHAR *filter = DBGetField(hResult, 0, 0, nullptr, 0);
         setAutoBindFilter(0, filter);
         MemFree(filter);

         filter = DBGetField(hResult, 0, 1, nullptr, 0);
         setAutoBindFilter(1, filter);
         MemFree(filter);

         m_autoBindFlags = DBGetFieldULong(hResult, 0, 2);
         DBFreeResult(hResult);
         success = true;
      }
      DBFreeStatement(hStmt);
   }

   return success;
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
      hStmt = DBPrepare(hdb, _T("UPDATE auto_bind_target SET bind_filter_1=?,bind_filter_2=?,flags=? WHERE object_id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO auto_bind_target (bind_filter_1,bind_filter_2,flags,object_id) VALUES (?,?,?,?)"));
   }
   if (hStmt != nullptr)
   {
      internalLock();
      DBBind(hStmt, 1, DB_SQLTYPE_TEXT, m_autoBindFilterSources[0], DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_autoBindFilterSources[1], DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_autoBindFlags);
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
 * Check if object should be automatically applied to given data collection target using given filter.
 * Returns AutoBindDecision_Bind if applicable, AutoBindDecision_Unbind if not,
 * AutoBindDecision_Ignore if no change required (script error or no auto apply)
 */
AutoBindDecision AutoBindTarget::isApplicable(const shared_ptr<NetObj>& target, const shared_ptr<DCObject>& dci, int filterNumber) const
{
   AutoBindDecision result = AutoBindDecision_Ignore;

   if (!isAutoBindEnabled(filterNumber))
      return result;

   NXSL_VM *filter = nullptr;
   internalLock();
   NXSL_Program *filterProgram = m_autoBindFilters[filterNumber];
   if ((filterProgram != nullptr) && !filterProgram->isEmpty())
   {
      filter = CreateServerScriptVM(filterProgram, target);
      if (filter == nullptr)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("AutoBind::%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), filterNumber);
         PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, _T("Script load error"), 0);
         nxlog_write(NXLOG_WARNING, _T("Failed to load autobind script for object %s [%u]"), m_this->getName(), m_this->getId());
      }
   }
   internalUnlock();

   if (filter == nullptr)
      return result;

   filter->setGlobalVariable("$container", m_this->createNXSLObject(filter));
   filter->setGlobalVariable("$service", m_this->createNXSLObject(filter));
   filter->setGlobalVariable("$template", m_this->createNXSLObject(filter));
   if (dci != nullptr)
      filter->setGlobalVariable("$dci", dci->createNXSLObject(filter));
   filter->setUserData(target.get());  // For PollerTrace()
   if (filter->run())
   {
      const NXSL_Value *value = filter->getResult();
      if (!value->isNull())
         result = value->isTrue() ? AutoBindDecision_Bind : AutoBindDecision_Unbind;
   }
   else
   {
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("AutoBind::%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), filterNumber);
      PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), 0);
      nxlog_write(NXLOG_WARNING, _T("Failed to execute autobind script for object %s [%u] (%s)"), m_this->getName(), m_this->getId(), filter->getErrorText());
   }
   delete filter;
   return result;
}

/**
 * Serialize object to JSON
 */
void AutoBindTarget::toJson(json_t *root)
{
   json_t *filters = json_array();
   internalLock();
   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
   {
      json_t *filter = json_object();
      json_object_set_new(filter, "autoBind", json_boolean(isAutoBindEnabled(i)));
      json_object_set_new(filter, "autoUnbind", json_boolean(isAutoUnbindEnabled(i)));
      json_object_set_new(filter, "source", json_string_t(m_autoBindFilterSources[i]));
      json_array_append_new(filters, filter);
   }
   internalUnlock();
   json_object_set_new(root, "autoBindFilters", filters);
}

/**
 * Create export record
 */
void AutoBindTarget::createExportRecord(StringBuffer &str)
{
   internalLock();
   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
   {
      if (m_autoBindFilterSources[i] == nullptr)
         continue;

      str.append(_T("\t\t\t<filter"));
      if (i > 0)
         str.append(i + 1);
      str.append(_T(" autoBind=\""));
      str.append(isAutoBindEnabled(i));
      str.append(_T("\" autoUnbind=\""));
      str.append(isAutoUnbindEnabled(i));
      str.append(_T("\">"));
      str.appendPreallocated(EscapeStringForXML(m_autoBindFilterSources[i], -1));
      str.append(_T("</filter"));
      if (i > 0)
         str.append(i + 1);
      str.append(_T(">\n"));
   }
   internalUnlock();
}

/**
 * Update from import record
 */
void AutoBindTarget::updateFromImport(const ConfigEntry& config, bool defaultAutoBindFlag)
{
   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
   {
      TCHAR entryName[32];
      if (i == 0)
         _tcscpy(entryName, _T("filter"));
      else
         _sntprintf(entryName, 32, _T("filter%d"), i + 1);
      ConfigEntry *filter = config.findEntry(entryName);
      if (filter != nullptr)
      {
         setAutoBindFilter(i, filter->getValue());
         setAutoBindMode(i, filter->getAttributeAsBoolean(_T("autoBind"), defaultAutoBindFlag), filter->getAttributeAsBoolean(_T("autoUnbind")));
      }
      else
      {
         setAutoBindFilter(i, nullptr);
         setAutoBindMode(i, false, false);
      }
   }
}
