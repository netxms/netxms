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
** File: discovery.cpp
**/

#include "ntopng.h"

/**
 * Test connection to ntopng instance and fill backend information
 */
TrafficConnectorStatus NtopngTestConnection(json_t *credentials, TrafficBackendInfo *info)
{
   TrafficConnectorStatus status;
   json_t *response = NtopngApiGet(credentials, "/lua/rest/v2/get/ntopng/about.lua", &status);
   if (response == nullptr)
      return status;

   json_t *about = json_object_get(response, "rsp");
   strlcpy(info->product, json_object_get_string_utf8(about, "product", "ntopng"), sizeof(info->product));
   strlcpy(info->version, json_object_get_string_utf8(about, "version", ""), sizeof(info->version));
   strlcpy(info->edition, json_object_get_string_utf8(about, "release", ""), sizeof(info->edition));
   json_decref(response);

   info->capabilities = NtopngGetCapabilities(credentials);
   return TrafficConnectorStatus::SUCCESS;
}

/**
 * Get capability set of ntopng instance. ntopng maintains an authoritative live
 * active-host table (idle age-out, not a top-N approximation), hence
 * HOST_SET_AUTHORITATIVE.
 */
uint64_t NtopngGetCapabilities(json_t *credentials)
{
   return TRAFFIC_CAPABILITY_HOST_L7 | TRAFFIC_CAPABILITY_HOST_PEERS |
      TRAFFIC_CAPABILITY_POINT_L7 | TRAFFIC_CAPABILITY_POINT_TOP_TALKERS |
      TRAFFIC_CAPABILITY_POINT_DSCP | TRAFFIC_CAPABILITY_SYNC_HOST_ALIASES |
      TRAFFIC_CAPABILITY_HOST_SET_AUTHORITATIVE;
}

/**
 * Get local networks configured for given ntopng interface
 */
static StringList *GetLocalNetworks(json_t *credentials, int32_t ifid)
{
   char path[256];
   snprintf(path, sizeof(path), "/lua/rest/v2/get/network/networks.lua?ifid=%d", ifid);
   json_t *response = NtopngApiGet(credentials, path);
   if (response == nullptr)
      return nullptr;

   StringList *networks = nullptr;
   json_t *list = json_object_get(response, "rsp");
   if (json_is_array(list) && (json_array_size(list) > 0))
   {
      networks = new StringList();
      size_t i;
      json_t *network;
      json_array_foreach(list, i, network)
      {
         const char *id = json_object_get_string_utf8(network, "id", nullptr);
         if (id != nullptr)
            networks->addUTF8String(id);
      }
   }
   json_decref(response);
   return networks;
}

/**
 * Discover observation points (ntopng interfaces)
 */
ObservationPointDescriptor *NtopngDiscoverPoints(json_t *credentials)
{
   TrafficConnectorStatus status;
   json_t *response = NtopngApiGet(credentials, "/lua/rest/v2/get/ntopng/interfaces.lua", &status);
   if (response == nullptr)
      return nullptr;

   ObservationPointDescriptor *head = nullptr, *tail = nullptr;
   json_t *interfaces = json_object_get(response, "rsp");
   if (json_is_array(interfaces))
   {
      size_t i;
      json_t *iface;
      json_array_foreach(interfaces, i, iface)
      {
         json_t *ifid = json_object_get(iface, "ifid");
         if (!json_is_integer(ifid))
            continue;

         auto d = new ObservationPointDescriptor();
         snprintf(d->externalId, sizeof(d->externalId), "%d", static_cast<int>(json_integer_value(ifid)));

         const char *name = json_object_get_string_utf8(iface, "name", nullptr);
         if (name == nullptr)
            name = json_object_get_string_utf8(iface, "ifname", d->externalId);
         strlcpy(d->name, name, sizeof(d->name));

         if (json_object_get_boolean(iface, "is_pcap_interface", false))
            strcpy(d->type, "pcap");
         else if (json_object_get_boolean(iface, "is_zmq_interface", false))
            strcpy(d->type, "zmq");
         else if (json_object_get_boolean(iface, "is_packet_interface", false))
            strcpy(d->type, "packet");
         else
            strcpy(d->type, "other");

         // Packet capture sees every frame; for flow-fed interfaces (ZMQ/other) the
         // effective sampling rate depends on the exporter and is not reported by ntopng
         d->samplingRate = (!strcmp(d->type, "packet") || !strcmp(d->type, "pcap")) ? 1 : 0;

         // ntopng reports only currently existing interfaces - all discovered points are active
         d->state = OBSERVATION_POINT_STATE_ACTIVE;
         strcpy(d->providerState, "active");

         d->localNetworks = GetLocalNetworks(credentials, static_cast<int32_t>(json_integer_value(ifid)));

         if (tail != nullptr)
            tail->next = d;
         else
            head = d;
         tail = d;

         nxlog_debug_tag(DEBUG_TAG, 6, L"NtopngDiscoverPoints: discovered interface %hs (%hs, type %hs)", d->externalId, d->name, d->type);
      }
   }

   json_decref(response);
   return head;
}
