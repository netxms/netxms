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
** File: dataexport.cpp
**
** Export of DCI history for external analysis: tagged DCIs under a root object (or an explicit DCI list) over a
** time period, written as a ZIP archive with a metric catalog, sample rows and a manifest. Optional anonymization
** replaces identities with stable pseudonyms.
**/

#include "nxcore.h"
#include <netxms-version.h>
#include <nxtask.h>
#include <nxzip.h>
#include <nxcrypto.h>
#include <nxstat.h>
#include <cmath>
#include <algorithm>

#define DEBUG_TAG L"dataexport"

#define DDIR_EXPORT              FS_PATH_SEPARATOR L"export"
#define EXPORT_FORMAT_VERSION    1
#define RAW_READ_CHUNK_MS        (31LL * 86400LL * 1000LL)     // History is read in one-month chunks to bound memory use
#define EXPORT_RETENTION_MS      3600000                       // Undownloaded export files and failed export state are forgotten after one hour
#define MAX_TAG_PATTERNS         1024                          // Upper bound for tag pattern count in NXCP request
#define MAX_EXPLICIT_DCI_COUNT   65536                         // Upper bound for explicit DCI list size in NXCP request

/**
 * Object classes that form the spatial path of a metric (nearest first when walking up)
 */
static const int s_spatialClasses[] = { OBJECT_RACK, OBJECT_ROOM, OBJECT_POWERDOMAIN, OBJECT_COOLINGZONE, OBJECT_FACILITY };

/**
 * Registered export (one per background task)
 */
struct DataExportState
{
   DataExportInfo info;
   bool archiveDownloaded;
   bool mappingDownloaded;

   DataExportState(uint32_t userId)
   {
      info.userId = userId;
      info.metricCount = 0;
      info.skippedCount = 0;
      info.sampleCount = 0;
      archiveDownloaded = false;
      mappingDownloaded = false;
   }
};

static Mutex s_exportLock(MutexType::FAST);
static std::map<uint64_t, DataExportState*> s_exports;

/**
 * Build export request from NXCP message (CMD_EXPORT_DCI_DATA / CMD_GET_DCI_DATA_EXPORT_METRICS)
 */
DataExportRequest::DataExportRequest(const NXCPMessage& msg)
{
   rootObjectId = msg.getFieldAsUInt32(VID_OBJECT_ID);

   // List sizes come from the client; bound them so that a malformed count cannot drive unbounded allocation
   int tagCount = msg.getFieldAsInt32(VID_NUM_TAGS);
   if (tagCount > MAX_TAG_PATTERNS)
      tagCount = MAX_TAG_PATTERNS;
   for(int i = 0; i < tagCount; i++)
   {
      wchar_t *pattern = msg.getFieldAsString(VID_TAG_LIST_BASE + i);
      if (pattern != nullptr)
         tagPatterns.addPreallocated(pattern);
   }

   int dciCount = msg.getFieldAsInt32(VID_NUM_ITEMS);
   if (dciCount > MAX_EXPLICIT_DCI_COUNT)
      dciCount = MAX_EXPLICIT_DCI_COUNT;
   uint32_t fieldId = VID_ITEM_LIST;
   for(int i = 0; i < dciCount; i++, fieldId += 2)
      dciList.push_back(std::make_pair(msg.getFieldAsUInt32(fieldId), msg.getFieldAsUInt32(fieldId + 1)));
   timeFrom = msg.getFieldAsInt64(VID_TIME_FROM);
   timeTo = msg.getFieldAsInt64(VID_TIME_TO);
   tier = static_cast<DciTier>(msg.getFieldAsInt16(VID_DCI_TIER));
   format = msg.getFieldAsInt16(VID_DATA_FORMAT);
   uint32_t flags = msg.getFieldAsUInt32(VID_FLAGS);
   anonymize = (flags & DATA_EXPORT_FLAG_ANONYMIZE) != 0;
   keepMapping = anonymize && ((flags & DATA_EXPORT_FLAG_KEEP_MAPPING) != 0);
}

/**
 * Fill NXCP message with metric information (10 fields per entry)
 */
void DataExportMetricInfo::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, ownerId);
   msg->setField(baseId + 1, dciId);
   msg->setField(baseId + 2, tag);
   msg->setField(baseId + 3, name);
   msg->setField(baseId + 4, description);
   msg->setField(baseId + 5, unit);
   msg->setField(baseId + 6, static_cast<int16_t>(origin));
   msg->setField(baseId + 7, static_cast<int16_t>(dataType));
   msg->setField(baseId + 8, static_cast<int16_t>(tier));
}

