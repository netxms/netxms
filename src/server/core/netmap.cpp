/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
#define MAX_DELETED_OBJECT_COUNT 1000

/**
 * Redefined status calculation for network maps group
 */
void NetworkMapGroup::calculateCompoundStatus(bool forcedRecalc)
{
   super::calculateCompoundStatus(forcedRecalc);
   if (m_status == STATUS_UNKNOWN)
      m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool NetworkMapGroup::showThresholdSummary() const
{
	return false;
}

/**
 * Network map object default constructor
 */
NetworkMap::NetworkMap() : super(), m_elements(0, 64, Ownership::True), m_links(0, 64, Ownership::True)
{
	m_mapType = NETMAP_USER_DEFINED;
	m_discoveryRadius = -1;
	m_flags = MF_SHOW_STATUS_ICON;
	m_layout = MAP_LAYOUT_MANUAL;
	m_status = STATUS_NORMAL;
	m_backgroundLatitude = 0;
	m_backgroundLongitude = 0;
	m_backgroundZoom = 1;
	m_backgroundColor = ConfigReadInt(_T("Objects.NetworkMaps.DefaultBackgroundColor"), 0xFFFFFF);
	m_defaultLinkColor = -1;
   m_defaultLinkRouting = 1;  // default routing type "direct"
   m_objectDisplayMode = 0;  // default display mode "icons"
	m_nextElementId = 1;
	m_nextLinkId = 1;
   m_filterSource = nullptr;
   m_filter = nullptr;
   m_linkStylingScriptSource = nullptr;
   m_linkStylingScript = nullptr;
   m_width = 0;
   m_height = 0;
}

/**
 * Network map object default constructor
 */
NetworkMap::NetworkMap(const NetworkMap &src) : super(), m_seedObjects(src.m_seedObjects), m_elements(0, 64, Ownership::True), m_links(0, 64, Ownership::True), m_deletedObjects(src.m_deletedObjects)
{
   m_mapType = src.m_mapType;
   m_discoveryRadius = src.m_discoveryRadius;
   m_flags = src.m_flags;
   m_layout = src.m_layout;
   m_status = STATUS_NORMAL;
   m_backgroundLatitude = src.m_backgroundLatitude;
   m_backgroundLongitude = src.m_backgroundLongitude;
   m_backgroundZoom = src.m_backgroundZoom;
   m_backgroundColor = src.m_backgroundColor;
   m_defaultLinkColor = src.m_defaultLinkColor;
   m_defaultLinkRouting = src.m_defaultLinkRouting;  // default routing type "direct"
   m_objectDisplayMode = src.m_objectDisplayMode;  // default display mode "icons"
   m_nextElementId = src.m_nextElementId;
   m_nextLinkId = src.m_nextLinkId;
   m_filterSource = nullptr;
   m_filter = nullptr;
   setFilter(src.m_filterSource);
   m_linkStylingScript = nullptr;
   m_linkStylingScriptSource = nullptr;
   setLinkStylingScript(src.m_linkStylingScriptSource);
   for(int i = 0; i < src.m_elements.size(); i++)
   {
      m_elements.add(src.m_elements.get(i)->clone());
   }
   for(int i = 0; i < src.m_links.size(); i++)
   {
      m_links.add(new NetworkMapLink(*src.m_links.get(i)));
   }
   m_isHidden = true;
   m_width = src.m_width;
   m_height = src.m_height;
   setCreationTime();
}

/**
 * Create network map object from user session
 */
NetworkMap::NetworkMap(int type, const IntegerArray<uint32_t>& seeds) : super(), m_seedObjects(seeds), m_elements(0, 64, Ownership::True), m_links(0, 64, Ownership::True)
{
	m_mapType = type;
	if (type == MAP_INTERNAL_COMMUNICATION_TOPOLOGY)
	{
	   m_seedObjects.add(FindLocalMgmtNode());
	}
	m_discoveryRadius = -1;
	m_flags = MF_SHOW_STATUS_ICON;
   m_layout = (type == NETMAP_USER_DEFINED) ? MAP_LAYOUT_MANUAL : MAP_LAYOUT_SPRING;
	m_status = STATUS_NORMAL;
	m_backgroundLatitude = 0;
	m_backgroundLongitude = 0;
	m_backgroundZoom = 1;
	m_backgroundColor = ConfigReadInt(_T("Objects.NetworkMaps.DefaultBackgroundColor"), 0xFFFFFF);
	m_defaultLinkColor = -1;
   m_defaultLinkRouting = 1;  // default routing type "direct"
   m_objectDisplayMode = 0;  // default display mode "icons"
	m_nextElementId = 1;
   m_nextLinkId = 1;
   m_filterSource = nullptr;
   m_filter = nullptr;
   m_linkStylingScriptSource = nullptr;
   m_linkStylingScript = nullptr;
   m_isHidden = true;
   m_width = 0;
   m_height = 0;
   setCreationTime();
}

/**
 * Network map object destructor
 */
NetworkMap::~NetworkMap()
{
   delete m_filter;
   MemFree(m_filterSource);
   delete m_linkStylingScript;
   MemFree(m_linkStylingScriptSource);
}

/**
 * Redefined status calculation for network maps
 */
void NetworkMap::calculateCompoundStatus(bool forcedRecalc)
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
               for(int i = 0; i < m_elements.size(); i++)
               {
                  NetworkMapElement *e = m_elements.get(i);
                  if (e->getType() != MAP_ELEMENT_OBJECT)
                     continue;

                  shared_ptr<NetObj> object = FindObjectById(static_cast<NetworkMapObject*>(e)->getObjectId());
                  if (object == nullptr)
                     continue;

                  iChildStatus = object->getPropagatedStatus();
                  if ((iChildStatus < STATUS_UNKNOWN) && (iChildStatus > iMostCriticalStatus))
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
               for(int i = 0; i < m_elements.size(); i++)
               {
                  NetworkMapElement *e = m_elements.get(i);
                  if (e->getType() != MAP_ELEMENT_OBJECT)
                     continue;

                  shared_ptr<NetObj> object = FindObjectById(static_cast<NetworkMapObject*>(e)->getObjectId());
                  if (object == nullptr)
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
         if ((iOldStatus != m_status) || forcedRecalc)
         {
            readLockParentList();
            for(int i = 0; i < getParentList().size(); i++)
               getParentList().get(i)->calculateCompoundStatus();
            unlockParentList();
            setModified(MODIFY_RUNTIME);
         }
      }
   }
   else if ((m_status != STATUS_NORMAL) && (m_status != STATUS_UNMANAGED))
   {
      m_status = STATUS_NORMAL;
      readLockParentList();
      for(int i = 0; i < getParentList().size(); i++)
         getParentList().get(i)->calculateCompoundStatus();
      unlockParentList();
      setModified(MODIFY_RUNTIME);
   }
}

