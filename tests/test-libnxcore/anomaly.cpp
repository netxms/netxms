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
** File: anomaly.cpp
**
** Unit tests for numeric part of DCI anomaly detection: slot baseline training,
** expected value lookup, and range, step, and drift checks.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <nxcore_anomaly.h>
#include <testtools.h>
#include <cmath>

/**
 * Interval between generated samples, in seconds
 */
#define SAMPLE_INTERVAL 300

/**
 * Value generator: day index (from start of the series), local time of the sample, sample index
 */
typedef double (*ValueGenerator)(int day, const struct tm& lt, int index);

/**
 * Deterministic noise source
 */
static uint32_t s_noiseState = 1;

/**
 * Get next noise value, uniformly distributed between -1 and 1
 */
static double Noise()
{
   s_noiseState = s_noiseState * 1664525 + 1013904223;
   return static_cast<double>(s_noiseState >> 8) / 8388608.0 - 1.0;
}

/**
 * Convert timestamp to broken down local time
 */
static void LocalTime(time_t t, struct tm *lt)
{
#ifdef _WIN32
   localtime_s(lt, &t);
#else
   localtime_r(&t, lt);
#endif
}

/**
 * Get timestamp for given local date and time
 */
static time_t TimeAt(int year, int month, int day, int hour)
{
   struct tm lt;
   memset(&lt, 0, sizeof(lt));
   lt.tm_year = year - 1900;
   lt.tm_mon = month - 1;
   lt.tm_mday = day;
   lt.tm_hour = hour;
   lt.tm_isdst = -1;
   return mktime(&lt);
}

/**
 * Generate series starting at given time
 */
static void Generate(time_t start, int days, ValueGenerator generator, std::vector<AnomalySample> *series)
{
   s_noiseState = 1;
   int count = days * 86400 / SAMPLE_INTERVAL;
   int day = 0, lastYearDay = -1;
   for(int i = 0; i < count; i++)
   {
      time_t t = start + static_cast<time_t>(i) * SAMPLE_INTERVAL;
      struct tm lt;
      LocalTime(t, &lt);
      if (lt.tm_yday != lastYearDay)
      {
         if (lastYearDay != -1)
            day++;
         lastYearDay = lt.tm_yday;
      }
      series->push_back({ t, generator(day, lt, i) });
   }
}

/**
 * Train model on generated series (hourly averages are calculated from the same series)
 */
static shared_ptr<AnomalyModel> Train(const std::vector<AnomalySample>& series, const AnomalySettings& settings)
{
   std::vector<AnomalySample> hourly;
   AggregateAnomalySamplesHourly(series, &hourly);
   return AnomalyModel::train(hourly, series, settings);
}

/**
 * Score single sample with fresh detector state
 */
static AnomalyScore ScoreAlone(const AnomalyModel& model, const AnomalySettings& settings, time_t timestamp, double value)
{
   AnomalyDetectorState state;
   return ScoreAnomalySample(model, settings, &state, timestamp, value);
}

/**
 * Check that two values are close to each other
 */
static inline bool IsClose(double a, double b, double tolerance)
{
   return fabs(a - b) <= tolerance;
}

/**
 * Settings parsing and serialization
 */
static void TestSettings()
{
   StartTest(_T("Anomaly detection - settings"));

   AnomalySettings defaults(nullptr);
   AssertTrue(defaults.sensitivity == 3.5);
   AssertTrue(defaults.direction == AnomalyDirection::BOTH);
   AssertTrue(defaults.mode == AnomalyDetectionMode::AUTO);
   AssertEquals(defaults.minTrainingDays, 7);
   AssertTrue(std::isnan(defaults.hardMin) && std::isnan(defaults.hardMax));
   AssertTrue((defaults.driftK == 1.0) && (defaults.driftH == 5.0));

   json_t *json = json_loads("{\"sensitivity\":2.5,\"direction\":\"down\",\"mode\":\"rate\",\"minTrainingDays\":14,"
      "\"hardBounds\":{\"min\":0,\"max\":100},\"detector\":{\"id\":\"cusum\",\"k\":0.25,\"h\":8}}", 0, nullptr);
   AssertNotNull(json);
   AnomalySettings parsed(json);
   json_decref(json);
   AssertTrue(parsed.sensitivity == 2.5);
   AssertTrue(parsed.direction == AnomalyDirection::DOWN);
   AssertTrue(parsed.mode == AnomalyDetectionMode::RATE);
   AssertEquals(parsed.minTrainingDays, 14);
   AssertTrue((parsed.hardMin == 0) && (parsed.hardMax == 100));
   AssertTrue((parsed.driftK == 0.25) && (parsed.driftH == 8));

   json = parsed.toJson();
   AnomalySettings copy(json);
   json_decref(json);
   AssertTrue(copy.sensitivity == 2.5);
   AssertTrue(copy.direction == AnomalyDirection::DOWN);
   AssertTrue(copy.mode == AnomalyDetectionMode::RATE);
   AssertEquals(copy.minTrainingDays, 14);
   AssertTrue((copy.hardMin == 0) && (copy.hardMax == 100));
   AssertTrue((copy.driftK == 0.25) && (copy.driftH == 8));

   // Invalid values and parameters of unknown detector are ignored
   json = json_loads("{\"sensitivity\":-1,\"direction\":\"sideways\",\"minTrainingDays\":0,\"detector\":{\"id\":\"other\",\"k\":3,\"h\":1}}", 0, nullptr);
   AssertNotNull(json);
   AnomalySettings invalid(json);
   json_decref(json);
   AssertTrue(invalid.sensitivity == 3.5);
   AssertTrue(invalid.direction == AnomalyDirection::BOTH);
   AssertEquals(invalid.minTrainingDays, 7);
   AssertTrue((invalid.driftK == 1.0) && (invalid.driftH == 5.0));

   AssertTrue(defaults.isDefault());
   AssertTrue(invalid.isDefault());
   AssertFalse(parsed.isDefault());
   AnomalySettings bounded;
   bounded.hardMax = 100;
   AssertFalse(bounded.isDefault());

   EndTest();
}

