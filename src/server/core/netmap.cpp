/*
** NetXMS - Network Management System
** Copyright (C) 2003-2018 Raden Solutions
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

#define DEBUG_TAG_NETMAP   _T("obj.netmap")

/**
 * Redefined status calculation for network maps group
 */
void NetworkMapGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool NetworkMapGroup::showThresholdSummary()
{
	return false;
}

/**
 * Network map object default constructor
 */
NetworkMap::NetworkMap() : super()
{
	m_mapType = NETMAP_USER_DEFINED;
	m_discoveryRadius = -1;
	m_flags = MF_SHOW_STATUS_ICON;
	m_layout = MAP_LAYOUT_MANUAL;
	m_status = STATUS_NORMAL;
	m_backgroundLatitude = 0;
	m_backgroundLongitude = 0;
	m_backgroundZoom = 1;
	m_backgroundColor = ConfigReadInt(_T("DefaultMapBackgroundColor"), 0xFFFFFF);
	m_defaultLinkColor = -1;
   m_defaultLinkRouting = 1;  // default routing type "direct"
   m_objectDisplayMode = 0;  // default display mode "icons"
	m_nextElementId = 1;
	m_elements = new ObjectArray<NetworkMapElement>(0, 32, true);
	m_links = new ObjectArray<NetworkMapLink>(0, 32, true);
   m_filterSource = NULL;
   m_filter = NULL;
   m_seedObjects = new IntegerArray<UINT32>();
}

/**
 * Create network map object from user session
 */
NetworkMap::NetworkMap(int type, IntegerArray<UINT32> *seeds) : super()
{
	m_mapType = type;
	if (type == MAP_INTERNAL_COMMUNICATION_TOPOLOGY)
	{
	   m_seedObjects = new IntegerArray<UINT32>();
	   m_seedObjects->add(FindLocalMgmtNode());
	}
	else
	   m_seedObjects = new IntegerArray<UINT32>(seeds);
	m_discoveryRadius = -1;
	m_flags = MF_SHOW_STATUS_ICON;
   m_layout = (type == NETMAP_USER_DEFINED) ? MAP_LAYOUT_MANUAL : MAP_LAYOUT_SPRING;
	m_status = STATUS_NORMAL;
	m_backgroundLatitude = 0;
	m_backgroundLongitude = 0;
	m_backgroundZoom = 1;
	m_backgroundColor = ConfigReadInt(_T("DefaultMapBackgroundColor"), 0xFFFFFF);
	m_defaultLinkColor = -1;
   m_defaultLinkRouting = 1;  // default routing type "direct"
   m_objectDisplayMode = 0;  // default display mode "icons"
	m_nextElementId = 1;
	m_elements = new ObjectArray<NetworkMapElement>(0, 32, true);
	m_links = new ObjectArray<NetworkMapLink>(0, 32, true);
   m_filterSource = NULL;
   m_filter = NULL;
	m_isHidden = true;
}

/**
 * Network map object destructor
 */
NetworkMap::~NetworkMap()
{
	delete m_elements;
	delete m_links;
   delete m_filter;
   delete m_seedObjects;
   free(m_filterSource);
}

/**
 * Redefined status calculation for network maps
 */
