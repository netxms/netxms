/*
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Raden Solutions
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
** File: nodelink.cpp
**
**/

#include "nxcore.h"
#include "nms_objects.h"

/**
 * NodeLink default constructor
 */
NodeLink::NodeLink() : ServiceContainer()
{
	_tcscpy(m_name, _T("Default"));
	m_nodeId = 0;
}

/**
 * Constructor for new nodelink object
 */
NodeLink::NodeLink(const TCHAR *name, UINT32 nodeId) : ServiceContainer(name)
{
	nx_strncpy(m_name, name, MAX_OBJECT_NAME);
	m_nodeId = nodeId;
}

/**
 * Nodelink class destructor
 */
NodeLink::~NodeLink()
{
}

/**
 * Create object from database data
 */
BOOL NodeLink::loadFromDatabase(UINT32 id)
{
	const int script_length = 1024;
	m_id = id;

	if (!ServiceContainer::loadFromDatabase(id))
		return FALSE;

	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT node_id FROM node_links WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from node_links"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load nodelink object %ld - record missing"), (long)m_id);
		return FALSE;
	}

	m_nodeId	= DBGetFieldLong(hResult, 0, 0);
	if (m_nodeId <= 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load nodelink object %ld - node id is missing"), (long)m_id);
		return FALSE;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	return TRUE;
}

/**
 * Save nodelink to database
 */
BOOL NodeLink::saveToDatabase(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT nodelink_id FROM node_links WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from node_links"));
		return FALSE;
	}

	lockProperties();

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? _T("INSERT INTO node_links (node_id,nodelink_id) VALUES (?,?)") :
											  _T("UPDATE node_links SET node_id=? WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		unlockProperties();
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_nodeId);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
	unlockProperties();
	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}
	DBFreeStatement(hStmt);

	saveACLToDB(hdb);

	lockProperties();
	m_isModified = false;
	unlockProperties();
	return ServiceContainer::saveToDatabase(hdb);
}

/**
 * Delete object from database
 */
bool NodeLink::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = ServiceContainer::deleteFromDatabase(hdb);
	if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM node_links WHERE nodelink_id=?"));
	return success;
}

/**
 * Create CSCP message with object's data
 */
void NodeLink::fillMessage(NXCPMessage *pMsg, BOOL alreadyLocked)
{
   if (!alreadyLocked)
		lockProperties();

	ServiceContainer::fillMessage(pMsg, TRUE);
	pMsg->setField(VID_NODE_ID, m_nodeId);

	if(!alreadyLocked)
      unlockProperties();
}

/**
 * Modify object from message
 */
UINT32 NodeLink::modifyFromMessage(NXCPMessage *pRequest, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		lockProperties();

	if (pRequest->isFieldExist(VID_NODE_ID))
	{
		m_nodeId = pRequest->getFieldAsUInt32(VID_NODE_ID);
	}

	return ServiceContainer::modifyFromMessage(pRequest, TRUE);
}

/**
 * Execute underlying checks for this node link
 */
void NodeLink::execute()
{
	DbgPrintf(6, _T("NodeLink::execute() started for %s [%ld]"), m_name, (long)m_id);

   LockChildList(FALSE);
	for (int i = 0; i < int(m_dwChildCount); i++)
	{
		if (m_pChildList[i]->getObjectClass() == OBJECT_SLMCHECK)
			((SlmCheck *)m_pChildList[i])->execute();
	}
   UnlockChildList();

	calculateCompoundStatus();

	DbgPrintf(6, _T("NodeLink::execute() finished for %s [%ld]"), m_name, (long)m_id);
}

/**
 * Apply single template check to this nodelink
 */
void NodeLink::applyTemplate(SlmCheck *tmpl)
{
	// Check if we already have check created from this template
	SlmCheck *check = NULL;
	LockChildList(FALSE);
	for(UINT32 i = 0; i < m_dwChildCount; i++)
	{
		if ((m_pChildList[i]->getObjectClass() == OBJECT_SLMCHECK) &&
		    (((SlmCheck *)m_pChildList[i])->getTemplateId() == tmpl->getId()))
		{
			check = (SlmCheck *)m_pChildList[i];
			break;
		}
	}
	UnlockChildList();

	if (check == NULL)
	{
		check = new SlmCheck(tmpl);
		check->AddParent(this);
		AddChild(check);
		NetObjInsert(check, TRUE);
		check->unhide();
	}
	else
	{
		check->updateFromTemplate(tmpl);
	}
}

/**
 * Apply templates from the upper levels to this nodelink
 */
void NodeLink::applyTemplates()
{
	ObjectArray<SlmCheck> templates;

	LockParentList(FALSE);
	for(UINT32 i = 0; i < m_dwParentCount; i++)
	{
		if (m_pParentList[i]->getObjectClass() != OBJECT_BUSINESSSERVICE)
			continue;

		BusinessService *parent = (BusinessService *)m_pParentList[i];
		parent->getApplicableTemplates(this, &templates);
	}
	UnlockParentList();

	for(int j = 0; j < templates.size(); j++)
	{
		SlmCheck *tmpl = templates.get(j);
		applyTemplate(tmpl);
		tmpl->decRefCount();
	}
}

/**
 * Object deletion thread
 */
static THREAD_RESULT THREAD_CALL DeleteThread(void *arg)
{
	((NodeLink *)arg)->deleteObject();
	return THREAD_OK;
}

/**
 * Object deletion handler
 */
void NodeLink::onObjectDelete(UINT32 dwObjectId)
{
	if (dwObjectId == m_nodeId)
	{
		// owning node being deleted
		// OnObjectDelete being called as callback from another
		// object's OnObjectDelete, so we cannot call this->deleteObject()
		// directly without potential deadlock
		DbgPrintf(4, _T("Scheduling deletion of nodelink object %s [%u] due to linked node deletion"), m_name, m_id);
		ThreadCreate(DeleteThread, 0, this);
	}
   ServiceContainer::onObjectDelete(dwObjectId);
}
