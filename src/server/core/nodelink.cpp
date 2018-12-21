/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Raden Solutions
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
NodeLink::NodeLink() : super()
{
	_tcscpy(m_name, _T("Default"));
	m_nodeId = 0;
}

/**
 * Constructor for new nodelink object
 */
NodeLink::NodeLink(const TCHAR *name, UINT32 nodeId) : super(name)
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
bool NodeLink::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
	m_id = id;

	if (!super::loadFromDatabase(hdb, id))
		return false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT node_id FROM node_links WHERE nodelink_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from node_links"));
		return false;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return false;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load nodelink object %ld - record missing"), (long)m_id);
		return false;
	}

	m_nodeId	= DBGetFieldLong(hResult, 0, 0);
	if (m_nodeId == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load nodelink object %ld - node id is missing"), (long)m_id);
		return false;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	return true;
}

/**
 * Save nodelink to database
 */
bool NodeLink::saveToDatabase(DB_HANDLE hdb)
{
   if (m_modified & MODIFY_OTHER)
   {
      static const TCHAR *columns[] = { _T("node_id"), NULL };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("node_links"), _T("nodelink_id"), m_id, columns);
      if (hStmt == NULL)
         return false;

      lockProperties();
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_nodeId);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
      unlockProperties();
      bool success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
      if (!success)
         return false;
   }
	return super::saveToDatabase(hdb);
}

/**
 * Delete object from database
 */
bool NodeLink::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = super::deleteFromDatabase(hdb);
	if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM node_links WHERE nodelink_id=?"));
	return success;
}

/**
 * Create CSCP message with object's data
 */
void NodeLink::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
	super::fillMessageInternal(pMsg, userId);
	pMsg->setField(VID_NODE_ID, m_nodeId);
}

/**
 * Modify object from message
 */
UINT32 NodeLink::modifyFromMessageInternal(NXCPMessage *pRequest)
{
	if (pRequest->isFieldExist(VID_NODE_ID))
	{
		m_nodeId = pRequest->getFieldAsUInt32(VID_NODE_ID);
	}

	return super::modifyFromMessageInternal(pRequest);
}

/**
 * Execute underlying checks for this node link
 */
void NodeLink::execute()
{
	DbgPrintf(6, _T("NodeLink::execute() started for %s [%ld]"), m_name, (long)m_id);

   lockChildList(false);
	for (int i = 0; i < m_childList->size(); i++)
	{
		if (m_childList->get(i)->getObjectClass() == OBJECT_SLMCHECK)
			((SlmCheck *)m_childList->get(i))->execute();
	}
   unlockChildList();

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
	lockChildList(false);
	for(int i = 0; i < m_childList->size(); i++)
	{
		if ((m_childList->get(i)->getObjectClass() == OBJECT_SLMCHECK) &&
		    (((SlmCheck *)m_childList->get(i))->getTemplateId() == tmpl->getId()))
		{
			check = (SlmCheck *)m_childList->get(i);
			break;
		}
	}
	unlockChildList();

	if (check == NULL)
	{
		check = new SlmCheck(tmpl);
		check->addParent(this);
		addChild(check);
		NetObjInsert(check, true, false);
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

	lockParentList(false);
	for(int i = 0; i < m_parentList->size(); i++)
	{
		if (m_parentList->get(i)->getObjectClass() != OBJECT_BUSINESSSERVICE)
			continue;

		BusinessService *parent = (BusinessService *)m_parentList->get(i);
		parent->getApplicableTemplates(this, &templates);
	}
	unlockParentList();

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
static void DeleteNodeLink(void *arg)
{
	((NodeLink *)arg)->deleteObject();
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
      ThreadPoolExecute(g_mainThreadPool, DeleteNodeLink, this);
	}
   super::onObjectDelete(dwObjectId);
}
