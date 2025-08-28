/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
#include <netxms_maps.h>

#define DEBUG_TAG_NETMAP   _T("netmap")
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
NetworkMap::NetworkMap() : super(), AutoBindTarget(this), Pollable(this, Pollable::AUTOBIND | Pollable::MAP_UPDATE), DelegateObject(this)
{
	m_mapType = NETMAP_USER_DEFINED;
	m_discoveryRadius = 0;
	m_flags = MF_SHOW_STATUS_ICON;
	m_layout = MAP_LAYOUT_MANUAL;
	m_status = STATUS_NORMAL;
	m_backgroundLatitude = 0;
	m_backgroundLongitude = 0;
	m_backgroundZoom = 1;
	m_backgroundColor = ConfigReadInt(_T("Objects.NetworkMaps.DefaultBackgroundColor"), 0xFFFFFF);
   m_width = 0;
   m_height = 0;
	m_defaultLinkColor = -1;
   m_defaultLinkRouting = 1;  // default routing type "direct"
   m_defaultLinkWidth = 0; // client default
   m_defaultLinkStyle = 1; // default value solid
   m_objectDisplayMode = 0;  // default display mode "icons"
	m_nextElementId = 1;
	m_nextLinkId = 1;
   m_filterSource = nullptr;
   m_filter = nullptr;
   m_linkStylingScriptSource = nullptr;
   m_linkStylingScript = nullptr;
   m_updateFailed = false;
   m_displayPriority = 0;
}

/**
 * Network map object default constructor
 */
NetworkMap::NetworkMap(const NetworkMap &src) : super(), AutoBindTarget(this), Pollable(this, Pollable::AUTOBIND | Pollable::MAP_UPDATE),
         DelegateObject(this, src), m_seedObjects(src.m_seedObjects), m_mapContent(src.m_mapContent), m_deletedObjects(src.m_deletedObjects)

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
   m_width = src.m_width;
   m_height = src.m_height;
   m_defaultLinkColor = src.m_defaultLinkColor;
   m_defaultLinkRouting = src.m_defaultLinkRouting;  // default routing type "direct"
   m_defaultLinkWidth  = src.m_defaultLinkWidth;
   m_defaultLinkStyle  = src.m_defaultLinkStyle; // default value solid
   m_objectDisplayMode = src.m_objectDisplayMode;  // default display mode "icons"
   m_nextElementId = src.m_nextElementId;
   m_nextLinkId = src.m_nextLinkId;
   m_filterSource = nullptr;
   m_filter = nullptr;
   setFilter(src.m_filterSource);
   m_linkStylingScript = nullptr;
   m_linkStylingScriptSource = nullptr;
   setLinkStylingScript(src.m_linkStylingScriptSource);
   m_isHidden = true;
   m_updateFailed = false;
   m_displayPriority = src.m_displayPriority;
   setCreationTime();
}

/**
 * Create network map object from user session
 */
NetworkMap::NetworkMap(int type, const IntegerArray<uint32_t>& seeds) : super(), AutoBindTarget(this),
         Pollable(this, Pollable::AUTOBIND | Pollable::MAP_UPDATE), DelegateObject(this), m_seedObjects(seeds)
{
	m_mapType = type;
	if (type == MAP_INTERNAL_COMMUNICATION_TOPOLOGY)
	{
	   m_seedObjects.add(FindLocalMgmtNode());
	}
	m_discoveryRadius = 0;
	m_flags = MF_SHOW_STATUS_ICON;
   m_layout = (type == NETMAP_USER_DEFINED) ? MAP_LAYOUT_MANUAL : MAP_LAYOUT_SPRING;
	m_status = STATUS_NORMAL;
	m_backgroundLatitude = 0;
	m_backgroundLongitude = 0;
	m_backgroundZoom = 1;
	m_backgroundColor = ConfigReadInt(_T("Objects.NetworkMaps.DefaultBackgroundColor"), 0xFFFFFF);
   m_width = 0;
   m_height = 0;
	m_defaultLinkColor = -1;
   m_defaultLinkRouting = 1;  // default routing type "direct"
   m_defaultLinkWidth = 0; // client default
   m_defaultLinkStyle = 1; // default value solid
   m_objectDisplayMode = 0;  // default display mode "icons"
	m_nextElementId = 1;
   m_nextLinkId = 1;
   m_filterSource = nullptr;
   m_filter = nullptr;
   m_linkStylingScriptSource = nullptr;
   m_linkStylingScript = nullptr;
   m_updateFailed = false;
   m_isHidden = true;
   m_displayPriority = 0;
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
               for(int i = 0; i < m_mapContent.m_elements.size(); i++)
               {
                  NetworkMapElement *e = m_mapContent.m_elements.get(i);
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
               for(int i = 0; i < m_mapContent.m_elements.size(); i++)
               {
                  NetworkMapElement *e = m_mapContent.m_elements.get(i);
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
      success = AutoBindTarget::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const wchar_t *columns[] = {
         L"map_type", L"layout", L"radius", L"background", L"bg_latitude", L"bg_longitude", L"bg_zoom",
         L"link_color", L"link_routing", L"link_width", L"link_style", L"bg_color", L"object_display_mode",
         L"filter", L"link_styling_script", L"width", L"height", L"display_priority", nullptr
      };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("network_maps"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_mapType);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_layout);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_discoveryRadius);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_background);
         DBBind(hStmt, 5, DB_SQLTYPE_DOUBLE, m_backgroundLatitude);
         DBBind(hStmt, 6, DB_SQLTYPE_DOUBLE, m_backgroundLongitude);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_backgroundZoom);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_defaultLinkColor);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_defaultLinkRouting);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_defaultLinkWidth);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_defaultLinkStyle);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_backgroundColor);
         DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, m_objectDisplayMode);
         DBBind(hStmt, 14, DB_SQLTYPE_VARCHAR, m_filterSource, DB_BIND_STATIC);
         DBBind(hStmt, 15, DB_SQLTYPE_TEXT, m_linkStylingScriptSource, DB_BIND_STATIC);
         DBBind(hStmt, 16, DB_SQLTYPE_INTEGER, m_width);
         DBBind(hStmt, 17, DB_SQLTYPE_INTEGER, m_height);
         DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, m_displayPriority);
         DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, m_id);

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
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_elements WHERE map_id=?");

      lockProperties();
      if (success && !m_mapContent.m_elements.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO network_map_elements (map_id,element_id,element_type,element_data,flags) VALUES (?,?,?,?,?)");
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_mapContent.m_elements.size()); i++)
            {
               NetworkMapElement *e = m_mapContent.m_elements.get(i);
               json_t *config = json_object();
               e->updateConfig(config);

               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, e->getId());
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, e->getType());
               DBBind(hStmt, 4, DB_SQLTYPE_TEXT, config, DB_BIND_TRANSIENT);
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, e->getFlags());

               success = DBExecute(hStmt);
               json_decref(config);
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
         success = executeQueryOnObject(hdb, L"DELETE FROM network_map_links WHERE map_id=?");

      lockProperties();
      if (success && !m_mapContent.m_links.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb,
                  L"INSERT INTO network_map_links (map_id,link_id,element1,interface1,element2,interface2,link_type,link_name,connector_name1,connector_name2,element_data,flags,color_source,color,color_provider) "
                  L"VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", m_mapContent.m_links.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; success && (i < m_mapContent.m_links.size()); i++)
            {
               NetworkMapLink *l = m_mapContent.m_links.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, l->getId());
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, l->getElement1());
               DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, l->getInterface1());
               DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, l->getElement2());
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, l->getInterface2());
               DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, l->getType());
               DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, l->getName(), DB_BIND_STATIC);
               DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, l->getConnector1Name(), DB_BIND_STATIC);
               DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, l->getConnector2Name(), DB_BIND_STATIC);
               DBBind(hStmt, 11, DB_SQLTYPE_VARCHAR, DB_CTYPE_UTF8_STRING, l->getConfig(), DB_BIND_STATIC);
               DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, l->getFlags());
               DBBind(hStmt, 13, DB_SQLTYPE_INTEGER, static_cast<int32_t>(l->getColorSource()));
               DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, l->getColor());
               DBBind(hStmt, 15, DB_SQLTYPE_VARCHAR, l->getColorProvider(), DB_BIND_STATIC);
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
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_seed_nodes WHERE map_id=?");

      lockProperties();
      if (success && !m_seedObjects.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO network_map_seed_nodes (map_id,seed_node_id) VALUES (?,?)", m_seedObjects.size() > 1);
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
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_deleted_nodes WHERE map_id=?");

      lockProperties();
      if (success && !m_seedObjects.isEmpty())
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO network_map_deleted_nodes (map_id,object_id,element_index,position_x,position_y) VALUES (?,?,?,?,?)", m_deletedObjects.size() > 1);
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
      success = executeQueryOnObject(hdb, L"DELETE FROM network_maps WHERE id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_elements WHERE map_id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_links WHERE map_id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_seed_nodes WHERE map_id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM network_map_deleted_nodes WHERE map_id=?");
   if (success)
      success = AutoBindTarget::deleteFromDatabase(hdb);
   return success;
}

