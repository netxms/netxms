/*
** NetXMS Oxidized integration module
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
** File: devices.cpp
**/

#include "oxidized.h"
#include <nms_objects.h>

/**
 * Tell Oxidized to reload its node list from the HTTP source
 */
static void ReloadOxidizedNodeList()
{
   json_t *response = OxidizedApiRequest("reload.json");
   if (response != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"Oxidized node list reload triggered");
      json_decref(response);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"Failed to trigger Oxidized node list reload");
   }
   InvalidateNodeCache();
}

/**
 * Check if device is registered in Oxidized
 */
bool OxidizedIsDeviceRegistered(const Node& node)
{
   return !node.getCustomAttribute(L"$oxidized.model").isEmpty();
}

/**
 * Validate device registration
 */
DeviceBackupApiStatus OxidizedValidateDeviceRegistration(Node *node)
{
   SharedString model = node->getCustomAttribute(L"$oxidized.model");
   if (model.isEmpty())
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;

   json_t *nodeList = GetCachedNodeList();
   if (nodeList == nullptr)
      return DeviceBackupApiStatus::EXTERNAL_API_ERROR;

   char nodeIdText[16];
   snprintf(nodeIdText, sizeof(nodeIdText), "%u", node->getId());

   json_t *oxidizedNode = FindOxidizedNodeByName(nodeList, nodeIdText);
   if (oxidizedNode == nullptr)
   {
      json_decref(nodeList);
      nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedValidateDeviceRegistration(%s [%u]): device not found in Oxidized, clearing registration",
         node->getName(), node->getId());
      node->deleteCustomAttribute(L"$oxidized.model");
      node->deleteCustomAttribute(L"$oxidized.group");
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;
   }

   // Update model from vendor mapping if it changed
   SharedString vendor = node->getVendor();
   std::string resolvedModel = ResolveOxidizedModel(vendor.cstr());
   if (!resolvedModel.empty())
   {
      WCHAR wmodel[64];
      utf8_to_wchar(resolvedModel.c_str(), -1, wmodel, 64);
      if (wcscmp(model.cstr(), wmodel))
      {
         node->setCustomAttribute(L"$oxidized.model", wmodel, StateChange::IGNORE);
         nxlog_debug_tag(DEBUG_TAG, 4, L"OxidizedValidateDeviceRegistration(%s [%u]): updated model to %hs (vendor: %s)",
            node->getName(), node->getId(), resolvedModel.c_str(), vendor.cstr());
         ReloadOxidizedNodeList();
      }
   }

   json_decref(nodeList);
   nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedValidateDeviceRegistration(%s [%u]): device found in Oxidized", node->getName(), node->getId());
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Register device in Oxidized
 */
DeviceBackupApiStatus OxidizedRegisterDevice(Node *node)
{
   SharedString vendor = node->getVendor();
   std::string model = ResolveOxidizedModel(vendor.cstr());
   if (model.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"OxidizedRegisterDevice(%s [%u]): cannot register - no model mapping for vendor \"%s\" and no DefaultModel configured",
         node->getName(), node->getId(), vendor.cstr());
      return DeviceBackupApiStatus::EXTERNAL_API_ERROR;
   }

   WCHAR wmodel[64];
   utf8_to_wchar(model.c_str(), -1, wmodel, 64);

   SharedString currentModel = node->getCustomAttribute(L"$oxidized.model");
   if (!wcscmp(currentModel.cstr(), wmodel))
      return DeviceBackupApiStatus::SUCCESS;

   bool isNew = currentModel.isEmpty();
   node->setCustomAttribute(L"$oxidized.model", wmodel, StateChange::IGNORE);

   if (isNew)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"OxidizedRegisterDevice(%s [%u]): registered with model %hs (vendor: %s)",
         node->getName(), node->getId(), model.c_str(), vendor.cstr());
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"OxidizedRegisterDevice(%s [%u]): updated model to %hs (vendor: %s)",
         node->getName(), node->getId(), model.c_str(), vendor.cstr());
   }

   ReloadOxidizedNodeList();
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * Delete device from Oxidized
 */
DeviceBackupApiStatus OxidizedDeleteDevice(Node *node)
{
   if (node->getCustomAttribute(L"$oxidized.model").isEmpty())
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;

   node->deleteCustomAttribute(L"$oxidized.model");
   node->deleteCustomAttribute(L"$oxidized.group");

   nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedDeleteDevice(%s [%u]): device unregistered", node->getName(), node->getId());

   ReloadOxidizedNodeList();
   return DeviceBackupApiStatus::SUCCESS;
}

/**
 * WebAPI handler: GET /oxidized/nodes
 * Returns JSON array of registered nodes for Oxidized HTTP source
 */
int H_OxidizedNodes(Context *context)
{
   json_t *nodes = json_array();

   g_idxNodeById.forEach([nodes](NetObj *obj) -> EnumerationCallbackResult {
      Node *node = static_cast<Node*>(obj);
      SharedString model = node->getCustomAttribute(L"$oxidized.model");
      if (!model.isEmpty())
      {
         json_t *entry = json_object();

         char nodeId[16];
         snprintf(nodeId, sizeof(nodeId), "%u", node->getId());
         json_object_set_new(entry, "name", json_string(nodeId));

         char ip[64];
         node->getPrimaryIpAddress().toStringA(ip);
         json_object_set_new(entry, "ip", json_string(ip));

         char modelUtf8[64];
         wchar_to_utf8(model.cstr(), -1, modelUtf8, sizeof(modelUtf8));
         json_object_set_new(entry, "model", json_string(modelUtf8));

         SharedString group = node->getCustomAttribute(L"$oxidized.group");
         if (!group.isEmpty())
         {
            char groupUtf8[128];
            wchar_to_utf8(group.cstr(), -1, groupUtf8, sizeof(groupUtf8));
            json_object_set_new(entry, "group", json_string(groupUtf8));
         }

         SharedString sshLogin = node->getSshLogin();
         if (!sshLogin.isEmpty())
         {
            char loginUtf8[128];
            wchar_to_utf8(sshLogin.cstr(), -1, loginUtf8, sizeof(loginUtf8));
            json_object_set_new(entry, "username", json_string(loginUtf8));
         }

         SharedString sshPassword = node->getSshPassword();
         if (!sshPassword.isEmpty())
         {
            char passwordUtf8[128];
            wchar_to_utf8(sshPassword.cstr(), -1, passwordUtf8, sizeof(passwordUtf8));
            json_object_set_new(entry, "password", json_string(passwordUtf8));
         }

         json_array_append_new(nodes, entry);
      }
      return _CONTINUE;
   });

   context->setResponseData(nodes);
   json_decref(nodes);
   return 200;
}
