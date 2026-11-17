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
** File: anomaly_engine.cpp
**
** Numeric part of DCI anomaly detection: slot baseline training, expected value
** lookup, and scoring (range, step, and drift checks). Kept free of database and
** object dependencies so that it can be unit tested in isolation.
**
**/

#include "nxcore.h"
#include <nxcore_anomaly.h>
#include <algorithm>
#include <cmath>

/**
 * Provider and detector identification stored with settings and model
 */
#define BASELINE_PROVIDER_ID        "slot"
#define BASELINE_PROVIDER_VERSION   1
#define CHANGE_DETECTOR_ID          "cusum"

/**
 * Defaults for settings
 */
#define DEFAULT_SENSITIVITY         3.5
#define DEFAULT_MIN_TRAINING_DAYS   7
#define DEFAULT_DRIFT_K             1.0
#define DEFAULT_DRIFT_H             5.0

/**
 * Minimum number of distinct days in a slot for its center to be used
 */
#define MIN_SLOT_DAYS               6

/**
 * Minimum number of hourly values within a day for using that day in estimation of spread of daily mean residual
 */
#define MIN_HOURS_IN_DAY            12

/**
 * Minimum number of samples for spread estimation
 */
#define MIN_SPREAD_SAMPLES          30

/**
 * Day of week gets own baseline when at least given number of its hours (out of at least given number
 * of comparable hours) differ from weekday/weekend level by more than given number of between-day
 * standard deviations
 */
#define WEEKDAY_EVIDENCE_MIN_HOURS  12
#define WEEKDAY_EVIDENCE_HOURS      3
#define WEEKDAY_EVIDENCE_DEVIATION  1.5

/**
 * Series is considered monotone when at least this share of non-zero hourly changes have same sign
 */
#define MONOTONE_SHARE              0.95

/**
 * Limit for hourly z-score fed into drift detector, and how far above decision threshold drift sum can grow
 */
#define DRIFT_Z_LIMIT               4.0
#define DRIFT_HEADROOM              2.0

/**
 * Conversion factors from median absolute deviation and from 99th percentile of absolute deviation to standard deviation
 */
#define MAD_TO_SIGMA                1.4826
#define P99_TO_SIGMA                2.576

/**
 * Spread floor relative to slot center and absolute
 */
#define RELATIVE_SPREAD_FLOOR       0.001
#define ABSOLUTE_SPREAD_FLOOR       1e-9

/**
 * Names of enum values. Used in persisted settings and model, WebAPI, event parameters and AI tool output.
 */
static const char *s_typeNames[] = { "none", "range", "step", "drift" };
static const char *s_directionNames[] = { "both", "up", "down" };
static const char *s_modeNames[] = { "auto", "value", "rate" };

/**
 * Get name of anomaly type
 */
const char *AnomalyTypeName(AnomalyType type)
{
   return s_typeNames[static_cast<int>(type)];
}

/**
 * Get name of anomaly direction
 */
const char *AnomalyDirectionName(AnomalyDirection direction)
{
   return s_directionNames[static_cast<int>(direction)];
}

/**
 * Get name of anomaly detection mode
 */
const char *AnomalyDetectionModeName(AnomalyDetectionMode mode)
{
   return s_modeNames[static_cast<int>(mode)];
}

/**
 * Day names for slot description
 */
static const wchar_t *s_dayNames[] = { L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };

/**
 * Find index of given name in name list (returns default value if not found)
 */
static int IndexFromName(const char *name, const char * const *names, int count, int defaultValue)
{
   if (name == nullptr)
      return defaultValue;
   for(int i = 0; i < count; i++)
      if (!stricmp(name, names[i]))
         return i;
   return defaultValue;
}

/**
 * Get real number from JSON object (NaN if absent or not a number)
 */
static double GetRealFromJson(json_t *object, const char *tag)
{
   json_t *v = json_object_get(object, tag);
   return json_is_number(v) ? json_number_value(v) : NAN;
}

/**
 * Convert timestamp to broken down local time
 */
static inline void LocalTime(time_t t, struct tm *lt)
{
#ifdef _WIN32
   localtime_s(lt, &t);
#else
   localtime_r(&t, lt);
#endif
}

/**
 * Get start of the local time hour that given timestamp belongs to. Slots are hours of local time,
 * which are not aligned to UTC hours in time zones with offset that is not a whole number of hours.
 */
time_t GetAnomalyHourStart(time_t timestamp)
{
   struct tm lt;
   LocalTime(timestamp, &lt);
   return timestamp - lt.tm_min * 60 - lt.tm_sec;
}

/**
 * Get slot index from broken down local time
 */
static inline int SlotIndex(const struct tm& lt)
{
   return lt.tm_wday * 24 + lt.tm_hour;
}

/**
 * Get day type (0 = weekday, 1 = weekend) from day of week
 */
static inline int DayType(int wday)
{
   return ((wday == 0) || (wday == 6)) ? 1 : 0;
}

/**
 * Get given quantile of non-empty vector (elements are reordered)
 */
static double Quantile(std::vector<double>& v, double q)
{
   size_t index = static_cast<size_t>(q * (v.size() - 1));
   std::nth_element(v.begin(), v.begin() + index, v.end());
   return v[index];
}

/**
 * Get median of non-empty vector (elements are reordered)
 */
