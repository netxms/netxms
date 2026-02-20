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
** File: backups.cpp
**/

#include "oxidized.h"
#include <unordered_map>
#include <vector>
#include <string>

/**
 * Cached version metadata
 */
struct VersionInfo
{
   std::string oid;
   time_t timestamp;

   VersionInfo(const char *_oid, time_t _timestamp) : oid((_oid != nullptr) ? _oid : ""), timestamp(_timestamp) {}
};

/**
 * Per-node version cache: maps node ID to ordered list of version metadata.
 * Backup ID is the index into this vector.
 */
static std::unordered_map<uint32_t, std::vector<VersionInfo>> s_versionCache;
static Mutex s_versionCacheMutex;

/**
 * Get Oxidized node name and group from custom attributes.
 * Returns false if node is not registered.
 */
static bool GetOxidizedNodeInfo(const Node& node, char *name, size_t nameSize, char *group, size_t groupSize)
{
   SharedString model = node.getCustomAttribute(L"$oxidized.model");
   if (model.isEmpty())
      return false;

   snprintf(name, nameSize, "%u", node.getId());

   SharedString wgroup = node.getCustomAttribute(L"$oxidized.group");
   if (!wgroup.isEmpty())
      wchar_to_utf8(wgroup.cstr(), -1, group, groupSize);
   else
      group[0] = 0;

   return true;
}

/**
 * Build node_full string for version API (group/name or just name)
 */
static void BuildNodeFull(const char *name, const char *group, char *nodeFullOut, size_t size)
{
   if ((group[0] != 0) && strcmp(group, "default"))
      snprintf(nodeFullOut, size, "%s/%s", group, name);
   else
      strlcpy(nodeFullOut, name, size);
}

/**
 * Build URL path with optional group prefix
 */
static void BuildNodePath(const char *prefix, const char *name, const char *group, const char *suffix, char *pathOut, size_t size)
{
   if ((group[0] != 0) && strcmp(group, "default"))
      snprintf(pathOut, size, "%s/%s/%s%s", prefix, group, name, suffix);
   else
      snprintf(pathOut, size, "%s/%s%s", prefix, name, suffix);
}

/**
 * Parse Oxidized timestamp string to time_t.
 * Format: "YYYY-MM-DD HH:MM:SS +ZZZZ" or "YYYY-MM-DD HH:MM:SS UTC"
 */
static time_t ParseOxidizedTimestamp(const char *ts)
{
   if (ts == nullptr)
      return 0;

   struct tm tm;
   memset(&tm, 0, sizeof(tm));
   if (strptime(ts, "%Y-%m-%d %H:%M:%S", &tm) != nullptr)
      return timegm(&tm);
   return 0;
}

/**
 * Fetch version list from Oxidized and update OID cache for given node.
 * Returns the parsed JSON array (caller must json_decref), or nullptr on failure.
 */
static json_t *FetchAndCacheVersionList(uint32_t nodeId, const char *name, const char *group)
{
   char nodeFull[512], encodedNodeFull[512], endpoint[512];
   BuildNodeFull(name, group, nodeFull, sizeof(nodeFull));
   URLEncode(nodeFull, encodedNodeFull, sizeof(encodedNodeFull));
   snprintf(endpoint, sizeof(endpoint), "node/version.json?node_full=%s", encodedNodeFull);

   json_t *versions = OxidizedApiRequest(endpoint);
   if (versions == nullptr || !json_is_array(versions))
   {
      if (versions != nullptr)
         json_decref(versions);
      return nullptr;
   }

   // Build version info list and update cache
   std::vector<VersionInfo> versionInfos;
   size_t index;
   json_t *entry;
   json_array_foreach(versions, index, entry)
   {
      const char *oid = json_object_get_string_utf8(entry, "oid", nullptr);
      time_t timestamp = ParseOxidizedTimestamp(json_object_get_string_utf8(entry, "time", nullptr));
      versionInfos.push_back(VersionInfo(oid, timestamp));
   }

   {
      LockGuard lg(s_versionCacheMutex);
      s_versionCache[nodeId] = std::move(versionInfos);
   }

   return versions;
}

/**
 * Look up cached version info by node ID and backup index.
 * Returns entry with empty OID if not found.
 */
static VersionInfo GetCachedVersion(uint32_t nodeId, int64_t backupId)
{
   LockGuard lg(s_versionCacheMutex);
   auto it = s_versionCache.find(nodeId);
   if (it == s_versionCache.end())
      return VersionInfo(nullptr, 0);
   if (backupId < 0 || static_cast<size_t>(backupId) >= it->second.size())
      return VersionInfo(nullptr, 0);
   return it->second[static_cast<size_t>(backupId)];
}