/**
 * Check result of settings document validation
 */
static bool ValidateSettings(const char *text)
{
   json_t *json = json_loads(text, 0, nullptr);
   bool valid = AnomalySettings::validate(json);
   json_decref(json);
   return valid;
}

/**
 * Test validation of settings document
 */
static void TestSettingsValidation()
{
   StartTest(_T("Anomaly detection - settings validation"));

   AnomalySettings defaults;
   json_t *json = defaults.toJson();
   AssertTrue(AnomalySettings::validate(json));
   json_decref(json);

   AssertTrue(ValidateSettings("{}"));
   AssertTrue(ValidateSettings("{\"sensitivity\":2,\"direction\":\"up\",\"mode\":\"value\",\"minTrainingDays\":30,"
      "\"hardBounds\":{\"min\":0,\"max\":100},\"detector\":{\"id\":\"cusum\",\"k\":0,\"h\":3}}"));
   AssertTrue(ValidateSettings("{\"sensitivity\":100,\"hardBounds\":null,\"enabled\":true,\"model\":{\"mode\":\"rate\"}}"));
   AssertTrue(ValidateSettings("{\"hardBounds\":{\"min\":null,\"max\":5}}"));
   AssertTrue(ValidateSettings("{\"hardBounds\":{\"min\":5,\"max\":5}}"));

   AssertFalse(ValidateSettings("[]"));
   AssertFalse(ValidateSettings("{\"sensitivity\":0}"));
   AssertFalse(ValidateSettings("{\"sensitivity\":-1}"));
   AssertFalse(ValidateSettings("{\"sensitivity\":500}"));
   AssertFalse(ValidateSettings("{\"sensitivity\":\"3.5\"}"));
   AssertFalse(ValidateSettings("{\"direction\":\"sideways\"}"));
   AssertFalse(ValidateSettings("{\"direction\":1}"));
   AssertFalse(ValidateSettings("{\"mode\":\"delta\"}"));
   AssertFalse(ValidateSettings("{\"minTrainingDays\":0}"));
   AssertFalse(ValidateSettings("{\"minTrainingDays\":366}"));
   AssertFalse(ValidateSettings("{\"minTrainingDays\":7.5}"));
   AssertFalse(ValidateSettings("{\"hardBounds\":5}"));
   AssertFalse(ValidateSettings("{\"hardBounds\":{\"min\":\"0\"}}"));
   AssertFalse(ValidateSettings("{\"hardBounds\":{\"min\":10,\"max\":5}}"));
   AssertFalse(ValidateSettings("{\"detector\":{\"id\":\"other\"}}"));
   AssertFalse(ValidateSettings("{\"detector\":{\"k\":-0.5}}"));
   AssertFalse(ValidateSettings("{\"detector\":{\"h\":0}}"));

   EndTest();
}

/**
 * Test saving and loading of detector state
 */
