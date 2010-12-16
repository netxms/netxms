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
** File: netmap.cpp
**
**/

#include "nxcore.h"


//
// Redefined status calculation for network maps group
//

void NetworkMapGroup::CalculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


//
// Network map object default constructor
//

NetworkMap::NetworkMap() : NetObj()
{
	m_mapType = NETMAP_USER_DEFINED;
	m_seedObject = 0;
	m_layout = MAP_LAYOUT_MANUAL;
	m_background = 0;
	m_numElements = 0;
	m_elements = NULL;
	m_numLinks = 0;
	m_links = NULL;
}


//
// Create network map object from user session
//

NetworkMap::NetworkMap(int type, DWORD seed) : NetObj()
{
	m_mapType = type;
	m_seedObject = seed;
	m_layout = MAP_LAYOUT_RADIAL;
	m_background = 0;
	m_numElements = 0;
	m_elements = NULL;
	m_numLinks = 0;
	m_links = NULL;
}


//
// Network map object destructor
//

NetworkMap::~NetworkMap()
{
	for(int i = 0; i < m_numElements; i++)
		delete m_elements[i];
	safe_free(m_elements);
	for(int i = 0; i < m_numLinks; i++)
		delete m_links[i];
	safe_free(m_links);
}


//
// Redefined status calculation for network maps
//

void NetworkMap::CalculateCompoundStatus(BOOL bForcedRecalc)
{
   m_iStatus = STATUS_NORMAL;
}


//
// Save to database
//

BOOL NetworkMap::SaveToDB(DB_HANDLE hdb)
{
	TCHAR query[1024];

	if (!SaveCommonProperties(hdb))
		return FALSE;

	// Check for object's existence in database
	bool isNewObject = true;
   _sntprintf(query, 256, _T("SELECT id FROM network_maps WHERE id=%d"), m_dwId);
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
         isNewObject = false;
      DBFreeResult(hResult);
   }

	if (isNewObject)
      _sntprintf(query, 8192,
                 _T("INSERT INTO network_maps (id,map_type,layout,seed,background) VALUES (%d,%d,%d,%d,%d)"),
                 m_dwId, m_mapType, m_layout, m_seedObject, m_background);
   else
      _sntprintf(query, 8192,
                 _T("UPDATE network_maps SET map_type=%d,layout=%d,seed=%d,background=%d WHERE id=%d"),
                 m_mapType, m_layout, m_seedObject, m_background, m_dwId);
   if (!DBQuery(hdb, query))
		return FALSE;

	if (!SaveACLToDB(hdb))
		return FALSE;

   // Save elements
   _sntprintf(query, 256, _T("DELETE FROM network_map_elements WHERE map_id=%d"), m_dwId);
   if (!DBQuery(hdb, query))
		return FALSE;
   for(int i = 0; i < m_numElements; i++)
   {
		Config *config = new Config();
		config->setTopLevelTag(_T("element"));
		m_elements[i]->updateConfig(config);
		String data = DBPrepareString(hdb, config->createXml());
		delete config;
		int len = data.getSize() + 256;
		TCHAR *eq = (TCHAR *)malloc(len * sizeof(TCHAR));
      _sntprintf(eq, len, _T("INSERT INTO network_map_elements (map_id,element_id,element_type,element_data) VALUES (%d,%d,%d,%s)"),
		           m_dwId, m_elements[i]->getId(), m_elements[i]->getType(), (const TCHAR *)data);
      DBQuery(hdb, eq);
		free(eq);
   }

	// Save links
   _sntprintf(query, 256, _T("DELETE FROM network_map_links WHERE map_id=%d"), m_dwId);
   if (!DBQuery(hdb, query))
		return FALSE;
   for(int i = 0; i < m_numLinks; i++)
   {
      _sntprintf(query, 1024, _T("INSERT INTO network_map_links (map_id,element1,element2,link_type,link_name,connector_name1,connector_name2) VALUES (%d,%d,%d,%d,%s,%s,%s)"),
		           m_dwId, m_links[i]->getElement1(), m_links[i]->getElement2(),
					  m_links[i]->getType(), (const TCHAR *)DBPrepareString(hdb, m_links[i]->getName(), 255), 
					  (const TCHAR *)DBPrepareString(hdb, m_links[i]->getConnector1Name(), 63),
					  (const TCHAR *)DBPrepareString(hdb, m_links[i]->getConnector2Name()), 63);
      DBQuery(hdb, query);
   }

	return TRUE;
}


