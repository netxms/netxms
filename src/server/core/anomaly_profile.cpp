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
#include <algorithm>
#include <cmath>

#define DEBUG_TAG          DEBUG_TAG_DC_ANOMALY
#define DEBUG_TAG_TUNING   L"ai.anomaly"

/**
 * Length of history used for training: raw samples (spread of single sample and of change between samples)
 * and hourly averages (expected level), in seconds
 */
#define RAW_TRAINING_RANGE       (30 * 86400)
#define HOURLY_TRAINING_RANGE    (84 * 86400)

/**
 * Most recent data is not used for training so that ongoing level shift is not absorbed by the
 * baseline before drift detector can report it (in seconds). Reduced while there is less history
 * than minimum number of training days.
 */
#define TRAINING_LAG             (7 * 86400)

/**
 * Settings suggested by LLM are reused for given time (in seconds)
 */
#define TUNING_LIFETIME          (30 * 86400)

/**
 * Number of anomaly profiles trained in parallel
 */
#define TRAINING_CONCURRENCY     4

/**
 * Maximum number of hourly aggregates loaded for training
 */
#define MAX_HOURLY_RECORDS       10000

/**
 * Load collected values of given DCI (in chronological order)
 */
static bool LoadRawSamples(const DataCollectionTarget& owner, const DCItem& dci, time_t timeFrom, time_t timeTo, std::vector<AnomalySample> *samples)
{
   bool hasV5Table = owner.hasV5IdataTable() && !(g_flags & AF_SINGLE_TABLE_PERF_DATA);

   StringBuffer query;
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         query.append(L"SELECT timestamptz_to_ms(idata_timestamp),idata_value FROM idata_sc_");
         query.append(DCObject::getStorageClassName(dci.getStorageClass()));
         query.append(L" WHERE item_id=? AND idata_timestamp BETWEEN ms_to_timestamptz(?) AND ms_to_timestamptz(?)");
      }
      else
      {
         query.append(L"SELECT idata_timestamp,idata_value FROM idata WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?");
      }
   }
   else if (hasV5Table)
   {
      query.append(L"SELECT idata_timestamp,idata_value FROM (SELECT idata_timestamp,idata_value FROM idata_");
      query.append(owner.getId());
      query.append(L" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?");
      query.append(L" UNION ALL SELECT ");
      query.append(V5TimestampToMs(false));
      query.append(L",idata_value FROM idata_v5_");
      query.append(owner.getId());
      query.append(L" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?) d");
   }
   else
   {
      query.append(L"SELECT idata_timestamp,idata_value FROM idata_");
      query.append(owner.getId());
      query.append(L" WHERE item_id=? AND idata_timestamp BETWEEN ? AND ?");
   }
   query.append(L" ORDER BY idata_timestamp");

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dci.getId());
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, static_cast<int64_t>(timeFrom) * 1000);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, static_cast<int64_t>(timeTo) * 1000);
      if (hasV5Table)
      {
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, dci.getId());
         DBBind(hStmt, 5, DB_SQLTYPE_BIGINT, static_cast<int64_t>(timeFrom));
         DBBind(hStmt, 6, DB_SQLTYPE_BIGINT, static_cast<int64_t>(timeTo));
      }

      DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
      if (hResult != nullptr)
      {
         while(DBFetch(hResult))
         {
            AnomalySample sample;
            sample.timestamp = DBGetFieldTimestamp(hResult, 0).asTime();
            sample.value = DBGetFieldDouble(hResult, 1);
            samples->push_back(sample);
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
 * Load hourly averages of given DCI from aggregated data (in chronological order). Returns false
 * if hourly aggregation is not active for the DCI or data cannot be read.
 */
static bool LoadHourlySamples(const DataCollectionTarget& owner, const DCItem& dci, time_t timeFrom, time_t timeTo, std::vector<AnomalySample> *samples)
{
   int64_t startMs = static_cast<int64_t>(timeFrom) * 1000;
   int64_t endMs = static_cast<int64_t>(timeTo) * 1000;
   if (ResolveDciTier(DCI_TIER_HOURLY, dci, DCO_TYPE_ITEM, startMs, endMs, owner.getRuntimeFlags(), 0) != DCI_TIER_HOURLY)
      return false;

   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = PrepareTieredDataSelect(hdb, owner.getId(), DCI_TIER_HOURLY, DCI_HAGG_AVG, MAX_HOURLY_RECORDS,
      (g_dbSyntax == DB_SYNTAX_TSDB) ? L" AND bucket_start BETWEEN ms_to_timestamptz(?) AND ms_to_timestamptz(?)" : L" AND bucket_start BETWEEN ? AND ?");
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dci.getId());
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, startMs);
      DBBind(hStmt, 3, DB_SQLTYPE_BIGINT, endMs);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         // Rows are ordered from newest to oldest
         int count = DBGetNumRows(hResult);
         samples->reserve(count);
         for(int i = count - 1; i >= 0; i--)
         {
            AnomalySample sample;
            sample.timestamp = static_cast<time_t>(DBGetFieldInt64(hResult, i, 0) / 1000);
            sample.value = DBGetFieldDouble(hResult, i, 1);
            samples->push_back(sample);
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
 * Remove samples newer than given time from chronologically ordered series
 */
static void RemoveSamplesAfter(std::vector<AnomalySample> *samples, time_t timestamp)
{
   while(!samples->empty() && (samples->back().timestamp > timestamp))
      samples->pop_back();
}

/**
 * Copy part of chronologically ordered series that is within given time range (inclusive)
 */
static void CopySamples(const std::vector<AnomalySample>& source, time_t timeFrom, time_t timeTo, std::vector<AnomalySample> *destination)
{
   auto first = std::lower_bound(source.begin(), source.end(), timeFrom, [](const AnomalySample& s, time_t t) { return s.timestamp < t; });
   auto last = std::upper_bound(first, source.end(), timeTo, [](time_t t, const AnomalySample& s) { return t < s.timestamp; });
   destination->assign(first, last);
}

/**
 * Select data for model training at given moment. Used both for training of live models and for replay of
 * recorded history, so that replayed models are trained on exactly same data as live ones would be.
 *
 * Expected level is trained from hourly averages. Aggregated data is used if it covers at least same period
 * as collected values and its buckets match hours of local time (they do not if time zone offset is not a
 * whole number of hours), otherwise averages are calculated from collected values.
 *
 * @param collected collected values in chronological order
 * @param aggregated hourly aggregates in chronological order (empty if not available)
 * @param baselineStart data before that time is not used
 * @param now moment of training
 * @param minTrainingDays minimum number of days in training data (training lag is reduced to get it)
 * @param raw selected collected values
 * @param hourly selected hourly averages (empty if there is no data for training)
 * @return true if hourly averages were taken from aggregated data
 */
static bool SelectTrainingData(const std::vector<AnomalySample>& collected, const std::vector<AnomalySample>& aggregated,
   time_t baselineStart, time_t now, int32_t minTrainingDays, std::vector<AnomalySample> *raw, std::vector<AnomalySample> *hourly)
{
   CopySamples(collected, std::max(baselineStart, now - TRAINING_LAG - RAW_TRAINING_RANGE), now, raw);
   CopySamples(aggregated, std::max(baselineStart, now - TRAINING_LAG - HOURLY_TRAINING_RANGE), now, hourly);

   bool hourlyTier = !hourly->empty() && (raw->empty() || (hourly->front().timestamp < raw->front().timestamp + 86400));
   for(size_t i = 0; hourlyTier && (i < hourly->size()); i++)
      hourlyTier = (GetAnomalyHourStart((*hourly)[i].timestamp) == (*hourly)[i].timestamp);
   if (!hourlyTier)
   {
      hourly->clear();
      AggregateAnomalySamplesHourly(*raw, hourly);
   }
   if (hourly->empty())
      return false;

   time_t trainingEnd = now - TRAINING_LAG;
   time_t minTrainingRange = static_cast<time_t>(minTrainingDays) * 86400;
   if (trainingEnd - hourly->front().timestamp < minTrainingRange)
      trainingEnd = std::min(now, hourly->front().timestamp + minTrainingRange);
   RemoveSamplesAfter(hourly, trainingEnd);
   RemoveSamplesAfter(raw, trainingEnd);
   return hourlyTier;
}

/**
 * Get given quantile from sorted non-empty vector
 */
static inline double SortedQuantile(const std::vector<double>& values, double q)
{
   return values[static_cast<size_t>(q * (values.size() - 1))];
}

/**
 * Create summary of training data and trained model for LLM
 */
static json_t *CreateTrainingSummary(const AnomalyModel& model, const std::vector<AnomalySample>& hourly)
{
   std::vector<double> values;
   values.reserve(hourly.size());
   for(size_t i = 0; i < hourly.size(); i++)
      if (std::isfinite(hourly[i].value))
         values.push_back(hourly[i].value);
   std::sort(values.begin(), values.end());

   json_t *summary = json_object();
   json_object_set_new(summary, "daysOfData", json_integer(model.getTrainingDays()));
   if (!values.empty())
   {
      json_t *distribution = json_object();
      json_object_set_new(distribution, "min", json_real(values.front()));
      json_object_set_new(distribution, "p1", json_real(SortedQuantile(values, 0.01)));
      json_object_set_new(distribution, "p5", json_real(SortedQuantile(values, 0.05)));
      json_object_set_new(distribution, "median", json_real(SortedQuantile(values, 0.5)));
      json_object_set_new(distribution, "p95", json_real(SortedQuantile(values, 0.95)));
      json_object_set_new(distribution, "p99", json_real(SortedQuantile(values, 0.99)));
      json_object_set_new(distribution, "max", json_real(values.back()));
      json_object_set_new(summary, "hourlyAverageDistribution", distribution);
   }
   json_object_set_new(summary, "seriesIsMonotone", json_boolean(model.getMode() == AnomalyDetectionMode::RATE));
   json_object_set_new(summary, "individualDaysOfWeekDiffer", json_boolean(model.isUsingWeekdays()));
   json_object_set_new(summary, "spreadOfHourlyAverage", json_real(model.getDriftScale()));

   // Expected level and spread of single sample by hour of day, for one weekday and one weekend day
   json_t *weekday = json_array(), *weekend = json_array();
   bool weekdayDone = false, weekendDone = false;
   for(time_t t = model.getTrainedTo() - 7 * 86400; (t <= model.getTrainedTo()) && !(weekdayDone && weekendDone); t += 3600)
   {
      struct tm lt;
#ifdef _WIN32
      localtime_s(&lt, &t);
#else
      localtime_r(&t, &lt);
#endif
      bool isWeekend = (lt.tm_wday == 0);
      if ((!isWeekend && (lt.tm_wday != 3)) || (isWeekend ? weekendDone : weekdayDone))
         continue;

      json_t *list = isWeekend ? weekend : weekday;
      if (json_array_size(list) != static_cast<size_t>(lt.tm_hour))
         continue;   // wait for start of the day, and skip hour repeated because of time shift

      ExpectedValue ev = model.getExpectedValue(t);
      json_t *element = json_object();
      json_object_set_new(element, "hour", json_integer(lt.tm_hour));
      json_object_set_new(element, "expected", json_real(ev.center));
      json_object_set_new(element, "spread", json_real(ev.scale));
      json_array_append_new(list, element);
      if (json_array_size(list) == 24)
      {
         if (isWeekend)
            weekendDone = true;
         else
            weekdayDone = true;
      }
   }
   json_object_set_new(summary, model.getMode() == AnomalyDetectionMode::RATE ? "wednesdayRateBaseline" : "wednesdayBaseline", weekday);
   json_object_set_new(summary, model.getMode() == AnomalyDetectionMode::RATE ? "sundayRateBaseline" : "sundayBaseline", weekend);
   return summary;
}

/**
 * Build LLM prompt for tuning of anomaly detection settings
 */
static std::string BuildTuningPrompt(const DCItem& dci, const DataCollectionTarget& owner, json_t *summary)
{
   StringBuffer prompt;
   prompt.append(L"You are tuning statistical anomaly detection for a network monitoring metric.\n\n");
   prompt.append(L"The detector learns expected level and normal spread of the metric for each hour of day and type of day, ");
   prompt.append(L"and reports an anomaly when a value deviates from expected level by more than \"sensitivity\" standard deviations, ");
   prompt.append(L"when change between consecutive values is unusually large, or when level stays shifted for hours.\n\n");

   prompt.append(L"METRIC INFORMATION:\n- Name: ");
   prompt.append(dci.getName().cstr());
   prompt.append(L"\n");

   SharedString description = dci.getDescription();
   if (!description.isEmpty())
   {
      prompt.append(L"- Description: ");
      prompt.append(description.cstr());
      prompt.append(L"\n");
   }

   SharedString unitName = dci.getUnitName();
   if (!unitName.isEmpty())
   {
      prompt.append(L"- Unit: ");
      prompt.append(unitName.cstr());
      prompt.append(L"\n");
   }

   prompt.appendFormattedString(L"- Polling interval: %d seconds\n- Object: %s\n\n", dci.getEffectivePollingInterval(), owner.getName());

   SharedString objectHint = owner.getAIHint();
   if (!objectHint.isEmpty())
   {
      prompt.append(L"OBJECT CONTEXT:\n");
      prompt.append(objectHint.cstr());
      prompt.append(L"\n\n");
   }

   SharedString dciHint = dci.getAIHint();
   if (!dciHint.isEmpty())
   {
      prompt.append(L"METRIC CONTEXT:\n");
      prompt.append(dciHint.cstr());
      prompt.append(L"\n\n");
   }

   char *summaryText = json_dumps(summary, JSON_INDENT(1));
   prompt.append(L"STATISTICS LEARNED FROM HISTORICAL DATA:\n");
   prompt.appendUtf8String(summaryText, strlen(summaryText));
   MemFree(summaryText);

   prompt.append(L"\n\nProvide settings as a JSON object with the following structure (omit any element to keep the default):\n"
      L"{\n"
      L"  \"sensitivity\": <number of standard deviations, from 2.0 to 8.0; default is 3.5, higher means fewer anomalies>,\n"
      L"  \"direction\": \"both\" | \"up\" | \"down\",\n"
      L"  \"mode\": \"value\" | \"rate\",\n"
      L"  \"hardBounds\": { \"min\": <number>, \"max\": <number> }\n"
      L"}\n\n"
      L"GUIDELINES:\n"
      L"- Consider the nature of the metric based on its name, description and context\n"
      L"- Prefer fewer false positives for production stability\n"
      L"- Use direction \"up\" when only growth is a problem (errors, latency, usage) and \"down\" when only decrease is a problem (free space, throughput of a busy service)\n"
      L"- Use mode \"rate\" only for values that normally only grow or only shrink (counters, used disk space), so that rate of change is checked instead of the value\n"
      L"- Set hard bounds only for values that are impossible or always wrong for this metric regardless of history\n"
      L"- Respond ONLY with the JSON object, no additional text\n");

   return prompt.getUTF8StdString();
}

/**
 * Parse LLM response and create validated tuning document (nullptr if response cannot be used)
 */
static json_t *ParseTuningResponse(const char *response)
{
   // Response may have text or code block markup around JSON object
   const char *start = strchr(response, '{');
   if (start == nullptr)
      return nullptr;

   int depth = 0;
   const char *end = start;
   while(*end != 0)
   {
      if (*end == '{')
      {
         depth++;
      }
      else if (*end == '}')
      {
         depth--;
         if (depth == 0)
         {
            end++;
            break;
         }
      }
      end++;
   }

   json_error_t error;
   json_t *suggested = json_loadb(start, end - start, 0, &error);
   if (!json_is_object(suggested))
   {
      nxlog_debug_tag(DEBUG_TAG_TUNING, 5, L"Cannot parse LLM response (%hs)", error.text);
      json_decref(suggested);
      return nullptr;
   }

   json_t *tuning = json_object();
   json_object_set_new(tuning, "generatedAt", json_integer(time(nullptr)));

   json_t *sensitivity = json_object_get(suggested, "sensitivity");
   if (json_is_number(sensitivity))
      json_object_set_new(tuning, "sensitivity", json_real(std::min(std::max(json_number_value(sensitivity), 2.0), 8.0)));

   const char *direction = json_object_get_string_utf8(suggested, "direction", "");
   if (!strcmp(direction, "both") || !strcmp(direction, "up") || !strcmp(direction, "down"))
      json_object_set_new(tuning, "direction", json_string(direction));

   const char *mode = json_object_get_string_utf8(suggested, "mode", "");
   if (!strcmp(mode, "value") || !strcmp(mode, "rate"))
      json_object_set_new(tuning, "mode", json_string(mode));

   json_t *hardBounds = json_object_get(suggested, "hardBounds");
   if (json_is_object(hardBounds))
   {
      json_t *bounds = json_object();
      json_t *min = json_object_get(hardBounds, "min");
      json_t *max = json_object_get(hardBounds, "max");
      if (json_is_number(min))
         json_object_set_new(bounds, "min", json_real(json_number_value(min)));
      if (json_is_number(max) && (!json_is_number(min) || (json_number_value(max) > json_number_value(min))))
         json_object_set_new(bounds, "max", json_real(json_number_value(max)));
      if (json_object_size(bounds) > 0)
         json_object_set(tuning, "hardBounds", bounds);
      json_decref(bounds);
   }

   json_decref(suggested);
   return tuning;
}

/**
 * Ask LLM for anomaly detection settings matching nature of the metric (returns nullptr on failure)
 */
static json_t *RequestTuning(const DCItem& dci, const DataCollectionTarget& owner, const AnomalyModel& model, const std::vector<AnomalySample>& hourly)
{
   json_t *summary = CreateTrainingSummary(model, hourly);
   std::string prompt = BuildTuningPrompt(dci, owner, summary);
   json_decref(summary);

   char *response = QueryAIAssistant(prompt.c_str(), nullptr, "analytical", false);
   if (response == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TUNING, 5, L"LLM query failed for DCI \"%s\" [%u] on %s [%u]", dci.getName().cstr(), dci.getId(), owner.getName(), owner.getId());
      return nullptr;
   }

   nxlog_debug_tag(DEBUG_TAG_TUNING, 7, L"LLM response for DCI [%u]: %hs", dci.getId(), response);
   json_t *tuning = ParseTuningResponse(response);
   MemFree(response);
   if (tuning != nullptr)
   {
      char *text = json_dumps(tuning, JSON_COMPACT);
      nxlog_debug_tag(DEBUG_TAG_TUNING, 5, L"LLM tuning for DCI \"%s\" [%u] on %s [%u]: %hs", dci.getName().cstr(), dci.getId(), owner.getName(), owner.getId(), text);
      MemFree(text);
   }
   return tuning;
}

/**
 * Use detection mode suggested by LLM if mode is not set explicitly
 */
static void ApplyTunedMode(AnomalySettings *settings, json_t *tuning)
{
   if (settings->mode != AnomalyDetectionMode::AUTO)
      return;

   const char *mode = json_object_get_string_utf8(tuning, "mode", "");
   if (!strcmp(mode, "value"))
      settings->mode = AnomalyDetectionMode::VALUE;
   else if (!strcmp(mode, "rate"))
      settings->mode = AnomalyDetectionMode::RATE;
}

/**
 * Train anomaly model for given DCI and store it in the DCI and in database. Existing model is kept if new one cannot be trained.
 *
 * @param refreshTuning true to request new settings from LLM (if LLM tuning is enabled) even if existing ones are recent
 * @return true if new model was trained
 */
static bool TrainAnomalyModel(const shared_ptr<DCItem>& dci, const shared_ptr<DataCollectionTarget>& owner, bool refreshTuning)
{
   if (!dci->isAnomalyDetectionEnabled())
      return false;

   if (!dci->isDataStorageEnabled())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DCI \"%s\" [%u] on %s [%u]: collected values are not stored, anomaly detection is not possible",
         dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId());
      return false;
   }
   if (dci->getSampleSaveInterval() > 1)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DCI \"%s\" [%u] on %s [%u]: only every %d-th value is stored, anomaly detection will be less accurate",
         dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId(), dci->getSampleSaveInterval());
   }
   if (dci->getEffectiveRetentionTime() * 86400 < TRAINING_LAG + RAW_TRAINING_RANGE)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DCI \"%s\" [%u] on %s [%u]: retention time (%d days) is shorter than training range (%d days), anomaly detection will be less accurate",
         dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId(), dci->getEffectiveRetentionTime(), (TRAINING_LAG + RAW_TRAINING_RANGE) / 86400);
   }

   AnomalySettings settings = dci->getAnomalySettings();
   time_t now = time(nullptr);
   time_t baselineStart = dci->getAnomalyBaselineStart();

   std::vector<AnomalySample> collected;
   if (!LoadRawSamples(*owner, *dci, std::max(baselineStart, now - TRAINING_LAG - RAW_TRAINING_RANGE), now, &collected))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"DCI \"%s\" [%u] on %s [%u]: cannot load collected values", dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId());
      return false;
   }

   std::vector<AnomalySample> aggregated;
   if (!LoadHourlySamples(*owner, *dci, std::max(baselineStart, now - TRAINING_LAG - HOURLY_TRAINING_RANGE), now, &aggregated))
      aggregated.clear();

   std::vector<AnomalySample> raw, hourly;
   bool hourlyTier = SelectTrainingData(collected, aggregated, baselineStart, now, settings.minTrainingDays, &raw, &hourly);
   if (hourly.empty())
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DCI \"%s\" [%u] on %s [%u]: no data for training", dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId());
      return false;
   }

   json_t *tuning = dci->isAnomalyTuningEnabled() ? dci->getAnomalyTuning() : nullptr;
   AnomalySettings trainingSettings = settings;
   ApplyTunedMode(&trainingSettings, tuning);

   shared_ptr<AnomalyModel> model = AnomalyModel::train(hourly, raw, trainingSettings);
   if (model == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DCI \"%s\" [%u] on %s [%u]: no data for training", dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId());
      json_decref(tuning);
      return false;
   }

   if (dci->isAnomalyTuningEnabled() && !model->isLearning(settings) &&
       (refreshTuning || (tuning == nullptr) || (now - json_object_get_time(tuning, "generatedAt") > TUNING_LIFETIME)))
   {
      json_t *newTuning = RequestTuning(*dci, *owner, *model, hourly);
      if (newTuning != nullptr)
      {
         json_decref(tuning);
         tuning = newTuning;

         trainingSettings = settings;
         ApplyTunedMode(&trainingSettings, tuning);
         if ((trainingSettings.mode != AnomalyDetectionMode::AUTO) && (trainingSettings.mode != model->getMode()))
            model = AnomalyModel::train(hourly, raw, trainingSettings);
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"DCI \"%s\" [%u] on %s [%u]: model trained (%s mode, %d days, %d hourly %s, %d collected values, %d slots at spread floor%s%s)",
      dci->getName().cstr(), dci->getId(), owner->getName(), owner->getId(), (model->getMode() == AnomalyDetectionMode::RATE) ? L"rate" : L"value",
      model->getTrainingDays(), model->getHourlySampleCount(), hourlyTier ? L"aggregates" : L"averages", model->getRawSampleCount(), model->getFlooredSlotCount(),
      model->isUsingWeekdays() ? L", individual days of week" : L"", model->isLearning(settings) ? L", learning" : L"");

   dci->setAnomalyProfile(model, tuning);
   json_decref(tuning);
   dci->saveAnomalyProfileToDatabase();
   return true;
}