void NetworkMap::calculateCompoundStatus(BOOL bForcedRecalc)
{
   if (m_flags & MF_CALCULATE_STATUS)
   {
      if (m_status != STATUS_UNMANAGED)
      {
         int iMostCriticalStatus, iCount, iStatusAlg;
         int nSingleThreshold, *pnThresholds, iOldStatus = m_status;
         int nRating[5], iChildStatus, nThresholds[4];

         lockProperties();
         if (m_statusCalcAlg == SA_CALCULATE_DEFAULT)
         {
            iStatusAlg = GetDefaultStatusCalculation(&nSingleThreshold, &pnThresholds);
         }
         else
         {
            iStatusAlg = m_statusCalcAlg;
            nSingleThreshold = m_statusSingleThreshold;
            pnThresholds = m_statusThresholds;
         }
         if (iStatusAlg == SA_CALCULATE_SINGLE_THRESHOLD)
         {
            for(int i = 0; i < 4; i++)
               nThresholds[i] = nSingleThreshold;
            pnThresholds = nThresholds;
         }

         switch(iStatusAlg)
         {
            case SA_CALCULATE_MOST_CRITICAL:
               iCount = 0;
               iMostCriticalStatus = -1;
               for(int i = 0; i < m_elements->size(); i++)
               {
                  NetworkMapElement *e = m_elements->get(i);
                  if (e->getType() != MAP_ELEMENT_OBJECT)
                     continue;

                  NetObj *object = FindObjectById(((NetworkMapObject *)e)->getObjectId());
                  if (object == NULL)
                     continue;

                  iChildStatus = object->getPropagatedStatus();
                  if ((iChildStatus < STATUS_UNKNOWN) &&
                      (iChildStatus > iMostCriticalStatus))
                  {
                     iMostCriticalStatus = iChildStatus;
                     iCount++;
                  }
               }
               m_status = (iCount > 0) ? iMostCriticalStatus : STATUS_NORMAL;
               break;
            case SA_CALCULATE_SINGLE_THRESHOLD:
            case SA_CALCULATE_MULTIPLE_THRESHOLDS:
               // Step 1: calculate severity ratings
               memset(nRating, 0, sizeof(int) * 5);
               iCount = 0;
               for(int i = 0; i < m_elements->size(); i++)
               {
                  NetworkMapElement *e = m_elements->get(i);
                  if (e->getType() != MAP_ELEMENT_OBJECT)
                     continue;

                  NetObj *object = FindObjectById(((NetworkMapObject *)e)->getObjectId());
                  if (object == NULL)
                     continue;

                  iChildStatus = object->getPropagatedStatus();
                  if (iChildStatus < STATUS_UNKNOWN)
                  {
                     while(iChildStatus >= 0)
                        nRating[iChildStatus--]++;
                     iCount++;
                  }
               }

               // Step 2: check what severity rating is above threshold
               if (iCount > 0)
               {
                  int i;
                  for(i = 4; i > 0; i--)
                     if (nRating[i] * 100 / iCount >= pnThresholds[i - 1])
                        break;
                  m_status = i;
               }
               else
               {
                  m_status = STATUS_NORMAL;
               }
               break;
            default:
               m_status = STATUS_NORMAL;
               break;
         }
         unlockProperties();

         // Cause parent object(s) to recalculate it's status
         if ((iOldStatus != m_status) || bForcedRecalc)
         {
            lockParentList(false);
            for(int i = 0; i < m_parentList->size(); i++)
               m_parentList->get(i)->calculateCompoundStatus();
            unlockParentList();
            lockProperties();
            setModified(MODIFY_RUNTIME);
            unlockProperties();
         }
      }
   }
   else
   {
      if (m_status != STATUS_NORMAL)
      {
         m_status = STATUS_NORMAL;
         lockParentList(false);
         for(int i = 0; i < m_parentList->size(); i++)
            m_parentList->get(i)->calculateCompoundStatus();
         unlockParentList();
         lockProperties();
         setModified(MODIFY_RUNTIME);
         unlockProperties();
      }
   }
}

/**
 * Save to database
 */