/**
 * Load from database
 */
bool NetworkMap::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   if (!Pollable::loadFromDatabase(hdb, id))
      return false;

   if (!m_isDeleted)
   {
	   loadACLFromDB(hdb, preparedStatements);

		DB_RESULT hResult = executeSelectOnObject(hdb, L"SELECT map_type,layout,radius,background,bg_latitude,bg_longitude,bg_zoom,link_color,link_routing,link_width,link_style,bg_color,object_display_mode,filter,link_styling_script,width,height,display_priority FROM network_maps WHERE id={id}");
		if (hResult == nullptr)
			return false;

		m_mapType = DBGetFieldUInt16(hResult, 0, 0);
		m_layout = DBGetFieldUInt16(hResult, 0, 1);
		m_discoveryRadius = DBGetFieldUInt16(hResult, 0, 2);
		m_background = DBGetFieldGUID(hResult, 0, 3);
		m_backgroundLatitude = DBGetFieldDouble(hResult, 0, 4);
		m_backgroundLongitude = DBGetFieldDouble(hResult, 0, 5);
		m_backgroundZoom = DBGetFieldInt16(hResult, 0, 6);
		m_defaultLinkColor = DBGetFieldUInt32(hResult, 0, 7);
		m_defaultLinkRouting = DBGetFieldUInt16(hResult, 0, 8);
		m_defaultLinkWidth = DBGetFieldUInt16(hResult, 0, 9);
      m_defaultLinkStyle = DBGetFieldUInt16(hResult, 0, 10);
		m_backgroundColor = DBGetFieldUInt32(hResult, 0, 11);
		m_objectDisplayMode = DBGetFieldUInt16(hResult, 0, 12);

      TCHAR *filter = DBGetField(hResult, 0, 13, nullptr, 0);
      setFilter(filter);
      MemFree(filter);

      TCHAR *script = DBGetField(hResult, 0, 14, nullptr, 0);
      setLinkStylingScript(script);
      MemFree(script);

      m_width = DBGetFieldInt32(hResult, 0, 15);
      m_height = DBGetFieldInt32(hResult, 0, 16);
      m_displayPriority = DBGetFieldInt32(hResult, 0, 17);

      DBFreeResult(hResult);

	   // Load elements
      hResult = executeSelectOnObject(hdb, L"SELECT element_id,element_type,element_data,flags FROM network_map_elements WHERE map_id={id}");
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				NetworkMapElement *e;
				uint32_t id = DBGetFieldULong(hResult, i, 0);
				uint32_t flags = DBGetFieldULong(hResult, i, 3);
				char *data = DBGetFieldUTF8(hResult, i, 2, nullptr, 0);
				if (data != nullptr)
				{
		         json_error_t error;
					json_t *json = json_loads(data, 0, &error);
					if (json == nullptr)
					   json = json_object();

					switch(DBGetFieldLong(hResult, i, 1))
					{
						case MAP_ELEMENT_OBJECT:
							e = new NetworkMapObject(id, json, flags);
			            m_objectSet.put(static_cast<NetworkMapObject *>(e)->getObjectId());
							break;
						case MAP_ELEMENT_DECORATION:
							e = new NetworkMapDecoration(id, json, flags);
							break;
                  case MAP_ELEMENT_DCI_CONTAINER:
                     e = new NetworkMapDCIContainer(id, json, flags);
                     static_cast<NetworkMapDCIContainer*>(e)->updateDciList(&m_dciSet, true);
                     break;
                  case MAP_ELEMENT_DCI_IMAGE:
                     e = new NetworkMapDCIImage(id, json, flags);
                     static_cast<NetworkMapDCIImage*>(e)->updateDciList(&m_dciSet, true);
                     break;
                  case MAP_ELEMENT_TEXT_BOX:
                     e = new NetworkMapTextBox(id, json, flags);
                     break;
						default:		// Unknown type, create generic element
							e = new NetworkMapElement(id, json, flags);
							break;
					}
					json_decref(json);
					MemFree(data);
				}
				else
				{
					e = new NetworkMapElement(id, flags);
				}
				m_mapContent.m_elements.add(e);
				if (m_nextElementId <= e->getId())
					m_nextElementId = e->getId() + 1;
			}
         DBFreeResult(hResult);
      }

		// Load links
      hResult = executeSelectOnObject(hdb, L"SELECT link_id,element1,interface1,element2,interface2,link_type,link_name,connector_name1,connector_name2,element_data,flags,color_source,color,color_provider FROM network_map_links WHERE map_id={id}");
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
			{
				TCHAR buffer[4096];

				NetworkMapLink *l = new NetworkMapLink(DBGetFieldULong(hResult, i, 0), // link ID
				   DBGetFieldULong(hResult, i, 1), DBGetFieldULong(hResult, i, 2),     // element1, interface1
				   DBGetFieldULong(hResult, i, 3), DBGetFieldULong(hResult, i, 4),     // element2, interface2
				   DBGetFieldLong(hResult, i, 5));                                     // link type
				l->setName(DBGetField(hResult, i, 6, buffer, 256));
				l->setConnector1Name(DBGetField(hResult, i, 7, buffer, 256));
				l->setConnector2Name(DBGetField(hResult, i, 8, buffer, 256));
				l->setConfig(DBGetFieldUTF8(hResult, i, 9, nullptr, 0));
				l->setFlags(DBGetFieldUInt32(hResult, i, 10));
            l->setColorSource(static_cast<MapLinkColorSource>(DBGetFieldInt32(hResult, i, 11)));
            l->setColor(DBGetFieldUInt32(hResult, i, 12));
            l->setColorProvider(DBGetField(hResult, i, 13, buffer, 4096));
            m_objectSet.put(l->getInterface1());
            m_objectSet.put(l->getInterface2());
            l->updateColorSourceObjectList(m_objectSet, true);
            l->updateDciList(m_dciSet, true);
            m_mapContent.m_links.add(l);
            if (m_nextLinkId <= l->getId())
               m_nextLinkId = l->getId() + 1;
			}
         DBFreeResult(hResult);
      }

      // Load seed nodes
      DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT seed_node_id FROM network_map_seed_nodes WHERE map_id=?");
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int nRows = DBGetNumRows(hResult);
            for(int i = 0; i < nRows; i++)
            {
               m_seedObjects.add(DBGetFieldUInt32(hResult, i, 0));
            }
            DBFreeResult(hResult);
         }
      }
      DBFreeStatement(hStmt);

      // Load deleted nodes positions
      hStmt = DBPrepare(hdb, L"SELECT object_id,position_x,position_y FROM network_map_deleted_nodes WHERE map_id=? ORDER BY element_index");
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
               loc.objectId = DBGetFieldUInt32(hResult, i, 0);
               loc.posX = DBGetFieldInt32(hResult, i, 1);
               loc.posY = DBGetFieldInt32(hResult, i, 2);
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
void NetworkMap::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
	super::fillMessageLocked(msg, userId);

	msg->setField(VID_MAP_TYPE, m_mapType);
	msg->setField(VID_LAYOUT, m_layout);
	msg->setFieldFromInt32Array(VID_SEED_OBJECTS, m_seedObjects);
	msg->setField(VID_DISCOVERY_RADIUS, m_discoveryRadius);
	msg->setField(VID_BACKGROUND, m_background);
	msg->setField(VID_BACKGROUND_LATITUDE, m_backgroundLatitude);
	msg->setField(VID_BACKGROUND_LONGITUDE, m_backgroundLongitude);
	msg->setField(VID_BACKGROUND_ZOOM, m_backgroundZoom);
   msg->setField(VID_WIDTH, m_width);
   msg->setField(VID_HEIGHT, m_height);
	msg->setField(VID_LINK_COLOR, m_defaultLinkColor);
	msg->setField(VID_LINK_ROUTING, m_defaultLinkRouting);
   msg->setField(VID_LINK_WIDTH, m_defaultLinkWidth);
   msg->setField(VID_LINK_STYLE, m_defaultLinkStyle);
	msg->setField(VID_DISPLAY_MODE, m_objectDisplayMode);
	msg->setField(VID_BACKGROUND_COLOR, m_backgroundColor);
   msg->setField(VID_FILTER, CHECK_NULL_EX(m_filterSource));
   msg->setField(VID_LINK_STYLING_SCRIPT, CHECK_NULL_EX(m_linkStylingScriptSource));
   msg->setField(VID_UPDATE_FAILED, m_updateFailed);
   msg->setField(VID_DISPLAY_PRIORITY, m_displayPriority);

	msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(m_mapContent.m_elements.size()));
	uint32_t fieldId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_mapContent.m_elements.size(); i++)
	{
	   m_mapContent.m_elements.get(i)->fillMessage(msg, fieldId);
		fieldId += 100;
	}

	msg->setField(VID_NUM_LINKS, static_cast<uint32_t>(m_mapContent.m_links.size()));
	fieldId = VID_LINK_LIST_BASE;
	for(int i = 0; i < m_mapContent.m_links.size(); i++)
	{
	   m_mapContent.m_links.get(i)->fillMessage(msg, fieldId);
		fieldId += 20;
	}
}

