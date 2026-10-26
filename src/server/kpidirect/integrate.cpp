/*
** NetXMS - Network Management System
** Direct KPI computation provider
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
** File: integrate.cpp
**
** Per-series daily integration. Pure functions over the provider input types: no clock, no
** configuration, no global state. The result must not depend on the order of input samples.
**/

#include "kpidirect.h"
#include <algorithm>

/**
 * Stored sample after ordering and duplicate removal
 */
struct StoredSample
{
   time_t timestamp;
   double value;
   SampleQuality quality;
};

/**
 * Add series-level diagnostic
 */
static inline void AddDiagnostic(std::vector<ComputationDiagnostic> *diagnostics, ComputationDiagnosticCode code, uint32_t dciId, time_t from = 0, time_t to = 0, double detail = NAN)
{
   ComputationDiagnostic d;
   d.component = -1;
   d.dciId = dciId;
   d.code = code;
   d.from = from;
   d.to = to;
   d.detail = detail;
   diagnostics->push_back(d);
}

/**
 * Check if sample is usable: measured quality and finite value
 */
static inline bool IsMeasured(const StoredSample& s)
{
   return (s.quality == SampleQuality::MEASURED) && std::isfinite(s.value);
}

/**
 * Order samples by ascending timestamp and collapse duplicate timestamps. Of two samples at one timestamp the
 * MEASURED one wins, then the one with the higher finite value, so the outcome is independent of input order.
 */
static void PrepareSamples(const ResolvedSeries& series, std::vector<StoredSample> *stored, std::vector<ComputationDiagnostic> *diagnostics)
{
   std::vector<StoredSample> samples;
   samples.reserve(series.samples.size());
   for(const AttributedSample& s : series.samples)
   {
      StoredSample ss;
      ss.timestamp = s.timestamp.asTime();
      ss.value = s.value;
      ss.quality = s.attributes.quality;
      samples.push_back(ss);
   }
   std::sort(samples.begin(), samples.end(), [](const StoredSample& a, const StoredSample& b) { return a.timestamp < b.timestamp; });

   stored->reserve(samples.size());
   for(const StoredSample& s : samples)
   {
      if (!stored->empty() && (stored->back().timestamp == s.timestamp))
      {
         AddDiagnostic(diagnostics, ComputationDiagnosticCode::DUPLICATE_TIMESTAMP, series.binding.dciId, s.timestamp, s.timestamp);
         StoredSample& kept = stored->back();
         bool keptMeasured = (kept.quality == SampleQuality::MEASURED);
         bool newMeasured = (s.quality == SampleQuality::MEASURED);
         if (newMeasured != keptMeasured)
         {
            if (newMeasured)
               kept = s;
         }
         else
         {
            double keptValue = std::isfinite(kept.value) ? kept.value : -INFINITY;
            double newValue = std::isfinite(s.value) ? s.value : -INFINITY;
            if (newValue > keptValue)
               kept = s;
         }
      }
      else
      {
         stored->push_back(s);
      }
   }
}

/**
 * Nominal interval: declared polling interval if known, else the median of spacings between consecutive stored
 * timestamps (timestamps are unique after preparation, so every spacing is positive). Returns 0 if it cannot be
 * determined.
 */
static double NominalInterval(const ResolvedSeries& series, const std::vector<StoredSample>& stored)
{
   if (series.binding.nominalInterval > 0)
      return static_cast<double>(series.binding.nominalInterval);

   if (stored.size() < 2)
      return 0;

   std::vector<double> spacings;
   spacings.reserve(stored.size() - 1);
   for(size_t i = 1; i < stored.size(); i++)
      spacings.push_back(static_cast<double>(stored[i].timestamp - stored[i - 1].timestamp));
   std::sort(spacings.begin(), spacings.end());
   size_t n = spacings.size();
   return (n % 2 == 1) ? spacings[n / 2] : (spacings[n / 2 - 1] + spacings[n / 2]) / 2.0;
}

/**
 * Overlap of [a, b] with the day [dayStart, dayEnd). Returns false if there is none.
 */
static inline bool Overlap(time_t a, time_t b, time_t dayStart, time_t dayEnd, time_t *lo, time_t *hi)
{
   *lo = std::max(a, dayStart);
   *hi = std::min(b, dayEnd);
   return *hi > *lo;
}

/**
 * Integrate one classified series over the day [dayStart, dayEnd). Values are converted to canonical units with the
 * binding's unit factor before any arithmetic. Series-level diagnostics are appended to the given list.
 */
