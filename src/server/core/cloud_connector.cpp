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
** File: cloud_connector.cpp
**
**/

#include "nxcore.h"
#include <cloud-connector.h>

#define DEBUG_TAG L"cloud.connector"

/**
 * Registered cloud connectors (name -> interface)
 */
static StringObjectMap<CloudConnectorInterface> s_connectors(Ownership::False);

/**
 * Get error message for given status
 */
const wchar_t *GetCloudConnectorErrorMessage(CloudConnectorStatus status)
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
 * Find cloud connector by name
 */
CloudConnectorInterface *FindCloudConnector(const wchar_t *name)
{
   return s_connectors.get(name);
}

/**
 * Initialize cloud connectors from loaded modules
 */
void InitializeCloudConnectors()
{
   int count = 0;
   ENUMERATE_MODULES(cloudConnector)
   {
      CloudConnectorStatus status = CURRENT_MODULE.cloudConnector->Initialize();
      if (status == CloudConnectorStatus::SUCCESS)
      {
         s_connectors.set(CURRENT_MODULE.name, CURRENT_MODULE.cloudConnector);
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Cloud connector \"%s\" registered successfully", CURRENT_MODULE.name);
         count++;
      }
      else
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG,
            L"Cloud connector provided by module %s cannot be initialized (%s)", CURRENT_MODULE.name, GetCloudConnectorErrorMessage(status));
      }
   }

   if (count > 0)
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"%d cloud connector(s) registered", count);
      InterlockedOr64(&g_flags, AF_CLOUD_CONNECTOR_ENABLED);
   }
   else
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"No cloud connectors available");
   }
}