static double Median(std::vector<double>& v)
{
   size_t mid = v.size() / 2;
   std::nth_element(v.begin(), v.begin() + mid, v.end());
   double m = v[mid];
   if ((v.size() % 2) == 0)
      m = (m + *std::max_element(v.begin(), v.begin() + mid)) / 2;
   return m;
}

/**
 * Replace vector elements with absolute deviations from given center
 */
static void ToAbsoluteDeviations(std::vector<double>& v, double center)
{
   for(size_t i = 0; i < v.size(); i++)
      v[i] = fabs(v[i] - center);
}

/**
 * Raise spread to the floor if needed. Returns true if floor was applied.
 */
static bool ApplySpreadFloor(double *spread, double center, double resolution)
{
   double floor = std::max(std::max(resolution, fabs(center) * RELATIVE_SPREAD_FLOOR), ABSOLUTE_SPREAD_FLOOR);
   if (*spread >= floor)
      return false;
   *spread = floor;
   return true;
}

/**
 * Settings constructor - set defaults
 */
AnomalySettings::AnomalySettings()
{
   sensitivity = DEFAULT_SENSITIVITY;
   direction = AnomalyDirection::BOTH;
   mode = AnomalyDetectionMode::AUTO;
   minTrainingDays = DEFAULT_MIN_TRAINING_DAYS;
   hardMin = NAN;
   hardMax = NAN;
   driftK = DEFAULT_DRIFT_K;
   driftH = DEFAULT_DRIFT_H;
}

/**
 * Settings constructor - create from JSON. Missing or invalid elements get default values.
 */
AnomalySettings::AnomalySettings(json_t *json) : AnomalySettings()
{
   if (!json_is_object(json))
      return;

   double v = GetRealFromJson(json, "sensitivity");
   if ((v > 0) && (v <= 100))
      sensitivity = v;

   direction = static_cast<AnomalyDirection>(IndexFromName(json_object_get_string_utf8(json, "direction", nullptr), s_directionNames, 3, 0));
   mode = static_cast<AnomalyDetectionMode>(IndexFromName(json_object_get_string_utf8(json, "mode", nullptr), s_modeNames, 3, 0));

   int32_t days = json_object_get_int32(json, "minTrainingDays", DEFAULT_MIN_TRAINING_DAYS);
   if ((days >= 1) && (days <= 365))
      minTrainingDays = days;

   json_t *hardBounds = json_object_get(json, "hardBounds");
   if (json_is_object(hardBounds))
   {
      hardMin = GetRealFromJson(hardBounds, "min");
      hardMax = GetRealFromJson(hardBounds, "max");
   }

   json_t *detector = json_object_get(json, "detector");
   if (json_is_object(detector) && !strcmp(json_object_get_string_utf8(detector, "id", CHANGE_DETECTOR_ID), CHANGE_DETECTOR_ID))
   {
      v = GetRealFromJson(detector, "k");
      if (v >= 0)
         driftK = v;
      v = GetRealFromJson(detector, "h");
      if (v > 0)
         driftH = v;
   }
}

/**
 * Check that optional element of JSON object is a number within given range (null is accepted as "use default")
 */
static bool IsValidNumber(json_t *object, const char *tag, double min, bool minInclusive, double max)
{
   json_t *v = json_object_get(object, tag);
   if ((v == nullptr) || json_is_null(v))
      return true;
   if (!json_is_number(v))
      return false;
   double n = json_number_value(v);
   return (minInclusive ? (n >= min) : (n > min)) && (n <= max);
}

/**
 * Check that optional element of JSON object is one of given names (null is accepted as "use default")
 */
static bool IsValidName(json_t *object, const char *tag, const char * const *names, int count)
{
   json_t *v = json_object_get(object, tag);
   if ((v == nullptr) || json_is_null(v))
      return true;
   return json_is_string(v) && (IndexFromName(json_string_value(v), names, count, -1) != -1);
}

/**
 * Validate settings document received from external source. Unlike constructor, which replaces
 * invalid elements with defaults, this method reports them. Unknown elements are ignored.
 */
bool AnomalySettings::validate(json_t *json)
{
   if (!json_is_object(json))
      return false;

   if (!IsValidNumber(json, "sensitivity", 0, false, 100) ||
       !IsValidName(json, "direction", s_directionNames, 3) ||
       !IsValidName(json, "mode", s_modeNames, 3))
      return false;

   json_t *days = json_object_get(json, "minTrainingDays");
   if ((days != nullptr) && !json_is_null(days) &&
       (!json_is_integer(days) || (json_integer_value(days) < 1) || (json_integer_value(days) > 365)))
      return false;

   json_t *hardBounds = json_object_get(json, "hardBounds");
   if ((hardBounds != nullptr) && !json_is_null(hardBounds))
   {
      if (!json_is_object(hardBounds) ||
          !IsValidNumber(hardBounds, "min", -HUGE_VAL, true, HUGE_VAL) ||
          !IsValidNumber(hardBounds, "max", -HUGE_VAL, true, HUGE_VAL))
         return false;
      double min = GetRealFromJson(hardBounds, "min"), max = GetRealFromJson(hardBounds, "max");
      if (!std::isnan(min) && !std::isnan(max) && (min > max))
         return false;
   }

   json_t *detector = json_object_get(json, "detector");
   if ((detector != nullptr) && !json_is_null(detector))
   {
      if (!json_is_object(detector))
         return false;
      json_t *id = json_object_get(detector, "id");
      if ((id != nullptr) && (!json_is_string(id) || strcmp(json_string_value(id), CHANGE_DETECTOR_ID)))
         return false;
      if (!IsValidNumber(detector, "k", 0, true, HUGE_VAL) ||
          !IsValidNumber(detector, "h", 0, false, HUGE_VAL))
         return false;
   }

   return true;
}

