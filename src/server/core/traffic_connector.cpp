/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: traffic_connector.cpp
**
**/

#include "nxcore.h"
#include <traffic-connector.h>

#define DEBUG_TAG L"traffic.connector"

/**
 * Registered traffic connectors (name -> interface)
 */
static StringObjectMap<TrafficConnectorInterface> s_connectors(Ownership::False);

/**
 * Get error message for given status
 */
const wchar_t *GetTrafficConnectorErrorMessage(TrafficConnectorStatus status)
{
   static const wchar_t *messages[] =
   {
      L"No errors",
      L"Connector unavailable",
      L"Function not implemented",
      L"External API error",
      L"Authentication error"
   };
   return (static_cast<int>(status) < 5) ? messages[static_cast<int>(status)] : L"Unknown error";
}

/**
 * Find traffic connector by name
 */
TrafficConnectorInterface NXCORE_EXPORTABLE *FindTrafficConnector(const wchar_t *name)
{
   return s_connectors.get(name);
}

/**
 * Get list of available traffic connectors
 */
StringList NXCORE_EXPORTABLE GetTrafficConnectorNames()
{
   return s_connectors.keys();
}

/**
 * Symbolic name for credential field type (external API surface)
 */
static const char *CredentialFieldTypeName(TrafficCredentialFieldType type)
{
   switch(type)
   {
      case TrafficCredentialFieldType::PASSWORD:
         return "password";
      case TrafficCredentialFieldType::INTEGER:
         return "integer";
      case TrafficCredentialFieldType::BOOLEAN:
         return "boolean";
      default:
         return "string";
   }
}

/**
 * Get available traffic connectors with their credential field descriptors as JSON array
 */
json_t NXCORE_EXPORTABLE *GetTrafficConnectorsAsJson()
{
   json_t *connectors = json_array();
   StringList names = GetTrafficConnectorNames();
   for(int i = 0; i < names.size(); i++)
   {
      TrafficConnectorInterface *connector = s_connectors.get(names.get(i));
      if (connector == nullptr)
         continue;

      json_t *fields = json_array();
      for(size_t j = 0; j < connector->credentialFieldCount; j++)
      {
         const TrafficCredentialField& field = connector->credentialFields[j];
         json_t *f = json_object();
         json_object_set_new(f, "name", json_string(field.name));
         json_object_set_new(f, "displayName", json_string_w(field.displayName));
         json_object_set_new(f, "description", (field.description != nullptr) ? json_string_w(field.description) : json_null());
         json_object_set_new(f, "type", json_string(CredentialFieldTypeName(field.type)));
         json_object_set_new(f, "required", json_boolean(field.required));
         json_object_set_new(f, "defaultValue", (field.defaultValue != nullptr) ? json_string_w(field.defaultValue) : json_null());
         json_array_append_new(fields, f);
      }

      json_t *c = json_object();
      json_object_set_new(c, "name", json_string_w(names.get(i)));
      json_object_set_new(c, "credentialFields", fields);
      json_array_append_new(connectors, c);
   }
   return connectors;
}

/**
 * Initialize traffic connectors from loaded modules
 */
void InitializeTrafficConnectors()
{
   int count = 0;
   ENUMERATE_MODULES(trafficConnector)
   {
      TrafficConnectorStatus status = (CURRENT_MODULE.trafficConnector->Initialize != nullptr) ?
               CURRENT_MODULE.trafficConnector->Initialize() : TrafficConnectorStatus::SUCCESS;
      if (status == TrafficConnectorStatus::SUCCESS)
      {
         s_connectors.set(CURRENT_MODULE.name, CURRENT_MODULE.trafficConnector);
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Traffic connector \"%s\" registered successfully", CURRENT_MODULE.name);
         count++;
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG,
            L"Traffic connector provided by module %s cannot be initialized (%s)", CURRENT_MODULE.name, GetTrafficConnectorErrorMessage(status));
      }
   }

   if (count > 0)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"%d traffic connector(s) registered", count);
      RegisterComponent(L"TRAFFIC");
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"No traffic connectors available");
   }
}