bool NetworkMap::saveToDatabase(DB_HANDLE hdb)
{
	lockProperties();

	bool success = saveCommonProperties(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("network_maps"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE network_maps SET map_type=?,layout=?,radius=?,background=?,bg_latitude=?,bg_longitude=?,bg_zoom=?,link_color=?,link_routing=?,bg_color=?,object_display_mode=?,filter=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO network_maps (map_type,layout,radius,background,bg_latitude,bg_longitude,bg_zoom,link_color,link_routing,bg_color,object_display_mode,filter,id) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)"));
      }
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT32)m_mapType);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT32)m_layout);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (INT32)m_discoveryRadius);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_background);
         DBBind(hStmt, 5, DB_SQLTYPE_DOUBLE, m_backgroundLatitude);
         DBBind(hStmt, 6, DB_SQLTYPE_DOUBLE, m_backgroundLongitude);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (INT32)m_backgroundZoom);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (INT32)m_defaultLinkColor);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (INT32)m_defaultLinkRouting);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, (INT32)m_backgroundColor);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (INT32)m_objectDisplayMode);
         DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, m_filterSource, DB_BIND_STATIC);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
	   success = saveACLToDB(hdb);

   // Save elements
   if (success && (m_modified & MODIFY_MAP_CONTENT))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_elements WHERE map_id=?"));

      if (success && (m_elements->size() > 0))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO network_map_elements (map_id,element_id,element_type,element_data,flags) VALUES (?,?,?,?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_elements->size()); i++)
            {
               NetworkMapElement *e = m_elements->get(i);
               Config *config = new Config();
               config->setTopLevelTag(_T("element"));
               e->updateConfig(config);

               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, e->getId());
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, e->getType());
               DBBind(hStmt, 4, DB_SQLTYPE_TEXT, config->createXml(), DB_BIND_TRANSIENT);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, e->getFlags());

               success = DBExecute(hStmt);
               delete config;
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }

      // Save links
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_links WHERE map_id=?"));

      if (success && (m_links->size() > 0))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO network_map_links (map_id,element1,element2,link_type,link_name,connector_name1,connector_name2,element_data,flags) VALUES (?,?,?,?,?,?,?,?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_links->size()); i++)
            {
               NetworkMapLink *l = m_links->get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, l->getElement1());
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, l->getElement2());
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)l->getType());
               DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, l->getName(), DB_BIND_STATIC);
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, l->getConnector1Name(), DB_BIND_STATIC);
               DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, l->getConnector2Name(), DB_BIND_STATIC);
               DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, l->getConfig(), DB_BIND_STATIC);
               DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, l->getFlags());
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

	// Save seed nodes
   if (success && (m_modified & MODIFY_OTHER))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_seed_nodes WHERE map_id=?"));

      if (success && (m_seedObjects->size() > 0))
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO network_map_seed_nodes (map_id,seed_node_id) VALUES (?,?)"));
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_seedObjects->size()); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_seedObjects->get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

   m_modified = 0;

   unlockProperties();
	return success;
}

/**
 * Delete from database
 */
bool NetworkMap::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_maps WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_elements WHERE map_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_links WHERE map_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_seed_nodes WHERE map_id=?"));
   return success;
}

/**
 * Load from database
 */