/**
 * Sequence of consecutive anomalous values found during replay
 */
struct AnomalyEpisode
{
   time_t start;
   time_t end;
   AnomalyType type;       // Type of first anomaly in sequence
   int32_t samples;
   AnomalyScore peak;      // Score with largest magnitude among anomalies of same type as first one
   double peakValue;
   MutableString slot;
   bool labelled;          // Overlaps with at least one labelled time range

   json_t *toJson() const
   {
      json_t *json = json_object();
      json_object_set_new(json, "start", json_integer(start));
      json_object_set_new(json, "end", json_integer(end));
      json_object_set_new(json, "type", json_string(AnomalyTypeName(type)));
      json_object_set_new(json, "samples", json_integer(samples));
      json_object_set_new(json, "score", json_real(peak.score));
      json_object_set_new(json, "value", json_real(peakValue));
      if (peak.scored)
      {
         json_object_set_new(json, "expectedLow", json_real(peak.expectedLow));
         json_object_set_new(json, "expectedHigh", json_real(peak.expectedHigh));
      }
      json_object_set_new(json, "slot", json_string_t(slot));
      json_object_set_new(json, "labelled", json_boolean(labelled));
      return json;
   }
};

/**
 * Replay recorded history of given DCI through anomaly detection (walk-forward evaluation). Model is trained
 * once per day of replayed period, as scheduled regeneration does, using only data available at that moment and
 * same selection of training data as for live models, and then used for checking values collected during that day.
 * Drift detector state is carried from day to day. Current anomaly detection settings of the DCI are used (LLM
 * tuning is used as it is now, and is not requested again); baseline start time of the DCI is ignored. Anomaly detection
 * does not have to be enabled for the DCI.
 *
 * Result is JSON object with the following elements:
 *    timeFrom, timeTo     replayed period (UNIX timestamps)
 *    samples              number of replayed values
 *    checkedSamples       number of values checked for anomalies (the rest fall on days when model was still learning)
 *    anomalousSamples     number of values reported as anomalous
 *    trainings            number of trained models
 *    days                 array with one element per replayed day: start, samples, anomalousSamples, learning
 *    episodes             array of sequences of consecutive anomalous values: start, end, type, samples, score, value,
 *                         expectedLow, expectedHigh, slot (all five for value with highest score), labelled
 *    episodesByType       number of episodes by type of first anomaly (range, step, drift)
 *    evaluation           present only if labelled time ranges are provided:
 *       events            array with one element per labelled time range: start, end, detected, delay (seconds from
 *                         start of range to first anomalous value within it)
 *       recall            share of labelled time ranges with at least one anomalous value
 *       precision         share of episodes that overlap with labelled time range
 *       detectionDelay    average delay for detected ranges, in seconds
 *       falseAlarms       number of episodes that do not overlap with any labelled time range
 *       falseAlarmsByType same by type of first anomaly
 *       falseAlarmsPerWeek false alarms per week of checked data
 *
 * @param labels time ranges (inclusive) when anomalies are expected (known incidents)
 * @return result of replay or nullptr on error
 */
