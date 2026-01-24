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
** File: anomaly_profile.cpp
**
**/

#include "nxcore.h"
#include <nxai.h>
#include <math.h>

#define DEBUG_TAG L"ai.anomaly"

/**
 * Minimum data points required for profile generation
 */
#define MIN_DATA_POINTS_FOR_PROFILE   1000

/**
 * Default data range in days for profile generation
 */
#define DEFAULT_DATA_RANGE_DAYS       30

/**
 * Cooldown period before re-generating a profile (in seconds)
 */
#define PROFILE_REGENERATION_COOLDOWN  86400   // 24 hours

/**
 * Maximum retry count for scheduled task
 */
#define MAX_RETRY_COUNT   3

/**
 * Hourly statistics structure
 */
struct HourlyStats
{
   int hour;
   double mean;
   double stddev;
   int count;
   double sum;
   double sumSquares;
};

/**
 * Day of week statistics structure
 */
struct WeekdayStats
{
   double sum;
   int count;
};

/**
 * Overall statistics structure
 */
struct OverallStats
{
   double min;
   double max;
   double sum;
   double sumSquares;
   int count;
   double percentile1;
   double percentile5;
   double percentile50;
   double percentile95;
   double percentile99;
   double maxDelta;
   double sumDelta;
   int deltaCount;
};

/**
 * Load DCI values for statistics calculation
 */
static bool LoadDciValuesForStats(uint32_t nodeId, uint32_t dciId, DCObjectStorageClass storageClass,
   time_t timeFrom, time_t timeTo, ObjectArray<std::pair<time_t, double>> *values)
{
   StringBuffer query(_T("SELECT idata_timestamp,idata_value"));
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         query.append(_T(" FROM idata_sc_"));
         query.append(DCObject::getStorageClassName(storageClass));
         query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ms_to_timestamptz(?) AND ms_to_timestamptz(?)"));
      }
      else
      {
         query.append(_T(" FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
      }
   }
   else
   {
      query.append(_T(" FROM idata_"));
      query.append(nodeId);
      query.append(_T(" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?"));
   }
   query.append(_T(" ORDER BY idata_timestamp"));

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   bool success = false;

   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dciId);
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, static_cast<int64_t>(timeFrom) * 1000);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, static_cast<int64_t>(timeTo) * 1000);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for (int i = 0; i < count; i++)
         {
            Timestamp timestamp = DBGetFieldTimestamp(hResult, i, 0);
            double value = DBGetFieldDouble(hResult, i, 1);
            values->add(new std::pair<time_t, double>(timestamp.asTime(), value));
         }
         DBFreeResult(hResult);
         success = true;
      }
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Calculate percentile from sorted array
 */
static double CalculatePercentile(const double *sortedValues, int count, double percentile)
{
   if (count == 0)
      return 0;
   if (count == 1)
      return sortedValues[0];

   double index = (percentile / 100.0) * (count - 1);
   int lower = static_cast<int>(floor(index));
   int upper = static_cast<int>(ceil(index));
   double fraction = index - lower;

   if (upper >= count)
      upper = count - 1;

   return sortedValues[lower] + fraction * (sortedValues[upper] - sortedValues[lower]);
}

/**
 * Compare function for qsort
 */
static int CompareDoubles(const void *a, const void *b)
{
   double da = *static_cast<const double*>(a);
   double db = *static_cast<const double*>(b);
   return (da < db) ? -1 : ((da > db) ? 1 : 0);
}

/**
 * Compute statistics from DCI historical data
 */