bool NetworkMap::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
	m_id = dwId;

	if (!loadCommonProperties(hdb))
   {
      DbgPrintf(2, _T("Cannot load common properties for network map object %d"), dwId);
      return false;
   }

   if (!m_isDeleted)
   {
		TCHAR query[256];

	   loadACLFromDB(hdb);

		_sntprintf(query, 256, _T("SELECT map_type,layout,radius,background,bg_latitude,bg_longitude,bg_zoom,link_color,link_routing,bg_color,object_display_mode,filter FROM network_maps WHERE id=%d"), dwId);
		DB_RESULT hResult = DBSelect(hdb, query);
		if (hResult == NULL)
			return false;

		m_mapType = DBGetFieldLong(hResult, 0, 0);
		m_layout = DBGetFieldLong(hResult, 0, 1);
		m_discoveryRadius = DBGetFieldLong(hResult, 0, 2);
		m_background = DBGetFieldGUID(hResult, 0, 3);
		m_backgroundLatitude = DBGetFieldDouble(hResult, 0, 4);
		m_backgroundLongitude = DBGetFieldDouble(hResult, 0, 5);
		m_backgroundZoom = (int)DBGetFieldLong(hResult, 0, 6);
		m_defaultLinkColor = DBGetFieldLong(hResult, 0, 7);
		m_defaultLinkRouting = DBGetFieldLong(hResult, 0, 8);
		m_backgroundColor = DBGetFieldLong(hResult, 0, 9);
		m_objectDisplayMode = DBGetFieldLong(hResult, 0, 10);

      TCHAR *filter = DBGetField(hResult, 0, 12, NULL, 0);
      setFilter(filter);
      MemFree(filter);

      DBFreeResult(hResult);

	   // Load elements
      _sntprintf(query, 256, _T("SELECT element_id,element_type,element_data,flags FROM network_map_elements WHERE map_id=%d"), m_id);
      hResult = DBSelect(hdb, query);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				NetworkMapElement *e;
				UINT32 id = DBGetFieldULong(hResult, i, 0);
				UINT32 flags = DBGetFieldULong(hResult, i, 3);
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
							e = new NetworkMapObject(id, config, flags);
							break;
						case MAP_ELEMENT_DECORATION:
							e = new NetworkMapDecoration(id, config, flags);
							break;
                  case MAP_ELEMENT_DCI_CONTAINER:
                     e = new NetworkMapDCIContainer(id, config, flags);
                     break;
                  case MAP_ELEMENT_DCI_IMAGE:
                     e = new NetworkMapDCIImage(id, config, flags);
                     break;
                  case MAP_ELEMENT_TEXT_BOX:
                     e = new NetworkMapTextBox(id, config, flags);
                     break;
						default:		// Unknown type, create generic element
							e = new NetworkMapElement(id, config, flags);
							break;
					}
				}
				else
				{
					e = new NetworkMapElement(id, flags);
				}
				delete config;
				m_elements->add(e);
				if (m_nextElementId <= e->getId())
					m_nextElementId = e->getId() + 1;
			}
         DBFreeResult(hResult);
      }

		// Load links
      _sntprintf(query, 256, _T("SELECT element1,element2,link_type,link_name,connector_name1,connector_name2,element_data,flags FROM network_map_links WHERE map_id=%d"), m_id);
      hResult = DBSelect(hdb, query);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				TCHAR buffer[4096];

				NetworkMapLink *l = new NetworkMapLink(DBGetFieldLong(hResult, i, 0), DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2));
				l->setName(DBGetField(hResult, i, 3, buffer, 256));
				l->setConnector1Name(DBGetField(hResult, i, 4, buffer, 256));
				l->setConnector2Name(DBGetField(hResult, i, 5, buffer, 256));
				l->setConfig(DBGetField(hResult, i, 6, buffer, 4096));
				l->setFlags(DBGetFieldULong(hResult, i, 7));
				m_links->add(l);
			}
         DBFreeResult(hResult);
      }

      // Load seed nodes
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT seed_node_id FROM network_map_seed_nodes WHERE map_id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            int nRows = DBGetNumRows(hResult);
            for(int i = 0; i < nRows; i++)
            {
               m_seedObjects->add(DBGetFieldULong(hResult, i, 0));
            }
            DBFreeResult(hResult);
         }
      }
      DBFreeStatement(hStmt);
	}

	m_status = STATUS_NORMAL;

	return true;
}

/**
 * Fill NXCP message with object's data
 */
void NetworkMap::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
	super::fillMessageInternal(msg, userId);

	msg->setField(VID_MAP_TYPE, (WORD)m_mapType);
	msg->setField(VID_LAYOUT, (WORD)m_layout);
	msg->setFieldFromInt32Array(VID_SEED_OBJECTS, m_seedObjects);
	msg->setField(VID_DISCOVERY_RADIUS, (UINT32)m_discoveryRadius);
	msg->setField(VID_BACKGROUND, m_background);
	msg->setField(VID_BACKGROUND_LATITUDE, m_backgroundLatitude);
	msg->setField(VID_BACKGROUND_LONGITUDE, m_backgroundLongitude);
	msg->setField(VID_BACKGROUND_ZOOM, (WORD)m_backgroundZoom);
	msg->setField(VID_LINK_COLOR, (UINT32)m_defaultLinkColor);
	msg->setField(VID_LINK_ROUTING, (INT16)m_defaultLinkRouting);
	msg->setField(VID_DISPLAY_MODE, (INT16)m_objectDisplayMode);
	msg->setField(VID_BACKGROUND_COLOR, (UINT32)m_backgroundColor);
   msg->setField(VID_FILTER, CHECK_NULL_EX(m_filterSource));

	msg->setField(VID_NUM_ELEMENTS, (UINT32)m_elements->size());
	UINT32 varId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_elements->size(); i++)
	{
		m_elements->get(i)->fillMessage(msg, varId);
		varId += 100;
	}

	msg->setField(VID_NUM_LINKS, (UINT32)m_links->size());
	varId = VID_LINK_LIST_BASE;
	for(int i = 0; i < m_links->size(); i++)
	{
		m_links->get(i)->fillMessage(msg, varId);
		varId += 20;
	}
}

/**
 * Update network map object from NXCP message
 */