/**
 * Save to database
 */
bool NetworkMap::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("network_maps"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE network_maps SET map_type=?,layout=?,radius=?,background=?,bg_latitude=?,bg_longitude=?,bg_zoom=?,link_color=?,link_routing=?,bg_color=?,object_display_mode=?,filter=?,link_styling_script=?,width=?,height=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO network_maps (map_type,layout,radius,background,bg_latitude,bg_longitude,bg_zoom,link_color,link_routing,bg_color,object_display_mode,filter,link_styling_script,width,height,id) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
      }
      if (hStmt != nullptr)
      {
         lockProperties();

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
         DBBind(hStmt, 13, DB_SQLTYPE_TEXT, m_linkStylingScriptSource, DB_BIND_STATIC);
         DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, m_width);
         DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, m_height);
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);

         unlockProperties();
      }
      else
      {
         success = false;
      }
   }

   // Save elements
   if (success && (m_modified & MODIFY_MAP_CONTENT))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_elements WHERE map_id=?"));

      lockProperties();
      if (success && !m_elements.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO network_map_elements (map_id,element_id,element_type,element_data,flags) VALUES (?,?,?,?,?)"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_elements.size()); i++)
            {
               NetworkMapElement *e = m_elements.get(i);
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
      unlockProperties();

      // Save links
      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_links WHERE map_id=?"));

      lockProperties();
      if (success && !m_links.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb,
                  _T("INSERT INTO network_map_links (map_id,link_id,element1,element2,link_type,link_name,connector_name1,connector_name2,element_data,flags,color_source,color,color_provider) ")
                  _T("VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)"), m_links.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_links.size()); i++)
            {
               NetworkMapLink *l = m_links.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, l->getId());
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, l->getElement1());
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, l->getElement2());
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, l->getType());
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, l->getName(), DB_BIND_STATIC);
               DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, l->getConnector1Name(), DB_BIND_STATIC);
               DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, l->getConnector2Name(), DB_BIND_STATIC);
               DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, l->getConfig(), DB_BIND_STATIC);
               DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, l->getFlags());
               DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, static_cast<int32_t>(l->getColorSource()));
               DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, l->getColor());
               DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, l->getColorProvider(), DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

	// Save seed nodes
   if (success && (m_modified & MODIFY_OTHER))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_seed_nodes WHERE map_id=?"));

      lockProperties();
      if (success && !m_seedObjects.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO network_map_seed_nodes (map_id,seed_node_id) VALUES (?,?)"), m_seedObjects.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_seedObjects.size()); i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_seedObjects.get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

   // Save deleted nodes location
   if (success && (m_modified & MODIFY_MAP_CONTENT))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_deleted_nodes WHERE map_id=?"));

      lockProperties();
      if (success && !m_seedObjects.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO network_map_deleted_nodes (map_id,object_id,element_index,position_x,position_y) VALUES (?,?,?,?,?)"), m_deletedObjects.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_deletedObjects.size()); i++)
            {
               NetworkMapObjectLocation *loc = m_deletedObjects.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, loc->objectId);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, i);
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, loc->posX);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, loc->posY);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockProperties();
   }

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
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_map_deleted_nodes WHERE map_id=?"));
   return success;
}

/**
 * Load from database
 */
