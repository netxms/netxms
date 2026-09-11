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
** File: physical_placement.cpp
**
** Shared read and write implementation of the "physicalPlacement" property group used by
** Node and Chassis on the WebAPI JSON path. Both directions live here so the serialized
** key set and the accepted key set cannot drift apart.
**
**/

#include "nxcore.h"

/**
 * Serialize physical placement property group.
 */
json_t *PhysicalPlacementToJson(const PhysicalPlacementRef& placement)
{
   json_t *group = json_object();
   json_object_set_new(group, "containerId", json_integer(*placement.containerId));
   json_object_set_new(group, "position", json_integer(*placement.position));
   json_object_set_new(group, "height", json_integer(*placement.height));
   json_object_set_new(group, "orientation", json_integer(*placement.orientation));
   json_object_set_new(group, "imageFront", placement.imageFront->toJson());
   json_object_set_new(group, "imageRear", placement.imageRear->toJson());
   return group;
}

/**
 * Check that a device of given height placed at given rack unit fits inside the rack.
 * Rack position is the unit occupied by the device's top edge when the rack is numbered
 * top to bottom, and by its bottom edge otherwise. Mirrors the client side check in
 * RackWidget, which silently skips an entity that fails it, so a layout accepted here is
 * one the rack view will actually draw.
 */
bool IsValidRackExtent(int position, int height, int rackHeight, bool topBottomNumbering)
{
   if ((position < 1) || (position > rackHeight))
      return false;
   if (topBottomNumbering)
      return position + height <= rackHeight + 1;
   return position - height >= 0;
}

/**
 * Apply physical placement property group as JSON merge-patch. Every field is staged and
 * validated before anything is written, so a rejected document leaves the object's
 * placement untouched - unlike the individual scalar properties handled by
 * NetObj::modifyFromJSONInternal, placement fields are only meaningful together.
 * The object being placed is needed to reject a container that is one of its own
 * descendants; it is only dereferenced once a container id actually resolves.
 */
uint32_t ModifyPhysicalPlacementFromJson(json_t *group, const NetObj *object, const PhysicalPlacementRef& placement, bool allowChassisContainer)
{
   if (!json_is_object(group))
      return RCC_INVALID_ARGUMENT;

   uint32_t containerId = *placement.containerId;
   int16_t position = *placement.position;
   int16_t height = *placement.height;
   RackOrientation orientation = *placement.orientation;
   uuid imageFront = *placement.imageFront;
   uuid imageRear = *placement.imageRear;

   if (!json_object_update_integer(group, "containerId", &containerId))
      return RCC_INVALID_ARGUMENT;

   // json_object_update_integer accepts null as 0, which is a valid clearing value for
   // containerId but not for position - 0 is "never placed", not a rack unit. Height and
   // orientation reject null on their own (height through the < 1 check below, orientation
   // through its explicit type check), so only position needs to be rejected here.
   if (json_is_null(json_object_get(group, "position")))
      return RCC_INVALID_ARGUMENT;
   if (!json_object_update_integer(group, "position", &position))
      return RCC_INVALID_ARGUMENT;
   if (!json_object_update_integer(group, "height", &height))
      return RCC_INVALID_ARGUMENT;

   // A height supplied by the document must be usable (this is also what rejects null, read
   // as 0 above). A stored height is checked below instead, and only for a rack.
   if ((json_object_get(group, "height") != nullptr) && (height < 1))
      return RCC_INVALID_ARGUMENT;

   json_t *value = json_object_get(group, "orientation");
   if (value != nullptr)
   {
      if (!json_is_integer(value))
         return RCC_INVALID_ARGUMENT;
      int64_t v = json_integer_value(value);
      if ((v < FILL) || (v > REAR))
         return RCC_INVALID_ARGUMENT;
      orientation = static_cast<RackOrientation>(v);
   }

   const struct { const char *key; uuid *target; } imageFields[] = {
      { "imageFront", &imageFront },
      { "imageRear",  &imageRear  }
   };
   for(const auto& field : imageFields)
   {
      value = json_object_get(group, field.key);
      if (value == nullptr)
         continue;
      if (json_is_null(value))
      {
         *field.target = uuid::NULL_UUID;
      }
      else if (json_is_string(value))
      {
         uuid_t parsed;
         if (_uuid_parseA(json_string_value(value), parsed) != 0)
            return RCC_INVALID_ARGUMENT;
         *field.target = uuid(parsed);
      }
      else
      {
         return RCC_INVALID_ARGUMENT;
      }
   }

   // Geometry is validated only when the document touches it, and only against a rack. Stored
   // values can be unusable - the NXCP path validates nothing, and the console stores height 0
   // for anything it places into a chassis - and such an object must stay patchable and
   // removable. Placing into a rack therefore needs an extent that fits, sent in the document
   // or already stored; a never placed object has position 0 and must supply one.
   if ((json_object_get(group, "containerId") != nullptr) || (json_object_get(group, "position") != nullptr) ||
       (json_object_get(group, "height") != nullptr))
   {
      if (containerId != 0)
      {
         shared_ptr<NetObj> container = FindObjectById(containerId);
         if (container == nullptr)
            return RCC_INVALID_OBJECT_ID;
         if (container->getObjectClass() == OBJECT_RACK)
         {
            if (height < 1)
               return RCC_INVALID_ARGUMENT;
            const Rack *rack = static_cast<const Rack*>(container.get());
            if (!IsValidRackExtent(position, height, rack->getHeight(), rack->isTopBottomNumbering()))
               return RCC_INVALID_ARGUMENT;
         }
         else if (!allowChassisContainer || (container->getObjectClass() != OBJECT_CHASSIS))
         {
            return RCC_INVALID_OBJECT_ID;
         }

         // Placing an object inside one of its own descendants would close a cycle in the
         // object tree. The binding that follows calls NetObj::clearInheritedAccessCache,
         // which walks the child list with no cycle detection and would recurse until the
         // server dies; ChangeObjectBinding guards the generic bind path the same way.
         if (object->isChild(containerId))
            return RCC_OBJECT_LOOP;
      }
   }

   *placement.containerId = containerId;
   *placement.position = position;
   *placement.height = height;
   *placement.orientation = orientation;
   *placement.imageFront = imageFront;
   *placement.imageRear = imageRear;
   return RCC_SUCCESS;
}