UINT32 NetworkMap::modifyFromMessageInternal(NXCPMessage *request)
{
	if (request->isFieldExist(VID_MAP_TYPE))
		m_mapType = (int)request->getFieldAsUInt16(VID_MAP_TYPE);

	if (request->isFieldExist(VID_LAYOUT))
		m_layout = (int)request->getFieldAsUInt16(VID_LAYOUT);

	if (request->isFieldExist(VID_FLAGS))
	{
	   UINT32 mask = request->isFieldExist(VID_FLAGS_MASK) ? request->getFieldAsUInt32(VID_FLAGS_MASK) : 0xFFFFFFFF;
	   m_flags &= ~mask;
		m_flags |= (request->getFieldAsUInt32(VID_FLAGS) & mask);
	}

	if (request->isFieldExist(VID_SEED_OBJECTS))
		request->getFieldAsInt32Array(VID_SEED_OBJECTS, m_seedObjects);

	if (request->isFieldExist(VID_DISCOVERY_RADIUS))
		m_discoveryRadius = (int)request->getFieldAsUInt32(VID_DISCOVERY_RADIUS);

	if (request->isFieldExist(VID_LINK_COLOR))
		m_defaultLinkColor = (int)request->getFieldAsUInt32(VID_LINK_COLOR);

	if (request->isFieldExist(VID_LINK_ROUTING))
		m_defaultLinkRouting = (int)request->getFieldAsInt16(VID_LINK_ROUTING);

	if (request->isFieldExist(VID_DISPLAY_MODE))
      m_objectDisplayMode = (int)request->getFieldAsInt16(VID_DISPLAY_MODE);

	if (request->isFieldExist(VID_BACKGROUND_COLOR))
		m_backgroundColor = (int)request->getFieldAsUInt32(VID_BACKGROUND_COLOR);

	if (request->isFieldExist(VID_BACKGROUND))
	{
		m_background = request->getFieldAsGUID(VID_BACKGROUND);
		m_backgroundLatitude = request->getFieldAsDouble(VID_BACKGROUND_LATITUDE);
		m_backgroundLongitude = request->getFieldAsDouble(VID_BACKGROUND_LONGITUDE);
		m_backgroundZoom = (int)request->getFieldAsUInt16(VID_BACKGROUND_ZOOM);
	}

   if (request->isFieldExist(VID_FILTER))
   {
      TCHAR *filter = request->getFieldAsString(VID_FILTER);
      if (filter != NULL)
         StrStrip(filter);
      setFilter(filter);
      MemFree(filter);
   }

	if (request->isFieldExist(VID_NUM_ELEMENTS))
	{
		m_elements->clear();

		int numElements = (int)request->getFieldAsUInt32(VID_NUM_ELEMENTS);
		if (numElements > 0)
		{
			UINT32 varId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < numElements; i++)
			{
				NetworkMapElement *e;
				int type = (int)request->getFieldAsUInt16(varId + 1);
				switch(type)
				{
					case MAP_ELEMENT_OBJECT:
						e = new NetworkMapObject(request, varId);
						break;
					case MAP_ELEMENT_DECORATION:
						e = new NetworkMapDecoration(request, varId);
						break;
               case MAP_ELEMENT_DCI_CONTAINER:
                  e = new NetworkMapDCIContainer(request, varId);
                  break;
               case MAP_ELEMENT_DCI_IMAGE:
                  e = new NetworkMapDCIImage(request, varId);
                  break;
               case MAP_ELEMENT_TEXT_BOX:
                  e = new NetworkMapTextBox(request, varId);
                  break;
					default:		// Unknown type, create generic element
						e = new NetworkMapElement(request, varId);
						break;
				}
				m_elements->add(e);
				varId += 100;

				if (m_nextElementId <= e->getId())
					m_nextElementId = e->getId() + 1;
			}
		}

		m_links->clear();
		int numLinks = request->getFieldAsUInt32(VID_NUM_LINKS);
		if (numLinks > 0)
		{
			UINT32 varId = VID_LINK_LIST_BASE;
			for(int i = 0; i < numLinks; i++)
			{
				m_links->add(new NetworkMapLink(request, varId));
				varId += 20;
			}
		}
	}

	return super::modifyFromMessageInternal(request);
}

