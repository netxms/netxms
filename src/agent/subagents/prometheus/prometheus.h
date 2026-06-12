/*
** NetXMS Prometheus subagent
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

#ifndef _prometheus_h_
#define _prometheus_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_agent.h>
#include "generated/remote.pb.h"

/**
 * Metric mapping structure
 */
struct MetricMapping
{
   TCHAR prometheusName[256];
#ifdef UNICODE
   char prometheusNameMB[256];   // multibyte version for matching against incoming metric names
#endif
   TCHAR netxmsName[256];
   StringList nameArguments;
   StringList valueArguments;
   bool useMetricValue;

   MetricMapping()
   {
      prometheusName[0] = 0;
#ifdef UNICODE
      prometheusNameMB[0] = 0;
#endif
      netxmsName[0] = 0;
      useMetricValue = true;
   }
};

/**
 * Single label of a parsed metric sample (UTF-8 strings)
 */
struct SampleLabel
{
   char *name;
   char *value;

   SampleLabel(const char *n, size_t nameLen, char *v)
   {
      name = MemAllocStringA(nameLen + 1);
      memcpy(name, n, nameLen);
      name[nameLen] = 0;
      value = v;   // takes ownership
   }

   ~SampleLabel()
   {
      MemFree(name);
      MemFree(value);
   }
};

/**
 * Single sample parsed from text exposition format
 */
class MetricSample
{
private:
   char *m_name;
   ObjectArray<SampleLabel> m_labels;
   double m_value;

public:
   MetricSample(const char *name, size_t nameLen) : m_labels(0, 8, Ownership::True)
   {
      m_name = MemAllocStringA(nameLen + 1);
      memcpy(m_name, name, nameLen);
      m_name[nameLen] = 0;
      m_value = 0;
   }

   ~MetricSample()
   {
      MemFree(m_name);
   }

   void addLabel(const char *name, size_t nameLen, char *value)
   {
      m_labels.add(new SampleLabel(name, nameLen, value));
   }

   void setValue(double value)
   {
      m_value = value;
   }

   const char *getName() const { return m_name; }
   double getValue() const { return m_value; }
   const ObjectArray<SampleLabel>& getLabels() const { return m_labels; }

   const char *getLabelValue(const char *name) const
   {
      for(int i = 0; i < m_labels.size(); i++)
      {
         if (strcmp(m_labels.get(i)->name, name) == 0)
            return m_labels.get(i)->value;
      }
      return nullptr;
   }
};

/**
 * Endpoint scrape target
 */
class ScrapeTarget
{
private:
   TCHAR m_name[64];
   char m_url[1024];
   uint32_t m_interval;         // milliseconds
   uint32_t m_requestTimeout;   // milliseconds
   char m_login[128];
   char m_password[256];
   char *m_bearerToken;
   bool m_verifyPeer;
   bool m_verifyHost;
   char m_caCertificate[1024];
   THREAD m_thread;
   Condition m_stopCondition;
   mutable Mutex m_lock;
   ObjectArray<MetricSample> *m_samples;
   time_t m_lastScrapeTime;
   bool m_lastScrapeSuccess;

   ScrapeTarget(const TCHAR *name);

   void scraperThread();
   bool scrape();

public:
   ~ScrapeTarget();

   static ScrapeTarget *createFromConfig(const ConfigEntry& config);

   void start();
   void stop();

   const TCHAR *getName() const { return m_name; }
   const char *getUrl() const { return m_url; }
   uint32_t getScrapeInterval() const { return m_interval / 1000; }

   bool isLastScrapeSuccessful() const
   {
      LockGuard lockGuard(m_lock);
      return m_lastScrapeSuccess;
   }

   time_t getLastScrapeTime() const
   {
      LockGuard lockGuard(m_lock);
      return m_lastScrapeTime;
   }

   int getSampleCount() const
   {
      LockGuard lockGuard(m_lock);
      return (m_samples != nullptr) ? m_samples->size() : 0;
   }

   LONG getValue(const char *metric, const ObjectArray<SampleLabel>& filters, TCHAR *value) const;
   LONG getLabelValues(const char *metric, const char *label, StringList *output) const;
   LONG fillSeriesTable(const char *metric, Table *table) const;
};

bool HandleWriteRequest(const char *data, size_t size);
const ObjectArray<MetricMapping>& GetMetricMappings();
void ProcessSample(const char *metricName, double value, const std::function<const char* (const char*)>& getLabel);
void AppendEscapedValue(StringBuffer& result, const TCHAR *value);

ObjectArray<MetricSample> *ParsePrometheusText(const char *data);

void PrintWriteRequest(const prometheus::WriteRequest& request);

bool StartReceiver(const TCHAR *listenAddress, uint16_t port, const TCHAR *endpoint);
void StopReceiver();

void LoadScrapeTargets(Config *config);
void StartScrapers();
void StopScrapers();

LONG H_TargetInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_TargetValue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_TargetList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_LabelValues(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_TargetsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SeriesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session);

#define DEBUG_TAG _T("prometheus")

#endif