/**
 * Serialize chassis placement geometry into the <placement> XML document stored in
 * Node::m_chassisPlacementConf. The orientation tag is deliberately spelled
 * "oritentaiton": that is the name the reader (Node::getChassisPlacement) and every
 * client expect, including the Java ChassisPlacement class, and correcting it here would
 * silently drop orientation for all of them. Every member is emitted, so the resulting
 * document always parses into a complete geometry. All values are integers or a UUID in
 * canonical form, so no XML escaping is needed.
 * Returns a MemAlloc'ed UTF-8 string; caller takes ownership.
 */
char NXCORE_EXPORTABLE *ChassisPlacementToXml(json_t *placement)
{
   char image[64];
   json_object_get_uuid(placement, "image").toStringA(image);

   char buffer[1024];
   snprintf(buffer, sizeof(buffer),
      "<placement>\n"
      "   <image>%s</image>\n"
      "   <height>%d</height>\n"
      "   <heightUnits>%d</heightUnits>\n"
      "   <width>%d</width>\n"
      "   <widthUnits>%d</widthUnits>\n"
      "   <positionHeight>%d</positionHeight>\n"
      "   <positionHeightUnits>%d</positionHeightUnits>\n"
      "   <positionWidth>%d</positionWidth>\n"
      "   <positionWidthUnits>%d</positionWidthUnits>\n"
      "   <oritentaiton>%d</oritentaiton>\n"
      "</placement>\n",
      image,
      json_object_get_int32(placement, "height"),
      json_object_get_int32(placement, "heightUnits"),
      json_object_get_int32(placement, "width"),
      json_object_get_int32(placement, "widthUnits"),
      json_object_get_int32(placement, "positionHeight"),
      json_object_get_int32(placement, "positionHeightUnits"),
      json_object_get_int32(placement, "positionWidth"),
      json_object_get_int32(placement, "positionWidthUnits"),
      json_object_get_int32(placement, "orientation"));
   return MemCopyStringA(buffer);
}

/**
 * Validate the type of every key in a chassis placement geometry document without mutating
 * anything. ChassisPlacementToXml reads the nine geometry keys with json_object_get_int32,
 * which silently coerces a non-integer value to 0 - without this check a malformed value
 * would not fail, it would just zero the corresponding stored dimension. Unknown keys are
 * ignored, not rejected, matching the group's merge-patch semantics; an absent key is fine
 * for the same reason. The image is checked for a parseable UUID as well, because
 * json_object_get_uuid coerces a malformed string to the null UUID, which would silently
 * clear the stored image. Range checks (e.g. height >= 1) are intentionally out of scope.
 */
uint32_t ValidateChassisPlacementJson(json_t *placement)
{
   if (!json_is_object(placement))
      return RCC_INVALID_ARGUMENT;

   static const char *integerKeys[] = {
      "height", "heightUnits", "width", "widthUnits",
      "positionHeight", "positionHeightUnits", "positionWidth", "positionWidthUnits",
      "orientation"
   };
   for(const char *key : integerKeys)
   {
      json_t *value = json_object_get(placement, key);
      if ((value != nullptr) && !json_is_integer(value))
         return RCC_INVALID_ARGUMENT;
   }

   json_t *image = json_object_get(placement, "image");
   if (image != nullptr)
   {
      if (json_is_string(image))
      {
         uuid_t parsed;
         if (_uuid_parseA(json_string_value(image), parsed) != 0)
            return RCC_INVALID_ARGUMENT;
      }
      else if (!json_is_null(image))
      {
         return RCC_INVALID_ARGUMENT;
      }
   }

   return RCC_SUCCESS;
}