/**
 * Update map content for seeded map types
 */
void NetworkMap::updateContent()
{
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): map type %d"), m_name, m_id, m_mapType);
   if (m_mapType != MAP_TYPE_CUSTOM)
   {
      NetworkMapObjectList objects;
      for(int i = 0; i < m_seedObjects->size(); i++)
      {
         Node *seed = (Node *)FindObjectById(m_seedObjects->get(i), OBJECT_NODE);
         if (seed != NULL)
         {
            UINT32 status;
            NetworkMapObjectList *topology;
            switch(m_mapType)
            {
               case MAP_TYPE_LAYER2_TOPOLOGY:
                  topology = seed->buildL2Topology(&status, m_discoveryRadius, (m_flags & MF_SHOW_END_NODES) != 0);
                  break;
               case MAP_TYPE_IP_TOPOLOGY:
                  topology = seed->buildIPTopology(&status, m_discoveryRadius, (m_flags & MF_SHOW_END_NODES) != 0);
                  break;
               case MAP_INTERNAL_COMMUNICATION_TOPOLOGY:
                  topology = seed->buildInternalCommunicationTopology();
                  break;
               default:
                  topology = NULL;
                  break;
            }
            if (topology != NULL)
            {
               objects.merge(topology);
               delete topology;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap::updateContent(%s [%u]): cannot get topology information for node %s [%d]"), m_name, m_id, seed->getName(), seed->getId());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap::updateContent(%s [%u]): seed object %d cannot be found"), m_name, m_id, m_seedObjects->get(i));
         }
      }
      updateObjects(&objects);
   }
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): completed"), m_name, m_id);
}

/**
 * Update objects from given list
 */