json_t *DetectAnomalies(const DataCollectionTarget& owner, uint32_t dciId, time_t timeFrom, time_t timeTo, const std::vector<std::pair<time_t, time_t>>& labels)
{
   shared_ptr<DCObject> dco = owner.getDCObjectById(dciId, 0);
   if ((dco == nullptr) || (dco->getType() != DCO_TYPE_ITEM) || (timeTo <= timeFrom))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DetectAnomalies: invalid DCI ID [%u] on object %s [%u] or invalid time range", dciId, owner.getName(), owner.getId());
      return nullptr;
   }
   const DCItem& dci = static_cast<const DCItem&>(*dco);

   std::vector<AnomalySample> collected;
   if (!LoadRawSamples(owner, dci, timeFrom - TRAINING_LAG - RAW_TRAINING_RANGE, timeTo, &collected))
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"DetectAnomalies: cannot load data for DCI \"%s\" [%u] on object %s [%u]", dci.getName().cstr(), dciId, owner.getName(), owner.getId());
      return nullptr;
   }

   std::vector<AnomalySample> aggregated;
   if (!LoadHourlySamples(owner, dci, timeFrom - TRAINING_LAG - HOURLY_TRAINING_RANGE, timeTo, &aggregated))
      aggregated.clear();

   AnomalySettings settings = dci.getAnomalySettings();
   AnomalySettings scoringSettings = dci.getAnomalyEffectiveSettings();
   AnomalySettings trainingSettings = settings;
   if (dci.isAnomalyTuningEnabled())
   {
      json_t *tuning = dci.getAnomalyTuning();
      ApplyTunedMode(&trainingSettings, tuning);
      json_decref(tuning);
   }

   shared_ptr<AnomalyModel> model;
   AnomalyDetectorState state;
   std::vector<AnomalyEpisode> episodes;
   std::vector<time_t> firstDetection(labels.size(), 0);
   bool episodeOpen = false;
   int32_t trainings = 0, checkedSamples = 0, anomalousSamples = 0, checkedDays = 0;
   json_t *days = json_array();

   auto next = std::lower_bound(collected.begin(), collected.end(), timeFrom, [](const AnomalySample& s, time_t t) { return s.timestamp < t; });
   size_t replayedSamples = 0;
   for(time_t dayStart = timeFrom; dayStart < timeTo; dayStart += 86400)
   {
      std::vector<AnomalySample> raw, hourly;
      SelectTrainingData(collected, aggregated, 0, dayStart, settings.minTrainingDays, &raw, &hourly);
      shared_ptr<AnomalyModel> newModel = hourly.empty() ? shared_ptr<AnomalyModel>() : AnomalyModel::train(hourly, raw, trainingSettings);
      if (newModel != nullptr)
      {
         if ((model != nullptr) && (model->getMode() != newModel->getMode()))
            state.reset();
         model = newModel;
         trainings++;
      }

      bool learning = (model == nullptr) || model->isLearning(scoringSettings);
      int32_t daySamples = 0, dayAnomalies = 0;
      time_t dayEnd = std::min(dayStart + 86400, timeTo + 1);
      for(; (next != collected.end()) && (next->timestamp < dayEnd); ++next)
      {
         daySamples++;
         if (model == nullptr)
            continue;

         AnomalyScore score = ScoreAnomalySample(*model, scoringSettings, &state, next->timestamp, next->value);
         if (score.learning)
            continue;

         checkedSamples++;
         if (score.type == AnomalyType::NONE)
         {
            episodeOpen = false;
            continue;
         }

         dayAnomalies++;
         if (!episodeOpen)
         {
            AnomalyEpisode episode;
            episode.start = next->timestamp;
            episode.type = score.type;
            episode.samples = 0;
            episode.peak = score;
            episode.peakValue = next->value;
            episode.slot = model->describeSlot(next->timestamp);
            episode.labelled = false;
            episodes.push_back(episode);
            episodeOpen = true;
         }
         AnomalyEpisode& episode = episodes.back();
         episode.end = next->timestamp;
         episode.samples++;
         if ((score.type == episode.type) && (fabs(score.score) > fabs(episode.peak.score)))
         {
            episode.peak = score;
            episode.peakValue = next->value;
            episode.slot = model->describeSlot(next->timestamp);
         }

         for(size_t i = 0; i < labels.size(); i++)
            if ((firstDetection[i] == 0) && (next->timestamp >= labels[i].first) && (next->timestamp <= labels[i].second))
               firstDetection[i] = next->timestamp;
      }
      replayedSamples += daySamples;
      anomalousSamples += dayAnomalies;
      if (!learning && (daySamples > 0))
         checkedDays++;

      json_t *day = json_object();
      json_object_set_new(day, "start", json_integer(dayStart));
      json_object_set_new(day, "samples", json_integer(daySamples));
      json_object_set_new(day, "anomalousSamples", json_integer(dayAnomalies));
      json_object_set_new(day, "learning", json_boolean(learning));
      json_array_append_new(days, day);
   }

   int32_t episodesByType[4] = { 0, 0, 0, 0 }, falseAlarmsByType[4] = { 0, 0, 0, 0 };
   int32_t falseAlarms = 0;
   json_t *episodeList = json_array();
   for(AnomalyEpisode& episode : episodes)
   {
      for(size_t i = 0; (i < labels.size()) && !episode.labelled; i++)
         episode.labelled = (episode.start <= labels[i].second) && (episode.end >= labels[i].first);
      episodesByType[static_cast<int>(episode.type)]++;
      if (!episode.labelled)
      {
         falseAlarms++;
         falseAlarmsByType[static_cast<int>(episode.type)]++;
      }
      json_array_append_new(episodeList, episode.toJson());
   }

   json_t *result = json_object();
   json_object_set_new(result, "timeFrom", json_integer(timeFrom));
   json_object_set_new(result, "timeTo", json_integer(timeTo));
   json_object_set_new(result, "samples", json_integer(replayedSamples));
   json_object_set_new(result, "checkedSamples", json_integer(checkedSamples));
   json_object_set_new(result, "anomalousSamples", json_integer(anomalousSamples));
   json_object_set_new(result, "trainings", json_integer(trainings));
   json_object_set_new(result, "days", days);
   json_object_set_new(result, "episodes", episodeList);
   json_t *byType = json_object();
   for(int i = 1; i < 4; i++)
      json_object_set_new(byType, AnomalyTypeName(static_cast<AnomalyType>(i)), json_integer(episodesByType[i]));
   json_object_set_new(result, "episodesByType", byType);

   if (!labels.empty())
   {
      json_t *evaluation = json_object();
      json_t *events = json_array();
      int32_t detected = 0;
      int64_t totalDelay = 0;
      for(size_t i = 0; i < labels.size(); i++)
      {
         json_t *event = json_object();
         json_object_set_new(event, "start", json_integer(labels[i].first));
         json_object_set_new(event, "end", json_integer(labels[i].second));
         json_object_set_new(event, "detected", json_boolean(firstDetection[i] != 0));
         if (firstDetection[i] != 0)
         {
            detected++;
            totalDelay += firstDetection[i] - labels[i].first;
            json_object_set_new(event, "delay", json_integer(firstDetection[i] - labels[i].first));
         }
         json_array_append_new(events, event);
      }
      json_object_set_new(evaluation, "events", events);
      json_object_set_new(evaluation, "recall", json_real(static_cast<double>(detected) / labels.size()));
      if (!episodes.empty())
         json_object_set_new(evaluation, "precision", json_real(static_cast<double>(episodes.size() - falseAlarms) / episodes.size()));
      if (detected > 0)
         json_object_set_new(evaluation, "detectionDelay", json_real(static_cast<double>(totalDelay) / detected));
      json_object_set_new(evaluation, "falseAlarms", json_integer(falseAlarms));
      byType = json_object();
      for(int i = 1; i < 4; i++)
         json_object_set_new(byType, AnomalyTypeName(static_cast<AnomalyType>(i)), json_integer(falseAlarmsByType[i]));
      json_object_set_new(evaluation, "falseAlarmsByType", byType);
      if (checkedDays > 0)
         json_object_set_new(evaluation, "falseAlarmsPerWeek", json_real(falseAlarms * 7.0 / checkedDays));
      json_object_set_new(result, "evaluation", evaluation);
   }

   nxlog_debug_tag(DEBUG_TAG, 5, L"DetectAnomalies: DCI \"%s\" [%u] on object %s [%u]: %d values replayed, %d checked, %d anomalous in %d episodes, %d models trained",
      dci.getName().cstr(), dciId, owner.getName(), owner.getId(), static_cast<int>(replayedSamples), checkedSamples, anomalousSamples, static_cast<int>(episodes.size()), trainings);
   return result;
}

