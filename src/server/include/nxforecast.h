/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxforecast.h
**
**/

#ifndef _nxforecast_h_
#define _nxforecast_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxsrvapi.h>

/**
 * Generic forecasting of daily activity with weekly/monthly rhythm.
 *
 * Model: daily activity is forecast as
 *    baseline * dowFactor[day of week] * domFactor[day of month] * adjustment
 * where baseline is a recency-weighted mean of observed daily activity, the
 * factors are shrinkage-regularized ratios learned from the same series, and
 * adjustment is an optional external per-day coefficient (e.g. weather)
 * supplied via the ForecastAdjustment interface. Dates in a holiday calendar
 * use the day-of-week factor of a configured reference day instead of their
 * actual weekday.
 *
 * The model is direction-neutral: activity is always a positive magnitude,
 * whether the underlying resource drains toward a minimum (cash, toner) or
 * grows toward a capacity (disk space). The caller computes the remaining
 * quantity as |current - threshold|.
 */

/**
 * One day of observed activity. Days excluded from learning (downtime,
 * faults) must be omitted from the series entirely, not zero-filled.
 * Negative values are ignored by learning.
 */
struct DailyActivitySample
{
   time_t day;    // local midnight of the observed day
   double value;  // activity during that day
};

/**
 * Holiday calendar: set of recurring (month/day) and one-off (specific date)
 * holidays plus the reference weekday holidays are forecast as.
 */
class LIBNXSRV_EXPORTABLE HolidayCalendar
{
private:
   IntegerArray<uint32_t> m_recurringDates;   // month * 100 + day
   IntegerArray<uint32_t> m_oneOffDates;      // year * 10000 + month * 100 + day
   int m_referenceDay;                        // tm_wday value (0 = Sunday)

public:
   HolidayCalendar() : m_referenceDay(0) { }

   bool parse(const wchar_t *list);
   void setReferenceDay(int dayOfWeek);

   int getReferenceDay() const { return m_referenceDay; }
   bool contains(const struct tm& date) const;
   bool isEmpty() const { return m_recurringDates.isEmpty() && m_oneOffDates.isEmpty(); }

   static int parseDayOfWeek(const wchar_t *value);
};

/**
 * Hook interface for external per-day forecast adjustment (e.g. weather
 * correction). Negative factors are treated as 0.
 */
class LIBNXSRV_EXPORTABLE ForecastAdjustment
{
public:
   virtual ~ForecastAdjustment() { }

   virtual double getFactor(time_t day) const = 0;   // multiplier for the day starting at given local midnight
};

/**
 * Tuning parameters for DailyActivityForecaster; defaults are the reference
 * values, subject to tuning via backtesting.
 */
struct DailyActivityForecasterConfig
{
   double recencyHalfLifeDays;   // half-life for baseline and day-of-week learning
   double domHalfLifeDays;       // longer half-life for day-of-month learning (one sample per month)
   double dowShrinkage;          // pseudo-weight pulling day-of-week factors toward 1.0
   double domShrinkage;          // pseudo-weight pulling day-of-month factors toward 1.0
   int minPredictionDays;        // fewer samples: no prediction at all
   int minFactorDays;            // fewer samples: flat baseline only, no factors
   int horizonDays;              // threshold crossing beyond this many days is reported as "none"

   DailyActivityForecasterConfig()
   {
      recencyHalfLifeDays = 14.0;
      domHalfLifeDays = 42.0;
      dowShrinkage = 1.0;
      domShrinkage = 3.0;
      minPredictionDays = 5;
      minFactorDays = 14;
      horizonDays = 90;
   }
};

/**
 * Daily activity forecaster: profile learned from a daily series; produces
 * per-day forecasts and threshold crossing predictions.
 */
class LIBNXSRV_EXPORTABLE DailyActivityForecaster
{
private:
   DailyActivityForecasterConfig m_config;
   double m_baseline;
   double m_dowFactor[7];
   double m_domFactor[31];
   double m_dowWeight[7];    // effective (recency-discounted) sample weight behind each factor
   double m_domWeight[31];
   int m_sampleCount;
   bool m_hasFactors;
   time_t m_learnTime;

   void reset();

public:
   DailyActivityForecaster(const DailyActivityForecasterConfig& config = DailyActivityForecasterConfig());

   void learn(const StructArray<DailyActivitySample>& series, time_t now);

   const DailyActivityForecasterConfig& getConfig() const { return m_config; }
   bool isValid() const { return m_sampleCount >= m_config.minPredictionDays; }
   bool hasFactors() const { return m_hasFactors; }
   int getSampleCount() const { return m_sampleCount; }
   time_t getLearnTime() const { return m_learnTime; }
   double getBaseline() const { return m_baseline; }
   double getDowFactor(int dayOfWeek) const { return ((dayOfWeek >= 0) && (dayOfWeek < 7)) ? m_dowFactor[dayOfWeek] : 1.0; }
   double getDomFactor(int dayOfMonth) const { return ((dayOfMonth >= 1) && (dayOfMonth <= 31)) ? m_domFactor[dayOfMonth - 1] : 1.0; }
   double getDowWeight(int dayOfWeek) const { return ((dayOfWeek >= 0) && (dayOfWeek < 7)) ? m_dowWeight[dayOfWeek] : 0.0; }
   double getDomWeight(int dayOfMonth) const { return ((dayOfMonth >= 1) && (dayOfMonth <= 31)) ? m_domWeight[dayOfMonth - 1] : 0.0; }

   double dailyForecast(time_t day, const HolidayCalendar& holidays, const ForecastAdjustment *adjustment = nullptr) const;
   time_t predictThresholdCrossing(double remaining, time_t now, const HolidayCalendar& holidays, const ForecastAdjustment *adjustment = nullptr) const;
};

#endif