/**
 * Serialize metric information to JSON
 */
json_t *DataExportMetricInfo::toJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "ownerId", json_integer(ownerId));
   json_object_set_new(json, "dciId", json_integer(dciId));
   json_object_set_new(json, "tag", json_string_w(tag));
   json_object_set_new(json, "name", json_string_w(name));
   json_object_set_new(json, "description", json_string_w(description));
   json_object_set_new(json, "unit", json_string_w(unit));
   json_object_set_new(json, "origin", json_string_w(CodeToText(origin, g_dciOriginNames, L"")));
   json_object_set_new(json, "dataType", json_string_w(CodeToText(dataType, g_dciDataTypeNames, L"")));
   json_object_set_new(json, "tier", json_string((tier == DCI_TIER_HOURLY) ? "hourly" : (tier == DCI_TIER_DAILY) ? "daily" : "raw"));
   return json;
}

/**
 * Build full path to export directory
 */
static String GetExportDirectory()
{
   StringBuffer path(g_netxmsdDataDir);
   path.append(DDIR_EXPORT);
   return path;
}

/**
 * Delete export files and forget export
 */
static void ReleaseDataExport(uint64_t taskId)
{
   s_exportLock.lock();
   auto it = s_exports.find(taskId);
   DataExportState *state = (it != s_exports.end()) ? it->second : nullptr;
   if (state != nullptr)
      s_exports.erase(it);
   s_exportLock.unlock();

   if (state == nullptr)
      return;

   if (!state->info.archiveFile.isEmpty())
      _wremove(state->info.archiveFile);
   if (!state->info.mappingFile.isEmpty())
      _wremove(state->info.mappingFile);
   nxlog_debug_tag(DEBUG_TAG, 5, L"Export files for task " UINT64_FMT L" removed", taskId);
   delete state;
}

/**
 * Initialize export subsystem: create export directory and remove files left from previous server run
 */
void InitDataExport()
{
   String directory = GetExportDirectory();
   if (!CreateDirectoryTree(directory))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Cannot create DCI data export directory %s", directory.cstr());
      return;
   }

   DIRHANDLEW *dir = OpenDirW(directory);
   if (dir == nullptr)
      return;

   int count = 0;
   DIRENTRYW *entry;
   while((entry = ReadDirW(dir)) != nullptr)
   {
      if (wcsncmp(entry->d_name, L"export-", 7) != 0)
         continue;
      StringBuffer path(directory);
      path.append(FS_PATH_SEPARATOR);
      path.append(entry->d_name);
      if (_wremove(path) == 0)
         count++;
   }
   CloseDirW(dir);
   if (count > 0)
      nxlog_debug_tag(DEBUG_TAG, 3, L"%d stale DCI data export file(s) removed", count);
}

/**
 * CSV field writer: RFC 4180 quoting, UTF-8
 */
static void AppendCsvField(std::string& line, const char *value, bool last = false)
{
   if ((value != nullptr) && (*value != 0))
   {
      bool quote = (strpbrk(value, ",\"\r\n") != nullptr);
      if (quote)
      {
         line.push_back('"');
         for(const char *p = value; *p != 0; p++)
         {
            if (*p == '"')
               line.push_back('"');
            line.push_back(*p);
         }
         line.push_back('"');
      }
      else
      {
         line.append(value);
      }
   }
   line.push_back(last ? '\n' : ',');
}

/**
 * CSV field writer for wide strings
 */
static void AppendCsvField(std::string& line, const wchar_t *value, bool last = false)
{
   if ((value != nullptr) && (*value != 0))
   {
      char *utf8 = UTF8StringFromWideString(value);
      AppendCsvField(line, utf8, last);
      MemFree(utf8);
   }
   else
   {
      AppendCsvField(line, "", last);
   }
}

/**
 * CSV field writer for unsigned integers
 */
static void AppendCsvField(std::string& line, uint64_t value, bool last = false)
{
   char buffer[32];
   snprintf(buffer, sizeof(buffer), UINT64_FMTA, value);
   AppendCsvField(line, buffer, last);
}

/**
 * CSV field writer for floating point values (empty for NaN)
 */
static void AppendCsvField(std::string& line, double value, bool last = false)
{
   if (std::isnan(value))
   {
      AppendCsvField(line, "", last);
   }
   else
   {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%.15g", value);
      AppendCsvField(line, buffer, last);
   }
}

