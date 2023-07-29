/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
 * Fill information from agent
 */
bool WinPerfObject::readDataFromAgent(AgentConnection *conn)
{
	TCHAR param[MAX_PARAM_NAME];
	_sntprintf(param, MAX_PARAM_NAME, _T("PDH.ObjectCounters(\"%s\")"), m_name.cstr());

	StringList *data;
	if (conn->getList(param, &data) != ERR_SUCCESS)
		return false;
	for(int i = 0; i < data->size(); i++)
		m_counters.add(data->get(i));
	delete data;

	_sntprintf(param, MAX_PARAM_NAME, _T("PDH.ObjectInstances(\"%s\")"), m_name.cstr());
	if (conn->getList(param, &data) != ERR_SUCCESS)
		return false;
   for(int i = 0; i < data->size(); i++)
		m_instances.add(data->get(i));
   delete data;

	return true;
}

/**
 * Fill NXCP message with object's data
 *
 * @return next available variable ID
 */
uint32_t WinPerfObject::fillMessage(NXCPMessage *msg, uint32_t baseId)
{
	msg->setField(baseId, m_name);
	msg->setField(baseId + 1, m_counters.size());
	msg->setField(baseId + 2, m_instances.size());

	uint32_t fieldId = baseId + 3;
	for(int i = 0; i < m_counters.size(); i++)
		msg->setField(fieldId++, m_counters.get(i));
	for(int i = 0; i < m_instances.size(); i++)
		msg->setField(fieldId++, m_instances.get(i));
	return fieldId;
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
		objects = new ObjectArray<WinPerfObject>(data->size(), 16, Ownership::True);
		for(int i = 0; i < data->size(); i++)
			objects->add(new WinPerfObject(data->get(i)));
		delete data;

		for(int i = 0; i < objects->size(); i++)
		{
			if (!objects->get(i)->readDataFromAgent(conn))
			{
				nxlog_debug_tag(_T("dc.winperf"), 5, _T("WinPerfObject::getWinPerfObjectsFromNode(%s [%u]): cannot read data for object \"%s\""), node->getName(), node->getId(), objects->get(i)->getName());
				objects->remove(i);
				i--;
			}
		}
		if (objects->size() == 0)
			delete_and_null(objects);
	}
	else
	{
	   nxlog_debug_tag(_T("dc.winperf"), 5, _T("WinPerfObject::getWinPerfObjectsFromNode(%s [%u]): cannot read PDH.Objects list"), node->getName(), node->getId());
		objects = nullptr;
	}
	return objects;
}
