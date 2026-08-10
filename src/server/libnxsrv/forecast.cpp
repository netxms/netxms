/*
** NetXMS - Network Management System
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
** File: forecast.cpp
**
**/

#include <nxforecast.h>
#include <math.h>

/**
 * Thread-safe localtime
 */
static inline void LocalTime(time_t t, struct tm *result)
{
#if HAVE_LOCALTIME_R
   localtime_r(&t, result);
#elif defined(_WIN32)
   localtime_s(result, &t);
#else
   *result = *localtime(&t);
#endif
}

/**
 * Get local midnight of the day containing given moment
 */
static time_t DayStart(time_t t)
{
   struct tm tmBuffer;
   LocalTime(t, &tmBuffer);
   tmBuffer.tm_hour = 0;
   tmBuffer.tm_min = 0;
   tmBuffer.tm_sec = 0;
   tmBuffer.tm_isdst = -1;
   return mktime(&tmBuffer);
}

/**
 * Get local midnight of the day after the day starting at given local midnight
 */
static time_t NextDayStart(time_t dayStart)
{
   struct tm tmBuffer;
   LocalTime(dayStart, &tmBuffer);
   tmBuffer.tm_mday++;
   tmBuffer.tm_hour = 0;
   tmBuffer.tm_min = 0;
   tmBuffer.tm_sec = 0;
   tmBuffer.tm_isdst = -1;
   return mktime(&tmBuffer);
}

/**
 * Extract calendar fields for a day given by its local midnight. Uses noon of
 * that day so DST transitions cannot shift the result to a neighboring day.
 */
static void GetDayFields(time_t dayStart, struct tm *fields)
{
   LocalTime(dayStart + 43200, fields);
}

/**
 * Check that month/day is a valid calendar date. Year 0 means a recurring
 * date, where February 29 is allowed.
 */
static bool IsValidMonthDay(uint32_t month, uint32_t day, uint32_t year)
{
   static const uint32_t daysInMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
   if ((month < 1) || (month > 12) || (day < 1))
      return false;
   uint32_t maxDay = daysInMonth[month - 1];
   if ((month == 2) && (year != 0) && !((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))))
      maxDay = 28;
   return day <= maxDay;
}

/**
 * Parse holiday list in format "MM-DD" (recurring) or "YYYY-MM-DD" (one-off),
 * comma-separated. Malformed entries are skipped; returns false if any entry
 * was skipped.
 */
bool HolidayCalendar::parse(const wchar_t *list)
{
   m_recurringDates.clear();
   m_oneOffDates.clear();

   bool success = true;
   StringList entries(list, L",");
   for(int i = 0; i < entries.size(); i++)
   {
      wchar_t entry[64];
      wcslcpy(entry, entries.get(i), 64);
      TrimW(entry);
      if (entry[0] == 0)
         continue;

      // %n against the entry length rejects trailing garbage, so an entry
      // like "12-25 01-01" (wrong list separator) is reported instead of
      // being silently accepted as its "12-25" prefix
      int len = static_cast<int>(wcslen(entry));
      uint32_t f1, f2, f3;
      int pos = -1;
      if ((swscanf(entry, L"%u-%u%n", &f1, &f2, &pos) == 2) && (pos == len) && IsValidMonthDay(f1, f2, 0))
      {
         m_recurringDates.add(f1 * 100 + f2);
         continue;
      }
      pos = -1;
      if ((swscanf(entry, L"%u-%u-%u%n", &f1, &f2, &f3, &pos) == 3) && (pos == len) &&
          (f1 >= 1970) && (f1 <= 2199) && IsValidMonthDay(f2, f3, f1))
      {
         m_oneOffDates.add(f1 * 10000 + f2 * 100 + f3);
         continue;
      }
      success = false;
   }
   return success;
}

/**
 * Set reference weekday (tm_wday value); out of range values are ignored
 */
