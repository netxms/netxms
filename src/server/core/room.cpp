/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: room.cpp
**
**/

#include "nxcore.h"

/**
 * Limits for room outline
 */
#define MIN_OUTLINE_VERTICES  3
#define MAX_OUTLINE_VERTICES  1024

/**
 * Dimensions of default rectangular outline (millimetres)
 */
#define DEFAULT_ROOM_WIDTH    6000
#define DEFAULT_ROOM_DEPTH    6000

/**
 * Symbolic names (index = enumeration value)
 */
static const char *s_roomTypeNames[] = { "COMPUTER_ROOM", "ELECTRICAL", "MECHANICAL", "TELECOM", "OTHER" };
static const char *s_gridLabelsNames[] = { "NONE", "LETTERS_NUMBERS", "NUMBERS_NUMBERS" };
static const char *s_elementTypeNames[] = { "COLUMN", "WALL", "RAMP", "STAIRS", "DOOR", "OTHER" };

/**
 * Find symbolic name in given list. Returns index or -1 if name is not recognized.
 */
static int FindSymbolicName(const char *name, const char **names, int count)
{
   for(int i = 0; i < count; i++)
      if (!stricmp(name, names[i]))
         return i;
   return -1;
}

/**
 * Read enumeration value from JSON: either symbolic name or integer. Returns -1 if value is invalid.
 */
static int EnumFromJson(json_t *value, const char **names, int count)
{
   if (json_is_string(value))
      return FindSymbolicName(json_string_value(value), names, count);
   if (json_is_integer(value))
   {
      json_int_t n = json_integer_value(value);
      return ((n >= 0) && (n < count)) ? static_cast<int>(n) : -1;
   }
   return -1;
}

/**
 * Get symbolic name of room type
 */
const char *RoomTypeName(RoomType type)
{
   return s_roomTypeNames[RoomTypeFromInt(type)];
}

/**
 * Convert symbolic name to room type. Returns false if name is not recognized.
 */
bool RoomTypeFromName(const char *name, RoomType *type)
{
   int n = FindSymbolicName(name, s_roomTypeNames, ROOM_OTHER + 1);
   if (n == -1)
      return false;
   *type = static_cast<RoomType>(n);
   return true;
}

/**
 * Get symbolic name of grid label scheme
 */
const char *RoomGridLabelsName(RoomGridLabels labels)
{
   return s_gridLabelsNames[RoomGridLabelsFromInt(labels)];
}

/**
 * Convert symbolic name to grid label scheme. Returns false if name is not recognized.
 */
bool RoomGridLabelsFromName(const char *name, RoomGridLabels *labels)
{
   int n = FindSymbolicName(name, s_gridLabelsNames, ROOM_GRID_LABELS_NUMBERS_NUMBERS + 1);
   if (n == -1)
      return false;
   *labels = static_cast<RoomGridLabels>(n);
   return true;
}

/**
 * Get symbolic name of room passive element type
 */
const char *RoomElementTypeName(RoomElementType type)
{
   return s_elementTypeNames[RoomElementTypeFromInt(type)];
}

/**
 * Convert symbolic name to room passive element type. Returns false if name is not recognized.
 */
bool RoomElementTypeFromName(const char *name, RoomElementType *type)
{
   int n = FindSymbolicName(name, s_elementTypeNames, ROOM_ELEMENT_OTHER + 1);
   if (n == -1)
      return false;
   *type = static_cast<RoomElementType>(n);
   return true;
}

/**
 * Calculate area of simple polygon in square metres using shoelace formula (vertices in millimetres)
 */
