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
** File: nxcore_anomaly.h
**
**/

#ifndef _nxcore_anomaly_h_
#define _nxcore_anomaly_h_

#include <vector>

/**
 * Anomaly model format version
 */
#define ANOMALY_MODEL_VERSION    2

/**
 * Number of slots in anomaly model (hour of day x day of week)
 */
#define ANOMALY_SLOT_COUNT       168

/**
 * Anomaly type
 */
enum class AnomalyType
{
   NONE = 0,
   RANGE = 1,     // value is outside of expected range for current slot
   STEP = 2,      // sudden change between consecutive samples
   DRIFT = 3      // sustained shift of level relative to baseline
};

/**
 * Direction of deviations reported as anomalies
 */
enum class AnomalyDirection
{
   BOTH = 0,
   UP = 1,
   DOWN = 2
};

/**
 * Anomaly detection mode
 */
enum class AnomalyDetectionMode
{
   AUTO = 0,      // selected by training (rate of change for strongly monotone series)
   VALUE = 1,     // score collected value
   RATE = 2       // score rate of change (per second)
};

/**
 * Granularity of data used for a slot (ordered from least to most specific)
 */
enum class AnomalySlotLevel
{
   GLOBAL = 0,    // all data
   HOUR = 1,      // same hour of day, all days
   DAY_TYPE = 2,  // same hour of day, weekdays or weekend
   WEEKDAY = 3    // same hour of day, same day of week
};

/**
 * Single sample for anomaly model training
 */
struct AnomalySample
{
   time_t timestamp;
   double value;
};

/**
 * Per-DCI anomaly detection settings (set by user, never changed by training)
 */
struct NXCORE_EXPORTABLE AnomalySettings
{
   double sensitivity;              // threshold for robust z-score
   AnomalyDirection direction;
   AnomalyDetectionMode mode;
   int32_t minTrainingDays;         // model with less distinct days of data is in learning state
   double hardMin;                  // hard lower bound for value (NaN if not set)
   double hardMax;                  // hard upper bound for value (NaN if not set)
   double driftK;                   // drift detector slack (in standard deviations of hourly residual)
   double driftH;                   // drift detector decision threshold

   AnomalySettings();
   AnomalySettings(json_t *json);

   static bool validate(json_t *json);

   json_t *toJson() const;
   bool isDefault() const;
};

/**
 * Expected value at given point in time. Scoring code depends only on this structure
 * and not on the way expectation was produced.
 */
struct ExpectedValue
{
   double center;          // expected value
   double scale;           // standard deviation equivalent spread of single sample around center
   AnomalySlotLevel level; // granularity of data behind this expectation
   bool valid;             // false if no expectation is available
};

/**
 * Anomaly model (slot baseline). Immutable after training or loading.
 */
class NXCORE_EXPORTABLE AnomalyModel
{
private:
   AnomalyDetectionMode m_mode;     // VALUE or RATE
   AnomalyDetectionMode m_requestedMode;  // mode that training was requested with (AUTO if mode was selected by training)
   time_t m_trainedFrom;
   time_t m_trainedTo;
   int32_t m_trainingDays;          // distinct local days with data
   int32_t m_hourlySamples;
   int32_t m_rawSamples;
   int32_t m_flooredSlots;          // number of slots where spread was raised to the floor
   bool m_useWeekdays;              // at least one day of week has own baseline
   double m_centers[ANOMALY_SLOT_COUNT];
   double m_scales[ANOMALY_SLOT_COUNT];
   BYTE m_levels[ANOMALY_SLOT_COUNT];
   double m_stepCenter;             // typical change between consecutive samples
   double m_stepScale;              // spread of change between consecutive samples (0 if step check is not available)
   double m_slotChangeScale;        // spread of change between consecutive samples from different slots, relative to expected change of level
   int32_t m_stepMaxGap;            // maximum interval between samples for step check, in seconds
   double m_driftScale;             // spread of hourly mean residual

public:
   AnomalyModel();