/**
 * Serialize settings to JSON
 */
json_t *AnomalySettings::toJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "sensitivity", json_real(sensitivity));
   json_object_set_new(json, "direction", json_string(s_directionNames[static_cast<int>(direction)]));
   json_object_set_new(json, "mode", json_string(s_modeNames[static_cast<int>(mode)]));
   json_object_set_new(json, "minTrainingDays", json_integer(minTrainingDays));

   if (!std::isnan(hardMin) || !std::isnan(hardMax))
   {
      json_t *hardBounds = json_object();
      if (!std::isnan(hardMin))
         json_object_set_new(hardBounds, "min", json_real(hardMin));
      if (!std::isnan(hardMax))
         json_object_set_new(hardBounds, "max", json_real(hardMax));
      json_object_set_new(json, "hardBounds", hardBounds);
   }

   json_t *provider = json_object();
   json_object_set_new(provider, "id", json_string(BASELINE_PROVIDER_ID));
   json_object_set_new(json, "provider", provider);

   json_t *detector = json_object();
   json_object_set_new(detector, "id", json_string(CHANGE_DETECTOR_ID));
   json_object_set_new(detector, "k", json_real(driftK));
   json_object_set_new(detector, "h", json_real(driftH));
   json_object_set_new(json, "detector", detector);

   return json;
}

/**
 * Check if all settings have default values
 */
bool AnomalySettings::isDefault() const
{
   return (sensitivity == DEFAULT_SENSITIVITY) && (direction == AnomalyDirection::BOTH) && (mode == AnomalyDetectionMode::AUTO) &&
          (minTrainingDays == DEFAULT_MIN_TRAINING_DAYS) && std::isnan(hardMin) && std::isnan(hardMax) &&
          (driftK == DEFAULT_DRIFT_K) && (driftH == DEFAULT_DRIFT_H);
}

/**
 * Model constructor
 */
AnomalyModel::AnomalyModel()
{
   m_mode = AnomalyDetectionMode::VALUE;
   m_requestedMode = AnomalyDetectionMode::AUTO;
   m_trainedFrom = 0;
   m_trainedTo = 0;
   m_trainingDays = 0;
   m_hourlySamples = 0;
   m_rawSamples = 0;
   m_flooredSlots = 0;
   m_useWeekdays = false;
   memset(m_centers, 0, sizeof(m_centers));
   memset(m_scales, 0, sizeof(m_scales));
   memset(m_levels, 0, sizeof(m_levels));
   m_stepCenter = 0;
   m_stepScale = 0;
   m_slotChangeScale = 0;
   m_stepMaxGap = 0;
   m_driftScale = 0;
}

/**
 * Create model from JSON. Returns nullptr if JSON is not a model of current version from slot baseline provider.
 */
shared_ptr<AnomalyModel> AnomalyModel::createFromJson(json_t *json)
{
   if (!json_is_object(json) || (json_object_get_int32(json, "version") != ANOMALY_MODEL_VERSION))
      return shared_ptr<AnomalyModel>();

   json_t *provider = json_object_get(json, "provider");
   if (!json_is_object(provider) ||
       strcmp(json_object_get_string_utf8(provider, "id", ""), BASELINE_PROVIDER_ID) ||
       (json_object_get_int32(provider, "version") != BASELINE_PROVIDER_VERSION))
      return shared_ptr<AnomalyModel>();

   json_t *centers = json_object_get(json, "centers");
   json_t *scales = json_object_get(json, "scales");
   json_t *levels = json_object_get(json, "levels");
   if ((json_array_size(centers) != ANOMALY_SLOT_COUNT) || (json_array_size(scales) != ANOMALY_SLOT_COUNT) || (json_array_size(levels) != ANOMALY_SLOT_COUNT))
      return shared_ptr<AnomalyModel>();

   auto model = make_shared<AnomalyModel>();
   model->m_mode = (IndexFromName(json_object_get_string_utf8(json, "mode", nullptr), s_modeNames, 3, 1) == 2) ? AnomalyDetectionMode::RATE : AnomalyDetectionMode::VALUE;
   model->m_requestedMode = static_cast<AnomalyDetectionMode>(IndexFromName(json_object_get_string_utf8(json, "requestedMode", nullptr), s_modeNames, 3, 0));
   model->m_trainedFrom = static_cast<time_t>(json_object_get_int64(json, "trainedFrom"));
   model->m_trainedTo = static_cast<time_t>(json_object_get_int64(json, "trainedTo"));
   model->m_trainingDays = json_object_get_int32(json, "trainingDays");
   model->m_hourlySamples = json_object_get_int32(json, "hourlySamples");
   model->m_rawSamples = json_object_get_int32(json, "rawSamples");
   model->m_flooredSlots = json_object_get_int32(json, "flooredSlots");
   model->m_useWeekdays = json_object_get_boolean(json, "useWeekdays");
   for(int i = 0; i < ANOMALY_SLOT_COUNT; i++)
   {
      model->m_centers[i] = json_number_value(json_array_get(centers, i));
      model->m_scales[i] = json_number_value(json_array_get(scales, i));
      json_int_t level = json_integer_value(json_array_get(levels, i));
      model->m_levels[i] = ((level >= 0) && (level <= static_cast<int>(AnomalySlotLevel::WEEKDAY))) ? static_cast<BYTE>(level) : 0;
   }
   model->m_stepCenter = json_number_value(json_object_get(json, "stepCenter"));
   model->m_stepScale = json_number_value(json_object_get(json, "stepScale"));
   model->m_slotChangeScale = json_number_value(json_object_get(json, "slotChangeScale"));
   if (!(model->m_slotChangeScale > model->m_stepScale))
      model->m_slotChangeScale = model->m_stepScale;   // also covers model saved without this element
   model->m_stepMaxGap = json_object_get_int32(json, "stepMaxGap");
   model->m_driftScale = json_number_value(json_object_get(json, "driftScale"));
   return model;
}