static void TestDetectorState()
{
   StartTest(_T("Anomaly detection - detector state"));

   AnomalyDetectorState state;
   state.driftUp = 3.25;
   state.driftUpOnset = 1700000000;
   state.bucketStart = 1700003600;
   state.bucketSum = 12;
   state.bucketCount = 4;
   state.lastTimestamp = 1700003900;
   state.lastValue = 42;

   // Only drift sums are persistent
   json_t *json = state.toJson();
   AnomalyDetectorState loaded;
   loaded.driftDown = 1;
   loaded.loadFromJson(json);
   json_decref(json);
   AssertTrue(loaded.driftUp == 3.25);
   AssertTrue(loaded.driftUpOnset == 1700000000);
   AssertTrue(loaded.driftDown == 0);
   AssertTrue(loaded.driftDownOnset == 0);
   AssertTrue(loaded.bucketStart == 0);
   AssertEquals(loaded.bucketCount, 0);
   AssertTrue(loaded.lastTimestamp == 0);

   // Invalid document resets the state
   loaded.loadFromJson(nullptr);
   AssertTrue(loaded.driftUp == 0);
   AssertTrue(loaded.driftUpOnset == 0);

   json = json_loads("{\"up\":-2,\"down\":\"x\",\"upOnset\":5}", 0, nullptr);
   AssertNotNull(json);
   loaded.loadFromJson(json);
   json_decref(json);
   AssertTrue((loaded.driftUp == 0) && (loaded.driftDown == 0) && (loaded.driftUpOnset == 0));

   EndTest();
}

/**
 * Series with different level during business hours on weekdays
 */
static void TestSeasonalBaseline()
{
   StartTest(_T("Anomaly detection - weekday/weekend baseline"));

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 42,   // starts on Monday
      [] (int day, const struct tm& lt, int index) -> double
      {
         bool businessHours = (lt.tm_wday >= 1) && (lt.tm_wday <= 5) && (lt.tm_hour >= 9) && (lt.tm_hour < 18);
         return (businessHours ? 150.0 : 100.0) + Noise() * 2;
      }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getMode() == AnomalyDetectionMode::VALUE);
   AssertFalse(model->isLearning(settings));
   AssertFalse(model->isUsingWeekdays());
   AssertEquals(model->getTrainingDays(), 42);
   AssertEquals(model->getFlooredSlotCount(), 0);

   time_t wednesdayNoon = TimeAt(2026, 2, 18, 12);
   time_t sundayNoon = TimeAt(2026, 2, 22, 12);

   ExpectedValue ev = model->getExpectedValue(wednesdayNoon);
   AssertTrue(ev.valid);
   AssertTrue(ev.level == AnomalySlotLevel::DAY_TYPE);
   AssertTrue(IsClose(ev.center, 150, 1));
   AssertTrue((ev.scale > 0.5) && (ev.scale < 3));
   AssertTrue(!wcscmp(model->describeSlot(wednesdayNoon), L"weekdays 12:00-13:00"));

   ev = model->getExpectedValue(sundayNoon);
   AssertTrue(IsClose(ev.center, 100, 1));
   AssertTrue(!wcscmp(model->describeSlot(sundayNoon), L"weekends 12:00-13:00"));

   AnomalyScore score = ScoreAlone(*model, settings, wednesdayNoon, 151);
   AssertTrue(score.type == AnomalyType::NONE);
   AssertTrue(score.scored && !score.learning);
   AssertTrue((score.expectedLow < 150) && (score.expectedHigh > 150));

   // Level that is normal for a weekday is an anomaly on Sunday and vice versa
   AssertTrue(ScoreAlone(*model, settings, sundayNoon, 150).type == AnomalyType::RANGE);
   AssertTrue(ScoreAlone(*model, settings, wednesdayNoon, 100).type == AnomalyType::RANGE);

   settings.direction = AnomalyDirection::DOWN;
   AssertTrue(ScoreAlone(*model, settings, sundayNoon, 150).type == AnomalyType::NONE);
   AssertTrue(ScoreAlone(*model, settings, wednesdayNoon, 100).type == AnomalyType::RANGE);

   settings.direction = AnomalyDirection::UP;
   AssertTrue(ScoreAlone(*model, settings, sundayNoon, 150).type == AnomalyType::RANGE);
   AssertTrue(ScoreAlone(*model, settings, wednesdayNoon, 100).type == AnomalyType::NONE);

   // Model survives serialization
   json_t *json = model->toJson();
   shared_ptr<AnomalyModel> copy = AnomalyModel::createFromJson(json);
   AssertNotNull(copy.get());
   AssertTrue(copy->getMode() == model->getMode());
   AssertEquals(copy->getTrainingDays(), model->getTrainingDays());
   AssertTrue(IsClose(copy->getStepScale(), model->getStepScale(), 1e-9));
   AssertTrue(IsClose(copy->getDriftScale(), model->getDriftScale(), 1e-9));
   AssertEquals(copy->getStepMaxGap(), model->getStepMaxGap());
   for(int hour = 0; hour < 168; hour++)
   {
      time_t t = wednesdayNoon + hour * 3600;
      ExpectedValue e1 = model->getExpectedValue(t), e2 = copy->getExpectedValue(t);
      AssertTrue(IsClose(e1.center, e2.center, 1e-9) && IsClose(e1.scale, e2.scale, 1e-9) && (e1.level == e2.level));
   }

   // Model without usable drift scale is rejected
   json_t *damaged = json_deep_copy(json);
   json_object_set_new(damaged, "driftScale", json_real(0));
   AssertNull(AnomalyModel::createFromJson(damaged).get());
   json_object_del(damaged, "driftScale");
   AssertNull(AnomalyModel::createFromJson(damaged).get());
   json_decref(damaged);

   // Model of other version is rejected
   json_object_set_new(json, "version", json_integer(1));
   AssertNull(AnomalyModel::createFromJson(json).get());
   json_decref(json);

   EndTest();
}

