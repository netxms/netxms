/*
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Raden Solutions
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
Rack::Rack() : super()
{
	m_height = 42;
	m_topBottomNumbering = false;
	m_passiveElements = new ObjectArray<RackPassiveElement>(0, 16, true);
}

/**
 * Constructor for creating new object
 */
Rack::Rack(const TCHAR *name, int height) : super(name, 0)
{
	m_height = height;
   m_topBottomNumbering = false;
   m_passiveElements = new ObjectArray<RackPassiveElement>(0, 16, true);
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
bool Rack::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
	if (!super::loadFromDatabase(hdb, id))
		return false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT height,top_bottom_num FROM racks WHERE id=?"));
	if (hStmt == NULL)
		return false;

	bool success = false;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
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

   hStmt = DBPrepare(hdb, _T("SELECT id,name,type,position,orientation,port_count FROM rack_passive_elements WHERE rack_id=?"));
   if (hStmt == NULL)
      success = false;
	if(success)
	{
	   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
	   DB_RESULT hResult = DBSelectPrepared(hStmt);
	   if (hResult != NULL)
	   {
	      int count = DBGetNumRows(hResult);
	      for(int i = 0; i < count; i++)
	      {
	         RackPassiveElement *element = new RackPassiveElement(hResult, i);
	         m_passiveElements->add(element);
	      }
	      DBFreeResult(hResult);
	      success = true;
	   }
	   else
	   {
	      success = false;
	   }
	}
	if (hStmt != NULL)
	   DBFreeStatement(hStmt);

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
	if (hStmt == NULL)
		return false;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (LONG)m_height);
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_topBottomNumbering ? _T("1") : _T("0"), DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
	BOOL success = DBExecute(hStmt);
	DBFreeStatement(hStmt);
	if (success)
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM rack_passive_elements WHERE rack_id=?"));
	for(int i = 0; i < m_passiveElements->size() && success; i++)
	   success = m_passiveElements->get(i)->saveToDatabase(hdb, m_id);
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
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Rack::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_HEIGHT, (WORD)m_height);
   pMsg->setField(VID_TOP_BOTTOM, (INT16)(m_topBottomNumbering ? 1 : 0));
   pMsg->setField(VID_NUM_ELEMENTS, m_passiveElements->size());
   UINT32 base = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < m_passiveElements->size(); i++)
   {
      m_passiveElements->get(i)->fillMessage(pMsg, base);
      base += 10;
   }
}

/**
 * Modify object from message
 */
UINT32 Rack::modifyFromMessageInternal(NXCPMessage *request)
{
	if (request->isFieldExist(VID_HEIGHT))
		m_height = request->getFieldAsUInt16(VID_HEIGHT);

   if (request->isFieldExist(VID_TOP_BOTTOM))
      m_topBottomNumbering = request->getFieldAsBoolean(VID_TOP_BOTTOM);

   if (request->isFieldExist(VID_NUM_ELEMENTS))
   {
      int count = request->getFieldAsInt32(VID_NUM_ELEMENTS);
      ObjectArray<RackPassiveElement> newElements(count, 16, false);
      UINT32 fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         newElements.add(new RackPassiveElement(request, fieldId));
         fieldId += 10;
      }

      for(int i = 0; i < m_passiveElements->size(); i++) //delete links for deleted patch panels
      {
         RackPassiveElement *e = m_passiveElements->get(i);
         if (e->getType() == PATCH_PANEL)
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

   return super::modifyFromMessageInternal(request);
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

   return root;
}

/**
 * Default constructor for rack passive element
 */
RackPassiveElement::RackPassiveElement()
{
   m_id = CreateUniqueId(IDG_RACK_ELEMENT);
   m_name = MemCopyString(_T(""));
   m_type = PATCH_PANEL;
   m_position = 0;
   m_orientation = FILL;
   m_portCount = 0;
}

/**
 * Create rack passive element form database
 */
RackPassiveElement::RackPassiveElement(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldLong(hResult, row, 0);
   m_name = DBGetField(hResult, row, 1, NULL, 0);
   m_type = (RackElementType)DBGetFieldLong(hResult, row, 2);
   m_position = DBGetFieldLong(hResult, row, 3);
   m_orientation = (RackOrientation)DBGetFieldLong(hResult, row, 4);
   m_portCount = DBGetFieldLong(hResult, row, 5);
}

RackPassiveElement::RackPassiveElement(NXCPMessage *pRequest, UINT32 base)
{
   m_id = pRequest->getFieldAsInt32(base++);
   if (m_id == 0)
   {
      m_id = CreateUniqueId(IDG_RACK_ELEMENT);
   }
   m_name = pRequest->getFieldAsString(base++);
   m_type = (RackElementType)pRequest->getFieldAsInt32(base++);
   m_position = pRequest->getFieldAsInt32(base++);
   m_orientation = (RackOrientation)pRequest->getFieldAsInt32(base++);
   m_portCount = pRequest->getFieldAsInt32(base++);
}

/**
 * To JSON method for rack passive element
 */
json_t *RackPassiveElement::toJson()
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "type", json_integer(m_type));
   json_object_set_new(root, "position", json_integer(m_position));
   json_object_set_new(root, "orientation", json_integer(m_orientation));
   if(m_type == PATCH_PANEL)
   {
      json_object_set_new(root, "portCount", json_integer(m_portCount));
   }

   return root;
}

/**
 * Saves rack passive element to database
 */
bool RackPassiveElement::saveToDatabase(DB_HANDLE hdb, UINT32 parentId)
{
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO rack_passive_elements (name,type,position,orientation,port_count,id,rack_id) VALUES (?,?,?,?,?,?,?)"));
   if (hStmt == NULL)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_type);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_position);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_orientation);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (m_type == PATCH_PANEL) ? m_portCount  : 0);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, parentId);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);

   return success;
}

/**
 * Delete child objects from database like patch panel port inventory
 */
bool RackPassiveElement::deleteChildren(DB_HANDLE hdb, UINT32 parentId)
{
   bool success = true;
   if(m_type == PATCH_PANEL)
   {
      DeletePatchPanelFromPhysicalLinks(parentId, m_id);
   }
   return success;
}

/**
 * Fill message for rack passive element
 */
void RackPassiveElement::fillMessage(NXCPMessage *pMsg, UINT32 base)
{
   pMsg->setField(base++, m_id);
   pMsg->setField(base++, m_name);
   pMsg->setField(base++, m_type);
   pMsg->setField(base++, m_position);
   pMsg->setField(base++, m_orientation);
   pMsg->setField(base++, m_portCount);
}
