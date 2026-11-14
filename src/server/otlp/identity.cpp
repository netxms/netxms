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
** File: identity.cpp
**
** Identity of exported objects (object ID, GUID and class, host identity for nodes, hardware
** identity) as OTLP resource attributes. Shared by the metric exporter and the event forwarder.
**
**/

#include "otlp.h"

/**
 * Add hardware identity attributes, skipping empty values
 */
static void AddHardwareAttributes(std::vector<OtlpExportAttribute> *attributes, const wchar_t *vendor, const wchar_t *model, const wchar_t *serialNumber)
{
   if (*vendor != 0)
      attributes->emplace_back("hw.vendor", vendor);
   if (*model != 0)
      attributes->emplace_back("hw.model", model);
   if (*serialNumber != 0)
      attributes->emplace_back("hw.serial_number", serialNumber);
}

/**
 * Add identity attributes of given object to OTLP resource attribute list. Host identity
 * (host.name, host.id) is only set for nodes, so that sensors, racks, facility objects and
 * other non-host objects do not appear as hosts in backends that build host inventories.
 */
void AddObjectIdentityAttributes(std::vector<OtlpExportAttribute> *attributes, const NetObj& object)
{
   if (object.getObjectClass() == OBJECT_NODE)
   {
      attributes->emplace_back("host.name", object.getName());
      attributes->emplace_back("host.id", object.getGuid());
   }

   attributes->emplace_back("netxms.object.id", static_cast<int64_t>(object.getId()));
   attributes->emplace_back("netxms.object.guid", object.getGuid());
   attributes->emplace_back("netxms.object.class", object.getObjectClassNameA());

   switch(object.getObjectClass())
   {
      case OBJECT_NODE:
      {
         const Node& node = static_cast<const Node&>(object);
         AddHardwareAttributes(attributes, node.getVendor().cstr(), node.getProductName().cstr(), node.getSerialNumber().cstr());
         break;
      }
      case OBJECT_SENSOR:
      {
         const Sensor& sensor = static_cast<const Sensor&>(object);
         AddHardwareAttributes(attributes, sensor.getVendor().cstr(), sensor.getModel().cstr(), sensor.getSerialNumber().cstr());
         attributes->emplace_back("netxms.sensor.class", SensorDeviceClassName(sensor.getDeviceClass()));
         break;
      }
      case OBJECT_ACCESSPOINT:
      {
         const AccessPoint& ap = static_cast<const AccessPoint&>(object);
         AddHardwareAttributes(attributes, ap.getVendor(), ap.getModel(), ap.getSerialNumber());
         break;
      }
      case OBJECT_MOBILEDEVICE:
      {
         const MobileDevice& device = static_cast<const MobileDevice&>(object);
         AddHardwareAttributes(attributes, device.getVendor().cstr(), device.getModel().cstr(), device.getSerialNumber().cstr());
         break;
      }
      default:
         break;
   }
}
