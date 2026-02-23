/*
** NetXMS Docker cloud connector module
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
** File: discovery.cpp
**/

#include "docker.h"

/**
 * Map Docker container state string to RESOURCE_STATE_* value
 */
int16_t DockerContainerStateToResourceState(const char *dockerState)
{
   if (dockerState == nullptr)
      return RESOURCE_STATE_UNKNOWN;
   if (!strcmp(dockerState, "running"))
      return RESOURCE_STATE_ACTIVE;
   if (!strcmp(dockerState, "created"))
      return RESOURCE_STATE_CREATING;
   if (!strcmp(dockerState, "restarting"))
      return RESOURCE_STATE_DEGRADED;
   if (!strcmp(dockerState, "paused"))
      return RESOURCE_STATE_INACTIVE;
   if (!strcmp(dockerState, "exited"))
      return RESOURCE_STATE_INACTIVE;
   if (!strcmp(dockerState, "removing"))
      return RESOURCE_STATE_DELETING;
   if (!strcmp(dockerState, "dead"))
      return RESOURCE_STATE_FAILED;
   return RESOURCE_STATE_UNKNOWN;
}

/**
 * Extract tags from Docker Labels JSON object
 */
static StringMap *ExtractLabels(json_t *labels)
{
   if (!json_is_object(labels) || (json_object_size(labels) == 0))
      return nullptr;

   StringMap *tags = new StringMap();
   const char *key;
   json_t *value;
   json_object_foreach(labels, key, value)
   {
      if (json_is_string(value))
      {
         WCHAR wKey[256], wValue[1024];
         utf8_to_wchar(key, -1, wKey, 256);
         utf8_to_wchar(json_string_value(value), -1, wValue, 1024);
         tags->set(wKey, wValue);
      }
   }
   return tags;
}

/**
 * Discover containers
 */
static ResourceDescriptor *DiscoverContainers(DockerClient *client)
{
   json_t *response = client->httpGet("/containers/json?all=true");
   if (response == nullptr)
      return nullptr;

   if (!json_is_array(response))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DiscoverContainers: unexpected response type");
      json_decref(response);
      return nullptr;
   }

   ResourceDescriptor *head = nullptr;
   ResourceDescriptor *tail = nullptr;
   size_t count = json_array_size(response);

   for (size_t i = 0; i < count; i++)
   {
      json_t *container = json_array_get(response, i);
      const char *id = json_object_get_string_utf8(container, "Id", nullptr);
      if (id == nullptr)
         continue;

      ResourceDescriptor *rd = new ResourceDescriptor();

      // Resource ID = full container SHA
      utf8_to_wchar(id, -1, rd->resourceId, 1024);

      // Name = first entry from Names array, strip leading '/'
      json_t *names = json_object_get(container, "Names");
      if (json_is_array(names) && json_array_size(names) > 0)
      {
         const char *name = json_string_value(json_array_get(names, 0));
         if (name != nullptr)
         {
            if (name[0] == '/')
               name++;
            utf8_to_wchar(name, -1, rd->name, MAX_OBJECT_NAME);
         }
      }

      wcscpy(rd->type, L"docker.container");

      // State
      const char *state = json_object_get_string_utf8(container, "State", nullptr);
      rd->state = DockerContainerStateToResourceState(state);
      if (state != nullptr)
         strlcpy(rd->providerState, state, 256);

      // Tags from Labels
      rd->tags = ExtractLabels(json_object_get(container, "Labels"));

      // Link hint - container's IP from first network
      json_t *networkSettings = json_object_get(container, "NetworkSettings");
      if (json_is_object(networkSettings))
      {
         json_t *networks = json_object_get(networkSettings, "Networks");
         if (json_is_object(networks))
         {
            const char *netName;
            json_t *netData;
            json_object_foreach(networks, netName, netData)
            {
               const char *ip = json_object_get_string_utf8(netData, "IPAddress", nullptr);
               if (ip != nullptr && ip[0] != 0)
               {
                  utf8_to_wchar(ip, -1, rd->linkHint, 256);
                  break;
               }
            }
         }
      }

      // Metric hints
      rd->metricHints = new StringList();
      rd->metricHints->add(L"cpu_percent");
      rd->metricHints->add(L"memory_usage");
      rd->metricHints->add(L"memory_percent");

      // Append to linked list
      if (tail != nullptr)
         tail->next = rd;
      else
         head = rd;
      tail = rd;
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"DiscoverContainers: found %zu containers", count);
   json_decref(response);
   return head;
}