static json_t *ComputeStatistics(uint32_t nodeId, uint32_t dciId, DCObjectStorageClass storageClass, time_t timeFrom, time_t timeTo)
{
   ObjectArray<std::pair<time_t, double>> values(1024, 1024, Ownership::True);
   if (!LoadDciValuesForStats(nodeId, dciId, storageClass, timeFrom, timeTo, &values))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ComputeStatistics: failed to load data for DCI [%u] on node [%u]"), dciId, nodeId);
      return nullptr;
   }

   int count = values.size();
   if (count < MIN_DATA_POINTS_FOR_PROFILE)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ComputeStatistics: insufficient data points (%d) for DCI [%u], need at least %d"),
         count, dciId, MIN_DATA_POINTS_FOR_PROFILE);
      return nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("ComputeStatistics: loaded %d data points for DCI [%u]"), count, dciId);

   // Initialize hourly stats
   HourlyStats hourlyStats[24];
   for (int i = 0; i < 24; i++)
   {
      hourlyStats[i].hour = i;
      hourlyStats[i].mean = 0;
      hourlyStats[i].stddev = 0;
      hourlyStats[i].count = 0;
      hourlyStats[i].sum = 0;
      hourlyStats[i].sumSquares = 0;
   }

   // Initialize weekday stats (0=Sunday, 6=Saturday)
   WeekdayStats weekdayStats[7];
   for (int i = 0; i < 7; i++)
   {
      weekdayStats[i].sum = 0;
      weekdayStats[i].count = 0;
   }

   // Initialize overall stats
   OverallStats overall;
   overall.min = values.get(0)->second;
   overall.max = values.get(0)->second;
   overall.sum = 0;
   overall.sumSquares = 0;
   overall.count = 0;
   overall.maxDelta = 0;
   overall.sumDelta = 0;
   overall.deltaCount = 0;

   // Allocate array for sorting (for percentiles)
   double *sortedValues = MemAllocArrayNoInit<double>(count);

   // First pass: collect basic statistics
   double prevValue = 0;
   time_t prevTimestamp = 0;
   for (int i = 0; i < count; i++)
   {
      std::pair<time_t, double> *p = values.get(i);
      time_t timestamp = p->first;
      double value = p->second;

      sortedValues[i] = value;

      // Overall stats
      if (value < overall.min)
         overall.min = value;
      if (value > overall.max)
         overall.max = value;
      overall.sum += value;
      overall.sumSquares += value * value;
      overall.count++;

      // Rate of change (delta)
      if (i > 0 && prevTimestamp > 0)
      {
         double elapsed = static_cast<double>(timestamp - prevTimestamp) / 60.0;  // minutes
         if (elapsed > 0)
         {
            double delta = fabs(value - prevValue);
            double rate = delta / elapsed;  // per minute
            if (rate > overall.maxDelta)
               overall.maxDelta = rate;
            overall.sumDelta += rate;
            overall.deltaCount++;
         }
      }
      prevValue = value;
      prevTimestamp = timestamp;

      // Get hour and weekday from timestamp
      struct tm timeInfo;
#ifdef _WIN32
      localtime_s(&timeInfo, &timestamp);
#else
      localtime_r(&timestamp, &timeInfo);
#endif

      int hour = timeInfo.tm_hour;
      int weekday = timeInfo.tm_wday;

      // Hourly stats
      hourlyStats[hour].sum += value;
      hourlyStats[hour].sumSquares += value * value;
      hourlyStats[hour].count++;

      // Weekday stats
      weekdayStats[weekday].sum += value;
      weekdayStats[weekday].count++;
   }

   // Sort values for percentile calculation
   qsort(sortedValues, count, sizeof(double), CompareDoubles);

   // Calculate percentiles
   overall.percentile1 = CalculatePercentile(sortedValues, count, 1);
   overall.percentile5 = CalculatePercentile(sortedValues, count, 5);
   overall.percentile50 = CalculatePercentile(sortedValues, count, 50);
   overall.percentile95 = CalculatePercentile(sortedValues, count, 95);
   overall.percentile99 = CalculatePercentile(sortedValues, count, 99);

   MemFree(sortedValues);

   // Calculate hourly means and stddevs
   for (int i = 0; i < 24; i++)
   {
      if (hourlyStats[i].count > 0)
      {
         hourlyStats[i].mean = hourlyStats[i].sum / hourlyStats[i].count;
         if (hourlyStats[i].count > 1)
         {
            double variance = (hourlyStats[i].sumSquares - (hourlyStats[i].sum * hourlyStats[i].sum / hourlyStats[i].count)) / (hourlyStats[i].count - 1);
            hourlyStats[i].stddev = (variance > 0) ? sqrt(variance) : 0;
         }
      }
   }

   // Calculate weekday factors relative to overall mean
   double overallMean = (overall.count > 0) ? overall.sum / overall.count : 0;
   double weekdayFactors[7];
   for (int i = 0; i < 7; i++)
   {
      if (weekdayStats[i].count > 0 && overallMean > 0)
      {
         double weekdayMean = weekdayStats[i].sum / weekdayStats[i].count;
         weekdayFactors[i] = weekdayMean / overallMean;
      }
      else
      {
         weekdayFactors[i] = 1.0;
      }
   }

   // Build JSON statistics object
   json_t *stats = json_object();

   // Data range
   json_t *dataRange = json_object();
   json_object_set_new(dataRange, "start", json_integer(timeFrom));
   json_object_set_new(dataRange, "end", json_integer(timeTo));
   json_object_set_new(dataRange, "dataPoints", json_integer(count));
   json_object_set_new(stats, "dataRange", dataRange);

   // Overall statistics
   json_t *overallJson = json_object();
   json_object_set_new(overallJson, "min", json_real(overall.min));
   json_object_set_new(overallJson, "max", json_real(overall.max));
   json_object_set_new(overallJson, "mean", json_real(overallMean));
   double overallVariance = (overall.count > 1) ? (overall.sumSquares - (overall.sum * overall.sum / overall.count)) / (overall.count - 1) : 0;
   json_object_set_new(overallJson, "stddev", json_real((overallVariance > 0) ? sqrt(overallVariance) : 0));
   json_object_set_new(overallJson, "p1", json_real(overall.percentile1));
   json_object_set_new(overallJson, "p5", json_real(overall.percentile5));
   json_object_set_new(overallJson, "p50", json_real(overall.percentile50));
   json_object_set_new(overallJson, "p95", json_real(overall.percentile95));
   json_object_set_new(overallJson, "p99", json_real(overall.percentile99));
   json_object_set_new(stats, "overall", overallJson);

   // Rate of change statistics
   json_t *rateJson = json_object();
   json_object_set_new(rateJson, "maxRatePerMinute", json_real(overall.maxDelta));
   json_object_set_new(rateJson, "meanRatePerMinute", json_real((overall.deltaCount > 0) ? overall.sumDelta / overall.deltaCount : 0));
   json_object_set_new(stats, "rateOfChange", rateJson);

   // Hourly baselines
   json_t *hourlyArray = json_array();
   for (int i = 0; i < 24; i++)
   {
      json_t *hourJson = json_object();
      json_object_set_new(hourJson, "hour", json_integer(i));
      json_object_set_new(hourJson, "mean", json_real(hourlyStats[i].mean));
      json_object_set_new(hourJson, "stddev", json_real(hourlyStats[i].stddev));
      json_object_set_new(hourJson, "count", json_integer(hourlyStats[i].count));
      json_array_append_new(hourlyArray, hourJson);
   }
   json_object_set_new(stats, "hourlyBaselines", hourlyArray);

   // Weekday factors
   json_t *weekdayJson = json_object();
   const char *weekdayNames[] = { "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday" };
   for (int i = 0; i < 7; i++)
   {
      json_object_set_new(weekdayJson, weekdayNames[i], json_real(weekdayFactors[i]));
   }
   json_object_set_new(stats, "weekdayFactors", weekdayJson);

   return stats;
}