void HolidayCalendar::setReferenceDay(int dayOfWeek)
{
   if ((dayOfWeek >= 0) && (dayOfWeek <= 6))
      m_referenceDay = dayOfWeek;
}

/**
 * Check if given date is a holiday
 */
bool HolidayCalendar::contains(const struct tm& date) const
{
   uint32_t md = (date.tm_mon + 1) * 100 + date.tm_mday;
   return m_recurringDates.contains(md) || m_oneOffDates.contains((date.tm_year + 1900) * 10000 + md);
}

/**
 * Parse day of week given as number 0..6 or English day name (full or three
 * letter abbreviation, case-insensitive). Returns -1 on failure.
 */
int HolidayCalendar::parseDayOfWeek(const wchar_t *value)
{
   wchar_t v[32];
   wcslcpy(v, value, 32);
   TrimW(v);

   if ((v[0] >= L'0') && (v[0] <= L'6') && (v[1] == 0))
      return v[0] - L'0';

   static const wchar_t *names[] = { L"sunday", L"monday", L"tuesday", L"wednesday", L"thursday", L"friday", L"saturday" };
   size_t l = wcslen(v);
   for(int i = 0; i < 7; i++)
   {
      if (((l == 3) || (l == wcslen(names[i]))) && (wcsnicmp(v, names[i], l) == 0))
         return i;
   }
   return -1;
}

/**
 * Constructor: empty forecaster (isValid() == false, forecasts are 0)
 */
DailyActivityForecaster::DailyActivityForecaster(const DailyActivityForecasterConfig& config) : m_config(config)
{
   reset();
}

/**
 * Reset learned model (configuration is kept)
 */
void DailyActivityForecaster::reset()
{
   m_baseline = 0;
   m_sampleCount = 0;
   m_hasFactors = false;
   m_learnTime = 0;
   for(int i = 0; i < 7; i++)
   {
      m_dowFactor[i] = 1.0;
      m_dowWeight[i] = 0;
   }
   for(int i = 0; i < 31; i++)
   {
      m_domFactor[i] = 1.0;
      m_domWeight[i] = 0;
   }
}

/**
 * Learn profile from daily activity series. The series should only contain
 * usable days (excluded days omitted, not zero-filled); order does not
 * matter. Samples with negative values are ignored.
 */
void DailyActivityForecaster::learn(const StructArray<DailyActivitySample>& series, time_t now)
{
   reset();
   m_learnTime = now;

   double sumWeights = 0, sumWeightedValue = 0;
   double sumDomWeights = 0, sumDomWeightedValue = 0;
   double dowValue[7] = { 0 }, domValue[31] = { 0 };
   for(int i = 0; i < series.size(); i++)
   {
      const DailyActivitySample *s = series.get(i);
      if (s->value < 0)
         continue;   // counter glitch; a negative sample would produce negative factors and forecasts

      m_sampleCount++;
      double age = static_cast<double>(now - s->day) / 86400.0;
      if (age < 0)
         age = 0;

      double w = pow(0.5, age / m_config.recencyHalfLifeDays);
      sumWeights += w;
      sumWeightedValue += w * s->value;

      struct tm dayFields;
      GetDayFields(s->day, &dayFields);
      m_dowWeight[dayFields.tm_wday] += w;
      dowValue[dayFields.tm_wday] += w * s->value;

      double wm = pow(0.5, age / m_config.domHalfLifeDays);
      sumDomWeights += wm;
      sumDomWeightedValue += wm * s->value;
      m_domWeight[dayFields.tm_mday - 1] += wm;
      domValue[dayFields.tm_mday - 1] += wm * s->value;
   }

   m_baseline = (sumWeights > 0) ? sumWeightedValue / sumWeights : 0;
   if ((m_sampleCount < m_config.minFactorDays) || (m_baseline <= 0))
      return;  // flat baseline only, factors stay at 1.0

   // Shrinkage-regularized factors: weighted mean for the slot divided by
   // baseline, with shrinkage acting as pseudo-weight of baseline-level samples
   double factorSum = 0;
   for(int i = 0; i < 7; i++)
   {
      m_dowFactor[i] = (dowValue[i] + m_config.dowShrinkage * m_baseline) / ((m_dowWeight[i] + m_config.dowShrinkage) * m_baseline);
      factorSum += m_dowFactor[i];
   }

   // Normalize day-of-week factors to mean 1.0 so they redistribute the
   // baseline rather than scale it
   double factorMean = factorSum / 7;
   if (factorMean > 0)
   {
      for(int i = 0; i < 7; i++)
         m_dowFactor[i] /= factorMean;
   }

   // Day-of-month slot means are discounted with the longer DOM half-life, so
   // they are compared against a baseline computed with the same weights - a
   // recent trend would otherwise shift all 31 factors in the same direction
   double domBaseline = (sumDomWeights > 0) ? sumDomWeightedValue / sumDomWeights : 0;
   if (domBaseline > 0)
   {
      factorSum = 0;
      double weightSum = 0;
      for(int i = 0; i < 31; i++)
      {
         m_domFactor[i] = (domValue[i] + m_config.domShrinkage * domBaseline) / ((m_domWeight[i] + m_config.domShrinkage) * domBaseline);
         factorSum += m_domFactor[i] * m_domWeight[i];
         weightSum += m_domWeight[i];
      }

      // Normalize day-of-month factors to mean 1.0 as well; slots occur with
      // different frequency (short months), so the mean is sample-weighted
      factorMean = (weightSum > 0) ? factorSum / weightSum : 1.0;
      if (factorMean > 0)
      {
         for(int i = 0; i < 31; i++)
            m_domFactor[i] /= factorMean;
      }
   }

   m_hasFactors = true;
}

