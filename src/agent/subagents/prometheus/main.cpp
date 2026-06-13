/*
** NetXMS Prometheus remote write receiver subagent
** Copyright (C) 2025-2026 Raden Solutions
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
#include <netxms-version.h>

static ObjectArray<MetricMapping> s_metricMappings(16, 16, Ownership::True);

/**
 * Parse metric mapping from configuration string
 */
static bool ParseMetricMapping(const TCHAR *config, MetricMapping **result)
{
   StringList parts = String(config).split(_T(":"));
   if (parts.size() < 3 || parts.size() > 4)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Invalid metric mapping format (expected 3 or 4 parts): %s"), config);
      return false;
   }

   MetricMapping *mapping = new MetricMapping();
   _tcslcpy(mapping->prometheusName, parts.get(0), 256);
#ifdef UNICODE
   WideCharToMultiByteSysLocale(mapping->prometheusName, mapping->prometheusNameMB, 256);
#endif
   _tcslcpy(mapping->netxmsName, parts.get(1), 256);

   StringList nameArgs = String(parts.get(2)).split(_T(","));
   for(int i = 0; i < nameArgs.size(); i++)
   {
      mapping->nameArguments.add(nameArgs.get(i));
   }

   if (parts.size() == 4)
   {
      mapping->useMetricValue = false;
      StringList valueArgs = String(parts.get(3)).split(_T(","));
      for(int i = 0; i < valueArgs.size(); i++)
      {
         mapping->valueArguments.add(valueArgs.get(i));
      }
   }
   else
   {
      mapping->useMetricValue = true;
   }

   *result = mapping;
   return true;
}

/**
 * Load metric mappings from configuration
 */
static void LoadMetricMappings(Config *config)
{
   ConfigEntry *metrics = config->getEntry(_T("/Prometheus/Metric"));
   if (metrics == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("No metric mappings configured"));
      return;
   }

   for(int i = 0; i < metrics->getValueCount(); i++)
   {
      MetricMapping *mapping;
      if (ParseMetricMapping(metrics->getValue(i), &mapping))
      {
         s_metricMappings.add(mapping);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Loaded metric mapping: %s -> %s (use value: %s)"),
            mapping->prometheusName, mapping->netxmsName,
            mapping->useMetricValue ? _T("yes") : _T("no"));
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Loaded %d metric mappings"), s_metricMappings.size());
}

/**
 * Get metric mappings
 */
const ObjectArray<MetricMapping>& GetMetricMappings()
{
   return s_metricMappings;
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   LoadMetricMappings(config);
   LoadScrapeTargets(config);

   if (config->getValueAsBoolean(_T("/Prometheus/EnableReceiver"), true))
   {
      uint16_t port = static_cast<uint16_t>(config->getValueAsUInt(_T("/Prometheus/Port"), 9090));
      if (port == 0)
      {
         nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Port not configured"));
         return false;
      }

      const TCHAR *endpoint = config->getValue(_T("/Prometheus/Endpoint"), _T("/api/v1/write"));
      const TCHAR *listen = config->getValue(_T("/Prometheus/Address"), _T("127.0.0.1"));

      if (!StartReceiver(listen, port, endpoint))
      {
         return false;
      }

      nxlog_debug_tag(DEBUG_TAG, 1, _T("Prometheus remote write receiver initialized at http://%s:%d%s"), listen, port, endpoint);
   }

   StartScrapers();

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Prometheus subagent initialized"));
   return true;
}

/**
 * Subagent shutdown
 */
static void SubagentShutdown()
{
   StopReceiver();
   StopScrapers();
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Prometheus subagent shutdown"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("Prometheus.Target.LastScrapeTime(*)"), H_TargetInfo, _T("T"), DCI_DT_UINT64, _T("Prometheus: timestamp of last scrape of target {instance}") },
   { _T("Prometheus.Target.SampleCount(*)"), H_TargetInfo, _T("C"), DCI_DT_UINT, _T("Prometheus: number of samples in last scrape of target {instance}") },
   { _T("Prometheus.Target.Status(*)"), H_TargetInfo, _T("S"), DCI_DT_INT, _T("Prometheus: status of last scrape of target {instance}") },
   { _T("Prometheus.Value(*)"), H_TargetValue, nullptr, DCI_DT_FLOAT, _T("Prometheus: value of metric {instance}") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("Prometheus.LabelValues(*)"), H_LabelValues, nullptr },
   { _T("Prometheus.Targets"), H_TargetList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("Prometheus.Series(*)"), H_SeriesTable, nullptr, _T("LABELS"), _T("Prometheus: time series of metric") },
   { _T("Prometheus.Targets"), H_TargetsTable, nullptr, _T("NAME"), _T("Prometheus: configured scrape targets") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("PROMETHEUS"), NETXMS_VERSION_STRING,
   SubagentInit, SubagentShutdown, nullptr, nullptr, nullptr,
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
   0, nullptr, // actions
   0, nullptr  // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PROMETHEUS)
{
   *ppInfo = &s_info;
   return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