/**
 * Build LLM prompt for anomaly profile generation
 */
static std::string BuildLLMPrompt(const wchar_t *dciName, const wchar_t *dciDescription, const wchar_t *unitName,
   int pollingInterval, const wchar_t *objectName, const wchar_t *objectAiHint, const wchar_t *dciAiHint, json_t *statistics)
{
   char *statsJson = json_dumps(statistics, JSON_INDENT(2));

   StringBuffer prompt;
   prompt.append(_T("You are analyzing a network monitoring metric to configure anomaly detection parameters.\n\n"));

   prompt.append(_T("METRIC INFORMATION:\n"));
   prompt.append(_T("- Name: "));
   prompt.append(dciName);
   prompt.append(_T("\n"));

   if (dciDescription != nullptr && dciDescription[0] != 0)
   {
      prompt.append(_T("- Description: "));
      prompt.append(dciDescription);
      prompt.append(_T("\n"));
   }

   if (unitName != nullptr && unitName[0] != 0)
   {
      prompt.append(_T("- Unit: "));
      prompt.append(unitName);
      prompt.append(_T("\n"));
   }

   prompt.append(_T("- Polling interval: "));
   prompt.appendFormattedString(_T("%d"), pollingInterval);
   prompt.append(_T(" seconds\n"));

   prompt.append(_T("- Object: "));
   prompt.append(objectName);
   prompt.append(_T("\n\n"));

   if (objectAiHint != nullptr && objectAiHint[0] != 0)
   {
      prompt.append(_T("OBJECT CONTEXT:\n"));
      prompt.append(objectAiHint);
      prompt.append(_T("\n\n"));
   }

   if (dciAiHint != nullptr && dciAiHint[0] != 0)
   {
      prompt.append(_T("METRIC CONTEXT:\n"));
      prompt.append(dciAiHint);
      prompt.append(_T("\n\n"));
   }

   prompt.append(_T("COMPUTED STATISTICS FROM HISTORICAL DATA:\n"));
   prompt.appendUtf8String(statsJson, strlen(statsJson));
   prompt.append(_T("\n\n"));

   prompt.append(_T("Based on this data, provide anomaly detection parameters as a JSON object with the following structure:\n"));
   prompt.append(_T("{\n"));
   prompt.append(_T("  \"hardBounds\": {\n"));
   prompt.append(_T("    \"enabled\": true/false,\n"));
   prompt.append(_T("    \"min\": <minimum acceptable value>,\n"));
   prompt.append(_T("    \"max\": <maximum acceptable value>\n"));
   prompt.append(_T("  },\n"));
   prompt.append(_T("  \"seasonalDetection\": {\n"));
   prompt.append(_T("    \"enabled\": true/false,\n"));
   prompt.append(_T("    \"sensitivity\": <number of standard deviations, typically 2.0-3.5>,\n"));
   prompt.append(_T("    \"useWeekdayFactors\": true/false\n"));
   prompt.append(_T("  },\n"));
   prompt.append(_T("  \"changeDetection\": {\n"));
   prompt.append(_T("    \"enabled\": true/false,\n"));
   prompt.append(_T("    \"maxRatePerMinute\": <maximum acceptable rate of change per minute>\n"));
   prompt.append(_T("  },\n"));
   prompt.append(_T("  \"sustainedHighDetection\": {\n"));
   prompt.append(_T("    \"enabled\": true/false,\n"));
   prompt.append(_T("    \"threshold\": <high value threshold>,\n"));
   prompt.append(_T("    \"durationMinutes\": <minutes sustained before anomaly>\n"));
   prompt.append(_T("  },\n"));
   prompt.append(_T("  \"suddenDropDetection\": {\n"));
   prompt.append(_T("    \"enabled\": true/false,\n"));
   prompt.append(_T("    \"dropPercent\": <percentage drop to trigger anomaly>\n"));
   prompt.append(_T("  }\n"));
   prompt.append(_T("}\n\n"));

   prompt.append(_T("GUIDELINES:\n"));
   prompt.append(_T("- Consider the metric's nature based on its name and description\n"));
   prompt.append(_T("- Use the provided statistics to set realistic thresholds\n"));
   prompt.append(_T("- Prefer fewer false positives for production stability\n"));
   prompt.append(_T("- Enable only detection methods that make sense for this metric type\n"));
   prompt.append(_T("- For metrics that should never go below zero, set hardBounds.min to 0\n"));
   prompt.append(_T("- For percentage metrics, set hardBounds.max to 100\n"));
   prompt.append(_T("- Set sensitivity based on observed variability in the data\n"));
   prompt.append(_T("- Respond ONLY with the JSON object, no additional text\n"));

   MemFree(statsJson);
   return prompt.getUTF8StdString();
}