/**
 * Fill NXCP message with object's data - stage 2
 * Object's properties are not locked when this method is called. Should be
 * used only to fill data where properties lock is not enough (like data
 * collection configuration).
 */
void NetworkMap::fillMessageUnlocked(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
}

/**
 * Update network map object from NXCP message
 */
uint32_t NetworkMap::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   AutoBindTarget::modifyFromMessage(msg);

   if (msg.isFieldExist(VID_DISPLAY_PRIORITY))
      m_displayPriority = msg.getFieldAsInt32(VID_DISPLAY_PRIORITY);

	if (msg.isFieldExist(VID_MAP_TYPE))
		m_mapType = msg.getFieldAsUInt16(VID_MAP_TYPE);

	if (msg.isFieldExist(VID_LAYOUT))
		m_layout = msg.getFieldAsUInt16(VID_LAYOUT);

	if (msg.isFieldExist(VID_SEED_OBJECTS))
	{
		msg.getFieldAsInt32Array(VID_SEED_OBJECTS, &m_seedObjects);
		m_seedObjects.deduplicate();
	}

	if (msg.isFieldExist(VID_DISCOVERY_RADIUS))
		m_discoveryRadius = msg.getFieldAsUInt16(VID_DISCOVERY_RADIUS);

	if (msg.isFieldExist(VID_LINK_COLOR))
		m_defaultLinkColor = msg.getFieldAsUInt32(VID_LINK_COLOR);

	if (msg.isFieldExist(VID_LINK_ROUTING))
		m_defaultLinkRouting = msg.getFieldAsUInt16(VID_LINK_ROUTING);

   if (msg.isFieldExist(VID_LINK_WIDTH))
      m_defaultLinkWidth = msg.getFieldAsUInt16(VID_LINK_WIDTH);

   if (msg.isFieldExist(VID_LINK_STYLE))
      m_defaultLinkStyle = msg.getFieldAsUInt16(VID_LINK_STYLE);

	if (msg.isFieldExist(VID_DISPLAY_MODE))
      m_objectDisplayMode = msg.getFieldAsUInt16(VID_DISPLAY_MODE);

	if (msg.isFieldExist(VID_BACKGROUND_COLOR))
		m_backgroundColor = msg.getFieldAsUInt32(VID_BACKGROUND_COLOR);

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
	   ObjectArray<NetworkMapElement> newElements(0, 128, Ownership::False);
	   m_dciSet.clear();
	   m_objectSet.clear();
		int numElements = msg.getFieldAsInt32(VID_NUM_ELEMENTS);
		if (numElements > 0)
		{
			uint32_t fieldId = VID_ELEMENT_LIST_BASE;
			for(int i = 0; i < numElements; i++)
			{
				NetworkMapElement *e;
				int type = msg.getFieldAsInt16(fieldId + 1);
				switch(type)
				{
					case MAP_ELEMENT_OBJECT:
						e = new NetworkMapObject(msg, fieldId);
                  m_objectSet.put(static_cast<NetworkMapObject *>(e)->getObjectId());
						break;
					case MAP_ELEMENT_DECORATION:
						e = new NetworkMapDecoration(msg, fieldId);
						break;
               case MAP_ELEMENT_DCI_CONTAINER:
                  e = new NetworkMapDCIContainer(msg, fieldId);
                  static_cast<NetworkMapDCIContainer*>(e)->updateDciList(&m_dciSet, true);
                  break;
               case MAP_ELEMENT_DCI_IMAGE:
                  e = new NetworkMapDCIImage(msg, fieldId);
                  static_cast<NetworkMapDCIImage*>(e)->updateDciList(&m_dciSet, true);
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
		   for(int j = 0; j < m_mapContent.m_elements.size(); j++)
		   {
	         NetworkMapElement *oldElement = m_mapContent.m_elements.get(j);
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
	               m_deletedObjects.remove(i);
	               break;
	            }
	         }
		   }
		}

		for(int j = 0; j < m_mapContent.m_elements.size(); j++)
      {
         NetworkMapElement *oldElement = m_mapContent.m_elements.get(j);
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
            if (m_deletedObjects.find([loc] (const NetworkMapObjectLocation *l) { return l->objectId == loc.objectId; }) == nullptr)
            {
               m_deletedObjects.insert(0, &loc);
               if (m_deletedObjects.size() > MAX_DELETED_OBJECT_COUNT)
                  m_deletedObjects.remove(MAX_DELETED_OBJECT_COUNT);
            }
         }
      }
		m_mapContent.m_elements.clear();
		m_mapContent.m_elements.addAll(newElements);

		m_mapContent.m_links.clear();
		int numLinks = msg.getFieldAsInt32(VID_NUM_LINKS);
		if (numLinks > 0)
		{
			uint32_t fieldId = VID_LINK_LIST_BASE;
			for(int i = 0; i < numLinks; i++)
			{
			   auto l = new NetworkMapLink(msg, fieldId);
            m_objectSet.put(l->getInterface1());
            m_objectSet.put(l->getInterface2());
            l->updateColorSourceObjectList(m_objectSet, true);
            l->updateDciList(m_dciSet, true);
            m_mapContent.m_links.add(l);
				fieldId += 20;

            if (m_nextLinkId <= l->getId())
               m_nextLinkId = l->getId() + 1;
			}
		}
	}

	return super::modifyFromMessageInternal(msg, session);
}