/**
 * Series where one day of week has its own level
 */
static void TestWeekdayBaseline()
{
   StartTest(_T("Anomaly detection - individual weekday baseline"));

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 56,
      [] (int day, const struct tm& lt, int index) -> double
      {
         bool businessHours = (lt.tm_wday >= 1) && (lt.tm_wday <= 5) && (lt.tm_hour >= 9) && (lt.tm_hour < 18);
         return (businessHours ? ((lt.tm_wday == 2) ? 200.0 : 150.0) : 100.0) + Noise() * 2;
      }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->isUsingWeekdays());

   time_t tuesdayNoon = TimeAt(2026, 3, 3, 12);
   time_t wednesdayNoon = TimeAt(2026, 3, 4, 12);

   ExpectedValue ev = model->getExpectedValue(tuesdayNoon);
   AssertTrue(ev.level == AnomalySlotLevel::WEEKDAY);
   AssertTrue(IsClose(ev.center, 200, 1));
   AssertTrue(!wcscmp(model->describeSlot(tuesdayNoon), L"Tuesday 12:00-13:00"));

   // Days that do not differ from common weekday level stay on weekday/weekend baseline
   ev = model->getExpectedValue(wednesdayNoon);
   AssertTrue(ev.level == AnomalySlotLevel::DAY_TYPE);
   AssertTrue(IsClose(ev.center, 150, 1));

   AssertTrue(ScoreAlone(*model, settings, tuesdayNoon, 200).type == AnomalyType::NONE);
   AssertTrue(ScoreAlone(*model, settings, tuesdayNoon, 150).type == AnomalyType::RANGE);
   AssertTrue(ScoreAlone(*model, settings, wednesdayNoon, 200).type == AnomalyType::RANGE);

   EndTest();
}

/**
 * Short history: learning state and slot fallback
 */
static void TestSparseHistory()
{
   StartTest(_T("Anomaly detection - learning state and slot fallback"));

   AnomalySettings settings;

   std::vector<AnomalySample> empty;
   AssertNull(AnomalyModel::train(empty, empty, settings).get());

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 3, [] (int day, const struct tm& lt, int index) -> double { return 100 + Noise(); }, &series);
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertEquals(model->getTrainingDays(), 3);
   AssertTrue(model->isLearning(settings));
   AnomalyScore score = ScoreAlone(*model, settings, TimeAt(2026, 1, 8, 12), 100000);
   AssertTrue(score.learning && !score.scored);
   AssertTrue(score.type == AnomalyType::NONE);

   // Hard bounds are checked while model is learning
   AnomalySettings bounded;
   bounded.hardMax = 1000;
   score = ScoreAlone(*model, bounded, TimeAt(2026, 1, 8, 12), 100000);
   AssertTrue(score.learning && !score.scored);
   AssertTrue(score.type == AnomalyType::RANGE);
   AssertTrue(ScoreAlone(*model, bounded, TimeAt(2026, 1, 8, 12), 500).type == AnomalyType::NONE);

   // Eight days from Monday: six weekdays are enough for weekday slots, two weekend days are not
   series.clear();
   Generate(TimeAt(2026, 1, 5, 0), 8, [] (int day, const struct tm& lt, int index) -> double { return 100 + Noise(); }, &series);
   model = Train(series, settings);
   AssertNotNull(model.get());
   AssertFalse(model->isLearning(settings));
   AssertTrue(model->getExpectedValue(TimeAt(2026, 1, 14, 12)).level == AnomalySlotLevel::DAY_TYPE);
   AssertTrue(model->getExpectedValue(TimeAt(2026, 1, 17, 12)).level == AnomalySlotLevel::HOUR);
   AssertTrue(!wcscmp(model->describeSlot(TimeAt(2026, 1, 17, 12)), L"all days 12:00-13:00"));

   settings.minTrainingDays = 14;
   AssertTrue(model->isLearning(settings));

   EndTest();
}

/**
 * Constant series: spread floor
 */