/**
 * Serialize model to JSON
 */
json_t *AnomalyModel::toJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "version", json_integer(ANOMALY_MODEL_VERSION));

   json_t *provider = json_object();
   json_object_set_new(provider, "id", json_string(BASELINE_PROVIDER_ID));
   json_object_set_new(provider, "version", json_integer(BASELINE_PROVIDER_VERSION));
   json_object_set_new(json, "provider", provider);

   json_object_set_new(json, "mode", json_string(s_modeNames[static_cast<int>(m_mode)]));
   json_object_set_new(json, "requestedMode", json_string(s_modeNames[static_cast<int>(m_requestedMode)]));
   json_object_set_new(json, "trainedFrom", json_integer(m_trainedFrom));
   json_object_set_new(json, "trainedTo", json_integer(m_trainedTo));
   json_object_set_new(json, "trainingDays", json_integer(m_trainingDays));
   json_object_set_new(json, "hourlySamples", json_integer(m_hourlySamples));
   json_object_set_new(json, "rawSamples", json_integer(m_rawSamples));
   json_object_set_new(json, "flooredSlots", json_integer(m_flooredSlots));
   json_object_set_new(json, "useWeekdays", json_boolean(m_useWeekdays));

   json_t *centers = json_array();
   json_t *scales = json_array();
   json_t *levels = json_array();
   for(int i = 0; i < ANOMALY_SLOT_COUNT; i++)
   {
      json_array_append_new(centers, json_real(m_centers[i]));
      json_array_append_new(scales, json_real(m_scales[i]));
      json_array_append_new(levels, json_integer(m_levels[i]));
   }
   json_object_set_new(json, "centers", centers);
   json_object_set_new(json, "scales", scales);
   json_object_set_new(json, "levels", levels);

   json_object_set_new(json, "stepCenter", json_real(m_stepCenter));
   json_object_set_new(json, "stepScale", json_real(m_stepScale));
   json_object_set_new(json, "slotChangeScale", json_real(m_slotChangeScale));
   json_object_set_new(json, "stepMaxGap", json_integer(m_stepMaxGap));
   json_object_set_new(json, "driftScale", json_real(m_driftScale));
   return json;
}

/**
 * Get expected value at given point in time
 */
ExpectedValue AnomalyModel::getExpectedValue(time_t timestamp) const
{
   struct tm lt;
   LocalTime(timestamp, &lt);
   int index = SlotIndex(lt);

   ExpectedValue ev;
   ev.center = m_centers[index];
   ev.scale = m_scales[index];
   ev.level = static_cast<AnomalySlotLevel>(m_levels[index]);
   ev.valid = (ev.scale > 0);
   return ev;
}

/**
 * Get human readable description of data behind expected value at given point in time
 */
String AnomalyModel::describeSlot(time_t timestamp) const
{
   struct tm lt;
   LocalTime(timestamp, &lt);

   StringBuffer sb;
   switch(static_cast<AnomalySlotLevel>(m_levels[SlotIndex(lt)]))
   {
      case AnomalySlotLevel::WEEKDAY:
         sb.append(s_dayNames[lt.tm_wday]);
         break;
      case AnomalySlotLevel::DAY_TYPE:
         sb.append((DayType(lt.tm_wday) == 1) ? L"weekends" : L"weekdays");
         break;
      case AnomalySlotLevel::HOUR:
         sb.append(L"all days");
         break;
      default:
         return String(L"all data");
   }
   sb.appendFormattedString(L" %02d:00-%02d:00", lt.tm_hour, (lt.tm_hour + 1) % 24);
   return String(sb);
}

/**
 * Aggregate chronologically ordered raw samples into hourly averages (buckets are aligned to hours of local time)
 */
void AggregateAnomalySamplesHourly(const std::vector<AnomalySample>& raw, std::vector<AnomalySample> *hourly)
{
   time_t bucketStart = 0;
   double sum = 0;
   int count = 0;
   for(size_t i = 0; i < raw.size(); i++)
   {
      if (!std::isfinite(raw[i].value))
         continue;

      time_t start = GetAnomalyHourStart(raw[i].timestamp);
      if ((start != bucketStart) && (count > 0))
      {
         hourly->push_back({ bucketStart, sum / count });
         sum = 0;
         count = 0;
      }
      bucketStart = start;
      sum += raw[i].value;
      count++;
   }
   if (count > 0)
      hourly->push_back({ bucketStart, sum / count });
}

/**
 * Check if series is strongly monotone
 */