/**
 * Discover networks
 */
static ResourceDescriptor *DiscoverNetworks(DockerClient *client)
{
   json_t *response = client->httpGet("/networks");
   if (response == nullptr)
      return nullptr;

   if (!json_is_array(response))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DiscoverNetworks: unexpected response type");
      json_decref(response);
      return nullptr;
   }

   ResourceDescriptor *head = nullptr;
   ResourceDescriptor *tail = nullptr;
   size_t count = json_array_size(response);

   for (size_t i = 0; i < count; i++)
   {
      json_t *network = json_array_get(response, i);
      const char *id = json_object_get_string_utf8(network, "Id", nullptr);
      if (id == nullptr)
         continue;

      ResourceDescriptor *rd = new ResourceDescriptor();

      utf8_to_wchar(id, -1, rd->resourceId, 1024);

      const char *name = json_object_get_string_utf8(network, "Name", nullptr);
      if (name != nullptr)
         utf8_to_wchar(name, -1, rd->name, MAX_OBJECT_NAME);

      wcscpy(rd->type, L"docker.network");
      rd->state = RESOURCE_STATE_ACTIVE;

      const char *driver = json_object_get_string_utf8(network, "Driver", nullptr);
      if (driver != nullptr)
         strlcpy(rd->providerState, driver, 256);

      rd->tags = ExtractLabels(json_object_get(network, "Labels"));

      if (tail != nullptr)
         tail->next = rd;
      else
         head = rd;
      tail = rd;
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"DiscoverNetworks: found %zu networks", count);
   json_decref(response);
   return head;
}

/**
 * Discover volumes
 */
static ResourceDescriptor *DiscoverVolumes(DockerClient *client)
{
   json_t *response = client->httpGet("/volumes");
   if (response == nullptr)
      return nullptr;

   json_t *volumes = json_object_get(response, "Volumes");
   if (!json_is_array(volumes))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DiscoverVolumes: unexpected response structure");
      json_decref(response);
      return nullptr;
   }

   ResourceDescriptor *head = nullptr;
   ResourceDescriptor *tail = nullptr;
   size_t count = json_array_size(volumes);

   for (size_t i = 0; i < count; i++)
   {
      json_t *volume = json_array_get(volumes, i);
      const char *name = json_object_get_string_utf8(volume, "Name", nullptr);
      if (name == nullptr)
         continue;

      ResourceDescriptor *rd = new ResourceDescriptor();

      // Volume name is used as resource ID (volumes have no separate ID)
      utf8_to_wchar(name, -1, rd->resourceId, 1024);
      utf8_to_wchar(name, -1, rd->name, MAX_OBJECT_NAME);

      wcscpy(rd->type, L"docker.volume");
      rd->state = RESOURCE_STATE_ACTIVE;

      const char *driver = json_object_get_string_utf8(volume, "Driver", nullptr);
      if (driver != nullptr)
         strlcpy(rd->providerState, driver, 256);

      rd->tags = ExtractLabels(json_object_get(volume, "Labels"));

      if (tail != nullptr)
         tail->next = rd;
      else
         head = rd;
      tail = rd;
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"DiscoverVolumes: found %zu volumes", count);
   json_decref(response);
   return head;
}

/**
 * Docker Discover entry point - implements CloudConnectorInterface::Discover
 */
ResourceDescriptor *DockerDiscover(json_t *credentials, const TCHAR *filter)
{
   DockerClient client;
   if (!client.configure(credentials))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DockerDiscover: failed to configure client");
      return nullptr;
   }

   // Discover all three resource types and chain them into a single linked list
   ResourceDescriptor *containers = DiscoverContainers(&client);
   ResourceDescriptor *networks = DiscoverNetworks(&client);
   ResourceDescriptor *volumeList = DiscoverVolumes(&client);

   // Chain lists together
   ResourceDescriptor *head = containers;
   ResourceDescriptor *tail = nullptr;

   // Find tail of containers list
   if (head != nullptr)
   {
      tail = head;
      while (tail->next != nullptr)
         tail = tail->next;
   }

   // Append networks
   if (networks != nullptr)
   {
      if (tail != nullptr)
         tail->next = networks;
      else
         head = networks;
      tail = networks;
      while (tail->next != nullptr)
         tail = tail->next;
   }

   // Append volumes
   if (volumeList != nullptr)
   {
      if (tail != nullptr)
         tail->next = volumeList;
      else
         head = volumeList;
   }

   return head;
}