bool NetworkMap::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   if (!m_isDeleted)
   {
	   loadACLFromDB(hdb);

		TCHAR query[256];
		_sntprintf(query, 256, _T("SELECT map_type,layout,radius,background,bg_latitude,bg_longitude,bg_zoom,link_color,link_routing,bg_color,object_display_mode,filter,link_styling_script,width,height FROM network_maps WHERE id=%d"), dwId);
		DB_RESULT hResult = DBSelect(hdb, query);
		if (hResult == nullptr)
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

      TCHAR *filter = DBGetField(hResult, 0, 11, nullptr, 0);
      setFilter(filter);
      MemFree(filter);

      TCHAR *script = DBGetField(hResult, 0, 12, nullptr, 0);
      setLinkStylingScript(script);
      MemFree(script);

      m_width = DBGetFieldLong(hResult, 0, 13);
      m_height = DBGetFieldLong(hResult, 0, 14);

      DBFreeResult(hResult);

	   // Load elements
      _sntprintf(query, 256, _T("SELECT element_id,element_type,element_data,flags FROM network_map_elements WHERE map_id=%d"), m_id);
      hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				NetworkMapElement *e;
				UINT32 id = DBGetFieldULong(hResult, i, 0);
				UINT32 flags = DBGetFieldULong(hResult, i, 3);
				Config *config = new Config();
				TCHAR *data = DBGetField(hResult, i, 2, nullptr, 0);
				if (data != nullptr)
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
				m_elements.add(e);
				if (m_nextElementId <= e->getId())
					m_nextElementId = e->getId() + 1;
			}
         DBFreeResult(hResult);
      }

		// Load links
      _sntprintf(query, 256, _T("SELECT link_id,element1,element2,link_type,link_name,connector_name1,connector_name2,element_data,flags,color_source,color,color_provider FROM network_map_links WHERE map_id=%u"), m_id);
      hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				TCHAR buffer[4096];

				NetworkMapLink *l = new NetworkMapLink(DBGetFieldLong(hResult, i, 0), DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2), DBGetFieldLong(hResult, i, 3));
				l->setName(DBGetField(hResult, i, 4, buffer, 256));
				l->setConnector1Name(DBGetField(hResult, i, 5, buffer, 256));
				l->setConnector2Name(DBGetField(hResult, i, 6, buffer, 256));
				l->setConfig(DBGetField(hResult, i, 7, buffer, 4096));
				l->setFlags(DBGetFieldULong(hResult, i, 8));
            l->setColorSource(static_cast<MapLinkColorSource>(DBGetFieldLong(hResult, i, 9)));
            l->setColor(DBGetFieldULong(hResult, i, 10));
            l->setColorProvider(DBGetField(hResult, i, 11, buffer, 4096));
				m_links.add(l);
            if (m_nextLinkId <= l->getId())
               m_nextLinkId = l->getId() + 1;
			}
         DBFreeResult(hResult);
      }

      // Load seed nodes
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT seed_node_id FROM network_map_seed_nodes WHERE map_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int nRows = DBGetNumRows(hResult);
            for(int i = 0; i < nRows; i++)
            {
               m_seedObjects.add(DBGetFieldULong(hResult, i, 0));
            }
            DBFreeResult(hResult);
         }
      }
      DBFreeStatement(hStmt);

      // Load deleted nodes positions
      hStmt = DBPrepare(hdb, _T("SELECT object_id,position_x,position_y FROM network_map_deleted_nodes WHERE map_id=? ORDER BY element_index"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               NetworkMapObjectLocation loc;
               loc.objectId = DBGetFieldULong(hResult, i, 0);
               loc.posX = DBGetFieldULong(hResult, i, 1);
               loc.posY = DBGetFieldULong(hResult, i, 2);
               m_deletedObjects.set(i, loc);
            }
            DBFreeResult(hResult);
         }
      }
      DBFreeStatement(hStmt);
	}

	return true;
}

/**
 * Fill NXCP message with object's data
 */
void NetworkMap::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
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
   msg->setField(VID_LINK_STYLING_SCRIPT, CHECK_NULL_EX(m_linkStylingScriptSource));
   msg->setField(VID_WIDTH, m_width);
   msg->setField(VID_HEIGHT, m_height);

	msg->setField(VID_NUM_ELEMENTS, (UINT32)m_elements.size());
	uint32_t fieldId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_elements.size(); i++)
	{
		m_elements.get(i)->fillMessage(msg, fieldId);
		fieldId += 100;
	}

	msg->setField(VID_NUM_LINKS, (UINT32)m_links.size());
	fieldId = VID_LINK_LIST_BASE;
	for(int i = 0; i < m_links.size(); i++)
	{
		m_links.get(i)->fillMessage(msg, fieldId);
		fieldId += 20;
	}
}

/**
 * Update network map object from NXCP message
 */