/**
 * Identity replacement for anonymized exports. Metric pseudonyms are HMAC-SHA256(server ID, DCI GUID) and therefore
 * stable across exports from the same server; object pseudonyms are per-export sequence numbers within object class.
 * Server ID is the HMAC key, so anonymized archives must not carry it (or any other real identifier) anywhere.
 */
class Anonymizer
{
private:
   bool m_enabled;
   std::map<uint32_t, std::string> m_objectNames;
   std::map<int, int> m_classCounters;
   std::string m_mapping;   // mapping.csv content

public:
   Anonymizer(bool enabled) : m_enabled(enabled)
   {
      if (m_enabled)
         m_mapping = "pseudonym,type,id,name\n";
   }

   bool isEnabled() const { return m_enabled; }
   const std::string& getMapping() const { return m_mapping; }

   /**
    * Get metric identifier for DCI
    */
   std::string metricId(const DCItem& dci)
   {
      char buffer[64];
      if (!m_enabled)
         return dci.getGuid().toStringA(buffer);

      BYTE key[8];
      for(int i = 0; i < 8; i++)
         key[i] = static_cast<BYTE>((g_serverId >> (56 - i * 8)) & 0xFF);
      BYTE signature[SHA256_DIGEST_SIZE];
      SignMessage(dci.getGuid().getValue(), UUID_LENGTH, key, sizeof(key), signature);
      uuid pseudonym(signature);
      pseudonym.toStringA(buffer);

      std::string line;
      AppendCsvField(line, buffer);
      AppendCsvField(line, "metric");
      char realId[64];
      AppendCsvField(line, dci.getGuid().toStringA(realId));
      AppendCsvField(line, dci.getDescription().cstr(), true);
      m_mapping.append(line);
      return buffer;
   }

   /**
    * Get display name for object
    */
   std::string objectName(const NetObj& object)
   {
      if (!m_enabled)
      {
         char *utf8 = UTF8StringFromWideString(object.getName());
         std::string name(utf8);
         MemFree(utf8);
         return name;
      }

      auto it = m_objectNames.find(object.getId());
      if (it != m_objectNames.end())
         return it->second;

      char *className = UTF8StringFromWideString(object.getObjectClassName());
      for(char *p = className; *p != 0; p++)
         *p = static_cast<char>(tolower(*p));
      char buffer[128];
      snprintf(buffer, sizeof(buffer), "%s-%d", className, ++m_classCounters[object.getObjectClass()]);
      MemFree(className);

      std::string pseudonym(buffer);
      m_objectNames[object.getId()] = pseudonym;

      std::string line;
      AppendCsvField(line, buffer);
      AppendCsvField(line, "object");
      AppendCsvField(line, static_cast<uint64_t>(object.getId()));
      AppendCsvField(line, object.getName(), true);
      m_mapping.append(line);
      return pseudonym;
   }
};

/**
 * Metric selected for export
 */
struct ExportedMetric
{
   shared_ptr<DataCollectionTarget> owner;
   shared_ptr<DCItem> dci;
   DciTier tier;
   std::string metricId;
};

/**
 * Collect DCIs selected by request. Only numeric single-value DCIs accessible to the user are exported.
 */