static bool IsMonotoneSeries(const std::vector<AnomalySample>& series)
{
   size_t up = 0, down = 0;
   for(size_t i = 1; i < series.size(); i++)
   {
      if (series[i].value > series[i - 1].value)
         up++;
      else if (series[i].value < series[i - 1].value)
         down++;
   }
   size_t changes = up + down;
   return (changes >= 10) && (changes * 4 >= series.size()) && (std::max(up, down) >= changes * MONOTONE_SHARE);
}

/**
 * Copy series, optionally converting values to rate of change per second (non-finite values are dropped)
 */
static void PrepareSeries(const std::vector<AnomalySample>& source, bool convertToRate, std::vector<AnomalySample> *series)
{
   series->reserve(source.size());
   bool hasPrev = false;
   AnomalySample prev = { 0, 0 };
   for(size_t i = 0; i < source.size(); i++)
   {
      if (!std::isfinite(source[i].value))
         continue;

      if (!convertToRate)
      {
         series->push_back(source[i]);
         continue;
      }

      if (hasPrev && (source[i].timestamp > prev.timestamp))
         series->push_back({ source[i].timestamp, (source[i].value - prev.value) / static_cast<double>(source[i].timestamp - prev.timestamp) });
      prev = source[i];
      hasPrev = true;
   }
}

/**
 * Get smallest positive difference between distinct values of the series (0 if all values are the same)
 */
static double GetSeriesResolution(const std::vector<AnomalySample>& series)
{
   std::vector<double> values(series.size());
   for(size_t i = 0; i < series.size(); i++)
      values[i] = series[i].value;
   std::sort(values.begin(), values.end());

   double resolution = 0;
   for(size_t i = 1; i < values.size(); i++)
   {
      double d = values[i] - values[i - 1];
      if ((d > 0) && ((resolution == 0) || (d < resolution)))
         resolution = d;
   }
   return resolution;
}

/**
 * Center and day count for group of hourly values
 */
struct GroupCenter
{
   double center;
   double spread;    // between-day spread
   size_t days;
};

/**
 * Calculate center and between-day spread for group of hourly values (elements are destroyed)
 */
static GroupCenter CalculateGroupCenter(std::vector<double>& values)
{
   GroupCenter g;
   g.days = values.size();
   if (values.empty())
   {
      g.center = 0;
      g.spread = 0;
      return g;
   }
   g.center = Median(values);
   ToAbsoluteDeviations(values, g.center);
   g.spread = Median(values) * MAD_TO_SIGMA;
   return g;
}

/**
 * Append content of one vector to another
 */
static inline void Append(std::vector<double> *target, const std::vector<double>& source)
{
   target->insert(target->end(), source.begin(), source.end());
}

/**
 * Train model.
 *
 * @param hourly chronologically ordered hourly averages (source for slot centers and drift spread)
 * @param raw chronologically ordered raw samples (source for single sample spread and step check, can be empty)
 * @param settings DCI anomaly detection settings
 * @return trained model or nullptr if there is no usable data
 */
