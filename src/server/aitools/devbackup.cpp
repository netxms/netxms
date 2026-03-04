/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: devbackup.cpp
**
**/

#include "aitools.h"
#include <device-backup.h>

/**
 * Maximum config size to include in response (64KB)
 */
#define MAX_CONFIG_SIZE 65536

/**
 * Find node by name or ID and check access rights
 */
static shared_ptr<Node> FindAndValidateNode(const char *objectName, uint32_t userId, uint32_t accessRights, std::string *errorMessage)
{
   if ((objectName == nullptr) || (objectName[0] == 0))
   {
      *errorMessage = "Object name or ID must be provided";
      return shared_ptr<Node>();
   }

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectByNameOrId(objectName, OBJECT_NODE));
   if (node == nullptr)
   {
      char buffer[256];
      snprintf(buffer, 256, "Node with name or ID \"%s\" is not known", objectName);
      *errorMessage = buffer;
      return shared_ptr<Node>();
   }

   if (!node->checkAccessRights(userId, accessRights))
   {
      char buffer[256];
      snprintf(buffer, 256, "Insufficient rights to access node \"%s\"", objectName);
      *errorMessage = buffer;
      return shared_ptr<Node>();
   }

   return node;
}

/**
 * Get backup job status name
 */
static const char *GetJobStatusName(DeviceBackupJobStatus status)
{
   switch(status)
   {
      case DeviceBackupJobStatus::SUCCESSFUL:
         return "SUCCESSFUL";
      case DeviceBackupJobStatus::FAILED:
         return "FAILED";
      default:
         return "UNKNOWN";
   }
}

/**
 * Format SHA-256 hash as hex string
 */
static void FormatHash(const BYTE *hash, char *buffer)
{
   BinToStrA(hash, SHA256_DIGEST_SIZE, buffer);
}

/**
 * Add config content to JSON object, handling binary and size limits
 */
static void AddConfigToJson(json_t *json, const char *fieldName, const BYTE *config, size_t configSize, bool isBinary)
{
   if (config == nullptr || configSize == 0)
   {
      json_object_set_new(json, fieldName, json_null());
      return;
   }

   if (isBinary)
   {
      size_t encodedSize = (configSize * 4 / 3) + 4;
      if (encodedSize > MAX_CONFIG_SIZE)
      {
         char note[256];
         snprintf(note, 256, "[Binary config truncated - original size: %u bytes]", static_cast<unsigned int>(configSize));
         json_object_set_new(json, fieldName, json_string(note));
         return;
      }
      char *encoded = static_cast<char*>(MemAlloc(encodedSize + 1));
      base64_encode(reinterpret_cast<const char*>(config), configSize, encoded, encodedSize + 1);
      json_object_set_new(json, fieldName, json_string(encoded));
      MemFree(encoded);
   }
   else
   {
      if (configSize > MAX_CONFIG_SIZE)
      {
         char *truncated = static_cast<char*>(MemAlloc(MAX_CONFIG_SIZE + 128));
         memcpy(truncated, config, MAX_CONFIG_SIZE);
         truncated[MAX_CONFIG_SIZE] = 0;
         snprintf(truncated + strlen(truncated), 128, "\n\n[Config truncated - original size: %u bytes]", static_cast<unsigned int>(configSize));
         json_object_set_new(json, fieldName, json_string(truncated));
         MemFree(truncated);
      }
      else
      {
         // Config may not be null-terminated, make a copy
         char *text = static_cast<char*>(MemAlloc(configSize + 1));
         memcpy(text, config, configSize);
         text[configSize] = 0;
         json_object_set_new(json, fieldName, json_string(text));
         MemFree(text);
      }
   }
}

/**
 * Get device backup registration status and last job info
 */