static void SelectMetrics(const DataExportRequest& request, uint32_t userId, std::vector<ExportedMetric>& metrics, uint32_t *skipped)
{
   auto consider = [&metrics, skipped] (const shared_ptr<DataCollectionTarget>& owner, const shared_ptr<DCObject>& dco)
   {
      if (dco->getType() != DCO_TYPE_ITEM)
      {
         (*skipped)++;
         return;
      }
      shared_ptr<DCItem> dci = static_pointer_cast<DCItem>(dco);
      if (dci->getTransformedDataType() == DCI_DT_STRING)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, L"DCI \"%s\" [%u] on %s [%u] skipped (non-numeric)", dci->getDescription().cstr(), dci->getId(), owner->getName(), owner->getId());
         (*skipped)++;
         return;
      }
      ExportedMetric m;
      m.owner = owner;
      m.dci = dci;
      m.tier = DCI_TIER_RAW;
      metrics.push_back(m);
   };

   if (!request.dciList.empty())
   {
      for(const std::pair<uint32_t, uint32_t>& ref : request.dciList)
      {
         shared_ptr<NetObj> object = FindObjectById(ref.first);
         if ((object == nullptr) || !object->isDataCollectionTarget() || !object->checkAccessRights(userId, OBJECT_ACCESS_READ))
         {
            (*skipped)++;
            continue;
         }
         shared_ptr<DataCollectionTarget> owner = static_pointer_cast<DataCollectionTarget>(object);
         shared_ptr<DCObject> dco = owner->getDCObjectById(ref.second, userId);
         if (dco == nullptr)
         {
            (*skipped)++;
            continue;
         }
         consider(owner, dco);
      }
   }
   else
   {
      shared_ptr<NetObj> rootObject = FindObjectById(request.rootObjectId);
      if (rootObject == nullptr)
         return;

      SharedObjectArray<DataCollectionTarget> targets;
      if (rootObject->isDataCollectionTarget())
         targets.add(static_pointer_cast<DataCollectionTarget>(rootObject));
      rootObject->addChildDCTargetsToList(&targets, userId);

      for(int i = 0; i < targets.size(); i++)
      {
         shared_ptr<DataCollectionTarget> owner = targets.getShared(i);
         if (!owner->checkAccessRights(userId, OBJECT_ACCESS_READ))
            continue;
         unique_ptr<SharedObjectArray<DCObject>> dcObjects = owner->getAllDCObjects(userId);
         for(int j = 0; j < dcObjects->size(); j++)
         {
            shared_ptr<DCObject> dco = dcObjects->getShared(j);
            SharedString tag = dco->getSystemTag();
            if (tag.isEmpty())
               continue;
            bool match = false;
            for(int k = 0; (k < request.tagPatterns.size()) && !match; k++)
               match = MatchString(request.tagPatterns.get(k), tag.cstr(), false);
            if (match)
               consider(owner, dco);
         }
      }
   }

   std::sort(metrics.begin(), metrics.end(),
      [] (const ExportedMetric& a, const ExportedMetric& b) -> bool
      {
         return (a.owner->getId() != b.owner->getId()) ? (a.owner->getId() < b.owner->getId()) : (a.dci->getId() < b.dci->getId());
      });
}

/**
 * Resolve tier actually read for each selected metric
 */
static void ResolveMetricTiers(const DataExportRequest& request, std::vector<ExportedMetric>& metrics)
{
   int autoSelectThreshold = ConfigReadInt(L"DataCollection.Aggregation.MaxAutoSelectPoints", 5000);
   for(ExportedMetric& m : metrics)
      m.tier = ResolveDciTier(request.tier, *m.dci, DCO_TYPE_ITEM, request.timeFrom, request.timeTo, m.owner->getRuntimeFlags(), autoSelectThreshold);
}

/**
 * Resolve metrics that given export request would select, without running the export
 */
void NXCORE_EXPORTABLE ResolveDataExportSelection(const DataExportRequest& request, uint32_t userId, std::vector<DataExportMetricInfo>& metrics, uint32_t *skipped)
{
   std::vector<ExportedMetric> selected;
   *skipped = 0;
   SelectMetrics(request, userId, selected, skipped);
   ResolveMetricTiers(request, selected);

   metrics.clear();
   metrics.reserve(selected.size());
   for(const ExportedMetric& m : selected)
   {
      DataExportMetricInfo info;
      info.ownerId = m.owner->getId();
      info.dciId = m.dci->getId();
      info.tag = m.dci->getSystemTag().cstr();
      info.name = m.dci->getName().cstr();
      info.description = m.dci->getDescription().cstr();
      info.unit = m.dci->getUnitName().cstr();
      info.origin = m.dci->getDataSource();
      info.dataType = m.dci->getTransformedDataType();
      info.tier = m.tier;
      metrics.push_back(info);
   }
}

/**
 * Build spatial path of object (facility / room / power domain / cooling zone / rack ancestors, outermost first).
 * Path stops at the first ancestor the user cannot read.
 */
static std::string BuildSpatialPath(const shared_ptr<NetObj>& object, Anonymizer& anonymizer, uint32_t userId)
{
   std::vector<std::string> elements;
   shared_ptr<NetObj> current = object;
   for(int depth = 0; (depth < 16) && (current != nullptr); depth++)
   {
      shared_ptr<NetObj> next;
      for(int classId : s_spatialClasses)
      {
         unique_ptr<SharedObjectArray<NetObj>> parents = current->getParents(classId);
         if (parents->size() > 0)
         {
            next = parents->getShared(0);
            break;
         }
      }
      if ((next == nullptr) || !next->checkAccessRights(userId, OBJECT_ACCESS_READ))
         break;
      elements.push_back(anonymizer.objectName(*next));
      current = next;
   }

   std::string path;
   for(auto it = elements.rbegin(); it != elements.rend(); ++it)
   {
      if (!path.empty())
         path.push_back('/');
      path.append(*it);
   }
   return path;
}