/**
 * Parse LLM response and build final profile JSON
 */
static json_t *ParseLLMResponseAndBuildProfile(const char *llmResponse, json_t *statistics)
{
   // Extract JSON from response (LLM might add markdown code blocks)
   const char *jsonStart = llmResponse;
   const char *p = strstr(llmResponse, "```json");
   if (p != nullptr)
   {
      jsonStart = p + 7;
      while (*jsonStart == '\n' || *jsonStart == '\r')
         jsonStart++;
   }
   else
   {
      p = strstr(llmResponse, "```");
      if (p != nullptr)
      {
         jsonStart = p + 3;
         while (*jsonStart == '\n' || *jsonStart == '\r')
            jsonStart++;
      }
   }

   // Find the start of JSON object
   while (*jsonStart != '{' && *jsonStart != 0)
      jsonStart++;

   if (*jsonStart == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ParseLLMResponse: no JSON object found in response"));
      return nullptr;
   }

   // Find matching closing brace
   int braceCount = 0;
   const char *jsonEnd = jsonStart;
   while (*jsonEnd != 0)
   {
      if (*jsonEnd == '{')
         braceCount++;
      else if (*jsonEnd == '}')
      {
         braceCount--;
         if (braceCount == 0)
         {
            jsonEnd++;
            break;
         }
      }
      jsonEnd++;
   }

   // Extract JSON substring
   size_t jsonLen = jsonEnd - jsonStart;
   char *jsonStr = MemAllocStringA(jsonLen + 1);
   memcpy(jsonStr, jsonStart, jsonLen);
   jsonStr[jsonLen] = 0;

   // Parse LLM parameters
   json_error_t error;
   json_t *llmParams = json_loads(jsonStr, 0, &error);
   MemFree(jsonStr);

   if (llmParams == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ParseLLMResponse: failed to parse JSON at line %d: %hs"), error.line, error.text);
      return nullptr;
   }

   // Build final profile
   json_t *profile = json_object();
   json_object_set_new(profile, "version", json_integer(1));
   json_object_set_new(profile, "generatedAt", json_integer(time(nullptr)));

   // Copy data range from statistics
   json_t *dataRange = json_object_get(statistics, "dataRange");
   if (dataRange != nullptr)
      json_object_set(profile, "dataRange", dataRange);

   // Copy hourly baselines from statistics
   json_t *hourlyBaselines = json_object_get(statistics, "hourlyBaselines");
   if (hourlyBaselines != nullptr)
      json_object_set(profile, "hourlyBaselines", hourlyBaselines);

   // Copy weekday factors from statistics
   json_t *weekdayFactors = json_object_get(statistics, "weekdayFactors");
   if (weekdayFactors != nullptr)
      json_object_set(profile, "weekdayFactors", weekdayFactors);

   // Copy LLM-provided detection parameters
   json_t *hardBounds = json_object_get(llmParams, "hardBounds");
   if (hardBounds != nullptr)
      json_object_set(profile, "hardBounds", hardBounds);

   json_t *seasonalDetection = json_object_get(llmParams, "seasonalDetection");
   if (seasonalDetection != nullptr)
      json_object_set(profile, "seasonalDetection", seasonalDetection);

   json_t *changeDetection = json_object_get(llmParams, "changeDetection");
   if (changeDetection != nullptr)
      json_object_set(profile, "changeDetection", changeDetection);

   json_t *sustainedHighDetection = json_object_get(llmParams, "sustainedHighDetection");
   if (sustainedHighDetection != nullptr)
      json_object_set(profile, "sustainedHighDetection", sustainedHighDetection);

   json_t *suddenDropDetection = json_object_get(llmParams, "suddenDropDetection");
   if (suddenDropDetection != nullptr)
      json_object_set(profile, "suddenDropDetection", suddenDropDetection);

   json_decref(llmParams);
   return profile;
}