std::string F_GetBackupStatus(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Node> node = FindAndValidateNode(json_object_get_string_utf8(arguments, "object", nullptr), userId, OBJECT_ACCESS_READ, &error);
   if (node == nullptr)
      return error;

   bool registered = DevBackupIsDeviceRegistered(*node);

   json_t *output = json_object();
   json_object_set_new(output, "registered", json_boolean(registered));

   if (registered)
   {
      auto jobResult = DevBackupGetLastJobStatus(*node);
      if (jobResult.first == DeviceBackupApiStatus::SUCCESS)
      {
         json_t *lastJob = json_object();
         json_object_set_new(lastJob, "status", json_string(GetJobStatusName(jobResult.second.status)));
         json_object_set_new(lastJob, "timestamp", json_time_string(jobResult.second.timestamp));
         char msg[256];
         wchar_to_utf8(jobResult.second.message, -1, msg, 256);
         json_object_set_new(lastJob, "message", json_string(msg));
         json_object_set_new(output, "lastJob", lastJob);
      }
      else
      {
         char msg[256];
         wchar_to_utf8(GetDeviceBackupApiErrorMessage(jobResult.first), -1, msg, 256);
         json_object_set_new(output, "lastJobError", json_string(msg));
      }
   }

   return JsonToString(output);
}

/**
 * Get list of backup metadata for a node
 */
std::string F_GetBackupList(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Node> node = FindAndValidateNode(json_object_get_string_utf8(arguments, "object", nullptr), userId, OBJECT_ACCESS_READ, &error);
   if (node == nullptr)
      return error;

   auto result = DevBackupGetBackupList(*node);
   if (result.first != DeviceBackupApiStatus::SUCCESS)
   {
      char msg[256];
      wchar_to_utf8(GetDeviceBackupApiErrorMessage(result.first), -1, msg, 256);
      return std::string(msg);
   }

   json_t *output = json_array();
   char hashStr[SHA256_DIGEST_SIZE * 2 + 1];
   for (const auto& b : result.second)
   {
      json_t *entry = json_object();
      json_object_set_new(entry, "id", json_integer(b.id));
      json_object_set_new(entry, "timestamp", json_time_string(b.timestamp));
      json_object_set_new(entry, "runningConfigSize", json_integer(static_cast<json_int_t>(b.runningConfigSize)));
      json_object_set_new(entry, "startupConfigSize", json_integer(static_cast<json_int_t>(b.startupConfigSize)));
      FormatHash(b.runningConfigHash, hashStr);
      json_object_set_new(entry, "runningConfigHash", json_string(hashStr));
      FormatHash(b.startupConfigHash, hashStr);
      json_object_set_new(entry, "startupConfigHash", json_string(hashStr));
      json_object_set_new(entry, "isBinary", json_boolean(b.isBinary));
      json_array_append_new(output, entry);
   }

   return JsonToString(output);
}

/**
 * Get backup content for a specific backup or latest
 */
std::string F_GetBackupContent(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Node> node = FindAndValidateNode(json_object_get_string_utf8(arguments, "object", nullptr), userId, OBJECT_ACCESS_READ, &error);
   if (node == nullptr)
      return error;

   int64_t backupId = json_object_get_int64(arguments, "backupId", 0);

   std::pair<DeviceBackupApiStatus, BackupData> result;
   if (backupId > 0)
      result = DevBackupGetBackupById(*node, backupId);
   else
      result = DevBackupGetLatestBackup(*node);

   if (result.first != DeviceBackupApiStatus::SUCCESS)
   {
      char msg[256];
      wchar_to_utf8(GetDeviceBackupApiErrorMessage(result.first), -1, msg, 256);
      return std::string(msg);
   }

   const BackupData& b = result.second;
   json_t *output = json_object();
   json_object_set_new(output, "id", json_integer(b.id));
   json_object_set_new(output, "timestamp", json_time_string(b.timestamp));
   json_object_set_new(output, "isBinary", json_boolean(b.isBinary));
   AddConfigToJson(output, "runningConfig", b.runningConfig, b.runningConfigSize, b.isBinary);
   AddConfigToJson(output, "startupConfig", b.startupConfig, b.startupConfigSize, b.isBinary);

   return JsonToString(output);
}