/**
 * Get key for serialized execution of training tasks (limits number of profiles trained in parallel)
 */
static inline void GetTrainingQueueKey(uint32_t ownerId, wchar_t *key)
{
   nx_swprintf(key, 32, L"AnomalyTraining-%u", ownerId % TRAINING_CONCURRENCY);
}

/**
 * Start new baseline for anomaly detection on given DCI and train new model in background.
 * See DCItem::startNewAnomalyBaseline() for baseline selection.
 */
void NXCORE_EXPORTABLE RegenerateAnomalyProfileAsync(uint32_t dciId, uint32_t ownerId)
{
   wchar_t key[32];
   GetTrainingQueueKey(ownerId, key);
   ThreadPoolExecuteSerialized(g_mainThreadPool, key,
      [dciId, ownerId] () -> void
      {
         shared_ptr<NetObj> object = FindObjectById(ownerId);
         if ((object == nullptr) || !object->isDataCollectionTarget())
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"RegenerateAnomalyProfileAsync: object [%u] not found or is not a data collection target", ownerId);
            return;
         }

         shared_ptr<DataCollectionTarget> owner = static_pointer_cast<DataCollectionTarget>(object);
         shared_ptr<DCObject> dco = owner->getDCObjectById(dciId, 0);
         if ((dco == nullptr) || (dco->getType() != DCO_TYPE_ITEM))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"RegenerateAnomalyProfileAsync: DCI [%u] not found on object [%u]", dciId, ownerId);
            return;
         }

         shared_ptr<DCItem> dci = static_pointer_cast<DCItem>(dco);
         time_t baselineStart = dci->startNewAnomalyBaseline();
         nxlog_debug_tag(DEBUG_TAG, 4, L"DCI \"%s\" [%u] on %s [%u]: profile regeneration requested (baseline starts at " INT64_FMT L")",
            dci->getName().cstr(), dciId, owner->getName(), ownerId, static_cast<int64_t>(baselineStart));
         if (!TrainAnomalyModel(dci, owner, true))
            dci->saveAnomalyProfileToDatabase();   // baseline start time has to be saved even without model
      });
}