/**
 * Forecast activity for the day starting at given local midnight. Returns
 * baseline (0 for an empty forecaster) when factors are not available.
 */
double DailyActivityForecaster::dailyForecast(time_t day, const HolidayCalendar& holidays, const ForecastAdjustment *adjustment) const
{
   double forecast;
   if (m_hasFactors)
   {
      struct tm dayFields;
      GetDayFields(day, &dayFields);
      int dow = holidays.contains(dayFields) ? holidays.getReferenceDay() : dayFields.tm_wday;
      forecast = m_baseline * m_dowFactor[dow] * m_domFactor[dayFields.tm_mday - 1];
   }
   else
   {
      forecast = m_baseline;
   }
   if (adjustment != nullptr)
   {
      double a = adjustment->getFactor(day);
      forecast *= (a > 0) ? a : 0;
   }
   return forecast;
}

/**
 * Predict the moment when cumulative forecast activity reaches given
 * remaining quantity. Returns 0 if the forecaster cannot predict
 * (insufficient data, zero activity) or the predicted moment is beyond the
 * forecast horizon; returns now if remaining quantity is already zero or
 * negative.
 */
time_t DailyActivityForecaster::predictThresholdCrossing(double remaining, time_t now, const HolidayCalendar& holidays, const ForecastAdjustment *adjustment) const
{
   if (!isValid() || (m_baseline <= 0))
      return 0;
   if (remaining <= 0)
      return now;

   time_t dayStart = DayStart(now);
   double cumulative = 0;
   for(int i = 0; i < m_config.horizonDays; i++)
   {
      time_t nextStart = NextDayStart(dayStart);
      time_t from = (i == 0) ? now : dayStart;
      double dayQuantity = dailyForecast(dayStart, holidays, adjustment);
      double effective = (i == 0) ? dayQuantity * (static_cast<double>(nextStart - now) / static_cast<double>(nextStart - dayStart)) : dayQuantity;
      if ((effective > 0) && (cumulative + effective >= remaining))
      {
         double fraction = (remaining - cumulative) / effective;
         return from + static_cast<time_t>(static_cast<double>(nextStart - from) * fraction);
      }
      cumulative += effective;
      dayStart = nextStart;
   }
   return 0;  // no crossing within horizon
}