/**
 * Map update poll handler
 */
void NetworkMap::mapUpdatePoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   pollerLock(mapUpdate);

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   poller->setStatus(_T("updating map"));

   updateContent();
   calculateCompoundStatus();

   pollerUnlock();
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
         for(int j = 0; j < m_mapContent.m_elements.size(); j++)
         {
            NetworkMapElement *oldElement = m_mapContent.m_elements.get(j);
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
         for (int j = 0; j < m_mapContent.m_links.size(); j++)
         {
            NetworkMapLink *oldLink = m_mapContent.m_links.get(j);
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
   NetworkMapObjectList objects;
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): map type %d"), m_name, m_id, m_mapType);
   if (m_mapType != MAP_TYPE_CUSTOM)
   {
      sendPollerMsg(_T("Collecting objects...\r\n"));
      NetworkMapObjectList objects;
      bool success = buildTopologyGraph(&objects, m_mapType);

      if (m_mapType == MAP_TYPE_HYBRID_TOPOLOGY)
      {
         unique_ptr<ObjectArray<IntegerArray<uint32_t>>> unconnectedSubgraphs = objects.getUnconnectedSets();
         if (!unconnectedSubgraphs->isEmpty())
         {
            success = connectTopologySubgraphs(&objects, MAP_TYPE_OSPF_TOPOLOGY, *unconnectedSubgraphs);
            if (success)
            {
               unconnectedSubgraphs = objects.getUnconnectedSets();
               if (!unconnectedSubgraphs->isEmpty())
               {
                  success = connectTopologySubgraphs(&objects, MAP_TYPE_IP_TOPOLOGY, *unconnectedSubgraphs);
               }
            }
         }
      }

      if (success)
      {
         if (!IsShutdownInProgress())
         {
            sendPollerMsg(_T("Updating objects...\r\n"));
            m_updateFailed = false;
            updateObjects(objects);
         }
      }
      else
      {
         if (!m_updateFailed)
         {
            m_updateFailed = true;
            setModified(MODIFY_RUNTIME);
         }
      }
   }
   sendPollerMsg(_T("Updating link texts and styling...\r\n"));
   updateLinks();
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::updateContent(%s [%u]): completed"), m_name, m_id);
   sendPollerMsg(_T("Map update completed\r\n"));
}

/**
 * Connect unconnected subgraphs within given topology graph using information from given map type
 */
bool NetworkMap::connectTopologySubgraphs(NetworkMapObjectList *objects, int mapType, const ObjectArray<IntegerArray<uint32_t>>& unconnectedSubgraphs)
{
   NetworkMapObjectList additionalObjects;
   bool success = buildTopologyGraph(&additionalObjects, mapType);
   if (!success)
      return false;

   for (ObjLink *link : additionalObjects.getLinks())
   {
      bool linkBetwenSubgraphs = false;
      for (int i = 0; i < unconnectedSubgraphs.size(); i++)
      {
         IntegerArray<uint32_t> *subgraph = unconnectedSubgraphs.get(i);
         if (subgraph->contains(link->object1) || subgraph->contains(link->object2))
         {
            if (!linkBetwenSubgraphs)
            {
               linkBetwenSubgraphs = true;
            }
            else
            {
               objects->linkObjects(link->object1,link->iface1, link->port1, link->object2, link->iface2, link->port2, link->name, link->type);
               break;
            }
         }
      }
   }
   return true;
}

/**
 * Build topology graph for updating map content for seeded map types
 */
bool NetworkMap::buildTopologyGraph(NetworkMapObjectList *graph, int mapType)
{
   sendPollerMsg(_T("Collecting objects...\r\n"));
   bool success = true;
   for(int i = 0; (i < m_seedObjects.size()) && success; i++)
   {
      uint32_t seedObjectId = m_seedObjects.get(i);
      shared_ptr<NetObj> seed = FindObjectById(seedObjectId);
      if (seed != nullptr)
      {
         if (seed->getObjectClass() == OBJECT_NODE)
         {
            success = buildTopologyGraphFromSeed(static_pointer_cast<Node>(seed), graph, mapType);
         }
         else if ((seed->getObjectClass() == OBJECT_CONTAINER) || (seed->getObjectClass() == OBJECT_COLLECTOR) ||
                  (seed->getObjectClass() == OBJECT_COLLECTOR) || (seed->getObjectClass() == OBJECT_CLUSTER) ||
                  (seed->getObjectClass() == OBJECT_RACK))
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::buildTopology(%s [%u]): seed object %s [%u] is a container, using child nodes as seeds"), m_name, m_id, seed->getName(), seedObjectId);
            sendPollerMsg(_T("   Seed object \"%s\" is a container, using child nodes as seeds\r\n"), seed->getName(), seedObjectId);
            unique_ptr<SharedObjectArray<NetObj>> children = seed->getAllChildren(true);
            for(int j = 0; (j < children->size()) && success; j++)
            {
               shared_ptr<NetObj> s = children->getShared(j);
               if (s->getObjectClass() == OBJECT_NODE)
               {
                  success = buildTopologyGraphFromSeed(static_pointer_cast<Node>(s), graph, mapType);
               }
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap::buildTopology(%s [%u]): seed object [%u] cannot be found"), m_name, m_id, seedObjectId);
         sendPollerMsg(POLLER_WARNING _T("   Cannot find seed object with ID %u\r\n"), seedObjectId);
      }
   }
   return success;
}

/**
 * Build topology graph for updating map content for seeded map types using specific seed
 */
bool NetworkMap::buildTopologyGraphFromSeed(const shared_ptr<Node>& seed, NetworkMapObjectList *graph, int mapType)
{
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 6, _T("NetworkMap::buildTopologyFromSeed(%s [%u]): reading topology information from node %s [%u]"), m_name, m_id, seed->getName(), seed->getId());

   shared_ptr<NetworkMapObjectList> topology;
   lockProperties();
   NetworkMap *filterProvider = (m_flags & MF_FILTER_OBJECTS) && (m_filter != nullptr) ? this : nullptr;
   unlockProperties();
   switch(mapType)
   {
      case MAP_TYPE_HYBRID_TOPOLOGY:
      case MAP_TYPE_LAYER2_TOPOLOGY:
         topology = seed->buildL2Topology(m_discoveryRadius, (m_flags & MF_SHOW_END_NODES) != 0, (m_flags & MF_USE_L1_TOPOLOGY) != 0, filterProvider);
         break;
      case MAP_TYPE_IP_TOPOLOGY:
         topology = shared_ptr<NetworkMapObjectList>(BuildIPTopology(seed, filterProvider, m_discoveryRadius, (m_flags & MF_SHOW_END_NODES) != 0).release());
         break;
      case MAP_INTERNAL_COMMUNICATION_TOPOLOGY:
         topology = seed->buildInternalCommunicationTopology();
         break;
      case MAP_TYPE_OSPF_TOPOLOGY:
         topology = shared_ptr<NetworkMapObjectList>(BuildOSPFTopology(seed, filterProvider, m_discoveryRadius).release());
         break;
      default:
         break;
   }

   if (topology == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap::buildTopologyFromSeed(%s [%u]): cannot get topology information for node %s [%d], map won't be updated"), m_name, m_id, seed->getName(), seed->getId());
      return false;
   }

   graph->merge(*topology);
   return true;
}