double CalculateRoomOutlineArea(const StructArray<RoomPoint>& outline)
{
   int count = outline.size();
   if (count < MIN_OUTLINE_VERTICES)
      return 0;

   int64_t sum = 0;
   for(int i = 0; i < count; i++)
   {
      const RoomPoint *p1 = outline.get(i);
      const RoomPoint *p2 = outline.get((i + 1) % count);
      sum += static_cast<int64_t>(p1->x) * p2->y - static_cast<int64_t>(p2->x) * p1->y;
   }
   if (sum < 0)
      sum = -sum;
   return static_cast<double>(sum) / 2000000.0;
}

/**
 * Parse outline from database text ("x,y;x,y;...")
 */
static void ParseOutline(const wchar_t *text, StructArray<RoomPoint> *outline)
{
   outline->clear();
   if (text == nullptr)
      return;

   const wchar_t *p = text;
   while(*p != 0)
   {
      wchar_t *eptr;
      RoomPoint point;
      point.x = static_cast<int32_t>(wcstol(p, &eptr, 10));
      if (*eptr != L',')
         break;
      point.y = static_cast<int32_t>(wcstol(eptr + 1, &eptr, 10));
      outline->add(point);
      if (*eptr != L';')
         break;
      p = eptr + 1;
   }
}

/**
 * Serialize outline to database text
 */
static StringBuffer OutlineToText(const StructArray<RoomPoint>& outline)
{
   StringBuffer text;
   for(int i = 0; i < outline.size(); i++)
   {
      const RoomPoint *p = outline.get(i);
      if (i > 0)
         text.append(L';');
      text.append(p->x);
      text.append(L',');
      text.append(p->y);
   }
   return text;
}

/**
 * Read outline from NXCP message (array of interleaved x,y values). Returns false if outline is invalid.
 */
static bool OutlineFromMessage(const NXCPMessage& msg, StructArray<RoomPoint> *outline)
{
   IntegerArray<uint32_t> values;
   msg.getFieldAsInt32Array(VID_OUTLINE, &values);
   int count = values.size() / 2;
   if (((values.size() % 2) != 0) || (count < MIN_OUTLINE_VERTICES) || (count > MAX_OUTLINE_VERTICES))
      return false;

   outline->clear();
   for(int i = 0; i < count; i++)
   {
      RoomPoint point;
      point.x = static_cast<int32_t>(values.get(i * 2));
      point.y = static_cast<int32_t>(values.get(i * 2 + 1));
      outline->add(point);
   }
   return true;
}

/**
 * Read outline from JSON array of {x, y} objects. Returns false if outline is invalid.
 */
static bool OutlineFromJson(json_t *json, StructArray<RoomPoint> *outline)
{
   if (!json_is_array(json))
      return false;

   size_t count = json_array_size(json);
   if ((count < MIN_OUTLINE_VERTICES) || (count > MAX_OUTLINE_VERTICES))
      return false;

   StructArray<RoomPoint> points(static_cast<int>(count));
   for(size_t i = 0; i < count; i++)
   {
      json_t *e = json_array_get(json, i);
      json_t *x = json_object_get(e, "x");
      json_t *y = json_object_get(e, "y");
      if (!json_is_integer(x) || !json_is_integer(y))
         return false;
      RoomPoint point;
      point.x = static_cast<int32_t>(json_integer_value(x));
      point.y = static_cast<int32_t>(json_integer_value(y));
      points.add(point);
   }

   outline->clear();
   for(int i = 0; i < points.size(); i++)
      outline->add(points.get(i));
   return true;
}

/**
 * Default constructor
 */
Room::Room() : super(), m_outline(0, 8), m_passiveElements(0, 16, Ownership::True)
{
   m_roomType = ROOM_OTHER;
   m_height = 0;
   m_gridOriginX = 0;
   m_gridOriginY = 0;
   m_gridTileSize = 0;
   m_gridLabels = ROOM_GRID_LABELS_NONE;
   m_backgroundScale = 0;
   m_backgroundX = 0;
   m_backgroundY = 0;
}

/**
 * Constructor from NXCP message (object creation request). Outline is taken from the request
 * if provided, otherwise rectangular outline is generated from width and depth.
 */