/**
 * Get tier name
 */
static const char *TierName(DciTier tier)
{
   switch(tier)
   {
      case DCI_TIER_HOURLY:
         return "hourly";
      case DCI_TIER_DAILY:
         return "daily";
      default:
         return "raw";
   }
}

/**
 * Get sample quality name
 */
static const char *QualityName(SampleQuality quality)
{
   switch(quality)
   {
      case SampleQuality::PROXY:
         return "proxy";
      case SampleQuality::ESTIMATED:
         return "estimated";
      case SampleQuality::INTERPOLATED:
         return "interpolated";
      case SampleQuality::MISSING:
         return "missing";
      default:
         return "measured";
   }
}

/**
 * Write metric catalog (metrics.csv). Anonymized catalog carries no real object identifiers; related objects and
 * spatial ancestors are included only when readable by the user.
 */
static bool WriteMetricCatalog(ZipArchiveWriter& archive, std::vector<ExportedMetric>& metrics, Anonymizer& anonymizer, uint32_t userId)
{
   if (!archive.beginEntry("metrics.csv"))
      return false;
   archive.write("metric_id,tag,owner_id,owner_name,owner_class,related_object_id,related_object_name,related_object_class,"
                 "name,description,unit,multiplier,data_type,origin,polling_interval,tier,facility_path\n");

   for(ExportedMetric& m : metrics)
   {
      m.metricId = anonymizer.metricId(*m.dci);

      std::string line;
      AppendCsvField(line, m.metricId.c_str());
      AppendCsvField(line, m.dci->getSystemTag().cstr());
      if (anonymizer.isEnabled())
         AppendCsvField(line, "");
      else
         AppendCsvField(line, static_cast<uint64_t>(m.owner->getId()));
      AppendCsvField(line, anonymizer.objectName(*m.owner).c_str());
      AppendCsvField(line, m.owner->getObjectClassName());

      shared_ptr<NetObj> relatedObject = FindObjectById(m.dci->getRelatedObject());
      if ((relatedObject != nullptr) && relatedObject->checkAccessRights(userId, OBJECT_ACCESS_READ))
      {
         if (anonymizer.isEnabled())
            AppendCsvField(line, "");
         else
            AppendCsvField(line, static_cast<uint64_t>(relatedObject->getId()));
         AppendCsvField(line, anonymizer.objectName(*relatedObject).c_str());
         AppendCsvField(line, relatedObject->getObjectClassName());
      }
      else
      {
         AppendCsvField(line, "");
         AppendCsvField(line, "");
         AppendCsvField(line, "");
      }

      AppendCsvField(line, anonymizer.isEnabled() ? L"" : m.dci->getName().cstr());
      AppendCsvField(line, anonymizer.isEnabled() ? L"" : m.dci->getDescription().cstr());
      AppendCsvField(line, m.dci->getUnitName().cstr());
      AppendCsvField(line, static_cast<uint64_t>(m.dci->getMultiplier()));
      AppendCsvField(line, CodeToText(m.dci->getTransformedDataType(), g_dciDataTypeNames, L""));
      AppendCsvField(line, CodeToText(m.dci->getDataSource(), g_dciOriginNames, L""));
      AppendCsvField(line, static_cast<uint64_t>(m.dci->getEffectivePollingInterval()));
      AppendCsvField(line, TierName(m.tier));
      AppendCsvField(line, BuildSpatialPath(m.owner, anonymizer, userId).c_str(), true);
      archive.write(line);
   }
   return archive.endEntry();
}

/**
 * Write raw samples of one metric. Returns number of rows written or -1 on database error.
 */
