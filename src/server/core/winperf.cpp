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
** File: winperf.cpp
**
**/

#include "nxcore.h"

/**
 * Create new empty performance object
 */
WinPerfObject::WinPerfObject(const TCHAR *name)
{
	m_name = _tcsdup(name);
	m_instances = new StringList;
	m_counters = new StringList;
}

/**
 * Destructor
 */
WinPerfObject::~WinPerfObject()
{
	free(m_name);
	delete m_instances;
	delete m_counters;
}

/**
 * Fill information from agent
 */
bool WinPerfObject::readDataFromAgent(AgentConnection *conn)
{
	TCHAR param[256];
	_sntprintf(param, 256, _T("PDH.ObjectCounters(\"%s\")"), m_name);

	StringList *data;
	if (conn->getList(param, &data) != ERR_SUCCESS)
		return false;
	for(int i = 0; i < data->size(); i++)
		m_counters->add(data->get(i));
	delete data;

	_sntprintf(param, 256, _T("PDH.ObjectInstances(\"%s\")"), m_name);
	if (conn->getList(param, &data) != ERR_SUCCESS)
		return false;
   for(int i = 0; i < data->size(); i++)
		m_instances->add(data->get(i));
   delete data;

	return true;
}

/**
 * Fill NXCP message with object's data
 *
 * @return next available variable ID
 */
UINT32 WinPerfObject::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
	msg->setField(baseId, m_name);
	msg->setField(baseId + 1, (UINT32)m_counters->size());
	msg->setField(baseId + 2, (UINT32)m_instances->size());

	UINT32 varId = baseId + 3;
	for(int i = 0; i < m_counters->size(); i++)
		msg->setField(varId++, m_counters->get(i));
	for(int i = 0; i < m_instances->size(); i++)
		msg->setField(varId++, m_instances->get(i));
	return varId;
}

/**
 * Get windows performance objects from node
 */
ObjectArray<WinPerfObject> *WinPerfObject::getWinPerfObjectsFromNode(Node *node, AgentConnection *conn)
{
	ObjectArray<WinPerfObject> *objects;
	StringList *data;
	if (conn->getList(_T("PDH.Objects"), &data) == ERR_SUCCESS)
	{
		objects = new ObjectArray<WinPerfObject>(data->size(), 16, true);
		for(int i = 0; i < data->size(); i++)
			objects->add(new WinPerfObject(data->get(i)));
		delete data;

		for(int i = 0; i < objects->size(); i++)
		{
			if (!objects->get(i)->readDataFromAgent(conn))
			{
				DbgPrintf(5, _T("WinPerfObject::getWinPerfObjectsFromNode(%s [%d]): cannot read data for object %s"), node->getName(), node->getId(), objects->get(i)->getName());
				objects->remove(i);
				i--;
			}
		}
		if (objects->size() == 0)
			delete_and_null(objects);
	}
	else
	{
		DbgPrintf(5, _T("WinPerfObject::getWinPerfObjectsFromNode(%s [%d]): cannot read PDH.Objects list"), node->getName(), node->getId());
		objects = NULL;
	}
	return objects;
}