/**
 * Generate anomaly profile for a DCI
 */
bool NXCORE_EXPORTABLE GenerateAnomalyProfile(const shared_ptr<DCItem>& dci, const shared_ptr<DataCollectionTarget>& owner)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("GenerateAnomalyProfile: starting for DCI \"%s\" [%u] on %s [%u]"),
      dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId());

   // Get object AI hint (available on all object types)
   SharedString objectAiHint = owner->getAIHint();

   // Calculate time range
   time_t timeTo = time(nullptr);
   time_t timeFrom = timeTo - (DEFAULT_DATA_RANGE_DAYS * 86400);

   // Compute statistics
   json_t *statistics = ComputeStatistics(owner->getId(), dci->getId(), dci->getStorageClass(), timeFrom, timeTo);
   if (statistics == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GenerateAnomalyProfile: failed to compute statistics for DCI [%u]"), dci->getId());
      return false;
   }

   // Build LLM prompt
   std::string prompt = BuildLLMPrompt(
      dci->getName().cstr(),
      dci->getDescription().cstr(),
      dci->getUnitName().cstr(),
      dci->getEffectivePollingInterval(),
      owner->getName(),
      objectAiHint.cstr(),
      dci->getAIHint().cstr(),
      statistics);

   nxlog_debug_tag(DEBUG_TAG, 7, _T("GenerateAnomalyProfile: LLM prompt length=%d"), static_cast<int>(prompt.length()));

   // Query LLM
   char *llmResponse = QueryAIAssistant(prompt.c_str(), nullptr, 4);
   if (llmResponse == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GenerateAnomalyProfile: LLM query failed for DCI [%u]"), dci->getId());
      json_decref(statistics);
      return false;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("GenerateAnomalyProfile: LLM response: %hs"), llmResponse);

   // Parse response and build profile
   json_t *profileJson = ParseLLMResponseAndBuildProfile(llmResponse, statistics);
   MemFree(llmResponse);
   json_decref(statistics);

   if (profileJson == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("GenerateAnomalyProfile: failed to parse LLM response for DCI [%u]"), dci->getId());
      return false;
   }

   dci->setAnomalyProfile(profileJson);
   json_decref(profileJson);

   // Cause database update
   // TODO: optimize to update only anomaly profile field
   owner->markAsModified(MODIFY_DATA_COLLECTION);

   nxlog_debug_tag(DEBUG_TAG, 4, _T("GenerateAnomalyProfile: successfully generated profile for DCI \"%s\" [%u]"),
      dci->getName().cstr(), dci->getId());

   return true;
}