static int64_t WriteRawSamples(ZipArchiveWriter& archive, const ExportedMetric& m, int64_t timeFrom, int64_t timeTo)
{
   int64_t rows = 0;
   std::vector<AttributedSample> samples;
   for(int64_t chunkStart = timeFrom; chunkStart <= timeTo; chunkStart += RAW_READ_CHUNK_MS)
   {
      int64_t chunkEnd = std::min<int64_t>(chunkStart + RAW_READ_CHUNK_MS - 1, timeTo);
      if (!ReadAttributedSamples(*m.dci, Timestamp::fromMilliseconds(chunkStart), Timestamp::fromMilliseconds(chunkEnd), samples))
         return -1;

      // Samples are returned newest first; export oldest first
      for(auto it = samples.rbegin(); it != samples.rend(); ++it)
      {
         const AttributedSample& s = *it;
         std::string line;
         AppendCsvField(line, m.metricId.c_str());
         AppendCsvField(line, FormatISO8601TimestampMs(s.timestamp.asMilliseconds()).c_str());
         AppendCsvField(line, s.value);
         AppendCsvField(line, "");  // min
         AppendCsvField(line, "");  // max
         AppendCsvField(line, "");  // count
         if (s.attributes.isDefault())
         {
            AppendCsvField(line, "");
            AppendCsvField(line, "");
            AppendCsvField(line, "");
            AppendCsvField(line, "");
            AppendCsvField(line, "", true);
         }
         else
         {
            AppendCsvField(line, QualityName(s.attributes.quality));
            AppendCsvField(line, s.attributes.bounded ? s.attributes.lower : NAN);
            AppendCsvField(line, s.attributes.bounded ? s.attributes.upper : NAN);
            AppendCsvField(line, s.attributes.completeness);
            if (s.attributes.methodId != 0)
               AppendCsvField(line, static_cast<uint64_t>(s.attributes.methodId), true);
            else
               AppendCsvField(line, "", true);
         }
         if (!archive.write(line))
            return -1;
         rows++;
      }
   }
   return rows;
}

/**
 * Write aggregated (hourly or daily) samples of one metric. Returns number of rows written or -1 on database error.
 */
static int64_t WriteTieredSamples(ZipArchiveWriter& archive, const ExportedMetric& m, int64_t timeFrom, int64_t timeTo)
{
   const wchar_t *condition = (g_dbSyntax == DB_SYNTAX_TSDB) ?
      L" AND bucket_start>=ms_to_timestamptz(?) AND bucket_start<=ms_to_timestamptz(?)" :
      L" AND bucket_start>=? AND bucket_start<=?";

   struct Row
   {
      int64_t timestamp;
      double avg, min, max;
      uint64_t count;
   };
   std::vector<Row> buckets;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = false;
   DB_STATEMENT hStmt = PrepareTieredDataSelect(hdb, m.owner->getId(), m.tier, DCI_HAGG_MINMAX, 0x7FFFFFFF, condition);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m.dci->getId());
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, timeFrom);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, timeTo);
      DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
      if (hResult != nullptr)
      {
         while(DBFetch(hResult))
         {
            Row r;
            r.timestamp = DBGetFieldInt64(hResult, 0);
            r.avg = DBGetFieldDouble(hResult, 1);
            r.min = DBGetFieldDouble(hResult, 2);
            r.max = DBGetFieldDouble(hResult, 3);
            r.count = static_cast<uint64_t>(DBGetFieldInt64(hResult, 4));
            buckets.push_back(r);
         }
         DBFreeResult(hResult);
         success = true;
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   if (!success)
      return -1;

   // Buckets are returned newest first; export oldest first
   for(auto it = buckets.rbegin(); it != buckets.rend(); ++it)
   {
      std::string line;
      AppendCsvField(line, m.metricId.c_str());
      AppendCsvField(line, FormatISO8601TimestampMs(it->timestamp).c_str());
      AppendCsvField(line, it->avg);
      AppendCsvField(line, it->min);
      AppendCsvField(line, it->max);
      AppendCsvField(line, it->count);
      AppendCsvField(line, "");
      AppendCsvField(line, "");
      AppendCsvField(line, "");
      AppendCsvField(line, "");
      AppendCsvField(line, "", true);
      if (!archive.write(line))
         return -1;
   }
   return static_cast<int64_t>(buckets.size());
}

/**
 * Write manifest.json
 */
