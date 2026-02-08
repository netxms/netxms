/*
** NetXMS Prometheus remote write receiver subagent
** Copyright (C) 2025 Raden Solutions
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
**/

#include "prometheus.h"

/**
 * Get metric type name
 */
static const TCHAR *GetMetricTypeName(int type)
{
   switch (type)
   {
      case prometheus::MetricMetadata::COUNTER:
         return _T("COUNTER");
      case prometheus::MetricMetadata::GAUGE:
         return _T("GAUGE");
      case prometheus::MetricMetadata::HISTOGRAM:
         return _T("HISTOGRAM");
      case prometheus::MetricMetadata::GAUGEHISTOGRAM:
         return _T("GAUGEHISTOGRAM");
      case prometheus::MetricMetadata::SUMMARY:
         return _T("SUMMARY");
      case prometheus::MetricMetadata::INFO:
         return _T("INFO");
      case prometheus::MetricMetadata::STATESET:
         return _T("STATESET");
      default:
         return _T("UNKNOWN");
   }
}

/**
 * Print contents of WriteRequest for debugging
 */
void PrintWriteRequest(const prometheus::WriteRequest& request)
{
   nxlog_debug_tag(DEBUG_TAG, 9, _T("Number of timeseries: %d"), request.timeseries_size());
   nxlog_debug_tag(DEBUG_TAG, 9, _T("Number of metadata: %d"), request.metadata_size());

   TCHAR timestampBuffer[32];
   for (int i = 0; i < request.timeseries_size(); i++)
   {
      const auto& ts = request.timeseries(i);

      nxlog_debug_tag(DEBUG_TAG, 9, _T("Timeseries %d:"), i + 1);

      StringBuffer labels;
      labels.append(_T("  Labels: "));
      bool first = true;
      for (const auto& label : ts.labels())
      {
         if (!first)
            labels.append(_T(", "));
         labels.appendFormattedString(_T("%hs=\"%hs\""), label.name().c_str(), label.value().c_str());
         first = false;
      }
      nxlog_debug_tag(DEBUG_TAG, 9, _T("%s"), labels.cstr());

      if (ts.samples_size() > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 9, _T("  Samples (%d):"), ts.samples_size());
         for (const auto& sample : ts.samples())
         {
            nxlog_debug_tag(DEBUG_TAG, 9, _T("    [%s] %.2f"),
                           FormatTimestampMs(sample.timestamp(), timestampBuffer), sample.value());
         }
      }

      if (ts.exemplars_size() > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 9, _T("  Exemplars (%d):"), ts.exemplars_size());
         for (const auto& exemplar : ts.exemplars())
         {
            StringBuffer exemplarInfo;
            exemplarInfo.appendFormattedString(_T("    [%s] %.2f"),
                                              FormatTimestampMs(exemplar.timestamp(), timestampBuffer),
                                              exemplar.value());
            if (exemplar.labels_size() > 0)
            {
               exemplarInfo.append(_T(" {"));
               bool firstLabel = true;
               for (const auto& label : exemplar.labels())
               {
                  if (!firstLabel)
                     exemplarInfo.append(_T(", "));
                  exemplarInfo.appendFormattedString(_T("%hs=\"%hs\""), label.name().c_str(), label.value().c_str());
                  firstLabel = false;
               }
               exemplarInfo.append(_T("}"));
            }
            nxlog_debug_tag(DEBUG_TAG, 9, _T("%s"), exemplarInfo.cstr());
         }
      }

      if (ts.histograms_size() > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 9, _T("  Histograms (%d):"), ts.histograms_size());
         for (const auto& histogram : ts.histograms())
         {
            nxlog_debug_tag(DEBUG_TAG, 9, _T("    [%s]"), FormatTimestampMs(histogram.timestamp(), timestampBuffer));
            nxlog_debug_tag(DEBUG_TAG, 9, _T("      Sum: %.2f"), histogram.sum());

            if (histogram.count_int() != 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 9, _T("      Count: %llu"), (unsigned long long)histogram.count_int());
            }
            else if (histogram.count_float() != 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 9, _T("      Count: %.2f"), histogram.count_float());
            }

            nxlog_debug_tag(DEBUG_TAG, 9, _T("      Schema: %d"), histogram.schema());

            if (histogram.positive_spans_size() > 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 9, _T("      Positive buckets: %d deltas, %d counts"),
                              histogram.positive_deltas_size(), histogram.positive_counts_size());
            }

            if (histogram.negative_spans_size() > 0)
            {
               nxlog_debug_tag(DEBUG_TAG, 9, _T("      Negative buckets: %d deltas, %d counts"),
                              histogram.negative_deltas_size(), histogram.negative_counts_size());
            }
         }
      }

      nxlog_debug_tag(DEBUG_TAG, 9, _T(""));
   }

   if (request.metadata_size() > 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 9, _T("Metadata:"));
      for (const auto& metadata : request.metadata())
      {
         nxlog_debug_tag(DEBUG_TAG, 9, _T("  Metric: %hs"), metadata.metric_family_name().c_str());
         nxlog_debug_tag(DEBUG_TAG, 9, _T("    Type: %d (%s)"), metadata.type(), GetMetricTypeName(metadata.type()));

         if (!metadata.help().empty())
         {
            nxlog_debug_tag(DEBUG_TAG, 9, _T("    Help: %hs"), metadata.help().c_str());
         }

         if (!metadata.unit().empty())
         {
            nxlog_debug_tag(DEBUG_TAG, 9, _T("    Unit: %hs"), metadata.unit().c_str());
         }

         nxlog_debug_tag(DEBUG_TAG, 9, _T(""));
      }
   }
}