shared_ptr<AnomalyModel> AnomalyModel::train(const std::vector<AnomalySample>& hourlySource, const std::vector<AnomalySample>& rawSource, const AnomalySettings& settings)
{
   AnomalyDetectionMode mode = settings.mode;
   if (mode == AnomalyDetectionMode::AUTO)
      mode = IsMonotoneSeries(hourlySource) ? AnomalyDetectionMode::RATE : AnomalyDetectionMode::VALUE;

   std::vector<AnomalySample> hourly, raw;
   PrepareSeries(hourlySource, mode == AnomalyDetectionMode::RATE, &hourly);
   PrepareSeries(rawSource, mode == AnomalyDetectionMode::RATE, &raw);
   if (hourly.empty())
      return shared_ptr<AnomalyModel>();

   auto model = make_shared<AnomalyModel>();
   model->m_mode = mode;
   model->m_requestedMode = settings.mode;
   model->m_trainedFrom = hourly[0].timestamp;
   model->m_trainedTo = hourly[hourly.size() - 1].timestamp;
   model->m_hourlySamples = static_cast<int32_t>(hourly.size());
   model->m_rawSamples = static_cast<int32_t>(raw.size());

   // Distribute hourly values by slot at each granularity
   std::vector<std::vector<double>> byWeekday(ANOMALY_SLOT_COUNT), byDayType(48), byHour(24);
   std::vector<double> all;
   std::vector<int> hourlySlots(hourly.size());
   std::vector<int> days, hourlyDays(hourly.size());
   all.reserve(hourly.size());
   for(size_t i = 0; i < hourly.size(); i++)
   {
      struct tm lt;
      LocalTime(hourly[i].timestamp, &lt);
      hourlySlots[i] = SlotIndex(lt);
      byWeekday[hourlySlots[i]].push_back(hourly[i].value);
      byDayType[DayType(lt.tm_wday) * 24 + lt.tm_hour].push_back(hourly[i].value);
      byHour[lt.tm_hour].push_back(hourly[i].value);
      all.push_back(hourly[i].value);
      hourlyDays[i] = lt.tm_year * 366 + lt.tm_yday;
      days.push_back(hourlyDays[i]);
   }
   std::sort(days.begin(), days.end());
   model->m_trainingDays = static_cast<int32_t>(std::unique(days.begin(), days.end()) - days.begin());

   GroupCenter weekdayCenters[ANOMALY_SLOT_COUNT], dayTypeCenters[48], hourCenters[24];
   for(int i = 0; i < ANOMALY_SLOT_COUNT; i++)
      weekdayCenters[i] = CalculateGroupCenter(byWeekday[i]);
   for(int i = 0; i < 48; i++)
      dayTypeCenters[i] = CalculateGroupCenter(byDayType[i]);
   for(int i = 0; i < 24; i++)
      hourCenters[i] = CalculateGroupCenter(byHour[i]);
   GroupCenter globalCenter = CalculateGroupCenter(all);

   // Use own baseline for a day of week only if it actually differs from weekday/weekend level
   bool useWeekday[7];
   for(int wday = 0; wday < 7; wday++)
   {
      int comparable = 0, different = 0;
      for(int hour = 0; hour < 24; hour++)
      {
         const GroupCenter& weekday = weekdayCenters[wday * 24 + hour];
         const GroupCenter& dayType = dayTypeCenters[DayType(wday) * 24 + hour];
         if ((weekday.days < MIN_SLOT_DAYS) || (dayType.days < MIN_SLOT_DAYS))
            continue;
         comparable++;
         double spread = dayType.spread;
         ApplySpreadFloor(&spread, dayType.center, 0);
         if (fabs(weekday.center - dayType.center) > spread * WEEKDAY_EVIDENCE_DEVIATION)
            different++;
      }
      useWeekday[wday] = (comparable >= WEEKDAY_EVIDENCE_MIN_HOURS) && (different >= WEEKDAY_EVIDENCE_HOURS);
      if (useWeekday[wday])
         model->m_useWeekdays = true;
   }

   // Select center for each slot from the most specific granularity with enough days
   for(int i = 0; i < ANOMALY_SLOT_COUNT; i++)
   {
      int hour = i % 24;
      const GroupCenter& dayType = dayTypeCenters[DayType(i / 24) * 24 + hour];
      if (useWeekday[i / 24] && (weekdayCenters[i].days >= MIN_SLOT_DAYS))
      {
         model->m_centers[i] = weekdayCenters[i].center;
         model->m_levels[i] = static_cast<BYTE>(AnomalySlotLevel::WEEKDAY);
      }
      else if (dayType.days >= MIN_SLOT_DAYS)
      {
         model->m_centers[i] = dayType.center;
         model->m_levels[i] = static_cast<BYTE>(AnomalySlotLevel::DAY_TYPE);
      }
      else if (hourCenters[hour].days >= MIN_SLOT_DAYS)
      {
         model->m_centers[i] = hourCenters[hour].center;
         model->m_levels[i] = static_cast<BYTE>(AnomalySlotLevel::HOUR);
      }
      else
      {
         model->m_centers[i] = globalCenter.center;
         model->m_levels[i] = static_cast<BYTE>(AnomalySlotLevel::GLOBAL);
      }
   }

   double resolution = GetSeriesResolution(raw.empty() ? hourly : raw);

   // Spread of hourly mean residual (used by drift detector)
   std::vector<double> deviations(hourly.size());
   for(size_t i = 0; i < hourly.size(); i++)
      deviations[i] = fabs(hourly[i].value - model->m_centers[hourlySlots[i]]);
   model->m_driftScale = Median(deviations) * MAD_TO_SIGMA;

   // Drift detector sums residuals of consecutive hours, so its scale has to cover deviations that persist
   // for many hours (some days are just busier than others). Spread of daily mean residual tells how
   // large such deviations normally are; for independent hourly residuals it matches hourly spread.
   std::vector<double> dailyResiduals;
   double daySum = 0;
   int dayCount = 0;
   for(size_t i = 0; i < hourly.size(); i++)
   {
      daySum += hourly[i].value - model->m_centers[hourlySlots[i]];
      dayCount++;
      if ((i == hourly.size() - 1) || (hourlyDays[i + 1] != hourlyDays[i]))
      {
         if (dayCount >= MIN_HOURS_IN_DAY)
            dailyResiduals.push_back(daySum / sqrt(static_cast<double>(dayCount)));
         daySum = 0;
         dayCount = 0;
      }
   }
   if (dailyResiduals.size() >= static_cast<size_t>(MIN_SLOT_DAYS))
   {
      double center = Median(dailyResiduals);
      for(size_t i = 0; i < dailyResiduals.size(); i++)
         dailyResiduals[i] = fabs(dailyResiduals[i] - center);
      model->m_driftScale = std::max(model->m_driftScale, Median(dailyResiduals) * MAD_TO_SIGMA);
   }
   ApplySpreadFloor(&model->m_driftScale, globalCenter.center, resolution);

   // Spread of single sample around slot center, pooled by hour and day type, then by hour, then globally
   std::vector<std::vector<double>> deviationsByDayType(48);
   std::vector<int> rawSlots(raw.size());
   for(size_t i = 0; i < raw.size(); i++)
   {
      struct tm lt;
      LocalTime(raw[i].timestamp, &lt);
      rawSlots[i] = SlotIndex(lt);
      deviationsByDayType[DayType(lt.tm_wday) * 24 + lt.tm_hour].push_back(fabs(raw[i].value - model->m_centers[rawSlots[i]]));
   }

   double dayTypeScales[48], hourScales[24], globalScale = 0;
   std::vector<double> allDeviations;
   allDeviations.reserve(raw.size());
   for(int hour = 0; hour < 24; hour++)
   {
      std::vector<double> hourDeviations;
      for(int dayType = 0; dayType < 2; dayType++)
      {
         std::vector<double>& d = deviationsByDayType[dayType * 24 + hour];
         Append(&hourDeviations, d);
         dayTypeScales[dayType * 24 + hour] = (d.size() >= MIN_SPREAD_SAMPLES) ? Median(d) * MAD_TO_SIGMA : -1;
      }
      Append(&allDeviations, hourDeviations);
      hourScales[hour] = (hourDeviations.size() >= MIN_SPREAD_SAMPLES) ? Median(hourDeviations) * MAD_TO_SIGMA : -1;
   }
   if (allDeviations.size() >= MIN_SPREAD_SAMPLES)
      globalScale = Median(allDeviations) * MAD_TO_SIGMA;
   else
      globalScale = model->m_driftScale;   // not enough raw samples, use hourly spread

   for(int i = 0; i < ANOMALY_SLOT_COUNT; i++)
   {
      int hour = i % 24;
      double scale = dayTypeScales[DayType(i / 24) * 24 + hour];
      if (scale < 0)
         scale = hourScales[hour];
      if (scale < 0)
         scale = globalScale;
      if (ApplySpreadFloor(&scale, model->m_centers[i], resolution))
         model->m_flooredSlots++;
      model->m_scales[i] = scale;
   }

   // Spread of change between consecutive samples. The 99th percentile guards against
   // series that are flat most of the time, where median deviation of change is zero.
   if ((mode == AnomalyDetectionMode::VALUE) && (raw.size() > MIN_SPREAD_SAMPLES))
   {
      std::vector<double> intervals;
      intervals.reserve(raw.size() - 1);
      for(size_t i = 1; i < raw.size(); i++)
         if (raw[i].timestamp > raw[i - 1].timestamp)
            intervals.push_back(static_cast<double>(raw[i].timestamp - raw[i - 1].timestamp));
      if (intervals.size() >= MIN_SPREAD_SAMPLES)
      {
         model->m_stepMaxGap = static_cast<int32_t>(std::min(Median(intervals) * 3, 86400.0));

         std::vector<double> changes;
         changes.reserve(intervals.size());
         for(size_t i = 1; i < raw.size(); i++)
         {
            time_t interval = raw[i].timestamp - raw[i - 1].timestamp;
            if ((interval > 0) && (interval <= model->m_stepMaxGap))
               changes.push_back(raw[i].value - raw[i - 1].value);
         }
         if (changes.size() >= MIN_SPREAD_SAMPLES)
         {
            model->m_stepCenter = Median(changes);
            ToAbsoluteDeviations(changes, model->m_stepCenter);
            model->m_stepScale = std::max(Median(changes) * MAD_TO_SIGMA, Quantile(changes, 0.99) / P99_TO_SIGMA);
            ApplySpreadFloor(&model->m_stepScale, 0, resolution);

            // Spread of change between samples from different slots after expected change of level is
            // subtracted. Tells how precisely change of level between slots is followed: slot centers
            // are estimates, and size of regular change differs from day to day.
            model->m_slotChangeScale = model->m_stepScale;
            changes.clear();
            for(size_t i = 1; i < raw.size(); i++)
            {
               time_t interval = raw[i].timestamp - raw[i - 1].timestamp;
               if ((interval > 0) && (interval <= model->m_stepMaxGap) && (rawSlots[i] != rawSlots[i - 1]))
                  changes.push_back(fabs(raw[i].value - raw[i - 1].value - (model->m_centers[rawSlots[i]] - model->m_centers[rawSlots[i - 1]]) - model->m_stepCenter));
            }
            if (changes.size() >= MIN_SPREAD_SAMPLES)
               model->m_slotChangeScale = std::max(model->m_stepScale, Quantile(changes, 0.99) / P99_TO_SIGMA);
         }
      }
   }

   return model;
}