/**
 * Start immediate device configuration backup job
 */
std::string F_StartBackup(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Node> node = FindAndValidateNode(json_object_get_string_utf8(arguments, "object", nullptr), userId, OBJECT_ACCESS_CONTROL, &error);
   if (node == nullptr)
      return error;

   if (!DevBackupIsDeviceRegistered(*node))
      return std::string("Device is not registered for configuration backup");

   DeviceBackupApiStatus status = DevBackupStartJob(*node);
   if (status != DeviceBackupApiStatus::SUCCESS)
   {
      char msg[256];
      wchar_to_utf8(GetDeviceBackupApiErrorMessage(status), -1, msg, 256);
      return std::string(msg);
   }

   return std::string("Configuration backup job started successfully");
}

/**
 * Compare two backup configs for diff analysis
 */
std::string F_CompareBackups(json_t *arguments, uint32_t userId)
{
   std::string error;
   shared_ptr<Node> node = FindAndValidateNode(json_object_get_string_utf8(arguments, "object", nullptr), userId, OBJECT_ACCESS_READ, &error);
   if (node == nullptr)
      return error;

   int64_t backupId1 = json_object_get_int64(arguments, "backupId1", 0);
   int64_t backupId2 = json_object_get_int64(arguments, "backupId2", 0);
   if (backupId1 <= 0 || backupId2 <= 0)
      return std::string("Both backupId1 and backupId2 must be provided");

   const char *configType = json_object_get_string_utf8(arguments, "configType", "running");

   auto result1 = DevBackupGetBackupById(*node, backupId1);
   if (result1.first != DeviceBackupApiStatus::SUCCESS)
   {
      char msg[256];
      snprintf(msg, 256, "Failed to get backup %lld: ", static_cast<long long>(backupId1));
      char errMsg[256];
      wchar_to_utf8(GetDeviceBackupApiErrorMessage(result1.first), -1, errMsg, 256);
      return std::string(msg) + errMsg;
   }

   auto result2 = DevBackupGetBackupById(*node, backupId2);
   if (result2.first != DeviceBackupApiStatus::SUCCESS)
   {
      char msg[256];
      snprintf(msg, 256, "Failed to get backup %lld: ", static_cast<long long>(backupId2));
      char errMsg[256];
      wchar_to_utf8(GetDeviceBackupApiErrorMessage(result2.first), -1, errMsg, 256);
      return std::string(msg) + errMsg;
   }

   bool useStartup = (strcmp(configType, "startup") == 0);

   json_t *output = json_object();
   json_object_set_new(output, "configType", json_string(configType));

   json_t *b1 = json_object();
   json_object_set_new(b1, "id", json_integer(result1.second.id));
   json_object_set_new(b1, "timestamp", json_time_string(result1.second.timestamp));
   if (useStartup)
      AddConfigToJson(b1, "config", result1.second.startupConfig, result1.second.startupConfigSize, result1.second.isBinary);
   else
      AddConfigToJson(b1, "config", result1.second.runningConfig, result1.second.runningConfigSize, result1.second.isBinary);
   json_object_set_new(output, "backup1", b1);

   json_t *b2 = json_object();
   json_object_set_new(b2, "id", json_integer(result2.second.id));
   json_object_set_new(b2, "timestamp", json_time_string(result2.second.timestamp));
   if (useStartup)
      AddConfigToJson(b2, "config", result2.second.startupConfig, result2.second.startupConfigSize, result2.second.isBinary);
   else
      AddConfigToJson(b2, "config", result2.second.runningConfig, result2.second.runningConfigSize, result2.second.isBinary);
   json_object_set_new(output, "backup2", b2);

   return JsonToString(output);
}
