/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
      m_autoBindFilterSources[filterNumber] = MemCopyString(filter);
      m_autoBindFilters[filterNumber] = CompileServerScript(filter, SCRIPT_CONTEXT_AUTOBIND, m_this, 0, L"AutoBind::%s::%s::%d", m_this->getObjectClassName(), m_this->getName(), filterNumber);
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
void AutoBindTarget::modifyFromMessage(const NXCPMessage& msg)
{
   if (msg.isFieldExist(VID_AUTOBIND_FLAGS))
   {
      internalLock();
      m_autoBindFlags = msg.getFieldAsUInt32(VID_AUTOBIND_FLAGS);
      internalUnlock();
   }

   // Change apply filter
   if (msg.isFieldExist(VID_AUTOBIND_FILTER))
   {
      TCHAR *filter = msg.getFieldAsString(VID_AUTOBIND_FILTER);
      setAutoBindFilter(0, filter);
      MemFree(filter);
   }

   // Change apply filter
   if (msg.isFieldExist(VID_AUTOBIND_FILTER_2))
   {
      TCHAR *filter = msg.getFieldAsString(VID_AUTOBIND_FILTER_2);
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
   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT bind_filter_1,bind_filter_2,flags FROM auto_bind_target WHERE object_id=?");
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

   static const wchar_t *columns[] = { L"bind_filter_1", L"bind_filter_2", L"flags", nullptr };
   DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"auto_bind_target", L"object_id", m_this->getId(), columns);
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
   return ExecuteQueryOnObject(hdb, m_this->getId(), L"DELETE FROM auto_bind_target WHERE object_id=?");
}

/**
 * Check if object should be automatically applied to given data collection target using given filter.
 * Filter VM will be cached in location pointed by cachedFilterVM and must be destroyed by the caller.
 * Returns AutoBindDecision_Bind if applicable, AutoBindDecision_Unbind if not,
 * AutoBindDecision_Ignore if no change required (script error or no auto apply)
 */