static void TestFlatSeries()
{
   StartTest(_T("Anomaly detection - flat series"));

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 14, [] (int day, const struct tm& lt, int index) -> double { return 5; }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getMode() == AnomalyDetectionMode::VALUE);
   AssertEquals(model->getFlooredSlotCount(), 168);

   time_t t = TimeAt(2026, 1, 20, 12);
   ExpectedValue ev = model->getExpectedValue(t);
   AssertTrue(ev.valid && (ev.scale > 0));

   AnomalyScore score = ScoreAlone(*model, settings, t, 5);
   AssertTrue(score.type == AnomalyType::NONE);
   AssertTrue(std::isfinite(score.score));

   score = ScoreAlone(*model, settings, t, 6);
   AssertTrue(score.type == AnomalyType::RANGE);
   AssertTrue(std::isfinite(score.score));

   // Non-finite value is not scored
   AssertFalse(ScoreAlone(*model, settings, t, NAN).scored);

   EndTest();
}

/**
 * Steadily growing series: rate of change mode
 */
static void TestMonotoneSeries()
{
   StartTest(_T("Anomaly detection - monotone series"));

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 14, [] (int day, const struct tm& lt, int index) -> double { return 1000.0 + index * 2 + Noise() * 0.5; }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getMode() == AnomalyDetectionMode::RATE);
   AssertTrue(model->getRequestedMode() == AnomalyDetectionMode::AUTO);
   AssertTrue(model->getStepScale() == 0);

   const AnomalySample& last = series[series.size() - 1];
   time_t t = last.timestamp + SAMPLE_INTERVAL;
   ExpectedValue ev = model->getExpectedValue(t);
   AssertTrue(IsClose(ev.center, 2.0 / SAMPLE_INTERVAL, 0.001));

   AnomalyDetectorState state;
   AnomalyScore score = ScoreAnomalySample(*model, settings, &state, last.timestamp, last.value);
   AssertFalse(score.scored);   // rate is not known for first sample
   AssertTrue(score.type == AnomalyType::NONE);

   score = ScoreAnomalySample(*model, settings, &state, t, last.value + 2);
   AssertTrue(score.scored);
   AssertTrue(score.type == AnomalyType::NONE);

   score = ScoreAnomalySample(*model, settings, &state, t + SAMPLE_INTERVAL, last.value + 5000);
   AssertTrue(score.type == AnomalyType::RANGE);

   // Explicit mode overrides automatic selection
   settings.mode = AnomalyDetectionMode::VALUE;
   model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getMode() == AnomalyDetectionMode::VALUE);
   AssertTrue(model->getRequestedMode() == AnomalyDetectionMode::VALUE);

   // Requested mode is kept in serialized model; model saved without it is treated as trained in automatic mode
   json_t *json = model->toJson();
   shared_ptr<AnomalyModel> copy = AnomalyModel::createFromJson(json);
   AssertNotNull(copy.get());
   AssertTrue(copy->getRequestedMode() == AnomalyDetectionMode::VALUE);
   json_object_del(json, "requestedMode");
   copy = AnomalyModel::createFromJson(json);
   json_decref(json);
   AssertNotNull(copy.get());
   AssertTrue(copy->getMode() == AnomalyDetectionMode::VALUE);
   AssertTrue(copy->getRequestedMode() == AnomalyDetectionMode::AUTO);

   EndTest();
}

/**
 * Level used by tests with regular change of level: high during business hours
 */
static double BusinessHoursLevel(int day, const struct tm& lt, int index)
{
   return (((lt.tm_hour >= 9) && (lt.tm_hour < 18)) ? 500.0 : 50.0) + Noise();
}

/**
 * Replay generated series through the model and count samples with given anomaly type
 */
static int CountAnomalies(const AnomalyModel& model, const AnomalySettings& settings, const std::vector<AnomalySample>& series, AnomalyType type)
{
   AnomalyDetectorState state;
   int count = 0;
   for(size_t i = 0; i < series.size(); i++)
      if (ScoreAnomalySample(model, settings, &state, series[i].timestamp, series[i].value).type == type)
         count++;
   return count;
}

/**
 * Parameters of generated counter
 */
static int s_counterIndexOffset = 0;
static double s_counterStep = 10;

/**
 * Steadily growing counter with noise on the level (as opposed to noise on increments)
 */
static double NoisyCounter(int day, const struct tm& lt, int index)
{
   return (index + s_counterIndexOffset) * s_counterStep + Noise() * 50;
}

/**
 * Drift detection in rate mode
 */