SeriesIntegrationResult IntegrateSeries(const ResolvedSeries& series, time_t dayStart, time_t dayEnd, std::vector<ComputationDiagnostic> *diagnostics)
{
   const ResolvedBinding& binding = series.binding;
   uint32_t dciId = binding.dciId;
   double factor = binding.unitFactor;
   double dayLength = static_cast<double>(dayEnd - dayStart);

   SeriesIntegrationResult result;
   result.value = 0;
   result.coverage = 0;
   result.accepted = false;

   std::vector<StoredSample> stored;
   PrepareSamples(series, &stored, diagnostics);

   double nominalInterval = NominalInterval(series, stored);
   if (nominalInterval <= 0)
   {
      AddDiagnostic(diagnostics, ComputationDiagnosticCode::TOO_FEW_SAMPLES, dciId);
      nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 6, L"DCI %u: refused, nominal interval cannot be determined", dciId);
      return result;
   }
   double gapLimit = std::min(KPIDIRECT_GAP_FACTOR * nominalInterval, KPIDIRECT_GAP_CAP);

   // Plausibility check applies to energy slots of binding objects with declared rated power
   bool energySlot = (binding.component != EnergyComponent::WIN) && (binding.component != EnergyComponent::WIN_FRE);
   double ratedPowerKW = binding.ratedPower / 1000.0;
   bool checkPlausibility = energySlot && (ratedPowerKW > 0);

   double value = 0;
   double covered = 0;

   if ((binding.kind == SeriesKind::RATE) || (binding.kind == SeriesKind::COUNTER))
   {
      // Usable samples: measured, finite, non-negative. Unusable samples are removed and their neighbours become consecutive.
      std::vector<StoredSample> usable;
      usable.reserve(stored.size());
      for(const StoredSample& s : stored)
      {
         if (!IsMeasured(s))
         {
            AddDiagnostic(diagnostics, ComputationDiagnosticCode::NON_MEASURED_SAMPLE, dciId, s.timestamp, s.timestamp);
            continue;
         }
         if (s.value < 0)
         {
            AddDiagnostic(diagnostics, ComputationDiagnosticCode::NEGATIVE_VALUE, dciId, s.timestamp, s.timestamp, s.value);
            continue;
         }
         StoredSample u = s;
         u.value = s.value * factor;
         usable.push_back(u);
      }
      if (usable.size() < 2)
      {
         AddDiagnostic(diagnostics, ComputationDiagnosticCode::TOO_FEW_SAMPLES, dciId);
         nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 6, L"DCI %u: refused, fewer than two usable samples", dciId);
         return result;
      }

      for(size_t i = 1; i < usable.size(); i++)
      {
         time_t ta = usable[i - 1].timestamp, tb = usable[i].timestamp;
         double va = usable[i - 1].value, vb = usable[i].value;
         time_t lo, hi;
         if (!Overlap(ta, tb, dayStart, dayEnd, &lo, &hi))
            continue;
         double dt = static_cast<double>(tb - ta);
         double span = static_cast<double>(hi - lo);

         if (binding.kind == SeriesKind::RATE)
         {
            if (dt > gapLimit + KPIDIRECT_EPS)
            {
               AddDiagnostic(diagnostics, ComputationDiagnosticCode::GAP_NOT_BRIDGED, dciId, lo, hi);
               nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 7, L"DCI %u: gap of %.0f s at %lld not bridged (limit %.0f s)", dciId, dt, static_cast<long long>(ta), gapLimit);
               continue;
            }
            // Trapezoid on the actual sample grid, cut at the day boundaries
            double vlo = va + (vb - va) * static_cast<double>(lo - ta) / dt;
            double vhi = va + (vb - va) * static_cast<double>(hi - ta) / dt;
            value += (vlo + vhi) / 2.0 * span / 3600.0;
            covered += span;
         }
         else
         {
            double increment = vb - va;
            if (increment < 0)
            {
               AddDiagnostic(diagnostics, ComputationDiagnosticCode::COUNTER_DECREASE, dciId, lo, hi, increment);
               nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 7, L"DCI %u: counter decrease of %f at %lld", dciId, increment, static_cast<long long>(tb));
               continue;
            }
            if (checkPlausibility && (increment / (dt / 3600.0) > KPIDIRECT_PLAUSIBILITY_FACTOR * ratedPowerKW + KPIDIRECT_EPS))
            {
               AddDiagnostic(diagnostics, ComputationDiagnosticCode::IMPLAUSIBLE_INCREMENT, dciId, lo, hi, increment / (dt / 3600.0));
               nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 7, L"DCI %u: implausible increment of %f at %lld (rated power %f kW)", dciId, increment, static_cast<long long>(tb), ratedPowerKW);
               continue;
            }
            // A pair across a day boundary needs a bridgeable spacing, because the split between the days is interpolated
            bool straddles = ((ta < dayStart) && (dayStart < tb)) || ((ta < dayEnd) && (dayEnd < tb));
            if (straddles && (dt > gapLimit + KPIDIRECT_EPS))
            {
               AddDiagnostic(diagnostics, ComputationDiagnosticCode::UNRESOLVED_BOUNDARY, dciId, lo, hi);
               nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 7, L"DCI %u: interval of %.0f s at %lld straddles day boundary and is not bridgeable", dciId, dt, static_cast<long long>(ta));
               continue;
            }
            // Full increment for a pair inside the day at any length (the register kept counting), linear share across a boundary
            value += increment * span / dt;
            covered += span;
         }
      }
   }
   else  // INTERVAL: each sample holds the quantity since the previous stored sample, whatever that sample's quality
   {
      size_t measuredCount = 0;
      for(const StoredSample& s : stored)
         if (IsMeasured(s))
            measuredCount++;
      if (measuredCount < 1)
      {
         AddDiagnostic(diagnostics, ComputationDiagnosticCode::TOO_FEW_SAMPLES, dciId);
         nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 6, L"DCI %u: refused, no measured samples", dciId);
         return result;
      }

      bool hasPredecessor = false;
      time_t start = 0;
      for(const StoredSample& s : stored)
      {
         time_t previous = start;
         bool hadPredecessor = hasPredecessor;
         start = s.timestamp;
         hasPredecessor = true;

         if (!IsMeasured(s))
         {
            AddDiagnostic(diagnostics, ComputationDiagnosticCode::NON_MEASURED_SAMPLE, dciId, s.timestamp, s.timestamp);
            continue;
         }
         if (!hadPredecessor)
         {
            // The interval could reach into the day
            if ((dayStart <= s.timestamp) && (static_cast<double>(s.timestamp) < static_cast<double>(dayEnd) + gapLimit))
               AddDiagnostic(diagnostics, ComputationDiagnosticCode::INTERVAL_WITHOUT_START, dciId, s.timestamp, s.timestamp);
            continue;
         }

         time_t lo, hi;
         if (!Overlap(previous, s.timestamp, dayStart, dayEnd, &lo, &hi))
            continue;
         double dt = static_cast<double>(s.timestamp - previous);
         double span = static_cast<double>(hi - lo);
         double quantity = s.value * factor;

         if (quantity < 0)
         {
            AddDiagnostic(diagnostics, ComputationDiagnosticCode::NEGATIVE_VALUE, dciId, lo, hi, quantity);
            continue;
         }
         if (dt > gapLimit + KPIDIRECT_EPS)
         {
            // After an outage it is not certain that the platform's delta still refers to the last stored value
            AddDiagnostic(diagnostics, ComputationDiagnosticCode::INTERVAL_AFTER_GAP, dciId, lo, hi);
            nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 7, L"DCI %u: interval of %.0f s at %lld follows a gap", dciId, dt, static_cast<long long>(previous));
            continue;
         }
         if (checkPlausibility && (quantity / (dt / 3600.0) > KPIDIRECT_PLAUSIBILITY_FACTOR * ratedPowerKW + KPIDIRECT_EPS))
         {
            AddDiagnostic(diagnostics, ComputationDiagnosticCode::IMPLAUSIBLE_INCREMENT, dciId, lo, hi, quantity / (dt / 3600.0));
            nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 7, L"DCI %u: implausible interval quantity %f at %lld (rated power %f kW)", dciId, quantity, static_cast<long long>(s.timestamp), ratedPowerKW);
            continue;
         }
         value += quantity * span / dt;
         covered += span;
      }
   }

   result.value = value;
   result.coverage = covered / dayLength;
   if (result.coverage < 1.0 - KPIDIRECT_EPS)
      AddDiagnostic(diagnostics, ComputationDiagnosticCode::UNCOVERED_TIME, dciId, 0, 0, dayLength - covered);
   result.accepted = (result.coverage >= KPIDIRECT_THETA - KPIDIRECT_EPS);
   if (!result.accepted)
      AddDiagnostic(diagnostics, ComputationDiagnosticCode::INSUFFICIENT_COVERAGE, dciId, 0, 0, result.coverage);

   nxlog_debug_tag(DEBUG_TAG_KPIDIRECT, 6, L"DCI %u: kind=%d nominalInterval=%.0f gapLimit=%.0f coverage=%f value=%f accepted=%s",
      dciId, static_cast<int>(binding.kind), nominalInterval, gapLimit, result.coverage, value, result.accepted ? L"yes" : L"no");
   return result;
}