Room::Room(const TCHAR *name, const NXCPMessage& request) : super(name), m_outline(0, 8), m_passiveElements(0, 16, Ownership::True)
{
   m_roomType = RoomTypeFromInt(request.getFieldAsInt32(VID_ROOM_TYPE));
   m_height = std::max(request.getFieldAsInt32(VID_HEIGHT), 0);
   m_gridOriginX = 0;
   m_gridOriginY = 0;
   m_gridTileSize = 0;
   m_gridLabels = ROOM_GRID_LABELS_NONE;
   m_backgroundScale = 0;
   m_backgroundX = 0;
   m_backgroundY = 0;
   if (!request.isFieldExist(VID_OUTLINE) || !OutlineFromMessage(request, &m_outline))
      setRectangularOutline(request.getFieldAsInt32(VID_WIDTH), request.getFieldAsInt32(VID_DEPTH));
}

/**
 * Constructor from JSON definition (REST API and NXSL)
 */
Room::Room(const TCHAR *name, json_t *json) : super(name), m_outline(0, 8), m_passiveElements(0, 16, Ownership::True)
{
   m_roomType = ROOM_OTHER;
   json_t *value = json_object_get(json, "roomType");
   if (value != nullptr)
   {
      int n = EnumFromJson(value, s_roomTypeNames, ROOM_OTHER + 1);
      if (n != -1)
         m_roomType = static_cast<RoomType>(n);
   }
   m_height = std::max(json_object_get_int32(json, "height"), 0);
   m_gridOriginX = 0;
   m_gridOriginY = 0;
   m_gridTileSize = 0;
   m_gridLabels = ROOM_GRID_LABELS_NONE;
   m_backgroundScale = 0;
   m_backgroundX = 0;
   m_backgroundY = 0;
   value = json_object_get(json, "outline");
   if ((value == nullptr) || !OutlineFromJson(value, &m_outline))
      setRectangularOutline(json_object_get_int32(json, "width"), json_object_get_int32(json, "depth"));
}

/**
 * Set rectangular outline with given dimensions (defaults are used for non-positive values)
 */
void Room::setRectangularOutline(int32_t width, int32_t depth)
{
   if (width <= 0)
      width = DEFAULT_ROOM_WIDTH;
   if (depth <= 0)
      depth = DEFAULT_ROOM_DEPTH;

   static const int32_t cx[] = { 0, 1, 1, 0 };
   static const int32_t cy[] = { 0, 0, 1, 1 };
   m_outline.clear();
   for(int i = 0; i < 4; i++)
   {
      RoomPoint point;
      point.x = cx[i] * width;
      point.y = cy[i] * depth;
      m_outline.add(point);
   }
}

/**
 * Load from database
 */
bool Room::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, L"SELECT room_type,outline,height,grid_origin_x,grid_origin_y,grid_tile_size,grid_labels,background_image,background_scale,background_x,background_y FROM rooms WHERE id=?");
   if (hStmt == nullptr)
      return false;

   bool success = false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         m_roomType = RoomTypeFromInt(DBGetFieldLong(hResult, 0, 0));
         wchar_t *outline = DBGetField(hResult, 0, 1, nullptr, 0);
         ParseOutline(outline, &m_outline);
         MemFree(outline);
         m_height = DBGetFieldLong(hResult, 0, 2);
         m_gridOriginX = DBGetFieldLong(hResult, 0, 3);
         m_gridOriginY = DBGetFieldLong(hResult, 0, 4);
         m_gridTileSize = DBGetFieldLong(hResult, 0, 5);
         m_gridLabels = RoomGridLabelsFromInt(DBGetFieldLong(hResult, 0, 6));
         m_backgroundImage = DBGetFieldGUID(hResult, 0, 7);
         m_backgroundScale = DBGetFieldLong(hResult, 0, 8);
         m_backgroundX = DBGetFieldLong(hResult, 0, 9);
         m_backgroundY = DBGetFieldLong(hResult, 0, 10);
         success = true;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   if (!success)
      return false;

   hStmt = DBPrepare(hdb, L"SELECT id,name,type,x,y,rotation,width,depth FROM room_passive_elements WHERE room_id=?");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
         m_passiveElements.add(new RoomPassiveElement(hResult, i));
      DBFreeResult(hResult);
   }
   else
   {
      success = false;
   }
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Save to database
 */