/**
 * Reset detector state
 */
void AnomalyDetectorState::reset()
{
   resetDrift();
   lastTimestamp = 0;
   lastValue = 0;
}

/**
 * Reset drift detection part of detector state
 */
void AnomalyDetectorState::resetDrift()
{
   driftUp = 0;
   driftDown = 0;
   driftUpOnset = 0;
   driftDownOnset = 0;
   bucketStart = 0;
   bucketSum = 0;
   bucketCount = 0;
}

/**
 * Serialize persistent part of detector state (drift sums as of last closed hour) to JSON
 */
json_t *AnomalyDetectorState::toJson() const
{
   json_t *json = json_object();
   json_object_set_new(json, "up", json_real(driftUp));
   json_object_set_new(json, "down", json_real(driftDown));
   json_object_set_new(json, "upOnset", json_integer(driftUpOnset));
   json_object_set_new(json, "downOnset", json_integer(driftDownOnset));
   return json;
}

/**
 * Reset state and load persistent part of it from JSON
 */
void AnomalyDetectorState::loadFromJson(json_t *json)
{
   reset();
   if (!json_is_object(json))
      return;

   double up = json_number_value(json_object_get(json, "up"));
   double down = json_number_value(json_object_get(json, "down"));
   if (std::isfinite(up) && (up > 0))
   {
      driftUp = up;
      driftUpOnset = static_cast<time_t>(json_object_get_int64(json, "upOnset"));
   }
   if (std::isfinite(down) && (down > 0))
   {
      driftDown = down;
      driftDownOnset = static_cast<time_t>(json_object_get_int64(json, "downOnset"));
   }
}

/**
 * Update one drift sum with hourly z-score
 */