/**
 * Add or update links in map content
 * Properties should be locked before calling this method.
 */
bool NetworkMap::mergeLinks(NetworkMapContent *content, const ObjectArray<ObjLink>& links)
{
   bool modified = false;
   // add new links and update existing
   for(int i = 0; i < links.size(); i++)
   {
      ObjLink *newLink = links.get(i);
      NetworkMapLink *link = nullptr;
      bool isNew = true;
      for(int j = 0; j < content->m_links.size(); j++)
      {
         NetworkMapLink *currLink = content->m_links.get(j);
         if (newLink->type == currLink->getType())
         {
            uint32_t obj1 = content->objectIdFromElementId(currLink->getElement1());
            uint32_t obj2 = content->objectIdFromElementId(currLink->getElement2());
            if ((newLink->object1 == obj1) && (newLink->iface1 == currLink->getInterface1()) && (newLink->object2 == obj2) && (newLink->iface2 == currLink->getInterface2()))
            {
               link = currLink;
               isNew = false;
               break;
            }
            if ((newLink->object1 == obj2) && (newLink->iface1 == currLink->getInterface2()) && (newLink->object2 == obj1) && (newLink->iface2 == currLink->getInterface1()))
            {
               link = currLink;
               newLink->swap();
               isNew = false;
               break;
            }
         }
      }

      // Add new link if needed
      if (link == nullptr)
      {
         uint32_t e1 = content->elementIdFromObjectId(newLink->object1);
         uint32_t e2 = content->elementIdFromObjectId(newLink->object2);
         // Element ID can be 0 if link points to object removed by filter
         if ((e1 != 0) && (e2 != 0))
         {
            link = new NetworkMapLink(m_nextLinkId++, e1, newLink->iface1, e2, newLink->iface2, newLink->type);
            link->setColorSource(MAP_LINK_COLOR_SOURCE_INTERFACE_STATUS);
            link->setFlags(AUTO_GENERATED);
            link->updateDciList(m_dciSet, true);
            link->updateColorSourceObjectList(m_objectSet, true);
            m_objectSet.put(link->getInterface1());
            m_objectSet.put(link->getInterface2());
            content->m_links.add(link);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 3, _T("NetworkMap(%s [%u])/updateLinks: cannot add link because elements are missing for object pair (%u,%u)"),
                     m_name, m_id, newLink->object1, newLink->object2);
         }
      }

      // Update link properties
      if (link != nullptr)
      {
         m_objectSet.remove(link->getInterface1());
         m_objectSet.remove(link->getInterface2());
         bool updated = link->update(*newLink, !(m_flags & MF_DONT_UPDATE_LINK_TEXT));
         m_objectSet.put(newLink->iface1);
         m_objectSet.put(newLink->iface2);
         if (updated || isNew)
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateLinks: link %u (%u) - %u (%u) %s"),
                     m_name, m_id, link->getElement1(), newLink->object1, link->getElement2(), newLink->object2, isNew ? _T("added") : _T("updated"));
            sendPollerMsg(_T("   %s link between \"%s\" and \"%s\"\r\n"), isNew ? _T("Added") : _T("Updated"), GetObjectName(newLink->object1, _T("unknown")), GetObjectName(newLink->object2, _T("unknown")));
            modified = true;
         }
      }
   }
   return modified;
}

/**
 * Update objects from given list
 */