bool Room::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const wchar_t *columns[] = { L"room_type", L"outline", L"height", L"grid_origin_x", L"grid_origin_y", L"grid_tile_size",
         L"grid_labels", L"background_image", L"background_scale", L"background_x", L"background_y", nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, L"rooms", L"id", m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_roomType));
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, OutlineToText(m_outline), DB_BIND_TRANSIENT);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_height);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_gridOriginX);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_gridOriginY);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_gridTileSize);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_gridLabels));
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, m_backgroundImage);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_backgroundScale);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_backgroundX);
         DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, m_backgroundY);
         DBBind(hStmt, 12, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);

         if (success)
            success = executeQueryOnObject(hdb, L"DELETE FROM room_passive_elements WHERE room_id=?");
         for(int i = 0; (i < m_passiveElements.size()) && success; i++)
            success = m_passiveElements.get(i)->saveToDatabase(hdb, m_id);
         unlockProperties();
      }
      else
      {
         success = false;
      }
   }
   return success;
}

/**
 * Delete from database
 */
bool Room::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM rooms WHERE id=?");
   if (success)
      success = executeQueryOnObject(hdb, L"DELETE FROM room_passive_elements WHERE room_id=?");
   return success;
}

/**
 * Get copy of room outline
 */
StructArray<RoomPoint> Room::getOutline() const
{
   lockProperties();
   StructArray<RoomPoint> outline(m_outline);
   unlockProperties();
   return outline;
}

/**
 * Get image library UUID of floor plan backdrop
 */
uuid Room::getBackgroundImage() const
{
   lockProperties();
   uuid image = m_backgroundImage;
   unlockProperties();
   return image;
}

/**
 * Get floor area in square metres
 */
double Room::getArea() const
{
   lockProperties();
   double area = CalculateRoomOutlineArea(m_outline);
   unlockProperties();
   return area;
}

/**
 * Fill message with object fields
 */
void Room::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_ROOM_TYPE, static_cast<int16_t>(m_roomType));
   msg->setFieldFromInt32Array(VID_OUTLINE, static_cast<size_t>(m_outline.size()) * 2, reinterpret_cast<const uint32_t*>(m_outline.getBuffer()));
   msg->setField(VID_AREA, CalculateRoomOutlineArea(m_outline));
   msg->setField(VID_HEIGHT, m_height);
   msg->setField(VID_GRID_ORIGIN_X, m_gridOriginX);
   msg->setField(VID_GRID_ORIGIN_Y, m_gridOriginY);
   msg->setField(VID_GRID_TILE_SIZE, m_gridTileSize);
   msg->setField(VID_GRID_LABELS, static_cast<int16_t>(m_gridLabels));
   msg->setField(VID_BACKGROUND, m_backgroundImage);
   msg->setField(VID_BACKGROUND_SCALE, m_backgroundScale);
   msg->setField(VID_BACKGROUND_X, m_backgroundX);
   msg->setField(VID_BACKGROUND_Y, m_backgroundY);
   msg->setField(VID_NUM_ELEMENTS, m_passiveElements.size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < m_passiveElements.size(); i++)
   {
      m_passiveElements.get(i)->fillMessage(msg, fieldId);
      fieldId += 10;
   }
}

/**
 * Modify object from message
 */