uint32_t NetworkMap::modifyFromMessageInternal(const NXCPMessage& msg)
{
	if (msg.isFieldExist(VID_MAP_TYPE))
		m_mapType = msg.getFieldAsInt16(VID_MAP_TYPE);

	if (msg.isFieldExist(VID_LAYOUT))
		m_layout = msg.getFieldAsInt16(VID_LAYOUT);

	if (msg.isFieldExist(VID_SEED_OBJECTS))
		msg.getFieldAsInt32Array(VID_SEED_OBJECTS, &m_seedObjects);

	if (msg.isFieldExist(VID_DISCOVERY_RADIUS))
		m_discoveryRadius = msg.getFieldAsInt32(VID_DISCOVERY_RADIUS);

	if (msg.isFieldExist(VID_LINK_COLOR))
		m_defaultLinkColor = msg.getFieldAsInt32(VID_LINK_COLOR);

	if (msg.isFieldExist(VID_LINK_ROUTING))
		m_defaultLinkRouting = msg.getFieldAsInt16(VID_LINK_ROUTING);

	if (msg.isFieldExist(VID_DISPLAY_MODE))
      m_objectDisplayMode = msg.getFieldAsInt16(VID_DISPLAY_MODE);

	if (msg.isFieldExist(VID_BACKGROUND_COLOR))
		m_backgroundColor = msg.getFieldAsInt32(VID_BACKGROUND_COLOR);

	if (msg.isFieldExist(VID_BACKGROUND))
	{
		m_background = msg.getFieldAsGUID(VID_BACKGROUND);
		m_backgroundLatitude = msg.getFieldAsDouble(VID_BACKGROUND_LATITUDE);
		m_backgroundLongitude = msg.getFieldAsDouble(VID_BACKGROUND_LONGITUDE);
		m_backgroundZoom = msg.getFieldAsInt16(VID_BACKGROUND_ZOOM);
	}

   if (msg.isFieldExist(VID_FILTER))
   {
      TCHAR *filter = Trim(msg.getFieldAsString(VID_FILTER));
      setFilter(filter);
      MemFree(filter);
   }

   if (msg.isFieldExist(VID_LINK_STYLING_SCRIPT))
   {
      TCHAR *script = Trim(msg.getFieldAsString(VID_LINK_STYLING_SCRIPT));
      setLinkStylingScript(script);
      MemFree(script);
   }

   if (msg.isFieldExist(VID_WIDTH))
      m_width = msg.getFieldAsInt32(VID_WIDTH);

   if (msg.isFieldExist(VID_HEIGHT))
      m_height = msg.getFieldAsInt32(VID_HEIGHT);

	if (msg.isFieldExist(VID_NUM_ELEMENTS))
	{
	   ObjectArray<NetworkMapElement> newElements(0, 64, Ownership::False);

		int numElements = (int)msg.getFieldAsUInt32(VID_NUM_ELEMENTS);
		if (numElements > 0)
		{
			uint32_t fieldId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < numElements; i++)
			{
				NetworkMapElement *e;
				int type = (int)msg.getFieldAsUInt16(fieldId + 1);
				switch(type)
				{
					case MAP_ELEMENT_OBJECT:
						e = new NetworkMapObject(msg, fieldId);
						break;
					case MAP_ELEMENT_DECORATION:
						e = new NetworkMapDecoration(msg, fieldId);
						break;
               case MAP_ELEMENT_DCI_CONTAINER:
                  e = new NetworkMapDCIContainer(msg, fieldId);
                  break;
               case MAP_ELEMENT_DCI_IMAGE:
                  e = new NetworkMapDCIImage(msg, fieldId);
                  break;
               case MAP_ELEMENT_TEXT_BOX:
                  e = new NetworkMapTextBox(msg, fieldId);
                  break;
					default:		// Unknown type, create generic element
						e = new NetworkMapElement(msg, fieldId);
						break;
				}
				newElements.add(e);
				fieldId += 100;

				if (m_nextElementId <= e->getId())
					m_nextElementId = e->getId() + 1;
			}
		}
		for(int i = 0; i < newElements.size(); i++)
		{
		   NetworkMapElement *newElement = newElements.get(i);

		   bool found = false;
		   for(int j = 0; j < m_elements.size(); j++)
		   {
	         NetworkMapElement *oldElement = m_elements.get(j);
		      if (newElement->getId() == oldElement->getId() && newElement->getType() == oldElement->getType())
		      {
		         newElement->updateInternalFields(oldElement);
		         found = true;
		         break;
		      }
		   }

         if (newElement->getType() != MAP_ELEMENT_OBJECT)
            continue;

		   if (!found)
		   {
	         for (int i = 0; i < m_deletedObjects.size(); i++)
	         {
	            NetworkMapObjectLocation *l = m_deletedObjects.get(i);
	            if (l->objectId == static_cast<NetworkMapObject*>(newElement)->getObjectId())
	            {
	               static_cast<NetworkMapObject*>(newElement)->setPosition(l->posX, l->posY);
	               m_deletedObjects.remove(i);
	               break;
	            }
	         }
		   }
		}

		for(int j = 0; j < m_elements.size(); j++)
      {
         NetworkMapElement *oldElement = m_elements.get(j);
         if (oldElement->getType() != MAP_ELEMENT_OBJECT)
            continue;

         bool found = false;
		   for(int i = 0; i < newElements.size(); i++)
         {
		      if (newElements.get(i)->getId() == oldElement->getId() )
            {
               found = true;
               break;
            }
         }
         if (!found)
         {
            NetworkMapObjectLocation loc;
            loc.objectId = static_cast<NetworkMapObject*>(oldElement)->getObjectId();
            loc.posX = oldElement->getPosX();
            loc.posY = oldElement->getPosY();
            m_deletedObjects.insert(0, &loc);
            if (m_deletedObjects.size() > MAX_DELETED_OBJECT_COUNT)
               m_deletedObjects.remove(MAX_DELETED_OBJECT_COUNT);
         }
      }
		m_elements.clear();
		m_elements.addAll(newElements);

		m_links.clear();
		int numLinks = msg.getFieldAsUInt32(VID_NUM_LINKS);
		if (numLinks > 0)
		{
			UINT32 varId = VID_LINK_LIST_BASE;
			for(int i = 0; i < numLinks; i++)
			{
			   auto l = new NetworkMapLink(msg, varId);
				m_links.add(l);
				varId += 20;

            if (m_nextLinkId <= l->getId())
               m_nextLinkId = l->getId() + 1;
			}
		}
	}

	return super::modifyFromMessageInternal(msg);
}