//
// Delete from database
//

BOOL NetworkMap::DeleteFromDB()
{
	TCHAR query[256];

	_sntprintf(query, 256, _T("DELETE FROM network_maps WHERE id=%d"), m_dwId);
	QueueSQLRequest(query);
	_sntprintf(query, 256, _T("DELETE FROM network_map_elements WHERE map_id=%d"), m_dwId);
	QueueSQLRequest(query);
	_sntprintf(query, 256, _T("DELETE FROM network_map_links WHERE map_id=%d"), m_dwId);
	QueueSQLRequest(query);
	return TRUE;
}


//
// Load from database
//

BOOL NetworkMap::CreateFromDB(DWORD dwId)
{
	m_dwId = dwId;

	if (!LoadCommonProperties())
   {
      DbgPrintf(2, _T("Cannot load common properties for network map object %d"), dwId);
      return FALSE;
   }

   if (!m_bIsDeleted)
   {
		TCHAR query[256];

	   LoadACLFromDB();

		_sntprintf(query, 256, _T("SELECT map_type,layout,seed,background FROM network_maps WHERE id=%d"), dwId);
		DB_RESULT hResult = DBSelect(g_hCoreDB, query);
		if (hResult == NULL)
			return FALSE;

		m_mapType = DBGetFieldLong(hResult, 0, 0);
		m_layout = DBGetFieldLong(hResult, 0, 1);
		m_seedObject = DBGetFieldULong(hResult, 0, 2);
		m_background = DBGetFieldLong(hResult, 0, 3);
		DBFreeResult(hResult);

	   // Load elements
      _sntprintf(query, 256, _T("SELECT element_id,element_type,element_data FROM network_map_elements WHERE map_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, query);
      if (hResult != NULL)
      {
         m_numElements = DBGetNumRows(hResult);
			if (m_numElements > 0)
			{
				m_elements = (NetworkMapElement **)malloc(sizeof(NetworkMapElement *) * m_numElements);
				for(int i = 0; i < m_numElements; i++)
				{
					DWORD id = DBGetFieldULong(hResult, i, 0);
					Config *config = new Config();
					TCHAR *data = DBGetField(hResult, i, 2, NULL, 0);
					if (data != NULL)
					{
#ifdef UNICODE
						char *utf8data = UTF8StringFromWideString(data);
						config->loadXmlConfigFromMemory(utf8data, (int)strlen(utf8data), _T("<database>"), "element");
						free(utf8data);
#else
						config->loadXmlConfigFromMemory(data, (int)strlen(data), _T("<database>"), "element");
#endif
						free(data);
						switch(DBGetFieldLong(hResult, i, 1))
						{
							case MAP_ELEMENT_OBJECT:
								m_elements[i] = new NetworkMapObject(id, config);
								break;
							case MAP_ELEMENT_DECORATION:
								m_elements[i] = new NetworkMapDecoration(id, config);
								break;
							default:		// Unknown type, create generic element
								m_elements[i] = new NetworkMapElement(id, config);
								break;
						}
					}
					else
					{
						m_elements[i] = new NetworkMapElement(id);
					}
					delete config;
				}
			}
         DBFreeResult(hResult);
      }

		// Load links
      _sntprintf(query, 256, _T("SELECT element1,element2,link_type,link_name,connector_name1,connector_name2 FROM network_map_links WHERE map_id=%d"), m_dwId);
      hResult = DBSelect(g_hCoreDB, query);
      if (hResult != NULL)
      {
         m_numLinks = DBGetNumRows(hResult);
			if (m_numLinks > 0)
			{
				m_links = (NetworkMapLink **)malloc(sizeof(NetworkMapLink *) * m_numLinks);
				for(int i = 0; i < m_numLinks; i++)
				{
					TCHAR buffer[256];

					m_links[i] = new NetworkMapLink(DBGetFieldLong(hResult, i, 0), DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2));
					m_links[i]->setName(DBGetField(hResult, i, 3, buffer, 256));
					m_links[i]->setConnector1Name(DBGetField(hResult, i, 4, buffer, 256));
					m_links[i]->setConnector2Name(DBGetField(hResult, i, 5, buffer, 256));
				}
			}
         DBFreeResult(hResult);
      }
	}

	return TRUE;
}


//
// Fill NXCP message with object's data
//

void NetworkMap::CreateMessage(CSCPMessage *msg)
{
	NetObj::CreateMessage(msg);

	msg->SetVariable(VID_MAP_TYPE, (WORD)m_mapType);
	msg->SetVariable(VID_LAYOUT, (WORD)m_layout);
	msg->SetVariable(VID_SEED_OBJECT, m_seedObject);
	msg->SetVariable(VID_BACKGROUND, (DWORD)m_background);

	msg->SetVariable(VID_NUM_ELEMENTS, (DWORD)m_numElements);
	DWORD varId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_numElements; i++)
	{
		m_elements[i]->fillMessage(msg, varId);
		varId += 100;
	}

	msg->SetVariable(VID_NUM_LINKS, (DWORD)m_numLinks);
	varId = VID_LINK_LIST_BASE;
	for(int i = 0; i < m_numLinks; i++)
	{
		m_links[i]->fillMessage(msg, varId);
		varId += 10;
	}
}