/**
 * Train anomaly models for all DCIs of given data collection target
 */
static void TrainAnomalyModels(const shared_ptr<DataCollectionTarget>& owner)
{
   unique_ptr<SharedObjectArray<DCObject>> dcObjects = owner->getAllDCObjects();
   for(int i = 0; (i < dcObjects->size()) && !IsShutdownInProgress(); i++)
   {
      if (dcObjects->get(i)->getType() == DCO_TYPE_ITEM)
         TrainAnomalyModel(static_pointer_cast<DCItem>(dcObjects->getShared(i)), owner, false);
   }
}

/**
 * Scheduled task handler for regenerating anomaly profiles
 */
void RegenerateAnomalyProfiles(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   unique_ptr<SharedObjectArray<NetObj>> targets = g_idxObjectById.getObjects(
      [] (NetObj *object) -> bool
      {
         return object->isDataCollectionTarget();
      });

   nxlog_debug_tag(DEBUG_TAG, 3, L"Starting scheduled regeneration of anomaly profiles");
   for(int i = 0; i < targets->size(); i++)
   {
      shared_ptr<DataCollectionTarget> owner = static_pointer_cast<DataCollectionTarget>(targets->getShared(i));
      wchar_t key[32];
      GetTrainingQueueKey(owner->getId(), key);
      ThreadPoolExecuteSerialized(g_mainThreadPool, key,
         [owner] () -> void
         {
            TrainAnomalyModels(owner);
         });
   }
}