void NetworkMap::updateObjects(const NetworkMapObjectList& objects)
{
   bool modified = false;

   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u]): updateObjects called"), m_name, m_id);
   if (nxlog_get_debug_level_tag(_T("netmap")) >= 7)
      objects.dumpToLog();

   // Make local copy of map content
   lockProperties();
   NetworkMapContent content(m_mapContent);
   unlockProperties();

   // remove non-existing links
   for(int i = 0; i < content.m_links.size(); i++)
   {
      NetworkMapLink *link = content.m_links.get(i);
      if (!link->checkFlagSet(AUTO_GENERATED))
         continue;

      uint32_t objID1 = content.objectIdFromElementId(link->getElement1());
      uint32_t objID2 = content.objectIdFromElementId(link->getElement2());
      bool linkExists = false;
      if (objects.isLinkExist(objID1, link->getInterface1(), objID2, link->getInterface2(), link->getType()))
      {
         linkExists = true;
      }
      else if (objects.isLinkExist(objID2, link->getInterface2(), objID1, link->getInterface1(), link->getType()))
      {
         link->swap();
         linkExists = true;
      }

      if (!linkExists)
      {
         sendPollerMsg(_T("   Link between \"%s\" and \"%s\" removed\r\n"), GetObjectName(objID1, _T("unknown")), GetObjectName(objID2, _T("unknown")));
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: link %u - %u removed"), m_name, m_id, link->getElement1(), link->getElement2());

         lockProperties();
         link->updateDciList(m_dciSet, false);
         link->updateColorSourceObjectList(m_objectSet, false);
         m_objectSet.remove(link->getInterface1());
         m_objectSet.remove(link->getInterface2());
         unlockProperties();

         content.m_links.remove(i);
         i--;
         modified = true;
      }
   }

   // Remove duplicate links
   for(int i = 0; i < content.m_links.size(); i++)
   {
      NetworkMapLink *link = content.m_links.get(i);
      if (!link->checkFlagSet(AUTO_GENERATED))
         continue;

      uint32_t objId1 = content.objectIdFromElementId(link->getElement1());
      uint32_t objId2 = content.objectIdFromElementId(link->getElement2());

      for(int j = i + 1; j < content.m_links.size(); j++)
      {
         NetworkMapLink *currLink = content.m_links.get(j);
         if (!currLink->checkFlagSet(AUTO_GENERATED))
            continue;

         if (currLink->getType() != link->getType())
            continue;

         uint32_t currObjId1 = content.objectIdFromElementId(currLink->getElement1());
         uint32_t currObjId2 = content.objectIdFromElementId(currLink->getElement2());

         bool duplicate = false;
         if ((objId1 == currObjId1) && (objId2 == currObjId2))
         {
            duplicate = (link->getInterface1() == currLink->getInterface1()) && (link->getInterface2() == currLink->getInterface2());
         }
         else if ((objId1 == currObjId2) && (objId2 == currObjId1))
         {
            duplicate = (link->getInterface1() == currLink->getInterface2()) && (link->getInterface2() == currLink->getInterface1());
         }

         if (duplicate)
         {
            sendPollerMsg(_T("   Duplicate link between \"%s\" and \"%s\" removed\r\n"), GetObjectName(currObjId1, _T("unknown")), GetObjectName(currObjId2, _T("unknown")));
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: duplicate link %u - %u removed"), m_name, m_id, currLink->getElement1(), currLink->getElement2());

            lockProperties();
            currLink->updateDciList(m_dciSet, false);
            currLink->updateColorSourceObjectList(m_objectSet, false);
            m_objectSet.remove(currLink->getInterface1());
            m_objectSet.remove(currLink->getInterface2());
            unlockProperties();

            content.m_links.remove(j);
            j--;
            modified = true;
         }
      }
   }

   // remove non-existing objects
   for(int i = 0; i < content.m_elements.size(); i++)
   {
      NetworkMapElement *e = content.m_elements.get(i);
      if ((e->getType() != MAP_ELEMENT_OBJECT) || !(e->getFlags() & AUTO_GENERATED))
         continue;

      NetworkMapObject *netMapObject = static_cast<NetworkMapObject*>(e);
      uint32_t objectId = netMapObject->getObjectId();
      if (!objects.isObjectExist(objectId))
      {
         sendPollerMsg(_T("   Object \"%s\" removed\r\n"), GetObjectName(objectId, _T("unknown")));
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: object element %u (object %u) removed"), m_name, m_id, e->getId(), objectId);

         NetworkMapObjectLocation loc;
         loc.objectId = objectId;
         loc.posX = netMapObject->getPosX();
         loc.posY = netMapObject->getPosY();

         lockProperties();
         if (m_deletedObjects.find([loc] (const NetworkMapObjectLocation *l) { return l->objectId == loc.objectId; }) == nullptr)
         {
            m_deletedObjects.insert(0, &loc);
            if (m_deletedObjects.size() > MAX_DELETED_OBJECT_COUNT)
               m_deletedObjects.remove(MAX_DELETED_OBJECT_COUNT);
            m_objectSet.remove(netMapObject->getObjectId());
         }
         unlockProperties();

         content.m_elements.remove(i);
         i--;
         modified = true;
      }
   }

   // add new objects
   for(int i = 0; i < objects.getNumObjects(); i++)
   {
      uint32_t objectId = objects.getObjects().get(i);
      bool found = false;
      NetworkMapElement *e = nullptr;
      for(int j = 0; j < content.m_elements.size(); j++)
      {
         e = content.m_elements.get(j);
         if (e->getType() != MAP_ELEMENT_OBJECT)
            continue;
         if (static_cast<NetworkMapObject*>(e)->getObjectId() == objectId)
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         lockProperties();
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
         m_objectSet.put(objectId);
         unlockProperties();

         content.m_elements.add(e);
         modified = true;
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s [%u])/updateObjects: new object %u (element ID %u) added"), m_name, m_id, objectId, e->getId());
         sendPollerMsg(_T("   Object \"%s\" added\r\n"), GetObjectName(objectId, _T("unknown")));
      }
   }

   lockProperties();
   if (mergeLinks(&content, objects.getLinks()))
      modified = true;
   unlockProperties();

   if (modified)
   {
      lockProperties();
      m_mapContent.m_elements.swap(content.m_elements);
      m_mapContent.m_links.swap(content.m_links);
      setModified(MODIFY_MAP_CONTENT);
      unlockProperties();
   }

   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s): updateObjects completed"), m_name);
}

static SharedObjectArray<DCObject> CollectDcis(shared_ptr<Node> node, uint32_t interfaceId)
{
   bool inTraffic = false;
   bool outTraffic = false;
   SharedObjectArray<DCObject> dcis = node->getDCObjectsByFilter([interfaceId, &inTraffic, &outTraffic] (DCObject *obj)
      {
         String tag = obj->getSystemTag();
         if (inTraffic && outTraffic)
            return false;
         if (!inTraffic && (obj->getType() == DCO_TYPE_ITEM) && (obj->getRelatedObject() == interfaceId) &&
                  tag.startsWith(L"iface-inbound-") && !tag.endsWith(L"-util"))
         {
            inTraffic = true;
            return true;
         }

         if (!outTraffic && (obj->getType() == DCO_TYPE_ITEM) && (obj->getRelatedObject() == interfaceId) &&
                  tag.startsWith(L"iface-outbound-") && !tag.endsWith(L"-util"))
         {
            outTraffic = true;
            return true;
         }
         return false;
      });
   return dcis;
}

/**
 * Update links with DCI information
 */
void NetworkMap::updateLinkDataSource(NetworkMapLinkContainer *linkContainer)
{
   shared_ptr<Interface> interface1 = static_pointer_cast<Interface>(FindObjectById(linkContainer->get()->getInterface1() , OBJECT_INTERFACE));
   shared_ptr<Interface> interface2 = static_pointer_cast<Interface>(FindObjectById(linkContainer->get()->getInterface2() , OBJECT_INTERFACE));
   shared_ptr<Node> node1 = (interface1 != nullptr) ? interface1->getParentNode() : nullptr;
   shared_ptr<Node> node2 = (interface2 != nullptr) ? interface2->getParentNode() : nullptr;
   if (isShowTraffic() && ((node1 != nullptr) || (node2 != nullptr)))
   {
      SharedObjectArray<DCObject> dcis;
      if (node1 != nullptr)
         dcis.addAll(CollectDcis(node1, interface1->getId()));
      if (node2 != nullptr)
         dcis.addAll(CollectDcis(node2, interface2->getId()));

      unique_ptr<ObjectArray<LinkDataSouce>> linkDataSource = linkContainer->getDataSource();
      for (int i = 0, j = 0; i < linkDataSource->size(); i++, j++)
      {
         LinkDataSouce *lds = linkDataSource->get(i);
         if (!lds->isSystem())
            continue;
         bool found = false;
         for (const shared_ptr<DCObject> &dci : dcis)
         {
            if (lds->getDciId() == dci->getId())
            {
               found = true;
               LinkDataLocation loc = (dci->getRelatedObject() == linkContainer->get()->getInterface1()) ? LinkDataLocation::OBJECT1 : LinkDataLocation::OBJECT2;
               if (lds->getLocation() != loc)
               {
                  linkContainer->updateDataSourceLocation(dci->createDescriptor(), loc);
               }
            }
         }
         if (!found)
         {
            linkContainer->removeDataSource(j);
            j--;
         }
      }

      for (const shared_ptr<DCObject> &dci : dcis)
      {
         bool found = false;
         for (const LinkDataSouce *lds : *linkDataSource)
         {
            if (lds->getDciId() == dci->getId())
            {
               //Location already checked and updated in previous cycle
               found = true;
               break;
            }
         }
         if (!found)
         {
            String tag = dci->getSystemTag();
            wchar_t format[128];
            _sntprintf(format, 128, L"%s: %s", tag.startsWith(L"iface-inbound-") ? L"RX" : L"TX", L" %{u,m}s");
            LinkDataLocation loc = (dci->getRelatedObject() == linkContainer->get()->getInterface1()) ? LinkDataLocation::OBJECT1 : LinkDataLocation::OBJECT2;
            linkContainer->addSystemDataSource(dci->createDescriptor(), format, loc);
         }
      }
   }
   else
   {
      unique_ptr<ObjectArray<LinkDataSouce>> linkDataSource = linkContainer->getDataSource();
      for (int i = 0; i < linkDataSource->size(); i++)
      {
         LinkDataSouce *lds = linkDataSource->get(i);
         if (lds->isSystem())
         {
            linkContainer->clearSystemDataSource();
         }
      }
   }
}