/**
 * Generate anomaly profile asynchronously
 */
void NXCORE_EXPORTABLE GenerateAnomalyProfileAsync(uint32_t dciId, uint32_t nodeId)
{
   ThreadPoolExecute(g_mainThreadPool,
      [dciId, nodeId]() -> void
      {
         shared_ptr<NetObj> node = FindObjectById(nodeId);
         if (node == nullptr || !node->isDataCollectionTarget())
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("GenerateAnomalyProfileAsync: node [%u] not found or not a DC target"), nodeId);
            return;
         }

         shared_ptr<DataCollectionTarget> dcTarget = static_pointer_cast<DataCollectionTarget>(node);
         shared_ptr<DCObject> dco = dcTarget->getDCObjectById(dciId, 0);
         if (dco == nullptr || dco->getType() != DCO_TYPE_ITEM)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("GenerateAnomalyProfileAsync: DCI [%u] not found on node [%u]"), dciId, nodeId);
            return;
         }

         shared_ptr<DCItem> dci = static_pointer_cast<DCItem>(dco);
         GenerateAnomalyProfile(dci, dcTarget);
      });
}

/**
 * Scheduled task handler for regenerating anomaly profiles
 */
void RegenerateAnomalyProfiles(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("RegenerateAnomalyProfiles: starting scheduled profile regeneration"));

   int retryCount = 0;
   if (parameters->m_persistentData != nullptr && parameters->m_persistentData[0] != 0)
   {
      retryCount = _tcstol(parameters->m_persistentData, nullptr, 10);
   }

   int successCount = 0;
   int failedCount = 0;
   int skippedCount = 0;
   time_t now = time(nullptr);

   // Find all DCIs with AI anomaly detection enabled
   g_idxObjectById.forEach(
      [&successCount, &failedCount, &skippedCount, now](NetObj *object) -> EnumerationCallbackResult
      {
         if (!object->isDataCollectionTarget())
            return _CONTINUE;

         DataCollectionTarget *dcTarget = static_cast<DataCollectionTarget*>(object);
         shared_ptr<SharedObjectArray<DCObject>> dcObjects = dcTarget->getAllDCObjects();

         for (int i = 0; i < dcObjects->size(); i++)
         {
            DCObject *dco = dcObjects->get(i);
            if (dco->getType() != DCO_TYPE_ITEM)
               continue;

            DCItem *dci = static_cast<DCItem*>(dco);

            // Check if AI anomaly detection is enabled
            if (!dci->inAnomalyDetectionByAIEnabled())
               continue;

            // Check cooldown
            time_t profileTimestamp = dci->getAnomalyProfileTimestamp();
            if (profileTimestamp > 0 && (now - profileTimestamp) < PROFILE_REGENERATION_COOLDOWN)
            {
               skippedCount++;
               continue;
            }

            // Generate profile
            shared_ptr<NetObj> ownerObj = FindObjectById(dcTarget->getId());
            if (ownerObj == nullptr || !ownerObj->isDataCollectionTarget())
            {
               failedCount++;
               continue;
            }

            shared_ptr<DataCollectionTarget> owner = static_pointer_cast<DataCollectionTarget>(ownerObj);
            shared_ptr<DCObject> dcoShared = owner->getDCObjectById(dci->getId(), 0);
            if (dcoShared == nullptr || dcoShared->getType() != DCO_TYPE_ITEM)
            {
               failedCount++;
               continue;
            }

            shared_ptr<DCItem> dciShared = static_pointer_cast<DCItem>(dcoShared);
            if (GenerateAnomalyProfile(dciShared, owner))
            {
               successCount++;
            }
            else
            {
               failedCount++;
            }
         }
         return _CONTINUE;
      });

   nxlog_debug_tag(DEBUG_TAG, 3, _T("RegenerateAnomalyProfiles: completed - success=%d, failed=%d, skipped=%d"),
      successCount, failedCount, skippedCount);

   // Schedule retry if there were failures and we haven't exceeded retry count
   if (failedCount > 0 && retryCount < MAX_RETRY_COUNT)
   {
      TCHAR retryData[16];
      _sntprintf(retryData, 16, _T("%d"), retryCount + 1);
      AddOneTimeScheduledTask(L"System.RegenerateAnomalyProfiles", time(nullptr) + 3600, retryData, nullptr, 0, 0, SYSTEM_ACCESS_FULL);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RegenerateAnomalyProfiles: scheduled retry %d in 1 hour"), retryCount + 1);
   }
}