/**
 * Get status of last backup job
 */
std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo> OxidizedGetLastJobStatus(const Node& node)
{
   char name[256], group[128];
   if (!GetOxidizedNodeInfo(node, name, sizeof(name), group, sizeof(group)))
      return std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, DeviceBackupJobInfo());

   json_t *nodeList = GetCachedNodeList();
   if (nodeList == nullptr)
      return std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::EXTERNAL_API_ERROR, DeviceBackupJobInfo());

   json_t *entry = FindOxidizedNodeByName(nodeList, name);
   if (entry == nullptr)
   {
      json_decref(nodeList);
      return std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, DeviceBackupJobInfo());
   }

   DeviceBackupJobInfo info;

   // Get last backup status from the "last" sub-object
   json_t *last = json_object_get(entry, "last");
   const char *status = nullptr;
   if (last != nullptr)
      status = json_object_get_string_utf8(last, "status", nullptr);

   // Also check top-level status
   if (status == nullptr)
      status = json_object_get_string_utf8(entry, "status", nullptr);

   if (status != nullptr)
   {
      if (!stricmp(status, "success"))
         info.status = DeviceBackupJobStatus::SUCCESSFUL;
      else if (!stricmp(status, "no_connection") || !stricmp(status, "timeout"))
         info.status = DeviceBackupJobStatus::FAILED;
      else if (!stricmp(status, "never"))
         info.status = DeviceBackupJobStatus::UNKNOWN;
      else
         info.status = DeviceBackupJobStatus::UNKNOWN;

      // Set failure message
      if (info.status == DeviceBackupJobStatus::FAILED)
      {
         WCHAR wstatus[256];
         utf8_to_wchar(status, -1, wstatus, 256);
         wcsncpy(info.message, wstatus, 255);
         info.message[255] = 0;
      }
   }

   // Parse timestamp from last.end
   if (last != nullptr)
   {
      const char *endTime = json_object_get_string_utf8(last, "end", nullptr);
      if (endTime != nullptr)
         info.timestamp = ParseOxidizedTimestamp(endTime);
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedGetLastJobStatus(%s [%u]): status=%hs",
      node.getName(), node.getId(), (status != nullptr) ? status : "unknown");

   json_decref(nodeList);
   return std::pair<DeviceBackupApiStatus, DeviceBackupJobInfo>(DeviceBackupApiStatus::SUCCESS, info);
}

/**
 * Initiate on-demand backup
 */
DeviceBackupApiStatus OxidizedStartJob(const Node& node)
{
   char name[256], group[128];
   if (!GetOxidizedNodeInfo(node, name, sizeof(name), group, sizeof(group)))
      return DeviceBackupApiStatus::DEVICE_NOT_REGISTERED;

   char endpoint[512];
   BuildNodePath("node/next", name, group, "", endpoint, sizeof(endpoint));

   char *text = OxidizedApiRequestText(endpoint);
   if (text != nullptr)
   {
      MemFree(text);
      nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedStartJob(%s [%u]): backup triggered", node.getName(), node.getId());
      InvalidateNodeCache();
      return DeviceBackupApiStatus::SUCCESS;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"OxidizedStartJob(%s [%u]): failed to trigger backup",
      node.getName(), node.getId());
   return DeviceBackupApiStatus::EXTERNAL_API_ERROR;
}

/**
 * Get latest backup for given device
 */
std::pair<DeviceBackupApiStatus, BackupData> OxidizedGetLatestBackup(const Node& node)
{
   char name[256], group[128];
   if (!GetOxidizedNodeInfo(node, name, sizeof(name), group, sizeof(group)))
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, BackupData());

   // Fetch configuration content
   char fetchEndpoint[512];
   BuildNodePath("node/fetch", name, group, "", fetchEndpoint, sizeof(fetchEndpoint));

   size_t configSize = 0;
   char *configText = OxidizedApiRequestText(fetchEndpoint, &configSize);
   if (configText == nullptr)
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::EXTERNAL_API_ERROR, BackupData());

   BackupData backup;
   backup.id = 0;  // Latest is always index 0
   backup.isBinary = false;

   // Fetch version metadata and update cache
   json_t *versions = FetchAndCacheVersionList(node.getId(), name, group);
   if ((versions != nullptr) && (json_array_size(versions) > 0))
   {
      json_t *latest = json_array_get(versions, 0);
      backup.timestamp = ParseOxidizedTimestamp(json_object_get_string_utf8(latest, "time", nullptr));
      backup.lastCheckTime = backup.timestamp;
   }
   if (versions != nullptr)
      json_decref(versions);

   // Set running config
   backup.runningConfig = reinterpret_cast<BYTE*>(configText);
   backup.runningConfigSize = configSize;

   // Compute SHA-256 hash
   CalculateSHA256Hash(backup.runningConfig, backup.runningConfigSize, backup.runningConfigHash);

   // startupConfig stays nullptr (Oxidized only provides one config)

   return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::SUCCESS, std::move(backup));
}