/**
 * Update object locations on network map from NXCP message
 */
void NetworkMap::updateObjectLocation(const NXCPMessage& msg)
{
   lockProperties();

   int numElements = msg.getFieldAsInt32(VID_NUM_ELEMENTS);
   if (numElements > 0)
   {
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < numElements; i++)
      {
         NetworkMapElement newElement(msg, fieldId);
         for(int j = 0; j < m_elements.size(); j++)
         {
            NetworkMapElement *oldElement = m_elements.get(j);
            if (oldElement->getId() == newElement.getId())
            {
               oldElement->setPosition(newElement.getPosX(), newElement.getPosY());
               break;
            }
         }
         fieldId += 100;
      }
   }

   int numLinks = msg.getFieldAsInt32(VID_NUM_LINKS);
   if (numLinks > 0)
   {
      uint32_t fieldId = VID_LINK_LIST_BASE;
      for(int i = 0; i < numLinks; i++)
      {
         NetworkMapLink newLink(msg, fieldId);
         for (int j = 0; j < m_links.size(); j++)
         {
            NetworkMapLink *oldLink = m_links.get(j);
            if (oldLink->getId() == newLink.getId())
            {
               oldLink->moveConfig(&newLink);
               break;
            }
         }
         fieldId += 20;
      }
   }

   setModified(MODIFY_MAP_CONTENT);
   unlockProperties();
}

/**
 * Update map content for seeded map types
 */
void NetworkMap::updateContent()
{
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): map type %d"), m_name, m_id, m_mapType);
   if ((m_mapType != MAP_TYPE_CUSTOM) && (m_status != STATUS_UNMANAGED))
   {
      NetworkMapObjectList objects;
      bool success = true;
      for(int i = 0; (i < m_seedObjects.size()) && success; i++)
      {
         shared_ptr<NetObj> seed = FindObjectById(m_seedObjects.get(i));
         if (seed != nullptr)
         {
            if (seed->getObjectClass() == OBJECT_NODE)
            {
               success = updateContent(static_pointer_cast<Node>(seed), &objects);
            }
            else if ((seed->getObjectClass() == OBJECT_CONTAINER) || (seed->getObjectClass() == OBJECT_CLUSTER) || (seed->getObjectClass() == OBJECT_RACK))
            {
               nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): seed object %s [%u] is a container, using child nodes as seeds"), m_name, m_id, seed->getName(), seed->getId());
               unique_ptr<SharedObjectArray<NetObj>> children = seed->getAllChildren(true);
               for(int j = 0; j < children->size(); j++)
               {
                  shared_ptr<NetObj> s = children->getShared(j);
                  if (s->getObjectClass() == OBJECT_NODE)
                  {
                     success = updateContent(static_pointer_cast<Node>(s), &objects);
                  }
               }
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap::updateContent(%s [%u]): seed object [%u] cannot be found"), m_name, m_id, m_seedObjects.get(i));
         }
      }
      if (!IsShutdownInProgress() && success)
      {
         updateObjects(&objects);
      }
   }
   if (m_status != STATUS_UNMANAGED)
   {
      updateLinks();
   }
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): completed"), m_name, m_id);
}

/**
 * Update map content for seeded map types using specific seed
 */
bool NetworkMap::updateContent(const shared_ptr<Node>& seed, NetworkMapObjectList *objects)
{
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): reading topology information from node %s [%u]"), m_name, m_id, seed->getName(), seed->getId());

   uint32_t status;
   shared_ptr<NetworkMapObjectList> topology;
   lockProperties();
   NetworkMap *filterProvider = (m_flags & MF_FILTER_OBJECTS) && (m_filter != nullptr) ? this : nullptr;
   unlockProperties();
   switch(m_mapType)
   {
      case MAP_TYPE_LAYER2_TOPOLOGY:
         topology = seed->buildL2Topology(&status, m_discoveryRadius, (m_flags & MF_SHOW_END_NODES) != 0, (m_flags & MF_USE_L1_TOPOLOGY) != 0, filterProvider);
         break;
      case MAP_TYPE_IP_TOPOLOGY:
         topology = shared_ptr<NetworkMapObjectList>(BuildIPTopology(seed, filterProvider, m_discoveryRadius, (m_flags & MF_SHOW_END_NODES) != 0).release());
         break;
      case MAP_INTERNAL_COMMUNICATION_TOPOLOGY:
         topology = shared_ptr<NetworkMapObjectList>(seed->buildInternalCommunicationTopology().release());
         break;
      case MAP_TYPE_OSPF_TOPOLOGY:
         topology = shared_ptr<NetworkMapObjectList>(BuildOSPFTopology(seed, filterProvider, m_discoveryRadius).release());
         break;
      default:
         break;
   }

   if (topology == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap::updateContent(%s [%u]): cannot get topology information for node %s [%d], map won't be updated"), m_name, m_id, seed->getName(), seed->getId());
      return false;
   }

   objects->merge(*topology);
   return true;
}