void NetworkMap::updateObjects(NetworkMapObjectList *objects)
{
   bool modified = false;

   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u]): updateObjects called"), m_name, m_id);

   // Filter out disallowed objects
   if ((m_flags & MF_FILTER_OBJECTS) && (m_filter != NULL))
   {
      for(int j = 0; j < objects->getNumObjects(); j++)
      {
         IntegerArray<UINT32> *idList = objects->getObjects();
         for(int i = 0; i < idList->size(); i++)
         {
            NetObj *object = FindObjectById(idList->get(i));
            if ((object == NULL) || !isAllowedOnMap(object))
            {
               idList->remove(i);
               i--;
            }
         }
      }
   }

   lockProperties();

   // remove non-existing links
   for(int i = 0; i < m_links->size(); i++)
   {
      NetworkMapLink *link = m_links->get(i);
      if (!link->checkFlagSet(AUTO_GENERATED))
         continue;

      UINT32 objID1 = objectIdFromElementId(link->getElement1());
      UINT32 objID2 = objectIdFromElementId(link->getElement2());
      bool linkExists = false;
      if (objects->isLinkExist(objID1, objID2))
      {
         linkExists = true;
      }
      else if (objects->isLinkExist(objID2, objID1))
      {
         link->swap();
         linkExists = true;
      }

      if (!linkExists)
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: link %u - %u removed"), m_name, m_id, link->getElement1(), link->getElement2());
         m_links->remove(i);
         i--;
         modified = true;
      }
   }

   // remove non-existing objects
   for(int i = 0; i < m_elements->size(); i++)
   {
      NetworkMapElement *e = m_elements->get(i);
      if ((e->getType() != MAP_ELEMENT_OBJECT) || !(e->getFlags() & AUTO_GENERATED))
         continue;

      UINT32 objectId = static_cast<NetworkMapObject*>(e)->getObjectId();
      if (!objects->isObjectExist(objectId))
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: object element %u (object %u) removed"), m_name, m_id, e->getId(), objectId);
         m_elements->remove(i);
         i--;
         modified = true;
      }
   }

   // add new objects
   for(int i = 0; i < objects->getNumObjects(); i++)
   {
      bool found = false;
      for(int j = 0; j < m_elements->size(); j++)
      {
         NetworkMapElement *e = m_elements->get(j);
         if (e->getType() != MAP_ELEMENT_OBJECT)
            continue;
         if (((NetworkMapObject *)e)->getObjectId() == objects->getObjects()->get(i))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         UINT32 objectId = objects->getObjects()->get(i);
         NetworkMapElement *e = new NetworkMapObject(m_nextElementId++, objectId, 1);
         m_elements->add(e);
         modified = true;
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: new object %u (element ID %u) added"), m_name, m_id, objectId, e->getId());
      }
   }

   // add new links
   for(int i = 0; i < objects->getNumLinks(); i++)
   {
      bool found = false;
      for(int j = 0; j < m_links->size(); j++)
      {
         NetworkMapLink *l = m_links->get(j);
         UINT32 obj1 = objectIdFromElementId(l->getElement1());
         UINT32 obj2 = objectIdFromElementId(l->getElement2());
         if ((objects->getLinks()->get(i)->id1 == obj1) && (objects->getLinks()->get(i)->id2 == obj2) && (objects->getLinks()->get(i)->type == l->getType()))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         ObjLink *linkInfo = objects->getLinks()->get(i);
         UINT32 e1 = elementIdFromObjectId(linkInfo->id1);
         UINT32 e2 = elementIdFromObjectId(linkInfo->id2);
         // Element ID can be 0 if link points to object removed by filter
         if ((e1 != 0) && (e2 != 0))
         {
            NetworkMapLink *l = new NetworkMapLink(e1, e2, linkInfo->type);
            l->setConnector1Name(linkInfo->port1);
            l->setConnector2Name(linkInfo->port2);
            l->setName(linkInfo->name);
            String config;
            config.append(_T("<config>\n"));
            config.append(_T("\t<dciList length=\"0\"/>\n"));
            config.append(_T("\t<objectStatusList class=\"java.util.ArrayList\">\n"));

            Node *node = static_cast<Node*>(FindObjectById(linkInfo->id1, OBJECT_NODE));
            if (node != NULL)
            {
               for(int n = 0; n < objects->getLinks()->get(i)->portIdCount; n++)
               {
                  Interface *iface = node->findInterfaceByIndex(linkInfo->portIdArray1[n]);
                  if (iface != NULL)
                  {
                     config.append(_T("\t\t<long>"));
                     config.append(iface->getId());
                     config.append(_T("</long>\n"));
                  }
               }
            }

            node = static_cast<Node*>(FindObjectById(linkInfo->id2, OBJECT_NODE));
            if (node != NULL)
            {
               for(int n = 0; n < linkInfo->portIdCount; n++)
               {
                  Interface *iface = node->findInterfaceByIndex(linkInfo->portIdArray2[n]);
                  if (iface != NULL)
                  {
                     config.append(_T("\t\t<long>"));
                     config.append(iface->getId());
                     config.append(_T("</long>\n"));
                  }
               }
            }

            config.append(_T("\t</objectStatusList>\n"));
            config.append(_T("\t<color>-1</color>\n"));
            config.append(_T("\t<routing>0</routing>\n"));
            config.append(_T("</config>"));
            l->setConfig(config);
            l->setFlags(AUTO_GENERATED);
            m_links->add(l);
            modified = true;
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: link %u (%u) - %u (%u) added"),
                     m_name, m_id, l->getElement1(), linkInfo->id1, l->getElement2(), linkInfo->id2);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: cannot add link because elements are missing for object pair (%u,%u)"),
                     m_name, m_id, linkInfo->id1, linkInfo->id2);
         }
      }
   }

   if (modified)
      setModified(MODIFY_MAP_CONTENT);

   unlockProperties();
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s): updateObjects completed"), m_name);
}

/**
 * Get object ID from map element ID
 * Assumes that object data already locked
 */
UINT32 NetworkMap::objectIdFromElementId(UINT32 eid)
{
	for(int i = 0; i < m_elements->size(); i++)
	{
		NetworkMapElement *e = m_elements->get(i);
		if (e->getId() == eid)
		{
			if (e->getType() == MAP_ELEMENT_OBJECT)
			{
				return ((NetworkMapObject *)e)->getObjectId();
			}
			else
			{
				return 0;
			}
		}
	}
	return 0;
}

/**
 * Get map element ID from object ID
 * Assumes that object data already locked
 */