static void UpdateDriftSum(double *sum, time_t *onset, double z, const AnomalySettings& settings, time_t bucketStart)
{
   *sum = std::min(std::max(*sum + z - settings.driftK, 0.0), settings.driftH + DRIFT_HEADROOM);
   if (*sum == 0)
      *onset = 0;
   else if (*onset == 0)
      *onset = bucketStart;
}

/**
 * Add residual to drift detector. Drift sums are updated when hour changes.
 */
static void UpdateDriftDetector(const AnomalyModel& model, const AnomalySettings& settings, AnomalyDetectorState *state, time_t timestamp, double residual)
{
   time_t bucketStart = GetAnomalyHourStart(timestamp);
   if (bucketStart < state->bucketStart)
      return;  // out of order sample

   if ((bucketStart > state->bucketStart) && (state->bucketCount > 0))
   {
      double z = (state->bucketSum / state->bucketCount) / model.getDriftScale();
      z = std::min(std::max(z, -DRIFT_Z_LIMIT), DRIFT_Z_LIMIT);
      UpdateDriftSum(&state->driftUp, &state->driftUpOnset, z, settings, state->bucketStart);
      UpdateDriftSum(&state->driftDown, &state->driftDownOnset, -z, settings, state->bucketStart);
      state->bucketSum = 0;
      state->bucketCount = 0;
   }

   state->bucketStart = bucketStart;
   state->bucketSum += residual;
   state->bucketCount++;
}

/**
 * Check if z-score is a deviation in direction of interest
 */
static inline bool IsDeviation(double z, const AnomalySettings& settings)
{
   switch(settings.direction)
   {
      case AnomalyDirection::UP:
         return z > settings.sensitivity;
      case AnomalyDirection::DOWN:
         return z < -settings.sensitivity;
      default:
         return fabs(z) > settings.sensitivity;
   }
}

/**
 * Score single sample against the model and update detector state. Samples should be passed in chronological order.
 */
AnomalyScore ScoreAnomalySample(const AnomalyModel& model, const AnomalySettings& settings, AnomalyDetectorState *state, time_t timestamp, double value)
{
   AnomalyScore result;
   result.type = AnomalyType::NONE;
   result.learning = false;
   result.scored = false;
   result.score = 0;
   result.expectedLow = 0;
   result.expectedHigh = 0;
   result.driftOnset = 0;

   if (!std::isfinite(value))
      return result;

   // Change since previous sample
   time_t prevTimestamp = state->lastTimestamp;
   bool hasChange = (state->lastTimestamp != 0) && (timestamp > state->lastTimestamp);
   double interval = hasChange ? static_cast<double>(timestamp - state->lastTimestamp) : 0;
   double change = hasChange ? value - state->lastValue : 0;
   if (timestamp >= state->lastTimestamp)
   {
      state->lastTimestamp = timestamp;
      state->lastValue = value;
   }

   if (model.isLearning(settings))
   {
      result.learning = true;
      return result;
   }

   if ((!std::isnan(settings.hardMin) && (value < settings.hardMin)) || (!std::isnan(settings.hardMax) && (value > settings.hardMax)))
      result.type = AnomalyType::RANGE;

   // Range check and drift accumulation
   ExpectedValue ev = model.getExpectedValue(timestamp);
   if (ev.valid && ((model.getMode() == AnomalyDetectionMode::VALUE) || hasChange))
   {
      double x = (model.getMode() == AnomalyDetectionMode::RATE) ? change / interval : value;
      result.score = (x - ev.center) / ev.scale;
      result.expectedLow = ev.center - settings.sensitivity * ev.scale;
      result.expectedHigh = ev.center + settings.sensitivity * ev.scale;
      result.scored = true;
      if (IsDeviation(result.score, settings))
         result.type = AnomalyType::RANGE;
      UpdateDriftDetector(model, settings, state, timestamp, x - ev.center);
   }

   if (result.type != AnomalyType::NONE)
      return result;

   // Step check. Change that follows expected change of level between slots (start of business hours,
   // scheduled job) is normal, so change has to be unusual both by itself and relative to the baseline.
   if ((model.getMode() == AnomalyDetectionMode::VALUE) && hasChange && (model.getStepScale() > 0) && (interval <= model.getStepMaxGap()))
   {
      double z = (change - model.getStepCenter()) / model.getStepScale();
      if (IsDeviation(z, settings))
      {
         double zb = z;
         if (GetAnomalyHourStart(prevTimestamp) != GetAnomalyHourStart(timestamp))
         {
            double expectedChange = ev.center - model.getExpectedValue(prevTimestamp).center;
            zb = (change - expectedChange - model.getStepCenter()) / model.getSlotChangeScale();
         }
         if (IsDeviation(zb, settings) && ((z > 0) == (zb > 0)))
         {
            result.type = AnomalyType::STEP;
            result.score = (fabs(zb) < fabs(z)) ? zb : z;
            return result;
         }
      }
   }

   // Drift check
   if ((settings.direction != AnomalyDirection::DOWN) && (state->driftUp > settings.driftH))
   {
      result.type = AnomalyType::DRIFT;
      result.score = state->driftUp;
      result.driftOnset = state->driftUpOnset;
   }
   else if ((settings.direction != AnomalyDirection::UP) && (state->driftDown > settings.driftH))
   {
      result.type = AnomalyType::DRIFT;
      result.score = -state->driftDown;
      result.driftOnset = state->driftDownOnset;
   }
   return result;
}