static bool WriteManifest(ZipArchiveWriter& archive, const DataExportRequest& request, const DataExportInfo& info)
{
   json_t *manifest = json_object();
   json_object_set_new(manifest, "formatVersion", json_integer(EXPORT_FORMAT_VERSION));
   json_object_set_new(manifest, "generator", json_string_w(L"NetXMS " NETXMS_VERSION_STRING));
   if (request.anonymize)
   {
      // Server ID is the key of metric pseudonyms and root object ID is a real identifier; neither may leave the server
      json_object_set_new(manifest, "serverId", json_null());
   }
   else
   {
      wchar_t serverId[32];
      nx_swprintf(serverId, 32, UINT64X_FMT(L"016"), g_serverId);
      json_object_set_new(manifest, "serverId", json_string_w(serverId));
   }
   json_object_set_new(manifest, "exportTime", json_time_string_ms(GetCurrentTimeMs()));
   json_object_set_new(manifest, "timeFrom", json_time_string_ms(request.timeFrom));
   json_object_set_new(manifest, "timeTo", json_time_string_ms(request.timeTo));
   json_object_set_new(manifest, "tier", json_string(TierName(request.tier)));
   json_object_set_new(manifest, "rootObjectId", (request.dciList.empty() && !request.anonymize) ? json_integer(request.rootObjectId) : json_null());
   json_object_set_new(manifest, "tagPatterns", request.tagPatterns.toJson());
   json_object_set_new(manifest, "explicitSelection", json_boolean(!request.dciList.empty()));
   json_object_set_new(manifest, "anonymized", json_boolean(request.anonymize));
   json_object_set_new(manifest, "metricCount", json_integer(info.metricCount));
   json_object_set_new(manifest, "skippedCount", json_integer(info.skippedCount));
   json_object_set_new(manifest, "sampleCount", json_integer(static_cast<json_int_t>(info.sampleCount)));
   json_t *files = json_object();
   json_object_set_new(files, "metrics", json_string("metrics.csv"));
   json_object_set_new(files, "data", json_string("data.csv"));
   json_object_set_new(manifest, "files", files);

   char *text = json_dumps(manifest, JSON_INDENT(3));
   json_decref(manifest);
   bool success = archive.beginEntry("manifest.json") && archive.write(text) && archive.write("\n") && archive.endEntry();
   MemFree(text);
   return success;
}

/**
 * Write mapping file (pseudonym to real identity), kept locally by the operator and never part of the archive
 */
static bool WriteMappingFile(const wchar_t *fileName, const std::string& mapping)
{
   int fd = _wopen(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IRUSR | S_IWUSR);
   if (fd == -1)
      return false;
   bool success = (_write(fd, mapping.data(), static_cast<unsigned int>(mapping.size())) == static_cast<int>(mapping.size()));
   _close(fd);
   return success;
}

/**
 * Background task body
 */
static bool RunDataExport(const DataExportRequest& request, DataExportInfo *info, BackgroundTask *task)
{
   uint64_t taskId = task->getId();
   nxlog_debug_tag(DEBUG_TAG, 4, L"Export task " UINT64_FMT L" started (user %u, root object %u, %d explicit DCIs, %d tag patterns, tier %d, anonymize=%s)",
      taskId, info->userId, request.rootObjectId, static_cast<int>(request.dciList.size()), request.tagPatterns.size(),
      static_cast<int>(request.tier), BooleanToString(request.anonymize));

   std::vector<ExportedMetric> metrics;
   SelectMetrics(request, info->userId, metrics, &info->skippedCount);
   info->metricCount = static_cast<uint32_t>(metrics.size());

   ResolveMetricTiers(request, metrics);

   wchar_t archiveFile[MAX_PATH];
   nx_swprintf(archiveFile, MAX_PATH, L"%s" FS_PATH_SEPARATOR L"export-" UINT64_FMT L".zip", GetExportDirectory().cstr(), taskId);
   info->archiveFile = archiveFile;

   ZipArchiveWriter archive;
   if (!archive.open(archiveFile))
      return task->failure(L"Cannot create export file %s", archiveFile);

   Anonymizer anonymizer(request.anonymize);
   if (!WriteMetricCatalog(archive, metrics, anonymizer, info->userId))
      return task->failure(L"Cannot write metric catalog");

   if (!archive.beginEntry("data.csv"))
      return task->failure(L"Cannot write sample data");
   archive.write("metric_id,timestamp,value,min,max,count,quality,lower_bound,upper_bound,completeness,method_id\n");
   int completed = 0;
   for(const ExportedMetric& m : metrics)
   {
      int64_t rows = (m.tier == DCI_TIER_RAW) ?
         WriteRawSamples(archive, m, request.timeFrom, request.timeTo) :
         WriteTieredSamples(archive, m, request.timeFrom, request.timeTo);
      if (rows < 0)
         return task->failure(L"Cannot read history of DCI \"%s\" [%u] on %s [%u]", m.dci->getDescription().cstr(), m.dci->getId(), m.owner->getName(), m.owner->getId());
      info->sampleCount += rows;
      completed++;
      task->markProgress(completed * 95 / static_cast<int>(metrics.size()));
      nxlog_debug_tag(DEBUG_TAG, 7, L"Export task " UINT64_FMT L": DCI [%u] on [%u] exported (" INT64_FMT L" rows, tier %hs)", taskId, m.dci->getId(), m.owner->getId(), rows, TierName(m.tier));
   }
   if (!archive.endEntry())
      return task->failure(L"Cannot write sample data");

   if (!WriteManifest(archive, request, *info))
      return task->failure(L"Cannot write manifest");

   if (!archive.close())
      return task->failure(L"Cannot finalize export file %s", archiveFile);

   if (request.keepMapping)
   {
      wchar_t mappingFile[MAX_PATH];
      nx_swprintf(mappingFile, MAX_PATH, L"%s" FS_PATH_SEPARATOR L"export-" UINT64_FMT L"-mapping.csv", GetExportDirectory().cstr(), taskId);
      if (!WriteMappingFile(mappingFile, anonymizer.getMapping()))
         return task->failure(L"Cannot write mapping file %s", mappingFile);
      info->mappingFile = mappingFile;
   }

   task->markProgress(100);
   nxlog_debug_tag(DEBUG_TAG, 4, L"Export task " UINT64_FMT L" completed: %u metrics, " UINT64_FMT L" samples, %u DCIs skipped, archive size " UINT64_FMT,
      taskId, info->metricCount, info->sampleCount, info->skippedCount, archive.getArchiveSize());
   return true;
}