AutoBindDecision AutoBindTarget::isApplicable(NXSL_VM **cachedFilterVM, const shared_ptr<NetObj>& target, const shared_ptr<DCObject>& dci, int filterNumber, NetObj *pollContext) const
{
   AutoBindDecision result = AutoBindDecision_Ignore;

   if (!isAutoBindEnabled(filterNumber))
      return result;

   NXSL_VM *filter = *cachedFilterVM;
   if (filter == nullptr)
   {
      internalLock();
      NXSL_Program *filterProgram = m_autoBindFilters[filterNumber];
      if ((filterProgram != nullptr) && !filterProgram->isEmpty())
      {
         filter = CreateServerScriptVM(filterProgram, target);
         if (filter != nullptr)
         {
            *cachedFilterVM = filter;
         }
         else
         {
            ReportScriptError(SCRIPT_CONTEXT_AUTOBIND, m_this, 0, L"Script load error", L"AutoBind::%s::%s::%d", m_this->getObjectClassName(), m_this->getName(), filterNumber);
         }
      }
      internalUnlock();
   }
   else
   {
      // Update variables in cached VM
      filter->setGlobalVariable("$object", target->createNXSLObject(filter));
      if (target->getObjectClass() == OBJECT_NODE)
         filter->setGlobalVariable("$node", target->createNXSLObject(filter));
      else
         filter->removeGlobalVariable("$node");
      filter->setGlobalVariable("$isCluster", filter->createValue(target->getObjectClass() == OBJECT_CLUSTER));
   }

   if (filter == nullptr)
      return result;

   filter->setGlobalVariable("$container", m_this->createNXSLObject(filter));
   if (m_this->getObjectClass() == OBJECT_DASHBOARD)
      filter->setGlobalVariable("$dashboard", m_this->createNXSLObject(filter));
   if (m_this->getObjectClass() == OBJECT_NETWORKMAP)
      filter->setGlobalVariable("$map", m_this->createNXSLObject(filter));
   if (m_this->getObjectClass() == OBJECT_BUSINESSSERVICE)
      filter->setGlobalVariable("$service", m_this->createNXSLObject(filter));
   if (m_this->getObjectClass() == OBJECT_TEMPLATE || m_this->getObjectClass() == OBJECT_DASHBOARDTEMPLATE)
      filter->setGlobalVariable("$template", m_this->createNXSLObject(filter));
   if (dci != nullptr)
      filter->setGlobalVariable("$dci", dci->createNXSLObject(filter));
   else
      filter->removeGlobalVariable("$dci");
   filter->setUserData((pollContext != nullptr) ? pollContext : target.get());  // For PollerTrace()
   if (filter->run())
   {
      const NXSL_Value *value = filter->getResult();
      if (!value->isNull())
         result = value->isTrue() ? AutoBindDecision_Bind : AutoBindDecision_Unbind;
   }
   else
   {
      ReportScriptError(SCRIPT_CONTEXT_AUTOBIND, m_this, dci != nullptr ? dci->getId() : 0, filter->getErrorText(), L"AutoBind::%s::%s::%d", m_this->getObjectClassName(), m_this->getName(), filterNumber);
   }
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
void AutoBindTarget::createExportRecord(TextFileWriter& xml)
{
   internalLock();
   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
   {
      if (m_autoBindFilterSources[i] == nullptr)
         continue;

      xml.appendUtf8String("\t\t\t<filter");
      if (i > 0)
         xml.append(i + 1);
      xml.appendUtf8String(" autoBind=\"");
      xml.append(isAutoBindEnabled(i));
      xml.appendUtf8String("\" autoUnbind=\"");
      xml.append(isAutoUnbindEnabled(i));
      xml.appendUtf8String("\">");
      xml.appendPreallocated(EscapeStringForXML(m_autoBindFilterSources[i], -1));
      xml.appendUtf8String("</filter");
      if (i > 0)
         xml.append(i + 1);
      xml.appendUtf8String(">\n");
   }
   internalUnlock();
}

/**
 * Update from import record
 */
void AutoBindTarget::updateFromImport(const ConfigEntry& config, bool defaultAutoBindFlag, bool nxslV5)
{
   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
   {
      wchar_t entryName[32];
      if (i == 0)
         wcscpy(entryName, L"filter");
      else
         _sntprintf(entryName, 32, L"filter%d", i + 1);
      ConfigEntry *filter = config.findEntry(entryName);
      if (filter != nullptr)
      {
         if (nxslV5)
            setAutoBindFilter(i, filter->getValue());
         else
         {
            StringBuffer output = NXSLConvertToV5(filter->getValue());
            setAutoBindFilter(i, output);
         }
         setAutoBindMode(i, filter->getAttributeAsBoolean(L"autoBind", defaultAutoBindFlag), filter->getAttributeAsBoolean(L"autoUnbind"));
      }
      else
      {
         setAutoBindFilter(i, nullptr);
         setAutoBindMode(i, false, false);
      }
   }
}

/**
 * Class filter data for object selection for Container and Collector, Template autobind
 */
struct AutoBindClassFilterData
{
   bool processAccessPoints;
   bool processClusters;
   bool processCollectors;
   bool processMobileDevices;
   bool processSensors;
};

/**
 * Object filter for autobind
 */
static bool AutoBindObjectFilter(NetObj* object, AutoBindClassFilterData* filterData)
{
   return (object->getObjectClass() == OBJECT_NODE) ||
         (filterData->processAccessPoints && (object->getObjectClass() == OBJECT_ACCESSPOINT)) ||
         (filterData->processClusters && (object->getObjectClass() == OBJECT_CLUSTER)) ||
         (filterData->processCollectors && (object->getObjectClass() == OBJECT_COLLECTOR)) ||
         (filterData->processMobileDevices && (object->getObjectClass() == OBJECT_MOBILEDEVICE)) ||
         (filterData->processSensors && (object->getObjectClass() == OBJECT_SENSOR));
}

/**
 * Get potential objects for autobind operation
 */
unique_ptr<SharedObjectArray<NetObj>> AutoBindTarget::getObjectsForAutoBind(const TCHAR *configurationSuffix)
{
   AutoBindClassFilterData filterData;
   filterData.processAccessPoints = ConfigReadBoolean(StringBuffer(_T("Objects.AccessPoints.")).append(configurationSuffix), false);
   filterData.processClusters = ConfigReadBoolean(StringBuffer(_T("Objects.Clusters.")).append(configurationSuffix), false);
   filterData.processCollectors = ConfigReadBoolean(StringBuffer(_T("Objects.Collectors.")).append(configurationSuffix), false);
   filterData.processMobileDevices = ConfigReadBoolean(StringBuffer(_T("Objects.MobileDevices.")).append(configurationSuffix), false);
   filterData.processSensors = ConfigReadBoolean(StringBuffer(_T("Objects.Sensors.")).append(configurationSuffix), false);
   return g_idxObjectById.getObjects(AutoBindObjectFilter, &filterData);
}
