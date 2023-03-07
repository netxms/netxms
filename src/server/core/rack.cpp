/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Raden Solutions
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
** File: rack.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
Rack::Rack() : super(), Asset(this)
{
	m_height = 42;
	m_topBottomNumbering = false;
	m_passiveElements = new ObjectArray<RackPassiveElement>(0, 16, Ownership::True);
}

/**
 * Constructor for creating new object
 */
Rack::Rack(const TCHAR *name, int height) : super(name, 0), Asset(this)
{
	m_height = height;
   m_topBottomNumbering = false;
   m_passiveElements = new ObjectArray<RackPassiveElement>(0, 16, Ownership::True);
}

/**
 * Destructor
 */
Rack::~Rack()
{
   delete m_passiveElements;
}

/**
 * Create object from database data
 */
bool Rack::loadFromDatabase(DB_HANDLE hdb, uint32_t id)
{
	if (!super::loadFromDatabase(hdb, id))
		return false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT height,top_bottom_num FROM racks WHERE id=?"));
	if (hStmt == nullptr)
		return false;

	bool success = false;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != nullptr)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			m_height = DBGetFieldLong(hResult, 0, 0);
			m_topBottomNumbering = DBGetFieldLong(hResult, 0, 1) ? true : false;
			success = true;
		}
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	if (success)
	{
      hStmt = DBPrepare(hdb, _T("SELECT id,name,type,position,height,orientation,port_count,image_front,image_rear FROM rack_passive_elements WHERE rack_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               m_passiveElements->add(new RackPassiveElement(hResult, i));
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
	}

   if (success)
      success = Asset::loadFromDatabase(hdb, id);

	return success;
}

/**
 * Save object to database
 */
bool Rack::saveToDatabase(DB_HANDLE hdb)
{
	if (!super::saveToDatabase(hdb))
		return false;

	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("racks"), _T("id"), m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE racks SET height=?,top_bottom_num=? WHERE id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO racks (height,top_bottom_num,id) VALUES (?,?,?)"));
	}
	if (hStmt == nullptr)
		return false;

	lockProperties();
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_height);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_topBottomNumbering ? _T("1") : _T("0"), DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
	bool success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
	if (success)
	{
      success = executeQueryOnObject(hdb, _T("DELETE FROM rack_passive_elements WHERE rack_id=?"));
      for(int i = 0; i < m_passiveElements->size() && success; i++)
         success = m_passiveElements->get(i)->saveToDatabase(hdb, m_id);
	}
	unlockProperties();

   if (success && (m_modified & MODIFY_AM_INSTANCES))
   {
      success = Asset::saveToDatabase(hdb);
   }

	return success;
}

/**
 * Delete object from database
 */
bool Rack::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM racks WHERE id=?"));
   if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM rack_passive_elements WHERE rack_id=?"));
   for(int i = 0; i < m_passiveElements->size() && success; i++)
      success = m_passiveElements->get(i)->deleteChildren(hdb, m_id);
   if (success)
      success = Asset::deleteFromDatabase(hdb);
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Rack::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageInternal(msg, userId);
   msg->setField(VID_HEIGHT, static_cast<uint16_t>(m_height));
   msg->setField(VID_TOP_BOTTOM, m_topBottomNumbering);
   msg->setField(VID_NUM_ELEMENTS, m_passiveElements->size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < m_passiveElements->size(); i++)
   {
      m_passiveElements->get(i)->fillMessage(msg, fieldId);
      fieldId += 10;
   }
}

/**
 * Create NXCP message with object's data - stage 2
 */
void Rack::fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageInternalStage2(msg, userId);
   getAassetData(msg);
}

/**
 * Modify object from message
 */
uint32_t Rack::modifyFromMessageInternal(const NXCPMessage& msg)
{
	if (msg.isFieldExist(VID_HEIGHT))
		m_height = msg.getFieldAsUInt16(VID_HEIGHT);

   if (msg.isFieldExist(VID_TOP_BOTTOM))
      m_topBottomNumbering = msg.getFieldAsBoolean(VID_TOP_BOTTOM);

   if (msg.isFieldExist(VID_NUM_ELEMENTS))
   {
      int count = msg.getFieldAsInt32(VID_NUM_ELEMENTS);
      ObjectArray<RackPassiveElement> newElements(count);
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         newElements.add(new RackPassiveElement(msg, fieldId));
         fieldId += 10;
      }

      for(int i = 0; i < m_passiveElements->size(); i++) //delete links for deleted patch panels
      {
         RackPassiveElement *e = m_passiveElements->get(i);
         if (e->getType() == RackElementType::PATCH_PANEL)
         {
            int j;
            for(j = 0; j < newElements.size(); j++)
               if (e->getId() == newElements.get(j)->getId())
                  break;
            if (j == newElements.size())
               DeletePatchPanelFromPhysicalLinks(m_id, e->getId());
         }
      }

      m_passiveElements->clear();
      for(int i = 0; i < count; i++)
      {
         m_passiveElements->add(newElements.get(i));
      }
   }

   if (msg.isFieldExist(VID_AM_DATA_BASE))
      Asset::modifyFromMessage(msg);

   return super::modifyFromMessageInternal(msg);
}

/**
 * Prepare object for deleiton
 */
void Rack::prepareForDeletion()
{
   DeleteObjectFromPhysicalLinks(m_id);
}