/**
 * Get list of backups for given device (metadata only)
 */
std::pair<DeviceBackupApiStatus, std::vector<BackupData>> OxidizedGetBackupList(const Node& node)
{
   char name[256], group[128];
   if (!GetOxidizedNodeInfo(node, name, sizeof(name), group, sizeof(group)))
      return std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, std::vector<BackupData>());

   json_t *versions = FetchAndCacheVersionList(node.getId(), name, group);
   if (versions == nullptr)
      return std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::EXTERNAL_API_ERROR, std::vector<BackupData>());

   std::vector<BackupData> backups;
   size_t index;
   json_t *entry;
   json_array_foreach(versions, index, entry)
   {
      BackupData backup;
      backup.id = static_cast<int64_t>(index);
      backup.timestamp = ParseOxidizedTimestamp(json_object_get_string_utf8(entry, "time", nullptr));
      backup.lastCheckTime = backup.timestamp;
      backup.isBinary = false;
      backups.push_back(std::move(backup));
   }

   json_decref(versions);

   nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedGetBackupList(%s [%u]): returned %zu versions",
      node.getName(), node.getId(), backups.size());

   return std::pair<DeviceBackupApiStatus, std::vector<BackupData>>(DeviceBackupApiStatus::SUCCESS, std::move(backups));
}

/**
 * Get backup by ID (with full content)
 */
std::pair<DeviceBackupApiStatus, BackupData> OxidizedGetBackupById(const Node& node, int64_t id)
{
   char name[256], group[128];
   if (!GetOxidizedNodeInfo(node, name, sizeof(name), group, sizeof(group)))
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::DEVICE_NOT_REGISTERED, BackupData());

   // Look up version info from cache
   VersionInfo vi = GetCachedVersion(node.getId(), id);
   if (vi.oid.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"OxidizedGetBackupById(%s [%u], %lld): OID not found in cache",
         node.getName(), node.getId(), static_cast<long long>(id));
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::EXTERNAL_API_ERROR, BackupData());
   }

   // Fetch config for specific version using version view endpoint
   char encodedName[512], encodedGroup[256];
   URLEncode(name, encodedName, sizeof(encodedName));
   URLEncode(group, encodedGroup, sizeof(encodedGroup));
   char fetchEndpoint[1024];
   snprintf(fetchEndpoint, sizeof(fetchEndpoint), "node/version/view.json?node=%s&group=%s&oid=%s",
      encodedName, encodedGroup, vi.oid.c_str());

   json_t *lines = OxidizedApiRequest(fetchEndpoint);
   if (lines == nullptr || !json_is_array(lines))
   {
      if (lines != nullptr)
         json_decref(lines);
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::EXTERNAL_API_ERROR, BackupData());
   }

   // Join JSON array of lines into a single string
   ByteStream configStream(32768);
   size_t index;
   json_t *line;
   json_array_foreach(lines, index, line)
   {
      const char *lineStr = json_string_value(line);
      if (lineStr != nullptr)
         configStream.write(lineStr, strlen(lineStr));
   }
   json_decref(lines);

   size_t configSize = configStream.size();
   if (configSize == 0)
      return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::EXTERNAL_API_ERROR, BackupData());

   configStream.write('\0');

   BackupData backup;
   backup.id = id;
   backup.timestamp = vi.timestamp;
   backup.lastCheckTime = vi.timestamp;
   backup.isBinary = false;
   backup.runningConfig = configStream.takeBuffer();
   backup.runningConfigSize = configSize;
   CalculateSHA256Hash(backup.runningConfig, backup.runningConfigSize, backup.runningConfigHash);

   nxlog_debug_tag(DEBUG_TAG, 5, L"OxidizedGetBackupById(%s [%u], %lld): config retrieved (%zu bytes)",
      node.getName(), node.getId(), static_cast<long long>(id), configSize);

   return std::pair<DeviceBackupApiStatus, BackupData>(DeviceBackupApiStatus::SUCCESS, std::move(backup));
}