/**
 * Update objects from given list
 */
void NetworkMap::updateObjects(NetworkMapObjectList *objects)
{
   bool modified = false;

   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u]): updateObjects called"), m_name, m_id);

   lockProperties();

   // remove non-existing links
   for(int i = 0; i < m_links.size(); i++)
   {
      NetworkMapLink *link = m_links.get(i);
      if (!link->checkFlagSet(AUTO_GENERATED))
         continue;

      uint32_t objID1 = objectIdFromElementId(link->getElement1());
      uint32_t objID2 = objectIdFromElementId(link->getElement2());
      bool linkExists = false;
      if (objects->isLinkExist(objID1, objID2, link->getType()))
      {
         linkExists = true;
      }
      else if (objects->isLinkExist(objID2, objID1, link->getType()))
      {
         link->swap();
         linkExists = true;
      }

      if (!linkExists)
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: link %u - %u removed"), m_name, m_id, link->getElement1(), link->getElement2());
         m_links.remove(i);
         i--;
         modified = true;
      }
   }

   // remove non-existing objects
   for(int i = 0; i < m_elements.size(); i++)
   {
      NetworkMapElement *e = m_elements.get(i);
      if ((e->getType() != MAP_ELEMENT_OBJECT) || !(e->getFlags() & AUTO_GENERATED))
         continue;

      NetworkMapObject *netMapObject = static_cast<NetworkMapObject*>(e);
      uint32_t objectId = netMapObject->getObjectId();
      if (!objects->isObjectExist(objectId))
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: object element %u (object %u) removed"), m_name, m_id, e->getId(), objectId);
         NetworkMapObjectLocation loc;
         loc.objectId = objectId;
         loc.posX = netMapObject->getPosX();
         loc.posY = netMapObject->getPosY();
         m_deletedObjects.insert(0, &loc);
         if (m_deletedObjects.size() > MAX_DELETED_OBJECT_COUNT)
            m_deletedObjects.remove(MAX_DELETED_OBJECT_COUNT);
         m_elements.remove(i);
         i--;
         modified = true;
      }
   }

   // add new objects
   for(int i = 0; i < objects->getNumObjects(); i++)
   {
      bool found = false;
      NetworkMapElement *e = nullptr;
      for(int j = 0; j < m_elements.size(); j++)
      {
         e = m_elements.get(j);
         if (e->getType() != MAP_ELEMENT_OBJECT)
            continue;
         if (static_cast<NetworkMapObject*>(e)->getObjectId() == objects->getObjects().get(i))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         uint32_t objectId = objects->getObjects().get(i);
         NetworkMapObject *e = new NetworkMapObject(m_nextElementId++, objectId, 1);
         for (int i = 0; i < m_deletedObjects.size(); i++)
         {
            NetworkMapObjectLocation *l = m_deletedObjects.get(i);
            if (l->objectId == objectId)
            {
               e->setPosition(l->posX, l->posY);
               m_deletedObjects.remove(i);
               break;
            }
         }
         m_elements.add(e);
         modified = true;
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: new object %u (element ID %u) added"), m_name, m_id, objectId, e->getId());
      }
   }

   // add new links and update existing
   const ObjectArray<ObjLink>& links = objects->getLinks();
   for(int i = 0; i < links.size(); i++)
   {
      ObjLink *newLink = links.get(i);
      NetworkMapLink *link = nullptr;
      bool isNew = true;
      for(int j = 0; j < m_links.size(); j++)
      {
         NetworkMapLink *currLink = m_links.get(j);
         uint32_t obj1 = objectIdFromElementId(currLink->getElement1());
         uint32_t obj2 = objectIdFromElementId(currLink->getElement2());
         if ((newLink->id1 == obj1) && (newLink->id2 == obj2) && (newLink->type == currLink->getType()))
         {
            link = currLink;
            isNew = false;
            break;
         }
      }

      // Add new link if needed
      if (link == nullptr)
      {
         uint32_t e1 = elementIdFromObjectId(newLink->id1);
         uint32_t e2 = elementIdFromObjectId(newLink->id2);
         // Element ID can be 0 if link points to object removed by filter
         if ((e1 != 0) && (e2 != 0))
         {
            link = new NetworkMapLink(m_nextLinkId++, e1, e2, newLink->type);
            link->setColorSource(MAP_LINK_COLOR_SOURCE_OBJECT_STATUS);
            link->setFlags(AUTO_GENERATED);
            m_links.add(link);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: cannot add link because elements are missing for object pair (%u,%u)"),
                     m_name, m_id, newLink->id1, newLink->id2);
         }
      }

      // Update link properties
      if (link != nullptr)
      {
         bool updated = link->update(*newLink, !(m_flags & MF_DONT_UPDATE_LINK_TEXT));
         if (updated || isNew)
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: link %u (%u) - %u (%u) %s"),
                     m_name, m_id, link->getElement1(), newLink->id1, link->getElement2(), newLink->id2, isNew ? _T("added") : _T("updated"));
            modified = true;
         }
      }
   }

   if (modified)
      setModified(MODIFY_MAP_CONTENT);

   unlockProperties();
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s): updateObjects completed"), m_name);
}

