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
** File: container.cpp
**
**/

#include "nxcore.h"

/**
 * Default abstract container class constructor
 */
AbstractContainer::AbstractContainer() : super()
{
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
}

/**
 * "Normal" abstract container class constructor
 */
AbstractContainer::AbstractContainer(const TCHAR *pszName, UINT32 dwCategory) : super()
{
   _tcslcpy(m_name, pszName, MAX_OBJECT_NAME);
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
   m_isHidden = true;
}

/**
 * Abstract container class destructor
 */
AbstractContainer::~AbstractContainer()
{
   MemFree(m_pdwChildIdList);
}

/**
 * Link child objects after loading from database
 * This method is expected to be called only at startup, so we don't lock
 */
void AbstractContainer::linkObjects()
{
   super::linkObjects();
   if (m_dwChildIdListSize > 0)
   {
      // Find and link child objects
      for(UINT32 i = 0; i < m_dwChildIdListSize; i++)
      {
         NetObj *pObject = FindObjectById(m_pdwChildIdList[i]);
         if (pObject != NULL)
            linkObject(pObject);
         else
            nxlog_write(MSG_INVALID_CONTAINER_MEMBER, EVENTLOG_ERROR_TYPE, "dd", m_id, m_pdwChildIdList[i]);
      }

      // Cleanup
      free(m_pdwChildIdList);
      m_pdwChildIdList = NULL;
      m_dwChildIdListSize = 0;
   }
}

/**
 * Calculate status for compound object based on childs' status
 */
void AbstractContainer::calculateCompoundStatus(BOOL bForcedRecalc)
{
	super::calculateCompoundStatus(bForcedRecalc);

	if ((m_status == STATUS_UNKNOWN) && (m_dwChildIdListSize == 0))
   {
		lockProperties();
		m_status = STATUS_NORMAL;
		setModified(MODIFY_RUNTIME);
		unlockProperties();
	}
}

/**
 * Create NXCP message with object's data
 */
void AbstractContainer::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
}

/**
 * Modify object from message
 */
UINT32 AbstractContainer::modifyFromMessageInternal(NXCPMessage *request)
{
   // Change flags
   if (request->isFieldExist(VID_FLAGS))
		m_flags = request->getFieldAsUInt32(VID_FLAGS);

   return super::modifyFromMessageInternal(request);
}

/**
 * Serialize object to JSON
 */
json_t *AbstractContainer::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "flags", json_integer(m_flags));
   return root;
}

/**
 * Create abstract container object from database data.
 *
 * @param dwId object ID
 */
bool AbstractContainer::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   UINT32 i;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   // Load access list
   loadACLFromDB(hdb);

   // Load child list for later linkage
   if (!m_isDeleted)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%d"), m_id);
      hResult = DBSelect(hdb, szQuery);
      if (hResult != NULL)
      {
         m_dwChildIdListSize = DBGetNumRows(hResult);
         if (m_dwChildIdListSize > 0)
         {
            m_pdwChildIdList = (UINT32 *)malloc(sizeof(UINT32) * m_dwChildIdListSize);
            for(i = 0; i < m_dwChildIdListSize; i++)
               m_pdwChildIdList[i] = DBGetFieldULong(hResult, i, 0);
         }
         DBFreeResult(hResult);
      }
   }

   return true;
}

/**
 * Save object to database
 *
 * @param hdb database connection handle
 */
bool AbstractContainer::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("object_class"), NULL };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("object_containers"), _T("id"), m_id, columns);
      if (hStmt == NULL)
      {
         unlockProperties();
         return false;
      }

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)getObjectClass());
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   if (success && (m_modified & MODIFY_RELATIONS))
   {
      TCHAR query[256];

      // Update members list
      _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM container_members WHERE container_id=%d"), m_id);
      DBQuery(hdb, query);
      lockChildList(false);
      for(int i = 0; i < m_childList->size(); i++)
      {
         _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("INSERT INTO container_members (container_id,object_id) VALUES (%d,%d)"), m_id, m_childList->get(i)->getId());
         DBQuery(hdb, query);
      }
      unlockChildList();
   }

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   unlockProperties();
   return success;
}

/**
 * Delete object from database
 */
bool AbstractContainer::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_containers WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
   return success;
}

/**
 * Create container object from database data.
 *
 * @param dwId object ID
 */
bool Container::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   bool success = super::loadFromDatabase(hdb, dwId);

   if(success)
      success = AutoBindTarget::loadFromDatabase(hdb, m_id);
   return success;
}

/**
 * Save object to database
 *
 * @param hdb database connection handle
 */
bool Container::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if(success)
   {
      if (!IsDatabaseRecordExist(hdb, _T("object_containers"), _T("id"), m_id))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO object_containers (id) VALUES (?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
            success = DBExecute(hStmt);
            DBFreeStatement(hStmt);
         }
      }
   }

   if(success && (m_modified & MODIFY_OTHER))
      success = AutoBindTarget::saveToDatabase(hdb);

   lockProperties();
   // Clear modifications flag and unlock object
   m_modified = 0;
   unlockProperties();

   return success;
}

/**
 * Delete object from database
 */
bool Container::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if(success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_containers WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
   if(success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   return success;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Container::showThresholdSummary()
{
   return true;
}

/**
 * Modify object from message
 */
UINT32 Container::modifyFromMessageInternal(NXCPMessage *request)
{
   AutoBindTarget::modifyFromMessageInternal(request);
   return super::modifyFromMessageInternal(request);
}

/**
 * Fill message with object fields
 */
void Container::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);
   AutoBindTarget::fillMessageInternal(msg, userId);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Container::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(new NXSL_Object(vm, &g_nxslContainerClass, this));
}


