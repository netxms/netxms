/*
** NetXMS ntopng traffic connector module
** Copyright (C) 2026 Victor Kirhenshtein
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
** File: sync.cpp
**/

#include "ntopng.h"

/**
 * Push host aliases (hostKey "ip@vlan" -> name) to ntopng. Empty name clears the alias.
 */
TrafficConnectorStatus NtopngSyncHostAliases(const StringMap& aliases, json_t *credentials)
{
   TrafficConnectorStatus result = TrafficConnectorStatus::SUCCESS;
   int synced = 0;
   for (KeyValuePair<const wchar_t> *alias : aliases)
   {
      char hostKey[128];
      wchar_to_utf8(alias->key, -1, hostKey, sizeof(hostKey));
      int32_t vlan = 0;
      char *separator = strrchr(hostKey, '@');
      if (separator != nullptr)
      {
         *separator = 0;
         vlan = strtol(separator + 1, nullptr, 10);
      }

      char name[256];
      wchar_to_utf8(alias->value, -1, name, sizeof(name));

      json_t *body = json_object();
      json_object_set_new(body, "host", json_string(hostKey));
      json_object_set_new(body, "vlan", json_integer(vlan));
      json_object_set_new(body, "custom_name", json_string(name));

      TrafficConnectorStatus status;
      json_t *response = NtopngApiPost(credentials, "/lua/rest/v2/set/host/alias.lua", body, &status);
      json_decref(body);
      if (response != nullptr)
      {
         json_decref(response);
         synced++;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, L"NtopngSyncHostAliases: cannot set alias for host %hs (VLAN %d)", hostKey, vlan);
         result = status;
         if (status == TrafficConnectorStatus::AUTH_ERROR)
            break;   // subsequent requests will fail the same way
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 6, L"NtopngSyncHostAliases: %d of %d aliases pushed", synced, aliases.size());
   return result;
}