//
// Update network map object from NXCP message
//

DWORD NetworkMap::ModifyFromMessage(CSCPMessage *request, BOOL bAlreadyLocked)
{
	if (!bAlreadyLocked)
		LockData();

	if (request->IsVariableExist(VID_MAP_TYPE))
		m_mapType = (int)request->GetVariableShort(VID_MAP_TYPE);

	if (request->IsVariableExist(VID_LAYOUT))
		m_layout = (int)request->GetVariableShort(VID_LAYOUT);

	if (request->IsVariableExist(VID_SEED_OBJECT))
		m_seedObject = request->GetVariableLong(VID_SEED_OBJECT);

	if (request->IsVariableExist(VID_BACKGROUND))
		m_background = (int)request->GetVariableLong(VID_BACKGROUND);

	if (request->IsVariableExist(VID_NUM_ELEMENTS))
	{
		for(int i = 0; i < m_numElements; i++)
			delete m_elements[i];

		m_numElements = request->GetVariableLong(VID_NUM_ELEMENTS);
		if (m_numElements > 0)
		{
			m_elements = (NetworkMapElement **)realloc(m_elements, sizeof(NetworkMapElement *) * m_numElements);
			DWORD varId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < m_numElements; i++)
			{
				int type = (int)request->GetVariableShort(varId + 1);
				switch(type)
				{
					case MAP_ELEMENT_OBJECT:
						m_elements[i] = new NetworkMapObject(request, varId);
						break;
					case MAP_ELEMENT_DECORATION:
						m_elements[i] = new NetworkMapDecoration(request, varId);
						break;
					default:		// Unknown type, create generic element
						m_elements[i] = new NetworkMapElement(request, varId);
						break;
				}
				varId += 100;
			}
		}
		else
		{
			safe_free_and_null(m_elements);
		}

		for(int i = 0; i < m_numLinks; i++)
			delete m_links[i];

		m_numLinks = request->GetVariableLong(VID_NUM_LINKS);
		if (m_numLinks > 0)
		{
			m_links = (NetworkMapLink **)realloc(m_elements, sizeof(NetworkMapLink *) * m_numLinks);
			DWORD varId = VID_LINK_LIST_BASE;
			for(int i = 0; i < m_numLinks; i++)
			{
				m_links[i] = new NetworkMapLink(request, varId);
				varId += 10;
			}
		}
		else
		{
			safe_free_and_null(m_links);
		}
	}

	return NetObj::ModifyFromMessage(request, TRUE);
}