uint32_t Room::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_OUTLINE) && !OutlineFromMessage(msg, &m_outline))
      return RCC_INVALID_ARGUMENT;

   if (msg.isFieldExist(VID_HEIGHT))
   {
      int32_t height = msg.getFieldAsInt32(VID_HEIGHT);
      if (height < 0)
         return RCC_INVALID_ARGUMENT;
      m_height = height;
   }

   if (msg.isFieldExist(VID_GRID_TILE_SIZE))
   {
      int32_t tileSize = msg.getFieldAsInt32(VID_GRID_TILE_SIZE);
      if (tileSize < 0)
         return RCC_INVALID_ARGUMENT;
      m_gridTileSize = tileSize;
   }

   if (msg.isFieldExist(VID_BACKGROUND_SCALE))
   {
      int32_t scale = msg.getFieldAsInt32(VID_BACKGROUND_SCALE);
      if (scale < 0)
         return RCC_INVALID_ARGUMENT;
      m_backgroundScale = scale;
   }

   if (msg.isFieldExist(VID_ROOM_TYPE))
      m_roomType = RoomTypeFromInt(msg.getFieldAsInt32(VID_ROOM_TYPE));
   if (msg.isFieldExist(VID_GRID_ORIGIN_X))
      m_gridOriginX = msg.getFieldAsInt32(VID_GRID_ORIGIN_X);
   if (msg.isFieldExist(VID_GRID_ORIGIN_Y))
      m_gridOriginY = msg.getFieldAsInt32(VID_GRID_ORIGIN_Y);
   if (msg.isFieldExist(VID_GRID_LABELS))
      m_gridLabels = RoomGridLabelsFromInt(msg.getFieldAsInt32(VID_GRID_LABELS));
   if (msg.isFieldExist(VID_BACKGROUND))
      m_backgroundImage = msg.getFieldAsGUID(VID_BACKGROUND);
   if (msg.isFieldExist(VID_BACKGROUND_X))
      m_backgroundX = msg.getFieldAsInt32(VID_BACKGROUND_X);
   if (msg.isFieldExist(VID_BACKGROUND_Y))
      m_backgroundY = msg.getFieldAsInt32(VID_BACKGROUND_Y);

   if (msg.isFieldExist(VID_NUM_ELEMENTS))
   {
      int count = msg.getFieldAsInt32(VID_NUM_ELEMENTS);
      m_passiveElements.clear();
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         m_passiveElements.add(new RoomPassiveElement(msg, fieldId));
         fieldId += 10;
      }
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Build "room" property group
 */
json_t *Room::roomConfigToJson()
{
   json_t *group = json_object();
   lockProperties();
   json_object_set_new(group, "roomType", json_string(RoomTypeName(m_roomType)));
   json_t *outline = json_array();
   for(int i = 0; i < m_outline.size(); i++)
   {
      const RoomPoint *p = m_outline.get(i);
      json_t *point = json_object();
      json_object_set_new(point, "x", json_integer(p->x));
      json_object_set_new(point, "y", json_integer(p->y));
      json_array_append_new(outline, point);
   }
   json_object_set_new(group, "outline", outline);
   json_object_set_new(group, "area", json_real(CalculateRoomOutlineArea(m_outline)));
   json_object_set_new(group, "height", json_integer(m_height));
   json_object_set_new(group, "gridOriginX", json_integer(m_gridOriginX));
   json_object_set_new(group, "gridOriginY", json_integer(m_gridOriginY));
   json_object_set_new(group, "gridTileSize", json_integer(m_gridTileSize));
   json_object_set_new(group, "gridLabels", json_string(RoomGridLabelsName(m_gridLabels)));
   json_object_set_new(group, "backgroundImage", m_backgroundImage.isNull() ? json_null() : json_string_t(m_backgroundImage.toString()));
   json_object_set_new(group, "backgroundScale", json_integer(m_backgroundScale));
   json_object_set_new(group, "backgroundX", json_integer(m_backgroundX));
   json_object_set_new(group, "backgroundY", json_integer(m_backgroundY));
   json_t *passiveElements = json_array();
   for(int i = 0; i < m_passiveElements.size(); i++)
      json_array_append_new(passiveElements, m_passiveElements.get(i)->toJson());
   json_object_set_new(group, "passiveElements", passiveElements);
   unlockProperties();
   return group;
}