static void TestRateDrift()
{
   StartTest(_T("Anomaly detection - drift in rate mode"));

   const int samplesPerDay = 86400 / SAMPLE_INTERVAL;
   s_counterIndexOffset = 0;
   s_counterStep = 10;
   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 28, NoisyCounter, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getMode() == AnomalyDetectionMode::RATE);

   // Same growth: noise on the level is much larger than increment, but hourly growth is steady
   s_counterIndexOffset = 28 * samplesPerDay;
   std::vector<AnomalySample> replay;
   Generate(TimeAt(2026, 2, 2, 0), 14, NoisyCounter, &replay);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::DRIFT), 0);

   // Growth becomes 20% faster: not visible in single sample, reported as drift within a day
   AnomalyDetectorState state;
   for(size_t i = 0; i < replay.size(); i++)
      ScoreAnomalySample(*model, settings, &state, replay[i].timestamp, replay[i].value);
   double level = (42 * samplesPerDay) * s_counterStep;
   time_t start = TimeAt(2026, 2, 16, 0);
   int first = -1;
   for(int i = 0; i < samplesPerDay; i++)
   {
      level += s_counterStep * 1.2;
      AnomalyScore score = ScoreAnomalySample(*model, settings, &state, start + static_cast<time_t>(i) * SAMPLE_INTERVAL, level + Noise() * 50);
      AssertTrue(score.type != AnomalyType::RANGE);
      if ((score.type == AnomalyType::DRIFT) && (first == -1))
         first = i;
   }
   AssertTrue(first > 0);

   EndTest();
}

/**
 * Regular change of level between slots is not a step
 */
static void TestExpectedLevelChange()
{
   StartTest(_T("Anomaly detection - expected level change"));

   // Sharp change of level at the same time every day
   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 28, BusinessHoursLevel, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getStepScale() > 0);

   std::vector<AnomalySample> replay;
   Generate(TimeAt(2026, 2, 2, 0), 7, BusinessHoursLevel, &replay);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::STEP), 0);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::RANGE), 0);

   // Same with level that slowly wanders, so slot range is wide and only step check can react
   ValueGenerator generator = [] (int day, const struct tm& lt, int index) -> double { return BusinessHoursLevel(day, lt, index) + 50 * sin(index * 6.283185 / (5 * 86400 / SAMPLE_INTERVAL)); };
   series.clear();
   Generate(TimeAt(2026, 1, 5, 0), 28, generator, &series);
   model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getStepScale() > 0);

   replay.clear();
   Generate(TimeAt(2026, 2, 2, 0), 7, generator, &replay);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::STEP), 0);

   time_t t = TimeAt(2026, 2, 3, 9);
   ExpectedValue before = model->getExpectedValue(t - SAMPLE_INTERVAL);
   ExpectedValue after = model->getExpectedValue(t);
   AssertTrue(IsClose(after.center - before.center, 450, 5));
   AssertTrue((before.scale > 15) && (after.scale > 15));

   // Expected change at slot boundary
   AnomalyDetectorState state;
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t - SAMPLE_INTERVAL, before.center).type == AnomalyType::NONE);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t, after.center).type == AnomalyType::NONE);

   // Change of similar size when level is not expected to change
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t + SAMPLE_INTERVAL, after.center - 30).type == AnomalyType::STEP);

   // Change at slot boundary that is much larger than expected one
   state.reset();
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t - SAMPLE_INTERVAL, before.center - 30).type == AnomalyType::NONE);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t, after.center + 30).type == AnomalyType::STEP);

   // Level does not change when it is expected to: value is far from the baseline, but there is no step
   state.reset();
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t - SAMPLE_INTERVAL, before.center + 30).type == AnomalyType::NONE);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t, before.center + 30).type != AnomalyType::STEP);

   EndTest();
}

/**
 * Sudden change that stays within wide slot range
 */
static void TestStepDetection()
{
   StartTest(_T("Anomaly detection - step change"));

   // Level changes from day to day, so slot range is wide, but consecutive samples are close
   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 28, [] (int day, const struct tm& lt, int index) -> double { return 100.0 + ((day * 37) % 100) + Noise(); }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(model->getMode() == AnomalyDetectionMode::VALUE);
   AssertTrue(model->getStepScale() > 0);
   AssertEquals(model->getStepMaxGap(), SAMPLE_INTERVAL * 3);

   time_t t = TimeAt(2026, 2, 3, 12);
   ExpectedValue ev = model->getExpectedValue(t);
   AssertTrue(ev.scale > 15);

   AnomalyDetectorState state;
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t, ev.center).type == AnomalyType::NONE);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t + SAMPLE_INTERVAL, ev.center + 1).type == AnomalyType::NONE);

   AnomalyScore score = ScoreAnomalySample(*model, settings, &state, t + SAMPLE_INTERVAL * 2, ev.center + 31);
   AssertTrue(score.type == AnomalyType::STEP);
   AssertTrue(score.score > settings.sensitivity);

   // Same change in direction that is not of interest
   settings.direction = AnomalyDirection::DOWN;
   state.reset();
   ScoreAnomalySample(*model, settings, &state, t, ev.center);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t + SAMPLE_INTERVAL, ev.center + 30).type == AnomalyType::NONE);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t + SAMPLE_INTERVAL * 2, ev.center).type == AnomalyType::STEP);

   // Change after long gap between samples is not a step
   settings.direction = AnomalyDirection::BOTH;
   state.reset();
   ScoreAnomalySample(*model, settings, &state, t, ev.center);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, t + 7200, ev.center + 30).type == AnomalyType::NONE);

   EndTest();
}

