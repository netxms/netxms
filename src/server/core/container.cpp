/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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


//
// Find container category by id
//

CONTAINER_CATEGORY NXCORE_EXPORTABLE *FindContainerCategory(DWORD dwId)
{
   DWORD i;

   for(i = 0; i < g_dwNumCategories; i++)
      if (g_pContainerCatList[i].dwCatId == dwId)
         return &g_pContainerCatList[i];
   return NULL;
}


//
// Default container class constructor
//

Container::Container()
          :NetObj()
{
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
   m_dwCategory = 1;
	m_bindFilter = NULL;
	m_bindFilterSource = NULL;
}


//
// _T("Normal") container class constructor
//

Container::Container(const TCHAR *pszName, DWORD dwCategory)
          :NetObj()
{
   nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME);
   m_pdwChildIdList = NULL;
   m_dwChildIdListSize = 0;
   m_dwCategory = dwCategory;
	m_bindFilter = NULL;
	m_bindFilterSource = NULL;
   m_bIsHidden = TRUE;
}


//
// Container class destructor
//

Container::~Container()
{
   safe_free(m_pdwChildIdList);
	safe_free(m_bindFilterSource);
	delete m_bindFilter;
}


//
// Create object from database data
//

BOOL Container::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   DWORD i;

   m_dwId = dwId;

   if (!loadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT category,enable_auto_bind,auto_bind_filter FROM containers WHERE id=%d"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      // No object with given ID in database
      DBFreeResult(hResult);
      return FALSE;
   }

   m_dwCategory = DBGetFieldULong(hResult, 0, 0);
	if (DBGetFieldLong(hResult, 0, 1))
	{
		// Auto-bind enabled
		m_bindFilterSource = DBGetField(hResult, 0, 2, NULL, 0);
		if (m_bindFilterSource != NULL)
		{
			TCHAR error[256];

			DecodeSQLString(m_bindFilterSource);
			m_bindFilter = NXSLCompile(m_bindFilterSource, error, 256);
			if (m_bindFilter == NULL)
				nxlog_write(MSG_CONTAINER_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_dwId, m_szName, error);
		}
	}
   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB();

   // Load child list for later linkage
   if (!m_bIsDeleted)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT object_id FROM container_members WHERE container_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         m_dwChildIdListSize = DBGetNumRows(hResult);
         if (m_dwChildIdListSize > 0)
         {
            m_pdwChildIdList = (DWORD *)malloc(sizeof(DWORD) * m_dwChildIdListSize);
            for(i = 0; i < m_dwChildIdListSize; i++)
               m_pdwChildIdList[i] = DBGetFieldULong(hResult, i, 0);
         }
         DBFreeResult(hResult);
      }
   }

   return TRUE;
}


//
// Save object to database
//

BOOL Container::SaveToDB(DB_HANDLE hdb)
{
   TCHAR szQuery[1024], *pszDynQuery, *pszEscScript;
   DB_RESULT hResult;
   DWORD i, len;
   BOOL bNewObject = TRUE;

   // Lock object's access
   LockData();

   saveCommonProperties(hdb);

   // Check for object's existence in database
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT id FROM containers WHERE id=%d"), m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
	pszEscScript = (m_bindFilterSource != NULL) ? EncodeSQLString(m_bindFilterSource) : _tcsdup(_T("#00"));
	len = (DWORD)_tcslen(pszEscScript) + 256;
	pszDynQuery = (TCHAR *)malloc(sizeof(TCHAR) * len);
   if (bNewObject)
      _sntprintf(pszDynQuery, len, _T("INSERT INTO containers (id,category,object_class,enable_auto_bind,auto_bind_filter) VALUES (%d,%d,%d,%d,'%s')"),
                 m_dwId, m_dwCategory, Type(), (m_bindFilterSource != NULL) ? 1 : 0, pszEscScript);
   else
      _sntprintf(pszDynQuery, len, _T("UPDATE containers SET category=%d,object_class=%d,enable_auto_bind=%d,auto_bind_filter='%s' WHERE id=%d"),
                 m_dwCategory, Type(), (m_bindFilterSource != NULL) ? 1 : 0, pszEscScript, m_dwId);
	free(pszEscScript);
   DBQuery(hdb, pszDynQuery);
	free(pszDynQuery);

   // Update members list
   _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM container_members WHERE container_id=%d"), m_dwId);
   DBQuery(hdb, szQuery);
   LockChildList(FALSE);
   for(i = 0; i < m_dwChildCount; i++)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO container_members (container_id,object_id) VALUES (%d,%d)"), m_dwId, m_pChildList[i]->Id());
      DBQuery(hdb, szQuery);
   }
   UnlockChildList();

   // Save access list
   saveACLToDB(hdb);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   UnlockData();

   return TRUE;
}