/**
 * Start export as background task. Produced files are available for download until retrieved or until retention expires.
 */
shared_ptr<BackgroundTask> NXCORE_EXPORTABLE StartDataExport(const DataExportRequest& request, uint32_t userId)
{
   // Request is copied for the task; state is registered once task ID is known, and the task body waits for that
   shared_ptr<DataExportRequest> requestCopy = make_shared<DataExportRequest>(request);
   shared_ptr<Condition> registered = make_shared<Condition>(true);
   shared_ptr<BackgroundTask> task = CreateBackgroundTask(g_mainThreadPool,
      [requestCopy, registered] (BackgroundTask *task) -> bool
      {
         registered->wait(INFINITE);
         uint64_t taskId = task->getId();

         // Work on a private copy of export information; it is published under lock once the task is done,
         // so concurrent readers (state and download handlers) never observe a partially updated record
         DataExportInfo info;
         s_exportLock.lock();
         auto it = s_exports.find(taskId);
         bool registered = (it != s_exports.end());
         if (registered)
            info = it->second->info;
         s_exportLock.unlock();
         if (!registered)
            return task->failure(L"Internal error: export state not registered");

         bool success = RunDataExport(*requestCopy, &info, task);
         if (!success)
         {
            // Partial files are useless; remove them now but keep the export state so that clients can read failure details
            nxlog_debug_tag(DEBUG_TAG, 4, L"Export task " UINT64_FMT L" failed: %s", taskId, task->getFailureReson().cstr());
            if (!info.archiveFile.isEmpty())
               _wremove(info.archiveFile);
            if (!info.mappingFile.isEmpty())
               _wremove(info.mappingFile);
            info.archiveFile = L"";
            info.mappingFile = L"";
         }

         s_exportLock.lock();
         it = s_exports.find(taskId);
         if (it != s_exports.end())
            it->second->info = info;
         s_exportLock.unlock();

         ThreadPoolScheduleRelative(g_mainThreadPool, EXPORT_RETENTION_MS, [taskId] () { ReleaseDataExport(taskId); });
         return success;
      }, L"DCI data export");

   if (task == nullptr)
      return nullptr;

   s_exportLock.lock();
   s_exports[task->getId()] = new DataExportState(userId);
   s_exportLock.unlock();
   registered->set();
   return task;
}

/**
 * Get export information by task ID. Statistics and file names are meaningful only when the task has completed.
 */
bool NXCORE_EXPORTABLE GetDataExportInfo(uint64_t taskId, DataExportInfo *info)
{
   LockGuard lockGuard(s_exportLock);
   auto it = s_exports.find(taskId);
   if (it == s_exports.end())
      return false;
   *info = it->second->info;
   return true;
}

/**
 * Register download of export file. Once every produced file has been downloaded, files are removed.
 */
void NXCORE_EXPORTABLE MarkDataExportFileDownloaded(uint64_t taskId, int fileType)
{
   s_exportLock.lock();
   auto it = s_exports.find(taskId);
   bool release = false;
   if (it != s_exports.end())
   {
      DataExportState *state = it->second;
      if (fileType == DATA_EXPORT_FILE_MAPPING)
         state->mappingDownloaded = true;
      else
         state->archiveDownloaded = true;
      release = state->archiveDownloaded && (state->info.mappingFile.isEmpty() || state->mappingDownloaded);
   }
   s_exportLock.unlock();

   if (release)
      ReleaseDataExport(taskId);
}