/**
 * Feed generated samples to the detector. Returns index of first sample reported as anomaly of given type or -1.
 */
static int FeedSamples(const AnomalyModel& model, const AnomalySettings& settings, AnomalyDetectorState *state, time_t start, int count,
   double shift, AnomalyType type, AnomalyScore *firstMatch)
{
   int first = -1;
   for(int i = 0; i < count; i++)
   {
      AnomalyScore score = ScoreAnomalySample(model, settings, state, start + static_cast<time_t>(i) * SAMPLE_INTERVAL, 100.0 + shift + Noise() * 10);
      if ((score.type == type) && (first == -1))
      {
         first = i;
         if (firstMatch != nullptr)
            *firstMatch = score;
      }
   }
   return first;
}

/**
 * Sustained small shift of level
 */
static void TestDriftDetection()
{
   StartTest(_T("Anomaly detection - drift"));

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 28, [] (int day, const struct tm& lt, int index) -> double { return 100.0 + Noise() * 10; }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue((model->getDriftScale() > 0.5) && (model->getDriftScale() < 4));

   const int samplesPerHour = 3600 / SAMPLE_INTERVAL;
   time_t start = TimeAt(2026, 2, 2, 0);
   AnomalyDetectorState state;

   // Two days without changes
   AssertEquals(FeedSamples(*model, settings, &state, start, samplesPerHour * 48, 0, AnomalyType::DRIFT, nullptr), -1);

   // Short excursion (two samples one standard deviation above normal) followed by normal day
   start += 48 * 3600;
   AssertEquals(FeedSamples(*model, settings, &state, start, 2, 7, AnomalyType::DRIFT, nullptr), -1);
   AssertEquals(FeedSamples(*model, settings, &state, start + SAMPLE_INTERVAL * 2, samplesPerHour * 24 - 2, 0, AnomalyType::DRIFT, nullptr), -1);

   // Sustained shift well within range of single sample
   start += 24 * 3600;
   AnomalyDetectorState stateCopy = state;
   AnomalyScore match;
   AssertEquals(FeedSamples(*model, settings, &stateCopy, start, samplesPerHour * 24, 4, AnomalyType::RANGE, nullptr), -1);
   int first = FeedSamples(*model, settings, &state, start, samplesPerHour * 24, 4, AnomalyType::DRIFT, &match);
   AssertTrue((first > 0) && (first <= samplesPerHour * 12));
   AssertTrue(match.score > settings.driftH);
   // Onset is where drift sum last left zero, which noise can place some hours before the actual shift
   AssertTrue((match.driftOnset >= start - 24 * 3600) && (match.driftOnset <= start + static_cast<time_t>(first) * SAMPLE_INTERVAL));

   // Drift in direction that is not of interest is not reported
   AnomalySettings downOnly;
   downOnly.direction = AnomalyDirection::DOWN;
   AssertTrue(ScoreAnomalySample(*model, downOnly, &stateCopy, start + 24 * 3600, 100).type == AnomalyType::NONE);
   AssertTrue(ScoreAnomalySample(*model, settings, &stateCopy, start + 24 * 3600 + SAMPLE_INTERVAL, 100).type == AnomalyType::DRIFT);

   // Drift indication clears after level returns to normal
   start += 24 * 3600;
   FeedSamples(*model, settings, &state, start, samplesPerHour * 24, 0, AnomalyType::DRIFT, nullptr);
   AssertTrue(ScoreAnomalySample(*model, settings, &state, start + 24 * 3600, 100).type == AnomalyType::NONE);

   // Downward shift, cleared by drift state reset
   start += 24 * 3600 + SAMPLE_INTERVAL;
   first = FeedSamples(*model, settings, &state, start, samplesPerHour * 24, -4, AnomalyType::DRIFT, &match);
   AssertTrue((first > 0) && (first <= samplesPerHour * 12));
   AssertTrue(match.score < -settings.driftH);
   state.resetDrift();
   AssertTrue(ScoreAnomalySample(*model, settings, &state, start + 24 * 3600, 100).type == AnomalyType::NONE);

   // Series where level differs from day to day: deviation that lasts for a day is normal and is not reported as drift
   std::vector<AnomalySample> unevenSeries;
   Generate(TimeAt(2026, 1, 5, 0), 28, [] (int day, const struct tm& lt, int index) -> double { return 100.0 + ((day * 7) % 5 - 2) * 3 + Noise() * 10; }, &unevenSeries);
   shared_ptr<AnomalyModel> unevenModel = Train(unevenSeries, settings);
   AssertNotNull(unevenModel.get());
   AssertTrue(unevenModel->getDriftScale() > model->getDriftScale() * 3);
   AnomalyDetectorState unevenState;
   AssertEquals(FeedSamples(*unevenModel, settings, &unevenState, TimeAt(2026, 2, 2, 0), samplesPerHour * 24, 4, AnomalyType::DRIFT, nullptr), -1);

   // Hard bounds are checked regardless of the baseline
   settings.hardMax = 105;
   AssertTrue(ScoreAlone(*model, settings, start, 104).type == AnomalyType::NONE);
   AssertTrue(ScoreAlone(*model, settings, start, 106).type == AnomalyType::RANGE);

   EndTest();
}