/**
 * Update links that have computed attributes (color, text, etc.)
 */
void NetworkMap::updateLinks()
{
   ObjectArray<NetworkMapLink> colorUpdateList(0, 64, Ownership::True);
   ObjectArray<NetworkMapLinkNXSLContainer> stylingUpdateList(0, 64, Ownership::True);

   lockProperties();
   for(int i = 0; i < m_links.size(); i++)
   {
      NetworkMapLink *link = m_links.get(i);
      if (link->getColorSource() == MAP_LINK_COLOR_SOURCE_SCRIPT)
      {
         // Replace element IDs with actual object IDs in temporary link object
         auto temp = new NetworkMapLink(*link);
         temp->setConnectedElements(objectIdFromElementId(link->getElement1()), objectIdFromElementId(link->getElement2()));
         colorUpdateList.add(temp);
      }
      if (m_linkStylingScript != nullptr)
      {
         // Replace element IDs with actual object IDs in temporary link object
         auto temp = new NetworkMapLinkNXSLContainer(*link);
         temp->get()->setConnectedElements(objectIdFromElementId(link->getElement1()), objectIdFromElementId(link->getElement2()));
         stylingUpdateList.add(temp);
      }
   }
   unlockProperties();

   if (colorUpdateList.isEmpty() && stylingUpdateList.isEmpty())
      return;

   for(int i = 0; i < colorUpdateList.size(); i++)
   {
      NetworkMapLink *link = colorUpdateList.get(i);
      ScriptVMHandle vm = CreateServerScriptVM(link->getColorProvider(), self());
      if (vm.isValid())
      {
         const shared_ptr<NetObj> endpoint1 = FindObjectById(link->getElement1());
         const shared_ptr<NetObj> endpoint2 = FindObjectById(link->getElement2());
         if (endpoint1 != nullptr)
            vm->setGlobalVariable("$endpoint1", endpoint1->createNXSLObject(vm));
         if (endpoint2 != nullptr)
            vm->setGlobalVariable("$endpoint2", endpoint2->createNXSLObject(vm));
         if (vm->run())
         {
            NXSL_Value *result = vm->getResult();
            if (result->isInteger())
            {
               Color color(result->getValueAsUInt32());
               color.swap();  // Assume color returned as 0xRRGGBB, but Java UI expects 0xBBGGRR
               link->setColor(color.toInteger());
            }
            else if (result->isString())
            {
               link->setColor(Color::parseCSS(result->getValueAsCString()).toInteger());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 4, _T("NetworkMap::updateLinks(%s [%u]): color provider script \"%s\" execution error: %s"),
                      m_name, m_id, link->getColorProvider(), vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_NETMAP, this, 0, vm->getErrorText(), link->getColorProvider());
         }
         vm.destroy();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap::updateLinks(%s [%u]): color provider script \"%s\" %s"), m_name, m_id,
                  link->getColorProvider(), (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) ? _T("is empty") : _T("not found"));
      }
   }

   bool modified = false;
   NXSL_VM *vm = nullptr;
   lockProperties();
   if (m_linkStylingScript != nullptr)
   {
      vm = CreateServerScriptVM(m_linkStylingScript, self());
   }
   unlockProperties();

   if (vm != nullptr)
   {
      for(int i = 0; i < stylingUpdateList.size(); i++)
      {
         vm->setGlobalVariable("$link", vm->createValue(vm->createObject(&g_nxslNetworkMapLinkClass, stylingUpdateList.get(i))));
         if (!vm->run())
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 4, _T("NetworkMap::updateLinks(%s [%u]): link styling script execution error: %s"),
                      m_name, m_id, vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_NETMAP, this, 0, vm->getErrorText(), _T("NetworkMap::%s::LinkStylingScript"), m_name);
         }
      }
      delete vm;
   }
   else
   {
      nxlog_write(NXLOG_WARNING, _T("Failed to create link update script VM for network map object %s [%u]"), m_name, m_id);
      ReportScriptError(SCRIPT_CONTEXT_NETMAP, this, 0, vm->getErrorText(), _T("NetworkMap::%s::LinkStylingScript"), m_name);
   }

   lockProperties();
   for(int i = 0; i < stylingUpdateList.size(); i++)
   {
      NetworkMapLinkNXSLContainer *linkUpdate = stylingUpdateList.get(i);
      for(int j = 0; j < m_links.size(); j++)
      {
         NetworkMapLink *link = m_links.get(j);
         if ((link->getId() == linkUpdate->get()->getId()))
         {
            if (linkUpdate->isModified())
            {
               linkUpdate->updateConfig();
               linkUpdate->get()->setConnectedElements(link->getElement1(), link->getElement2());
               m_links.replace(j, linkUpdate->take());
               modified = true;
            }
            break;
         }
      }
   }
   for(int i = 0; i < colorUpdateList.size(); i++)
   {
      NetworkMapLink *linkUpdate = colorUpdateList.get(i);
      for(int j = 0; j < m_links.size(); j++)
      {
         NetworkMapLink *link = m_links.get(j);
         if ((link->getId() == linkUpdate->getId()))
         {
            if ((link->getColorSource() == MAP_LINK_COLOR_SOURCE_SCRIPT) && (link->getColor() != linkUpdate->getColor()))
            {
               link->setColor(linkUpdate->getColor());
               modified = true;
            }
            break;
         }
      }
   }

   if (modified)
      setModified(MODIFY_MAP_CONTENT);
   unlockProperties();
}

