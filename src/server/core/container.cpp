/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
 * Default container class constructor
 */
Container::Container() : NetObj()
{
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
	m_flags = 0;
	m_bindFilter = NULL;
	m_bindFilterSource = NULL;
}

/**
 * "Normal" container class constructor
 */
Container::Container(const TCHAR *pszName, UINT32 dwCategory) : NetObj()
{
   nx_strncpy(m_name, pszName, MAX_OBJECT_NAME);
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
	m_flags = 0;
	m_bindFilter = NULL;
	m_bindFilterSource = NULL;
   m_isHidden = true;
}

/**
 * Container class destructor
 */
Container::~Container()
{
   safe_free(m_pdwChildIdList);
	safe_free(m_bindFilterSource);
	delete m_bindFilter;
}

/**
 * Create container object from database data.
 *
 * @param dwId object ID
 */
bool Container::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   UINT32 i;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT flags,auto_bind_filter FROM object_containers WHERE id=%d"), dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return false;
   }

	m_flags = DBGetFieldULong(hResult, 0, 0);
   m_bindFilterSource = DBGetField(hResult, 0, 1, NULL, 0);
   if (m_bindFilterSource != NULL)
   {
      TCHAR error[256];
      m_bindFilter = NXSLCompile(m_bindFilterSource, error, 256, NULL);
      if (m_bindFilter == NULL)
      {
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("Container::%s::%d"), m_name, m_id);
         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
         nxlog_write(MSG_CONTAINER_SCRIPT_COMPILATION_ERROR, NXLOG_WARNING, "dss", m_id, m_name, error);
      }
   }
   DBFreeResult(hResult);

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
bool Container::saveToDatabase(DB_HANDLE hdb)
{
   // Lock object's access
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("object_containers"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE object_containers SET object_class=?,auto_bind_filter=?,flags=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO object_containers (object_class,auto_bind_filter,flags,id) VALUES (?,?,?,?)"));
      }
      if (hStmt == NULL)
      {
         unlockProperties();
         return false;
      }

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)getObjectClass());
      DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_bindFilterSource, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_flags);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
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
   bool success = NetObj::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_containers WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM container_members WHERE container_id=?"));
   return success;
}

/**
 * Link child objects after loading from database
 * This method is expected to be called only at startup, so we don't lock
 */
void Container::linkObjects()
{
   NetObj::linkObjects();
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
void Container::calculateCompoundStatus(BOOL bForcedRecalc)
{
	NetObj::calculateCompoundStatus(bForcedRecalc);

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
void Container::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   NetObj::fillMessageInternal(msg, userId);
	msg->setField(VID_FLAGS, m_flags);
	msg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_bindFilterSource));
}

/**
 * Modify object from message
 */
UINT32 Container::modifyFromMessageInternal(NXCPMessage *request)
{
   // Change flags
   if (request->isFieldExist(VID_FLAGS))
		m_flags = request->getFieldAsUInt32(VID_FLAGS);

   // Change auto-bind filter
	if (request->isFieldExist(VID_AUTOBIND_FILTER))
	{
		TCHAR *script = request->getFieldAsString(VID_AUTOBIND_FILTER);
		setAutoBindFilterInternal(script);
		free(script);
	}

   return NetObj::modifyFromMessageInternal(request);
}

/**
 * Set container's autobind script
 */
void Container::setAutoBindFilterInternal(const TCHAR *script)
{
	if (script != NULL)
	{
		free(m_bindFilterSource);
		delete m_bindFilter;
		m_bindFilterSource = _tcsdup(script);
		if (m_bindFilterSource != NULL)
		{
			TCHAR error[256];

			m_bindFilter = NXSLCompile(m_bindFilterSource, error, 256, NULL);
			if (m_bindFilter == NULL)
			{
	         TCHAR buffer[1024];
	         _sntprintf(buffer, 1024, _T("Container::%s::%d"), m_name, m_id);
	         PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, error, m_id);
				nxlog_write(MSG_CONTAINER_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, error);
			}
		}
		else
		{
			m_bindFilter = NULL;
		}
	}
	else
	{
		delete_and_null(m_bindFilter);
		safe_free_and_null(m_bindFilterSource);
	}
	setModified(MODIFY_OTHER);
}

/**
 * Set auito bind mode for container
 */
void Container::setAutoBindMode(bool doBind, bool doUnbind)
{
   lockProperties();

   if (doBind)
      m_flags |= CF_AUTO_BIND;
   else
      m_flags &= ~CF_AUTO_BIND;

   if (doUnbind)
      m_flags |= CF_AUTO_UNBIND;
   else
      m_flags &= ~CF_AUTO_UNBIND;

   setModified(MODIFY_OTHER);
   unlockProperties();
}

/**
 * Check if object should be placed into container
 */
AutoBindDecision Container::isSuitableForObject(NetObj *object)
{
   AutoBindDecision result = AutoBindDecision_Ignore;

   NXSL_VM *filter = NULL;
	lockProperties();
	if ((m_flags & CF_AUTO_BIND) && (m_bindFilter != NULL))
	{
	   filter = new NXSL_VM(new NXSL_ServerEnv());
	   if (!filter->load(m_bindFilter))
	   {
	      TCHAR buffer[1024];
	      _sntprintf(buffer, 1024, _T("Container::%s::%d"), m_name, m_id);
	      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
	      nxlog_write(MSG_CONTAINER_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dss", m_id, m_name, filter->getErrorText());
	      delete_and_null(filter);
	   }
	}
   unlockProperties();

   if (filter == NULL)
      return result;

   filter->setGlobalVariable(_T("$object"), object->createNXSLObject());
   if (object->getObjectClass() == OBJECT_NODE)
      filter->setGlobalVariable(_T("$node"), object->createNXSLObject());
   if (filter->run())
   {
      NXSL_Value *value = filter->getResult();
      if (!value->isNull())
         result = ((value != NULL) && (value->getValueAsInt32() != 0)) ? AutoBindDecision_Bind : AutoBindDecision_Unbind;
   }
   else
   {
      lockProperties();
      TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("Container::%s::%d"), m_name, m_id);
      PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_id);
      nxlog_write(MSG_CONTAINER_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dss", m_id, m_name, filter->getErrorText());
      unlockProperties();
   }
   delete filter;
	return result;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Container::showThresholdSummary()
{
	return true;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Container::createNXSLObject()
{
   return new NXSL_Value(new NXSL_Object(&g_nxslContainerClass, this));
}

/**
 * Serialize object to JSON
 */
json_t *Container::toJson()
{
   json_t *root = NetObj::toJson();
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "bindFilter", json_string_t(m_bindFilterSource));
   return root;
}
