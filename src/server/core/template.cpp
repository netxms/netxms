/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: template.cpp
**
**/

#include "nxcore.h"

/**
 * Redefined status calculation for template group
 */
void TemplateGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool TemplateGroup::showThresholdSummary()
{
   return false;
}

/**
 * Create template object from import file
 */
Template::Template(ConfigEntry *config) : DataCollectionOwner(config), AutoBindTarget(this, config), VersionableObject(this, config)
{
   UINT32 flags = config->getSubEntryValueAsUInt(_T("flags"), 0, 0);
   if(flags != 0)
   {
      if(flags & AAF_AUTO_APPLY)
         m_autoBindFlag = true;
      if(flags & AAF_AUTO_REMOVE)
         m_autoUnbindFlag = true;
      flags &= !(AAF_AUTO_APPLY | AAF_AUTO_REMOVE);
   }
   nx_strncpy(m_name, config->getSubEntryValue(_T("name"), 0, _T("Unnamed Template")), MAX_OBJECT_NAME);
}


bool Template::saveToDatabase(DB_HANDLE hdb)
{
   bool success = DataCollectionOwner::saveToDatabase(hdb);
   if(success)
   {
      if ((m_modified & MODIFY_OTHER) && !IsDatabaseRecordExist(hdb, _T("templates"), _T("id"), m_id))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO templates (id) VALUES (?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
      }
   }
   if(success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);
   if(success && (m_modified & MODIFY_OTHER))
      success = VersionableObject::saveToDatabase(hdb);

   // Clear modifications flag
   lockProperties();
   m_modified = 0;
   unlockProperties();

   return success;
}

bool Template::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = DataCollectionOwner::deleteFromDatabase(hdb);
   if(success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM templates WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dct_node_map WHERE template_id=?"));
   if(success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   if(success)
      success = VersionableObject::deleteFromDatabase(hdb);
   return success;
}

bool Template::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   bool success = DataCollectionOwner::loadFromDatabase(hdb, id);

   if(success)
      success = AutoBindTarget::loadFromDatabase(hdb, id);
   if(success)
      success = VersionableObject::loadFromDatabase(hdb, id);
   return success;
}

/**
 * Create management pack record
 */
void Template::createExportRecord(String &str)
{
   TCHAR guid[48];
   str.appendFormattedString(_T("\t\t<template id=\"%d\">\n\t\t\t<guid>%s</guid>\n\t\t\t<name>%s</name>\n\t\t\t<flags>%d</flags>\n"),
                             m_id, m_guid.toString(guid), (const TCHAR *)EscapeStringForXML2(m_name), m_flags);

   // Path in groups
   StringList path;
   ObjectArray<NetObj> *list = getParentList(OBJECT_TEMPLATEGROUP);
   TemplateGroup *parent = NULL;
   while(list->size() > 0)
   {
      parent = (TemplateGroup *)list->get(0);
      path.add(parent->getName());
      delete list;
      list = parent->getParentList(OBJECT_TEMPLATEGROUP);
   }
   delete list;

   str.append(_T("\t\t\t<path>\n"));
   for(int j = path.size() - 1, id = 1; j >= 0; j--, id++)
   {
      str.append(_T("\t\t\t\t<element id=\""));
      str.append(id);
      str.append(_T("\">"));
      str.append(path.get(j));
      str.append(_T("</element>\n"));
   }
   str.append(_T("\t\t\t</path>\n\t\t\t<dataCollection>\n"));

   lockDciAccess(false);
   for(int i = 0; i < m_dcObjects->size(); i++)
      m_dcObjects->get(i)->createExportRecord(str);
   unlockDciAccess();

   str.append(_T("\t\t\t</dataCollection>\n"));
   AutoBindTarget::createExportRecord(str);
   str.append(_T("\t\t</template>\n"));
}

/**
 * Unlock data collection items list
 */
bool Template::unlockDCIList(int sessionId)
{
   if (m_dciListModified)
   {
      if (getObjectClass() == OBJECT_TEMPLATE)
         updateVersion();
      setModified(MODIFY_OTHER);
   }
   return DataCollectionOwner::unlockDCIList(sessionId);
}

/**
 * Create NXCP message with object's data
 */
void Template::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   DataCollectionOwner::fillMessageInternal(pMsg, userId);
   AutoBindTarget::fillMessageInternal(pMsg, userId);
   VersionableObject::fillMessageInternal(pMsg, userId);
}

UINT32 Template::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Change flags
   if (pRequest->isFieldExist(VID_FLAGS))
   {
      UINT32 mask = pRequest->isFieldExist(VID_FLAGS_MASK) ? pRequest->getFieldAsUInt32(VID_FLAGS_MASK) : 0xFFFFFFFF;
      m_flags &= ~mask;
      m_flags |= pRequest->getFieldAsUInt32(VID_FLAGS) & mask;
   }
   AutoBindTarget::modifyFromMessageInternal(pRequest);
   return DataCollectionOwner::modifyFromMessageInternal(pRequest);
}

void Template::updateFromImport(ConfigEntry *config)
{
   DataCollectionOwner::updateFromImport(config);
   AutoBindTarget::updateFromImport(config);
   VersionableObject::updateFromImport(config);
}

/**
 * Serialize object to JSON
 */
json_t *Template::toJson()
{
   json_t *root = DataCollectionOwner::toJson();
   AutoBindTarget::toJson(root);
   VersionableObject::toJson(root);
   return root;
}

/**
 * Called when data collection configuration changed
 */
void Template::onDataCollectionChange()
{
   queueUpdate();
}

/**
 * Prepare template for deletion
 */
void Template::prepareForDeletion()
{
   lockChildList(false);
   for(int i = 0; i < m_childList->size(); i++)
   {
      NetObj *object = m_childList->get(i);
      if (object->isDataCollectionTarget())
         queueRemoveFromTarget(object->getId(), true);
   }
   unlockChildList();
   DataCollectionOwner::prepareForDeletion();
}
