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
** File: device_backup.cpp
**
**/

#include "webapi.h"
#include <device-backup.h>
#include <nxtask.h>

/**
 * Load node by URL placeholder "object-id". On failure returns nullptr and writes HTTP status code to *httpCode.
 */
static shared_ptr<Node> LoadNode(Context *context, int *httpCode)
{
   uint32_t objectId = context->getPlaceholderValueAsUInt32(_T("object-id"));
   if (objectId == 0)
   {
      context->setErrorResponse("Invalid object ID");
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
   {
      *httpCode = 404;
      return shared_ptr<Node>();
   }

   if (object->getObjectClass() != OBJECT_NODE)
   {
      context->setErrorResponse("Object is not a node");
      *httpCode = 400;
      return shared_ptr<Node>();
   }

   return static_pointer_cast<Node>(object);
}

/**
 * Convert backup provider status to HTTP response. Provider failures are reported as 500 with reason text.
 */
static int BackupApiStatusToHttpCode(Context *context, DeviceBackupApiStatus status)
{
   if (status == DeviceBackupApiStatus::SUCCESS)
      return 200;
   context->setErrorResponse(GetDeviceBackupApiErrorMessage(status));
   return (status == DeviceBackupApiStatus::NOT_IMPLEMENTED) ? 501 : 500;
}

/**
 * Serialize backup metadata (common part of list and single backup representations)
 */
static json_t *BackupMetadataToJson(const BackupData& backup)
{
   json_t *json = json_object();
   json_object_set_new(json, "id", json_integer(backup.id));
   json_object_set_new(json, "timestamp", json_time_string(backup.timestamp));
   json_object_set_new(json, "lastCheckTime", json_time_string(backup.lastCheckTime));
   json_object_set_new(json, "isBinary", json_boolean(backup.isBinary));

   char hashText[SHA256_DIGEST_SIZE * 2 + 1];
   json_object_set_new(json, "runningConfigSize", json_integer(static_cast<json_int_t>(backup.runningConfigSize)));
   json_object_set_new(json, "runningConfigHash", json_string(BinToStrA(backup.runningConfigHash, SHA256_DIGEST_SIZE, hashText)));
   json_object_set_new(json, "startupConfigSize", json_integer(static_cast<json_int_t>(backup.startupConfigSize)));
   json_object_set_new(json, "startupConfigHash", json_string(BinToStrA(backup.startupConfigHash, SHA256_DIGEST_SIZE, hashText)));
   return json;
}

/**
 * Serialize configuration content: UTF-8 text as JSON string, binary configuration as base64 string
 */
static json_t *ConfigContentToJson(const BYTE *config, size_t size, bool isBinary)
{
   if ((config == nullptr) || (size == 0))
      return json_null();
   if (isBinary)
      return json_base64_string(config, size);
   return json_stringn(reinterpret_cast<const char*>(config), size);
}

/**
 * Handler for GET /v1/objects/:object-id/device-config-backups
 */
int H_DeviceConfigBackups(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNode(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   if (!node->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ))
      return 403;

   auto result = DevBackupGetBackupList(*node);
   if (result.first != DeviceBackupApiStatus::SUCCESS)
      return BackupApiStatusToHttpCode(context, result.first);

   json_t *output = json_array();
   for(const BackupData& backup : result.second)
      json_array_append_new(output, BackupMetadataToJson(backup));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for GET /v1/objects/:object-id/device-config-backups/:backup-id
 */
int H_DeviceConfigBackupDetails(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNode(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   if (!node->checkAccessRights(context->getUserId(), OBJECT_ACCESS_READ_DEVICE_CONFIG))
   {
      context->writeAuditLog(AUDIT_OBJECTS, false, node->getId(), L"Access denied on reading device configuration backup");
      return 403;
   }

   int64_t backupId = wcstoll(context->getPlaceholderValue(_T("backup-id")), nullptr, 10);
   if (backupId <= 0)
   {
      context->setErrorResponse("Invalid backup ID");
      return 400;
   }

   auto result = DevBackupGetBackupById(*node, backupId);
   if (result.first != DeviceBackupApiStatus::SUCCESS)
      return BackupApiStatusToHttpCode(context, result.first);

   json_t *output = BackupMetadataToJson(result.second);
   json_object_set_new(output, "runningConfig", ConfigContentToJson(result.second.runningConfig, result.second.runningConfigSize, result.second.isBinary));
   json_object_set_new(output, "startupConfig", ConfigContentToJson(result.second.startupConfig, result.second.startupConfigSize, result.second.isBinary));
   context->setResponseData(output);
   json_decref(output);
   return 200;
}

/**
 * Handler for POST /v1/objects/:object-id/restore-device-config
 *
 * Request body selects configuration source: either a stored backup ("sourceObjectId" + "backupId",
 * optional "useStartupConfig" and "force") or client-supplied text ("config").
 */
int H_DeviceConfigRestore(Context *context)
{
   int httpCode;
   shared_ptr<Node> node = LoadNode(context, &httpCode);
   if (node == nullptr)
      return httpCode;

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      context->setErrorResponse("Request body is missing");
      return 400;
   }

   DeviceConfigRestoreStatus status;
   shared_ptr<BackgroundTask> task;
   if (json_object_get(request, "sourceObjectId") != nullptr)
   {
      shared_ptr<NetObj> sourceObject = FindObjectById(json_object_get_uint32(request, "sourceObjectId"));
      if ((sourceObject == nullptr) || (sourceObject->getObjectClass() != OBJECT_NODE))
      {
         context->setErrorResponse("Invalid source object ID");
         return 400;
      }
      status = StartDeviceConfigRestoreFromBackup(*context, node, static_pointer_cast<Node>(sourceObject),
            json_object_get_int64(request, "backupId"), json_object_get_boolean(request, "useStartupConfig"),
            json_object_get_boolean(request, "force"), &task);
   }
   else
   {
      const char *config = json_object_get_string_utf8(request, "config", nullptr);
      if (config == nullptr)
      {
         context->setErrorResponse("Either sourceObjectId or config must be provided");
         return 400;
      }
      status = StartDeviceConfigRestoreFromText(*context, node, reinterpret_cast<const BYTE*>(config), strlen(config), &task);
   }

   switch(status)
   {
      case DeviceConfigRestoreStatus::SUCCESS:
      {
         json_t *output = json_object();
         json_object_set_new(output, "taskId", json_integer(static_cast<json_int_t>(task->getId())));
         context->setResponseData(output);
         json_decref(output);
         return 202;
      }
      case DeviceConfigRestoreStatus::ACCESS_DENIED:
         return 403;
      case DeviceConfigRestoreStatus::NOT_SUPPORTED:
         context->setErrorResponse("Configuration restore is not supported by device driver of target node");
         return 400;
      case DeviceConfigRestoreStatus::NO_TRANSPORT:
         context->setErrorResponse("Neither interactive SSH nor NETCONF is available on target node");
         return 400;
      case DeviceConfigRestoreStatus::DRIVER_MISMATCH:
         context->setErrorResponse("Device driver of target node differs from device driver of source node (set \"force\" to apply anyway)");
         return 409;
      case DeviceConfigRestoreStatus::BACKUP_NOT_SUPPORTED:
         context->setErrorResponse("Backup provider does not support backup retrieval");
         return 501;
      case DeviceConfigRestoreStatus::BACKUP_UNAVAILABLE:
         context->setErrorResponse("Cannot retrieve requested backup");
         return 404;
      case DeviceConfigRestoreStatus::BINARY_BACKUP:
         context->setErrorResponse("Binary configuration backup cannot be restored");
         return 400;
      case DeviceConfigRestoreStatus::EMPTY_CONFIG:
         context->setErrorResponse("Configuration is empty");
         return 400;
      default:
         return 500;
   }
}