/**
 * Reload anomaly profiles, baseline start times, and detector states of all DCIs from database. Used
 * at cluster node activation: these are written to the database directly and are not covered by
 * the change journal, so a standby holds them as of its last object load.
 */
bool ReloadAnomalyProfiles()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb,
      L"SELECT i.item_id,i.node_id,i.anomaly_profile,i.anomaly_profile_timestamp,i.anomaly_baseline_start,r.anomaly_state "
      L"FROM items i LEFT OUTER JOIN raw_dci_values r ON r.item_id=i.item_id "
      L"WHERE i.anomaly_profile_timestamp<>0 OR i.anomaly_baseline_start<>0");
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   int count = 0;
   while(DBFetch(hResult))
   {
      shared_ptr<NetObj> owner = FindObjectById(DBGetFieldUInt32(hResult, 1));
      if ((owner == nullptr) || !owner->isDataCollectionTarget())
         continue;

      shared_ptr<DCObject> dco = static_cast<DataCollectionTarget&>(*owner).getDCObjectById(DBGetFieldUInt32(hResult, 0), 0);
      if ((dco == nullptr) || (dco->getType() != DCO_TYPE_ITEM))
         continue;

      char *profileText = DBGetFieldUTF8(hResult, 2, nullptr, 0);
      json_t *profile = (profileText != nullptr) ? json_loads(profileText, 0, nullptr) : nullptr;
      MemFree(profileText);

      char stateText[1024];
      json_t *state = (DBGetFieldUTF8(hResult, 5, stateText, 1024) != nullptr) ? json_loads(stateText, 0, nullptr) : nullptr;

      static_cast<DCItem&>(*dco).reloadAnomalyProfile(profile, static_cast<time_t>(DBGetFieldInt64(hResult, 3)), static_cast<time_t>(DBGetFieldInt64(hResult, 4)), state);
      json_decref(profile);
      json_decref(state);
      count++;
   }
   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 2, L"Anomaly profiles reloaded from database (%d DCIs)", count);
   return true;
}