#ifndef _WIN32

/**
 * Slots follow local time across daylight saving time change
 */
static void TestDaylightSavingTime()
{
   StartTest(_T("Anomaly detection - daylight saving time"));

   char *savedTimeZone = MemCopyStringA(getenv("TZ"));
   setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
   tzset();

   // Daylight saving time starts on March 29, in the middle of the series
   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 3, 2, 0), 56, [] (int day, const struct tm& lt, int index) -> double { return ((lt.tm_hour == 8) ? 200.0 : 100.0) + Noise(); }, &series);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = Train(series, settings);
   AssertNotNull(model.get());
   AssertTrue(IsClose(model->getExpectedValue(TimeAt(2026, 4, 29, 7)).center, 100, 1));
   AssertTrue(IsClose(model->getExpectedValue(TimeAt(2026, 4, 29, 8)).center, 200, 1));
   AssertTrue(IsClose(model->getExpectedValue(TimeAt(2026, 4, 29, 9)).center, 100, 1));
   AssertTrue(model->getExpectedValue(TimeAt(2026, 4, 29, 8)).scale < 3);

   if (savedTimeZone != nullptr)
      setenv("TZ", savedTimeZone, 1);
   else
      unsetenv("TZ");
   tzset();
   MemFree(savedTimeZone);

   EndTest();
}

/**
 * Hourly averages follow hours of local time in time zone with offset that is not a whole number of hours
 */
static void TestFractionalTimeZone()
{
   StartTest(_T("Anomaly detection - time zone with half hour offset"));

   char *savedTimeZone = MemCopyStringA(getenv("TZ"));
   setenv("TZ", "IST-5:30", 1);
   tzset();

   std::vector<AnomalySample> series;
   Generate(TimeAt(2026, 1, 5, 0), 28, BusinessHoursLevel, &series);

   std::vector<AnomalySample> hourly;
   AggregateAnomalySamplesHourly(series, &hourly);
   AssertEquals(static_cast<int>(hourly.size()), 28 * 24);
   AssertEquals(static_cast<int>(hourly[0].timestamp % 3600), 1800);
   AssertTrue(hourly[0].timestamp == series[0].timestamp);

   AnomalySettings settings;
   shared_ptr<AnomalyModel> model = AnomalyModel::train(hourly, series, settings);
   AssertNotNull(model.get());
   AssertTrue(IsClose(model->getExpectedValue(TimeAt(2026, 2, 3, 8)).center, 50, 1));
   AssertTrue(model->getExpectedValue(TimeAt(2026, 2, 3, 8)).scale < 3);
   AssertTrue(IsClose(model->getExpectedValue(TimeAt(2026, 2, 3, 9)).center, 500, 1));
   AssertTrue(IsClose(model->getExpectedValue(TimeAt(2026, 2, 3, 17)).center, 500, 1));
   AssertTrue(model->getExpectedValue(TimeAt(2026, 2, 3, 17)).scale < 3);
   AssertTrue(model->getDriftScale() < 3);

   std::vector<AnomalySample> replay;
   Generate(TimeAt(2026, 2, 2, 0), 7, BusinessHoursLevel, &replay);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::DRIFT), 0);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::RANGE), 0);
   AssertEquals(CountAnomalies(*model, settings, replay, AnomalyType::STEP), 0);

   if (savedTimeZone != nullptr)
      setenv("TZ", savedTimeZone, 1);
   else
      unsetenv("TZ");
   tzset();
   MemFree(savedTimeZone);

   EndTest();
}

#endif

/**
 * Anomaly detection tests
 */
void TestAnomalyDetection()
{
   TestSettings();
   TestSettingsValidation();
   TestDetectorState();
   TestSeasonalBaseline();
   TestWeekdayBaseline();
   TestSparseHistory();
   TestFlatSeries();
   TestMonotoneSeries();
   TestStepDetection();
   TestExpectedLevelChange();
   TestDriftDetection();
   TestRateDrift();
#ifndef _WIN32
   TestDaylightSavingTime();
   TestFractionalTimeZone();
#endif
}