/**
 * Update links that have computed attributes (color, text, etc.)
 */
void NetworkMap::updateLinks()
{
   ObjectArray<NetworkMapLink> colorUpdateList(0, 128, Ownership::True);
   ObjectArray<NetworkMapLinkContainer> stylingUpdateList(0, 128, Ownership::True);

   lockProperties();
   for(int i = 0; i < m_mapContent.m_links.size(); i++)
   {
      NetworkMapLink *link = m_mapContent.m_links.get(i);
      if (link->getColorSource() == MAP_LINK_COLOR_SOURCE_SCRIPT)
      {
         // Replace element IDs with actual object IDs in temporary link object
         auto temp = new NetworkMapLink(*link);
         temp->setConnectedElements(m_mapContent.objectIdFromElementId(link->getElement1()), m_mapContent.objectIdFromElementId(link->getElement2()));
         colorUpdateList.add(temp);
      }

      if (!link->isExcludeFromAutoUpdate())
      {
         // Replace element IDs with actual object IDs in temporary link object
         auto temp = new NetworkMapLinkContainer(*link);
         temp->get()->setConnectedElements(m_mapContent.objectIdFromElementId(link->getElement1()), m_mapContent.objectIdFromElementId(link->getElement2()));
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
            sendPollerMsg(POLLER_WARNING _T("   Link color provider script \"%s\" execution failed (%s)\r\n"), link->getColorProvider(), vm->getErrorText());
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 4, _T("NetworkMap::updateLinks(%s [%u]): color provider script \"%s\" execution failed (%s)"),
                      m_name, m_id, link->getColorProvider(), vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_NETMAP, this, 0, vm->getErrorText(), link->getColorProvider());
         }
         vm.destroy();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap::updateLinks(%s [%u]): color provider script \"%s\" cannot be loaded (%s)"), m_name, m_id, link->getColorProvider(), vm.failureReasonText());
      }
   }

   bool modified = false;
   NXSL_VM *vm = nullptr;
   lockProperties();
   if (m_linkStylingScript != nullptr)
   {
      ScriptVMHandle h = CreateServerScriptVM(m_linkStylingScript, self());
      if (h.isValid())
      {
         vm = h.vm();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap::updateLinks(%s [%u]): styling script cannot be loaded (%s)"), m_name, m_id, h.failureReasonText());
      }
   }
   unlockProperties();

   if (vm != nullptr)
   {
      for(int i = 0; i < stylingUpdateList.size(); i++)
      {
         vm->setGlobalVariable("$link", vm->createValue(vm->createObject(&g_nxslNetworkMapLinkClass, stylingUpdateList.get(i))));
         if (!vm->run())
         {
            nxlog_debug_tag(DEBUG_TAG_NETMAP, 4, _T("NetworkMap::updateLinks(%s [%u]): link styling script execution error: %s"), m_name, m_id, vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_NETMAP, this, 0, vm->getErrorText(), _T("NetworkMap::%s::LinkStylingScript"), m_name);
         }
      }
      delete vm;
   }

   for(int i = 0; i < stylingUpdateList.size(); i++)
   {
      NetworkMapLinkContainer *lc = stylingUpdateList.get(i);
      updateLinkDataSource(lc);
   }

   lockProperties();
   for(int i = 0; i < stylingUpdateList.size(); i++)
   {
      NetworkMapLinkContainer *linkUpdate = stylingUpdateList.get(i);
      for(int j = 0; j < m_mapContent.m_links.size(); j++)
      {
         NetworkMapLink *link = m_mapContent.m_links.get(j);
         if ((link->getId() == linkUpdate->get()->getId()))
         {
            if (linkUpdate->isModified())
            {
               nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap::updateLinks(%s [%u]): link %u [%u -- %u] is modified by styling script or by system"), m_name, m_id, link->getId(), link->getElement1(), link->getElement2());

               linkUpdate->updateConfig();
               linkUpdate->get()->setConnectedElements(link->getElement1(), link->getElement2());

               link->updateDciList(m_dciSet, false);
               link->updateColorSourceObjectList(m_objectSet, false);
               m_objectSet.remove(link->getInterface1());
               m_objectSet.remove(link->getInterface2());

               link = linkUpdate->take();
               link->updateDciList(m_dciSet, true);
               link->updateColorSourceObjectList(m_objectSet, true);
               m_objectSet.put(link->getInterface1());
               m_objectSet.put(link->getInterface2());

               m_mapContent.m_links.replace(j, link);

               modified = true;
            }
            break;
         }
      }
   }
   for(int i = 0; i < colorUpdateList.size(); i++)
   {
      NetworkMapLink *linkUpdate = colorUpdateList.get(i);
      for(int j = 0; j < m_mapContent.m_links.size(); j++)
      {
         NetworkMapLink *link = m_mapContent.m_links.get(j);
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
		m_filter = CompileServerScript(filter, SCRIPT_CONTEXT_NETMAP, this, 0, _T("NetworkMap::%s::Filter"), m_name);
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
      ScriptVMHandle vm = CreateServerScriptVM(m_filter, object);
      unlockProperties();

      if (vm.isValid())
      {
         vm->setGlobalVariable("$map", createNXSLObject(vm));
         if (vm->run())
         {
            result = vm->getResult()->getValueAsBoolean();
         }
         else
         {
            ReportScriptError(SCRIPT_CONTEXT_NETMAP, object.get(), 0, vm->getErrorText(), _T("NetworkMap::%s::Filter"), m_name);
         }
         vm.destroy();
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap::isAllowedOnMap(%s [%u]): filter script cannot be loaded (%s)"), m_name, m_id, vm.failureReasonText());
      }
	}
	else
	{
      unlockProperties();
	}
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 7, _T("NetworkMap(%s [%u]): running object filter for %s [%u], result = %s"), m_name, m_id, object->getName(), object->getId(), BooleanToString(result));
	return result;
}

/**
*  Delete object from the map if it's deleted in session
*/
void NetworkMap::onObjectDelete(const NetObj& object)
{
   uint32_t objectId = object.getId();
   bool modified = false;

   lockProperties();

   uint32_t elementId = 0;
   for(int i = 0; i < m_mapContent.m_elements.size(); i++)
   {
      NetworkMapElement *element = m_mapContent.m_elements.get(i);
      if ((element->getType() == MAP_ELEMENT_OBJECT) && (static_cast<NetworkMapObject*>(element)->getObjectId() == objectId))
      {
         elementId = element->getId();
         m_objectSet.remove(objectId);
         m_mapContent.m_elements.remove(i);
         modified = true;
         break;
      }
   }

   if (elementId != 0)
   {
      int i = 0;
      while(i < m_mapContent.m_links.size())
      {
         NetworkMapLink *link = m_mapContent.m_links.get(i);
         if (link->getElement1() == elementId || link->getElement2() == elementId)
         {
            link->updateDciList(m_dciSet, false);
            link->updateColorSourceObjectList(m_objectSet, false);
            m_objectSet.remove(link->getInterface1());
            m_objectSet.remove(link->getInterface2());
            m_mapContent.m_links.remove(i);
            modified = true;
         }
         else
         {
            i++;
         }
      }
   }

   if (modified)
      setModified(MODIFY_MAP_CONTENT);

   unlockProperties();

   super::onObjectDelete(object);
}