//
// Delete object from database
//

BOOL Container::DeleteFromDB()
{
   TCHAR szQuery[256];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM containers WHERE id=%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM container_members WHERE container_id=%d"), (int)m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Link child objects after loading from database
// This method is expected to be called only at startup, so we don't lock
//

void Container::linkChildObjects()
{
   NetObj *pObject;
   DWORD i;

   if (m_dwChildIdListSize > 0)
   {
      // Find and link child objects
      for(i = 0; i < m_dwChildIdListSize; i++)
      {
         pObject = FindObjectById(m_pdwChildIdList[i]);
         if (pObject != NULL)
            linkObject(pObject);
         else
            nxlog_write(MSG_INVALID_CONTAINER_MEMBER, EVENTLOG_ERROR_TYPE, "dd", m_dwId, m_pdwChildIdList[i]);
      }

      // Cleanup
      free(m_pdwChildIdList);
      m_pdwChildIdList = NULL;
      m_dwChildIdListSize = 0;
   }
}


//
// Create CSCP message with object's data
//

void Container::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_CATEGORY, m_dwCategory);
	pMsg->SetVariable(VID_ENABLE_AUTO_BIND, (WORD)((m_bindFilterSource != NULL) ? 1 : 0));
	pMsg->SetVariable(VID_AUTO_BIND_FILTER, CHECK_NULL_EX(m_bindFilterSource));
}


//
// Modify object from message
//

DWORD Container::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Change auto-bind filter
	if (pRequest->GetVariableShort(VID_ENABLE_AUTO_BIND))
	{
		TCHAR *script = pRequest->GetVariableStr(VID_AUTO_BIND_FILTER);
		setAutoBindFilter(script);
		safe_free(script);
	}
	else
	{
		setAutoBindFilter(NULL);
	}

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Set container's autobind script
//

void Container::setAutoBindFilter(const TCHAR *script)
{
	if (script != NULL)
	{
		safe_free(m_bindFilterSource);
		delete m_bindFilter;
		m_bindFilterSource = _tcsdup(script);
		if (m_bindFilterSource != NULL)
		{
			TCHAR error[256];

			m_bindFilter = NXSLCompile(m_bindFilterSource, error, 256);
			if (m_bindFilter == NULL)
				nxlog_write(MSG_CONTAINER_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_dwId, m_szName, error);
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
	Modify();
}


//
// Check if node should be placed into container
//

bool Container::isSuitableForNode(Node *node)
{
	NXSL_ServerEnv *pEnv;
	NXSL_Value *value;
	bool result = false;

	LockData();
	if (m_bindFilter != NULL)
	{
		pEnv = new NXSL_ServerEnv;
		m_bindFilter->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, node)));
		if (m_bindFilter->run(pEnv, 0, NULL) == 0)
		{
			value = m_bindFilter->getResult();
			result = ((value != NULL) && (value->getValueAsInt32() != 0));
		}
		else
		{
			TCHAR buffer[1024];

			_sntprintf(buffer, 1024, _T("Container::%s::%d"), m_szName, m_dwId);
			PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_bindFilter->getErrorText(), m_dwId);
			nxlog_write(MSG_CONTAINER_SCRIPT_EXECUTION_ERROR, NXLOG_WARNING, "dss", m_dwId, m_szName, m_bindFilter->getErrorText());
		}
	}
	UnlockData();
	return result;
}
