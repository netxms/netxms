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
#include <snappy.h>

static const char *GetLabelValue(const prometheus::TimeSeries& ts, const char *name)
{
   for (const auto& label : ts.labels())
   {
      if (strcmp(label.name().c_str(), name) == 0)
      {
         return label.value().c_str();
      }
   }
   return nullptr;
}

static String BuildMetricName(const MetricMapping& mapping, const prometheus::TimeSeries& ts)
{
   StringBuffer result;
   result.append(mapping.netxmsName);
   result.append(_T('('));

   // TODO: Add escaping for special characters in argument values (parentheses, commas, quotes)

   bool first = true;
   for(int i = 0; i < mapping.nameArguments.size(); i++)
   {
      if (!first)
         result.append(_T(','));

#ifdef UNICODE
      char nameBuffer[256];
      WideCharToMultiByteSysLocale(mapping.nameArguments.get(i), nameBuffer, 256);
      const char *value = GetLabelValue(ts, nameBuffer);
      if (value != nullptr)
      {
         WCHAR valueBuffer[1024];
         MultiByteToWideCharSysLocale(value, valueBuffer, 1024);
         result.append(valueBuffer);
      }
#else
      const char *value = GetLabelValue(ts, mapping.nameArguments.get(i));
      if (value != nullptr)
      {
         result.append(value);
      }
#endif
      first = false;
   }

   result.append(_T(')'));
   return result;
}

static String BuildValueFromLabels(const MetricMapping& mapping, const prometheus::TimeSeries& ts)
{
   StringBuffer result;
   bool first = true;
   for(int i = 0; i < mapping.valueArguments.size(); i++)
   {
      if (!first)
         result.append(_T(' '));

#ifdef UNICODE
      char nameBuffer[256];
      WideCharToMultiByteSysLocale(mapping.valueArguments.get(i), nameBuffer, 256);
      const char *value = GetLabelValue(ts, nameBuffer);
      if (value != nullptr)
      {
         WCHAR valueBuffer[1024];
         MultiByteToWideCharSysLocale(value, valueBuffer, 1024);
         result.append(valueBuffer);
      }
#else
      const char *value = GetLabelValue(ts, mapping.valueArguments.get(i));
      if (value != nullptr)
      {
         result.append(value);
      }
#endif
      first = false;
   }

   return result;
}

static void ProcessWriteRequest(const prometheus::WriteRequest& request)
{
   const ObjectArray<MetricMapping>& mappings = GetMetricMappings();
   if (mappings.isEmpty())
   {
      return;
   }

   for (int i = 0; i < request.timeseries_size(); i++)
   {
      const auto& ts = request.timeseries(i);
      const char *metricName = GetLabelValue(ts, "__name__");
      if (metricName == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Timeseries without __name__ label"));
         continue;
      }

      const MetricMapping *matchedMapping = nullptr;
      for(int j = 0; j < mappings.size(); j++)
      {
#ifdef UNICODE
         char prometheusNameMB[256];
         WideCharToMultiByteSysLocale(mappings.get(j)->prometheusName, prometheusNameMB, 256);
         if (strcmp(metricName, prometheusNameMB) == 0)
#else
         if (strcmp(metricName, mappings.get(j)->prometheusName) == 0)
#endif
         {
            matchedMapping = mappings.get(j);
            break;
         }
      }

      if (matchedMapping == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("No mapping found for metric: %hs"), metricName);
         continue;
      }

      if (ts.samples_size() == 0)
      {
         continue;
      }

      for (const auto& sample : ts.samples())
      {
         String metricNameStr = BuildMetricName(*matchedMapping, ts);

         if (matchedMapping->useMetricValue)
         {
            AgentPushParameterDataDouble(metricNameStr, sample.value());
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Pushed metric: %s = %.2f"), metricNameStr.cstr(), sample.value());
         }
         else
         {
            String value = BuildValueFromLabels(*matchedMapping, ts);
            AgentPushParameterData(metricNameStr, value);
            nxlog_debug_tag(DEBUG_TAG, 7, _T("Pushed metric: %s = %s"), metricNameStr.cstr(), value.cstr());
         }
      }
   }
}

/**
 * Handle Prometheus remote write request
 */
bool HandleWriteRequest(const char *data, size_t size)
{
   size_t uncompressedSize;
   if (!snappy::GetUncompressedLength(data, size, &uncompressedSize))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Failed to get uncompressed length"));
      return false;
   }

   char *uncompressed = static_cast<char*>(MemAlloc(uncompressedSize));
   if (uncompressed == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Memory allocation failed"));
      return false;
   }

   if (!snappy::RawUncompress(data, size, uncompressed))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Snappy decompression failed"));
      MemFree(uncompressed);
      return false;
   }

   prometheus::WriteRequest writeRequest;
   if (!writeRequest.ParseFromArray(uncompressed, uncompressedSize))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Protobuf parsing failed"));
      MemFree(uncompressed);
      return false;
   }

   MemFree(uncompressed);

   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 9)
      PrintWriteRequest(writeRequest);

   ProcessWriteRequest(writeRequest);
   return true;
}