/**
 * Serialize object to JSON
 */
json_t *Room::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);
   json_object_set_new(root, "room", roomConfigToJson());
   return root;
}

/**
 * Build floor plan: room geometry with passive elements, and racks located in the room with their
 * footprint, placement and status. Racks are filtered by the requesting user's read access.
 */
json_t *Room::getFloorPlan(uint32_t userId)
{
   json_t *root = roomConfigToJson();
   json_object_set_new(root, "roomId", json_integer(m_id));
   json_object_set_new(root, "name", json_string_t(m_name));

   json_t *racks = json_array();
   unique_ptr<SharedObjectArray<NetObj>> children = getChildren(OBJECT_RACK);
   for(int i = 0; i < children->size(); i++)
   {
      Rack *rack = static_cast<Rack*>(children->get(i));
      if (!rack->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      json_t *object = rack->roomPlacementToJson();
      json_object_set_new(object, "id", json_integer(rack->getId()));
      json_object_set_new(object, "name", json_string_t(rack->getName()));
      json_object_set_new(object, "status", json_integer(rack->getStatus()));
      json_object_set_new(object, "height", json_integer(rack->getHeight()));
      json_object_set_new(object, "width", json_integer(rack->getWidth()));
      json_object_set_new(object, "depth", json_integer(rack->getDepth()));
      json_array_append_new(racks, object);
   }
   json_object_set_new(root, "racks", racks);
   return root;
}

/**
 * Modify object from JSON document (WebAPI path). Handles the "room" property group (passive elements
 * are managed through their own methods); all other fields are delegated to the base class implementation.
 */
uint32_t Room::modifyFromJSONInternal(json_t *json, GenericClientSession *session)
{
   json_t *group = json_object_get(json, "room");
   if (group != nullptr)
   {
      if (!json_is_object(group))
         return RCC_INVALID_ARGUMENT;

      json_t *value = json_object_get(group, "roomType");
      if (value != nullptr)
      {
         int n = EnumFromJson(value, s_roomTypeNames, ROOM_OTHER + 1);
         if (n == -1)
            return RCC_INVALID_ARGUMENT;
         m_roomType = static_cast<RoomType>(n);
      }

      value = json_object_get(group, "gridLabels");
      if (value != nullptr)
      {
         int n = EnumFromJson(value, s_gridLabelsNames, ROOM_GRID_LABELS_NUMBERS_NUMBERS + 1);
         if (n == -1)
            return RCC_INVALID_ARGUMENT;
         m_gridLabels = static_cast<RoomGridLabels>(n);
      }

      value = json_object_get(group, "outline");
      if ((value != nullptr) && !OutlineFromJson(value, &m_outline))
         return RCC_INVALID_ARGUMENT;

      int32_t height = m_height, gridTileSize = m_gridTileSize, backgroundScale = m_backgroundScale;
      if (!json_object_update_integer(group, "height", &height) ||
          !json_object_update_integer(group, "gridTileSize", &gridTileSize) ||
          !json_object_update_integer(group, "backgroundScale", &backgroundScale) ||
          (height < 0) || (gridTileSize < 0) || (backgroundScale < 0))
         return RCC_INVALID_ARGUMENT;
      m_height = height;
      m_gridTileSize = gridTileSize;
      m_backgroundScale = backgroundScale;

      if (!json_object_update_integer(group, "gridOriginX", &m_gridOriginX) ||
          !json_object_update_integer(group, "gridOriginY", &m_gridOriginY) ||
          !json_object_update_integer(group, "backgroundX", &m_backgroundX) ||
          !json_object_update_integer(group, "backgroundY", &m_backgroundY))
         return RCC_INVALID_ARGUMENT;

      // null clears background image
      value = json_object_get(group, "backgroundImage");
      if (value != nullptr)
      {
         if (json_is_null(value))
            m_backgroundImage = uuid::NULL_UUID;
         else if (json_is_string(value))
            m_backgroundImage = json_object_get_uuid(group, "backgroundImage");
         else
            return RCC_INVALID_ARGUMENT;
      }
   }
   return super::modifyFromJSONInternal(json, session);
}

/**
 * Validate passive room element fields present in a JSON document
 */
static bool ValidatePassiveElementJson(json_t *json)
{
   json_t *type = json_object_get(json, "type");
   if ((type != nullptr) && (EnumFromJson(type, s_elementTypeNames, ROOM_ELEMENT_OTHER + 1) == -1))
      return false;

   static const char *dimensions[] = { "width", "depth" };
   for(int i = 0; i < 2; i++)
   {
      json_t *value = json_object_get(json, dimensions[i]);
      if ((value != nullptr) && (!json_is_integer(value) || (json_integer_value(value) < 0)))
         return false;
   }
   return true;
}

/**
 * Create a new passive element from JSON document. On success returns RCC_SUCCESS and stores the JSON
 * representation of the created element (with its server-assigned identifier) in *element.
 */
uint32_t Room::createPassiveElementFromJson(json_t *json, json_t **element)
{
   if (!ValidatePassiveElementJson(json))
      return RCC_INVALID_ARGUMENT;

   RoomPassiveElement *e = new RoomPassiveElement(json);

   lockProperties();
   m_passiveElements.add(e);
   *element = e->toJson();
   setModified(MODIFY_OTHER);
   unlockProperties();

   return RCC_SUCCESS;
}

/**
 * Update an existing passive element from JSON document using merge-patch semantics. Returns
 * RCC_INVALID_OBJECT_ID if no element with the given identifier exists.
 */
uint32_t Room::updatePassiveElementFromJson(uint32_t elementId, json_t *json, json_t **element)
{
   if (!ValidatePassiveElementJson(json))
      return RCC_INVALID_ARGUMENT;

   uint32_t rcc = RCC_INVALID_OBJECT_ID;
   lockProperties();
   for(int i = 0; i < m_passiveElements.size(); i++)
   {
      RoomPassiveElement *e = m_passiveElements.get(i);
      if (e->getId() == elementId)
      {
         e->updateFromJson(json);
         *element = e->toJson();
         setModified(MODIFY_OTHER);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   unlockProperties();
   return rcc;
}

/**
 * Delete passive element by identifier. Returns RCC_INVALID_OBJECT_ID if no element with the given identifier exists.
 */
uint32_t Room::deletePassiveElement(uint32_t elementId)
{
   uint32_t rcc = RCC_INVALID_OBJECT_ID;
   lockProperties();
   for(int i = 0; i < m_passiveElements.size(); i++)
   {
      if (m_passiveElements.get(i)->getId() == elementId)
      {
         m_passiveElements.remove(i);
         setModified(MODIFY_OTHER);
         rcc = RCC_SUCCESS;
         break;
      }
   }
   unlockProperties();
   return rcc;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Room::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslRoomClass, new shared_ptr<Room>(self())));
}

/**
 * Create passive room element from database
 */
RoomPassiveElement::RoomPassiveElement(DB_RESULT hResult, int row)
{
   m_id = DBGetFieldULong(hResult, row, 0);
   m_name = DBGetField(hResult, row, 1, nullptr, 0);
   m_type = RoomElementTypeFromInt(DBGetFieldLong(hResult, row, 2));
   m_x = DBGetFieldLong(hResult, row, 3);
   m_y = DBGetFieldLong(hResult, row, 4);
   m_rotation = DBGetFieldLong(hResult, row, 5);
   m_width = DBGetFieldLong(hResult, row, 6);
   m_depth = DBGetFieldLong(hResult, row, 7);
}

/**
 * Create passive room element from NXCP message. New unique identifier is assigned if identifier in the message is 0.
 */
RoomPassiveElement::RoomPassiveElement(const NXCPMessage& request, uint32_t base)
{
   m_id = request.getFieldAsUInt32(base++);
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_ROOM_ELEMENT);
   m_name = request.getFieldAsString(base++);
   m_type = RoomElementTypeFromInt(request.getFieldAsInt16(base++));
   m_x = request.getFieldAsInt32(base++);
   m_y = request.getFieldAsInt32(base++);
   m_rotation = request.getFieldAsInt32(base++);
   m_width = std::max(request.getFieldAsInt32(base++), 0);
   m_depth = std::max(request.getFieldAsInt32(base++), 0);
}

