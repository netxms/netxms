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
 * Initialize traffic connectors from loaded modules
 */
void InitializeTrafficConnectors()
{
   int count = 0;
   ENUMERATE_MODULES(trafficConnector)
   {
      TrafficConnectorStatus status = CURRENT_MODULE.trafficConnector->Initialize();
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