/**
 * Get object ID from map element ID
 * Assumes that object data already locked
 */
uint32_t NetworkMap::objectIdFromElementId(uint32_t eid)
{
	for(int i = 0; i < m_elements.size(); i++)
	{
		NetworkMapElement *e = m_elements.get(i);
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
uint32_t NetworkMap::elementIdFromObjectId(uint32_t oid)
{
	for(int i = 0; i < m_elements.size(); i++)
	{
		NetworkMapElement *e = m_elements.get(i);
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
 * @param filter new filter script code or nullptr to clear filter
 */
void NetworkMap::setFilter(const TCHAR *filter)
{
	MemFree(m_filterSource);
	delete m_filter;
	if ((filter != nullptr) && (*filter != 0))
	{
		m_filterSource = MemCopyString(filter);
		NXSL_CompilationDiagnostic diag;
		m_filter = NXSLCompileAndCreateVM(m_filterSource, new NXSL_ServerEnv(), &diag);
		if (m_filter == nullptr)
			nxlog_write(NXLOG_WARNING, _T("Failed to compile filter script for network map object %s [%u] (%s)"), m_name, m_id, diag.errorText.cstr());
	}
	else
	{
		m_filterSource = nullptr;
		m_filter = nullptr;
	}
}

/**
 * Set link styling script. Object properties must be already locked.
 *
 * @param script new link styling script code or nullptr to clear filter
 */
void NetworkMap::setLinkStylingScript(const TCHAR *script)
{
   MemFree(m_linkStylingScriptSource);
   delete m_linkStylingScript;
   if ((script != nullptr) && (*script != 0))
   {
      m_linkStylingScriptSource = MemCopyString(script);
      m_linkStylingScript = CompileServerScript(script, SCRIPT_CONTEXT_NETMAP, this, 0, _T("NetworkMap::%s::LinkStylingScript"), m_name);
   }
   else
   {
      m_linkStylingScriptSource = nullptr;
      m_linkStylingScript = nullptr;
   }
}

/**
 * Check if given object should be placed on map
 */
bool NetworkMap::isAllowedOnMap(const shared_ptr<NetObj>& object)
{
	bool result = true;

	lockProperties();
	if (m_filter != nullptr)
	{
	   SetupServerScriptVM(m_filter, object, shared_ptr<DCObjectInfo>());
		if (m_filter->run())
		{
			result = m_filter->getResult()->getValueAsBoolean();
		}
		else
		{
		   ReportScriptError(SCRIPT_CONTEXT_NETMAP, object.get(), 0, m_filter->getErrorText(), _T("NetworkMap::%s::Filter"), m_name);
		}
	}
	unlockProperties();
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap(%s [%u]): running object filter for %s [%u], result = %s"), m_name, m_id, object->getName(), object->getId(), BooleanToString(result));
	return result;
}

/**
*  Delete object from the map if it's deleted in session
*/
void NetworkMap::onObjectDelete(const NetObj& object)
{
   lockProperties();

   uint32_t elementId = elementIdFromObjectId(object.getId());

   int i = 0;
   while(i < m_links.size())
   {
      NetworkMapLink *link = m_links.get(i);
      if (link->getElement1() == elementId || link->getElement2() == elementId)
      {
         m_links.remove(i);
      }
      else
      {
         i++;
      }
   }

   i = 0;
   while(i < m_elements.size())
   {
      NetworkMapElement *element = m_elements.get(i);
      if ((element->getType() == MAP_ELEMENT_OBJECT) && (static_cast<NetworkMapObject*>(element)->getObjectId() == object.getId()))
      {
         m_elements.remove(i);
         break;
      }
      else
      {
         i++;
      }
   }

   setModified(MODIFY_MAP_CONTENT);

   unlockProperties();

   super::onObjectDelete(object);
}

/**
 * Clone network map
 */
void NetworkMap::clone(const TCHAR *name, const TCHAR *alias)
{
   lockProperties();
   shared_ptr<NetworkMap> object = make_shared<NetworkMap>(*this);
   unlockProperties();

   object->setName(name);
   object->setAlias(alias);
   NetObjInsert(object, true, false);
   auto parentList = getParentList();
   parentList.get(0)->addChild(object);
   object->addParent(parentList.getShared(0));
   parentList.get(0)->calculateCompoundStatus();
   object->unhide();
}

/**
 * Serialize object to JSON
 */
json_t *NetworkMap::toJson()
{
   json_t *root = super::toJson();

   lockProperties();

   json_object_set_new(root, "mapType", json_integer(m_mapType));
   json_object_set_new(root, "seedObjects", m_seedObjects.toJson());
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
   json_object_set_new(root, "linkScript", json_string_t(m_linkStylingScriptSource));
   json_object_set_new(root, "width", json_integer(m_width));
   json_object_set_new(root, "height", json_integer(m_height));

   unlockProperties();
   return root;
}