UINT32 NetworkMap::elementIdFromObjectId(UINT32 oid)
{
	for(int i = 0; i < m_elements->size(); i++)
	{
		NetworkMapElement *e = m_elements->get(i);
		if ((e->getType() == MAP_ELEMENT_OBJECT) && (((NetworkMapObject *)e)->getObjectId() == oid))
		{
			return e->getId();
		}
	}
	return 0;
}

/**
 * Set filter. Object properties must be already locked.
 *
 * @param filter new filter script code or NULL to clear filter
 */
void NetworkMap::setFilter(const TCHAR *filter)
{
	MemFree(m_filterSource);
	delete m_filter;
	if ((filter != NULL) && (*filter != 0))
	{
		TCHAR error[256];

		m_filterSource = _tcsdup(filter);
		m_filter = NXSLCompileAndCreateVM(m_filterSource, error, 256, new NXSL_ServerEnv);
		if (m_filter == NULL)
			nxlog_write(MSG_NETMAP_SCRIPT_COMPILATION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, error);
	}
	else
	{
		m_filterSource = NULL;
		m_filter = NULL;
	}
}

/**
 * Check if given object should be placed on map
 */
bool NetworkMap::isAllowedOnMap(NetObj *object)
{
	bool result = true;

	lockProperties();
	if (m_filter != NULL)
	{
	   SetupServerScriptVM(m_filter, object, NULL);
		if (m_filter->run())
		{
			NXSL_Value *value = m_filter->getResult();
			result = ((value != NULL) && (value->getValueAsInt32() != 0));
		}
		else
		{
			TCHAR buffer[1024];

			_sntprintf(buffer, 1024, _T("NetworkMap::%s::%d"), m_name, m_id);
			PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_filter->getErrorText(), m_id);
			nxlog_write(MSG_NETMAP_SCRIPT_EXECUTION_ERROR, EVENTLOG_WARNING_TYPE, "dss", m_id, m_name, m_filter->getErrorText());
		}
	}
	unlockProperties();
	return result;
}

/**
*  Delete object from the map if it's deleted in session
*/
void NetworkMap::onObjectDelete(UINT32 dwObjectId)
{
   lockProperties();
   UINT32 elementId = elementIdFromObjectId(dwObjectId);
   int i = 0;
   while(i < m_links->size())
   {
      NetworkMapLink* nmLink = (NetworkMapLink*) m_links->get(i);

      if (nmLink->getElement1() == elementId || nmLink->getElement2() == elementId)
      {
         m_links->remove(i);
      }
      else
      {
         i++;
      }
   }

   i = 0;
   while(i < m_elements->size())
   {
      NetworkMapObject* nmObject = (NetworkMapObject*) m_elements->get(i);
      if (nmObject->getObjectId() == dwObjectId)
      {
         m_elements->remove(i);
         break;
      }
      else
      {
         i++;
      }
   }

   setModified(MODIFY_MAP_CONTENT);

   unlockProperties();

   super::onObjectDelete(dwObjectId);
}

/**
 * Serialize object to JSON
 */
json_t *NetworkMap::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "mapType", json_integer(m_mapType));
   json_object_set_new(root, "seedObjects", m_seedObjects->toJson());
   json_object_set_new(root, "discoveryRadius", json_integer(m_discoveryRadius));
   json_object_set_new(root, "layout", json_integer(m_layout));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "backgroundColor", json_integer(m_backgroundColor));
   json_object_set_new(root, "defaultLinkColor", json_integer(m_defaultLinkColor));
   json_object_set_new(root, "defaultLinkRouting", json_integer(m_defaultLinkRouting));
   json_object_set_new(root, "objectDisplayMode", json_integer(m_objectDisplayMode));
   json_object_set_new(root, "background", m_background.toJson());
   json_object_set_new(root, "backgroundLatitude", json_real(m_backgroundLatitude));
   json_object_set_new(root, "backgroundLongitude", json_real(m_backgroundLongitude));
   json_object_set_new(root, "backgroundZoom", json_integer(m_backgroundZoom));
   json_object_set_new(root, "elements", json_object_array(m_elements));
   json_object_set_new(root, "links", json_object_array(m_links));
   json_object_set_new(root, "filter", json_string_t(m_filterSource));
   return root;
}