   static shared_ptr<AnomalyModel> train(const std::vector<AnomalySample>& hourly, const std::vector<AnomalySample>& raw, const AnomalySettings& settings);
   static shared_ptr<AnomalyModel> createFromJson(json_t *json);
   json_t *toJson() const;

   ExpectedValue getExpectedValue(time_t timestamp) const;
   String describeSlot(time_t timestamp) const;

   AnomalyDetectionMode getMode() const { return m_mode; }
   AnomalyDetectionMode getRequestedMode() const { return m_requestedMode; }
   time_t getTrainedFrom() const { return m_trainedFrom; }
   time_t getTrainedTo() const { return m_trainedTo; }
   int32_t getTrainingDays() const { return m_trainingDays; }
   int32_t getHourlySampleCount() const { return m_hourlySamples; }
   int32_t getRawSampleCount() const { return m_rawSamples; }
   int32_t getFlooredSlotCount() const { return m_flooredSlots; }
   bool isUsingWeekdays() const { return m_useWeekdays; }
   double getStepCenter() const { return m_stepCenter; }
   double getStepScale() const { return m_stepScale; }
   double getSlotChangeScale() const { return m_slotChangeScale; }
   int32_t getStepMaxGap() const { return m_stepMaxGap; }
   double getDriftScale() const { return m_driftScale; }
   bool isLearning(const AnomalySettings& settings) const { return m_trainingDays < settings.minTrainingDays; }
};

/**
 * Runtime state of anomaly detector for single DCI
 */
struct NXCORE_EXPORTABLE AnomalyDetectorState
{
   double driftUp;            // upward drift sum
   double driftDown;          // downward drift sum
   time_t driftUpOnset;       // start of the hour when upward sum last left zero (0 if sum is zero)
   time_t driftDownOnset;     // start of the hour when downward sum last left zero (0 if sum is zero)
   time_t bucketStart;        // start of the hour being accumulated (0 if none)
   double bucketSum;          // sum of residuals within current hour
   int32_t bucketCount;       // number of residuals within current hour
   time_t lastTimestamp;      // timestamp of previous sample (0 if none)
   double lastValue;          // value of previous sample

   AnomalyDetectorState() { reset(); }

   void reset();
   void resetDrift();

   json_t *toJson() const;
   void loadFromJson(json_t *json);
};

/**
 * Result of scoring single sample
 */
struct AnomalyScore
{
   AnomalyType type;
   bool learning;             // model does not have enough data, sample was not scored
   bool scored;               // score and expected range are set
   double score;              // robust z-score for range and step, drift sum for drift
   double expectedLow;        // lower bound of expected range (value or rate, depending on model mode)
   double expectedHigh;       // upper bound of expected range
   time_t driftOnset;         // estimated start of level shift (only for drift)
};

NXCORE_EXPORTABLE const char *AnomalyTypeName(AnomalyType type);
NXCORE_EXPORTABLE const char *AnomalyDirectionName(AnomalyDirection direction);
NXCORE_EXPORTABLE const char *AnomalyDetectionModeName(AnomalyDetectionMode mode);

time_t GetAnomalyHourStart(time_t timestamp);
NXCORE_EXPORTABLE void AggregateAnomalySamplesHourly(const std::vector<AnomalySample>& raw, std::vector<AnomalySample> *hourly);
NXCORE_EXPORTABLE AnomalyScore ScoreAnomalySample(const AnomalyModel& model, const AnomalySettings& settings,
   AnomalyDetectorState *state, time_t timestamp, double value);

/**
 * Isolation tree node
 */
struct IsoTreeNode
{
   double splitValue;    // split threshold (for internal nodes)
   size_t left;          // left child index (0 = leaf)
   size_t right;         // right child index (0 = leaf)
   double score;         // leaf score = depth + c(remaining_count)
};

void FitIsolationForest(const double *data, size_t nrows, size_t ntrees, std::vector<std::vector<IsoTreeNode>> &trees, std::vector<size_t> &treeRoots);
std::vector<double> PredictIsolationForest(const double *data, size_t nrows, const std::vector<std::vector<IsoTreeNode>> &trees, const std::vector<size_t> &treeRoots);

#endif