/**
 * Perform automatic assignment to objects
 */
void NetworkMap::autobindPoll(PollerInfo *poller, ClientSession *session, uint32_t rqId)
{
   poller->setStatus(_T("wait for lock"));
   pollerLock(autobind);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   m_pollRequestor = session;
   m_pollRequestId = rqId;
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Starting autobind poll of network map %s [%u]"), m_name, m_id);
   poller->setStatus(_T("checking objects"));

   if (!isAutoBindEnabled())
   {
      sendPollerMsg(L"Automatic object binding is disabled\r\n");
      nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of network map %s [%u])"), m_name, m_id);
      pollerUnlock();
      return;
   }

   NXSL_VM *cachedFilterVM = nullptr;
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(
      [] (NetObj *object, void *context) -> bool
      {
         if (object->isDataCollectionTarget())
            return true;
         int objectClass = object->getObjectClass();
         return (objectClass == OBJECT_NETWORK) || (objectClass == OBJECT_SERVICEROOT) || (objectClass == OBJECT_SUBNET) || (objectClass == OBJECT_ZONE) || (objectClass == OBJECT_CONDITION) || (objectClass == OBJECT_CONTAINER) || (objectClass == OBJECT_COLLECTOR) || (objectClass == OBJECT_RACK);
      }, nullptr);

   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);

      AutoBindDecision decision = isApplicable(&cachedFilterVM, object, this);
      if ((decision == AutoBindDecision_Ignore) || ((decision == AutoBindDecision_Unbind) && !isAutoUnbindEnabled()))
         continue;   // Decision cannot affect checks

      if (decision == AutoBindDecision_Bind)
      {
         if (object->addDashboard(m_id))
         {
            StringBuffer cn(object->getObjectClassName());
            cn.toLowercase();
            sendPollerMsg(_T("   Adding to %s %s\r\n"), cn.cstr(), object->getName());
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("NetworkMap::autobindPoll(): adding network map \"%s\" [%u] to %s \"%s\" [%u]"), m_name, m_id, cn.cstr(), object->getName(), object->getId());
         }
      }
      else if (decision == AutoBindDecision_Unbind)
      {
         if (object->removeDashboard(m_id))
         {
            StringBuffer cn(object->getObjectClassName());
            cn.toLowercase();
            nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 4, _T("NetworkMap::autobindPoll(): removing network map \"%s\" [%u] from %s \"%s\" [%u]"), m_name, m_id, cn.cstr(), object->getName(), object->getId());
            sendPollerMsg(_T("   Removing from %s %s\r\n"), cn.cstr(), object->getName());
         }
      }
   }
   delete cachedFilterVM;

   pollerUnlock();
   nxlog_debug_tag(DEBUG_TAG_AUTOBIND_POLL, 5, _T("Finished autobind poll of network map %s [%u])"), m_name, m_id);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *NetworkMap::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslNetworkMapClass, new shared_ptr<NetworkMap>(self())));
}

/**
 * Create list of seed objects for NXSL script
 */
NXSL_Array *NetworkMap::getSeedObjectsForNXSL(NXSL_VM *vm) const
{
   NXSL_Array *objects = new NXSL_Array(vm);
   lockProperties();
   for(int i = 0; i < m_seedObjects.size(); i++)
   {
      shared_ptr<NetObj> o = FindObjectById(m_seedObjects.get(i));
      if (o != nullptr)
         objects->append(o->createNXSLObject(vm));
   }
   unlockProperties();
   return objects;
}

/**
 * Clone network map
 */
void NetworkMap::clone(const TCHAR *name, const TCHAR *alias)
{
   lockProperties();
   shared_ptr<NetworkMap> clonedMap = make_shared<NetworkMap>(*this);
   unlockProperties();

   clonedMap->setName(name);
   clonedMap->setAlias(alias);
   NetObjInsert(clonedMap, true, false);
   auto parentList = getParentList();
   linkObjects(parentList.getShared(0), clonedMap);
   parentList.get(0)->calculateCompoundStatus();
   clonedMap->unhide();
}

/**
 * Serialize object to JSON
 */
json_t *NetworkMap::toJson()
{
   json_t *root = super::toJson();
   AutoBindTarget::toJson(root);

   lockProperties();

   json_object_set_new(root, "mapType", json_integer(m_mapType));
   json_object_set_new(root, "seedObjects", m_seedObjects.toJson());
   json_object_set_new(root, "discoveryRadius", json_integer(m_discoveryRadius));
   json_object_set_new(root, "layout", json_integer(m_layout));
   json_object_set_new(root, "flags", json_integer(m_flags));
   json_object_set_new(root, "backgroundColor", json_integer(m_backgroundColor));
   json_object_set_new(root, "defaultLinkColor", json_integer(m_defaultLinkColor));
   json_object_set_new(root, "defaultLinkRouting", json_integer(m_defaultLinkRouting));
   json_object_set_new(root, "defaultLinkWidht", json_integer(m_defaultLinkWidth));
   json_object_set_new(root, "defaultLinkStyle", json_integer(m_defaultLinkStyle));
   json_object_set_new(root, "objectDisplayMode", json_integer(m_objectDisplayMode));
   json_object_set_new(root, "background", m_background.toJson());
   json_object_set_new(root, "backgroundLatitude", json_real(m_backgroundLatitude));
   json_object_set_new(root, "backgroundLongitude", json_real(m_backgroundLongitude));
   json_object_set_new(root, "backgroundZoom", json_integer(m_backgroundZoom));
   json_object_set_new(root, "elements", json_object_array(m_mapContent.m_elements));
   json_object_set_new(root, "links", json_object_array(m_mapContent.m_links));
   json_object_set_new(root, "filter", json_string_t(m_filterSource));
   json_object_set_new(root, "linkScript", json_string_t(m_linkStylingScriptSource));
   json_object_set_new(root, "width", json_integer(m_width));
   json_object_set_new(root, "height", json_integer(m_height));
   json_object_set_new(root, "displayPriority", json_integer(m_displayPriority));

   unlockProperties();
   return root;
}

/**
 * Connect nodes automatically based on L2 topology
 */
void NetworkMap::autoConnectNodes(const IntegerArray<uint32_t> &nodeList)
{
   ObjectArray<ObjLink> links;
   for (int i = 0; i < nodeList.size(); i++)
   {
      uint32_t firstNodeId = nodeList.get(i);
      for (int j = i + 1; j < nodeList.size(); j++)
      {
         uint32_t secondNodeId = nodeList.get(j);

         shared_ptr<Node> firstNode = static_pointer_cast<Node>(FindObjectById(firstNodeId, OBJECT_NODE));
         shared_ptr<Node> secondNode = static_pointer_cast<Node>(FindObjectById(secondNodeId, OBJECT_NODE));
         if ((firstNode != nullptr) && (secondNode != nullptr))
         {
            firstNode->getL2Connections(secondNodeId, &links);
         }
      }
   }

   lockProperties();
   if (mergeLinks(&m_mapContent, links))
   {
      setModified(MODIFY_MAP_CONTENT);
   }
   unlockProperties();
   nxlog_debug_tag(DEBUG_TAG_NETMAP, 5, _T("NetworkMap(%s): auto connect completed"), m_name);
}