/**
 * Create passive room element from JSON document. A new unique identifier is always assigned;
 * any "id" field in the document is ignored.
 */
RoomPassiveElement::RoomPassiveElement(json_t *json)
{
   m_id = CreateUniqueId(IDG_ROOM_ELEMENT);
   m_name = json_object_get_string_t(json, "name", L"");
   m_type = ROOM_ELEMENT_OTHER;
   m_x = 0;
   m_y = 0;
   m_rotation = 0;
   m_width = 0;
   m_depth = 0;
   json_t *type = json_object_get(json, "type");
   if (type != nullptr)
      m_type = RoomElementTypeFromInt(EnumFromJson(type, s_elementTypeNames, ROOM_ELEMENT_OTHER + 1));
   updateFromJson(json);
}

/**
 * Update mutable fields of a passive room element from a JSON document using merge-patch semantics.
 * The element identifier and its type are immutable and never changed here.
 */
void RoomPassiveElement::updateFromJson(json_t *json)
{
   if (json_object_get(json, "name") != nullptr)
   {
      MemFree(m_name);
      m_name = json_object_get_string_t(json, "name", L"");
   }
   m_x = json_object_get_int32(json, "x", m_x);
   m_y = json_object_get_int32(json, "y", m_y);
   m_rotation = json_object_get_int32(json, "rotation", m_rotation);
   m_width = json_object_get_int32(json, "width", m_width);
   m_depth = json_object_get_int32(json, "depth", m_depth);
}