/**
 * Serialize object to JSON
 */
json_t *Rack::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "height", json_integer(m_height));
   json_object_set_new(root, "topBottomNumbering", json_boolean(m_topBottomNumbering));
   json_t *passiveElements = json_array();
   for(int i = 0; i < m_passiveElements->size(); i++)
   {
      json_array_append_new(passiveElements, m_passiveElements->get(i)->toJson());
   }
   json_object_set_new(root, "passiveElements", passiveElements);
   unlockProperties();

   Asset::assetToJson(root);

   return root;
}

String Rack::getRackPasiveElementDescription(uint32_t id)
{
   StringBuffer description;
   lockProperties();
   const RackPassiveElement *el = nullptr;
   for(int i = 0; i < m_passiveElements->size(); i++)
   {
      if (m_passiveElements->get(i)->getId() == id)
      {
         el = m_passiveElements->get(i);
         break;
      }
   }
   if (el == nullptr)
   {
      description.append(_T("["));
      description.append(id);
      description.append(_T("]"));
   }
   else
   {
      description = el->toString();
   }
   unlockProperties();
   return description;
}

/**
 * Create rack passive element form database
 */
RackPassiveElement::RackPassiveElement(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_name = DBGetField(hResult, row, 1, nullptr, 0);
   m_type = static_cast<RackElementType>(DBGetFieldLong(hResult, row, 2));
   m_position = static_cast<uint16_t>(DBGetFieldLong(hResult, row, 3));
   m_height = static_cast<uint16_t>(DBGetFieldLong(hResult, row, 4));
   m_orientation = static_cast<RackOrientation>(DBGetFieldLong(hResult, row, 5));
   m_portCount = DBGetFieldLong(hResult, row, 6);
   m_imageFront = DBGetFieldGUID(hResult, row, 7);
   m_imageRear = DBGetFieldGUID(hResult, row, 8);
}

/**
 * Create passive rack element from NXCP message
 */
RackPassiveElement::RackPassiveElement(const NXCPMessage& request, uint32_t base)
{
   m_id = request.getFieldAsInt32(base++);
   if (m_id == 0)
   {
      m_id = CreateUniqueId(IDG_RACK_ELEMENT);
   }
   m_name = request.getFieldAsString(base++);
   m_type = (RackElementType)request.getFieldAsInt16(base++);
   m_position = request.getFieldAsInt16(base++);
   m_height = request.getFieldAsInt16(base++);
   m_orientation = (RackOrientation)request.getFieldAsInt16(base++);
   m_portCount = request.getFieldAsInt32(base++);
   m_imageFront = request.getFieldAsGUID(base++);
   m_imageRear = request.getFieldAsGUID(base++);
}

/**
 * To JSON method for rack passive element
 */
json_t *RackPassiveElement::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "type", json_integer(static_cast<int32_t>(m_type)));
   json_object_set_new(root, "position", json_integer(m_position));
   json_object_set_new(root, "height", json_integer(m_height));
   json_object_set_new(root, "orientation", json_integer(m_orientation));
   if (m_type == RackElementType::PATCH_PANEL)
      json_object_set_new(root, "portCount", json_integer(m_portCount));
   if (!m_imageFront.isNull())
      json_object_set_new(root, "imageFront", json_string_t(m_imageFront.toString()));
   if (!m_imageRear.isNull())
      json_object_set_new(root, "imageFront", json_string_t(m_imageRear.toString()));
   return root;
}

/**
 * Saves rack passive element to database
 */
bool RackPassiveElement::saveToDatabase(DB_HANDLE hdb, uint32_t parentId) const
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO rack_passive_elements (id,rack_id,name,type,position,height,orientation,port_count,image_front,image_rear) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, parentId);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_type));
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_position);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_height);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_orientation);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (m_type == RackElementType::PATCH_PANEL) ? m_portCount  : 0);
   DBBind(hStmt, 9, DB_SQLTYPE_VARCHAR, m_imageFront);
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, m_imageRear);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);

   return success;
}

/**
 * Delete child objects from database like patch panel port inventory
 */
bool RackPassiveElement::deleteChildren(DB_HANDLE hdb, uint32_t parentId)
{
   if (m_type == RackElementType::PATCH_PANEL)
      DeletePatchPanelFromPhysicalLinks(parentId, m_id);
   return true;
}

/**
 * Fill message for rack passive element
 */
void RackPassiveElement::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base++, m_id);
   msg->setField(base++, m_name);
   msg->setField(base++, static_cast<uint16_t>(m_type));
   msg->setField(base++, m_position);
   msg->setField(base++, m_height);
   msg->setField(base++, static_cast<uint16_t>(m_orientation));
   msg->setField(base++, m_portCount);
   msg->setField(base++, m_imageFront);
   msg->setField(base++, m_imageRear);
}

static const TCHAR *s_passiveElementTypeLabel[] = {
    _T("FILL"),
    _T("FRONT"),
    _T("REAR")
};

/**
 * Rack Passive element representation
 */
String RackPassiveElement::toString() const
{
   StringBuffer sb;
   sb.append(s_passiveElementTypeLabel[m_orientation]);
   sb.append(_T(" "));
   sb.append(m_position);
   sb.append(_T("/"));
   sb.append(m_height);
   sb.append(_T(" ("));
   sb.append(m_name);
   sb.append(_T(")"));
   return sb;
}