/**
 * Serialize passive room element to JSON
 */
json_t *RoomPassiveElement::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "type", json_string(RoomElementTypeName(m_type)));
   json_object_set_new(root, "x", json_integer(m_x));
   json_object_set_new(root, "y", json_integer(m_y));
   json_object_set_new(root, "rotation", json_integer(m_rotation));
   json_object_set_new(root, "width", json_integer(m_width));
   json_object_set_new(root, "depth", json_integer(m_depth));
   return root;
}

/**
 * Save passive room element to database
 */
bool RoomPassiveElement::saveToDatabase(DB_HANDLE hdb, uint32_t roomId) const
{
   DB_STATEMENT hStmt = DBPrepare(hdb, L"INSERT INTO room_passive_elements (id,room_id,name,type,x,y,rotation,width,depth) VALUES (?,?,?,?,?,?,?,?,?)");
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, roomId);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_type));
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_x);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_y);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_rotation);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_width);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_depth);

   bool success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Fill NXCP message with passive room element data
 */
void RoomPassiveElement::fillMessage(NXCPMessage *msg, uint32_t base) const
{
   msg->setField(base++, m_id);
   msg->setField(base++, m_name);
   msg->setField(base++, static_cast<int16_t>(m_type));
   msg->setField(base++, m_x);
   msg->setField(base++, m_y);
   msg->setField(base++, m_rotation);
   msg->setField(base++, m_width);
   msg->setField(base++, m_depth);
}
