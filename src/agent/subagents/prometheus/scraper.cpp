/*
** NetXMS Prometheus subagent
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
**/

#include "prometheus.h"
#include <netxms-version.h>
#include <nxlibcurl.h>

/**
 * Configured scrape targets (created during initialization, read-only afterwards)
 */
static ObjectArray<ScrapeTarget> s_targets(8, 8, Ownership::True);

/**
 * Create scrape target
 */
ScrapeTarget::ScrapeTarget(const TCHAR *name) : m_stopCondition(true), m_lock(MutexType::FAST)
{
   _tcslcpy(m_name, name, 64);
   m_url[0] = 0;
   m_interval = 60000;
   m_requestTimeout = 10000;
   m_login[0] = 0;
   m_password[0] = 0;
   m_bearerToken = nullptr;
   m_verifyPeer = true;
   m_verifyHost = true;
   m_caCertificate[0] = 0;
   m_thread = INVALID_THREAD_HANDLE;
   m_samples = nullptr;
   m_lastScrapeTime = 0;
   m_lastScrapeSuccess = false;
}

/**
 * Destroy scrape target
 */
ScrapeTarget::~ScrapeTarget()
{
   MemFree(m_bearerToken);
   delete m_samples;
}

/**
 * Create scrape target from configuration entry
 */
ScrapeTarget *ScrapeTarget::createFromConfig(const ConfigEntry& config)
{
   const TCHAR *url = config.getSubEntryValue(_T("URL"));
   if ((url == nullptr) || (*url == 0))
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Missing URL in definition of scrape target %s"), config.getName());
      return nullptr;
   }

   ScrapeTarget *target = new ScrapeTarget(config.getName());
   tchar_to_utf8(url, -1, target->m_url, sizeof(target->m_url));

   uint32_t interval = config.getSubEntryValueAsUInt(_T("ScrapeInterval"), 0, 60);
   if (interval < 1)
      interval = 1;
   target->m_interval = interval * 1000;

   uint32_t timeout = config.getSubEntryValueAsUInt(_T("Timeout"), 0, 10);
   if (timeout < 1)
      timeout = 1;
   target->m_requestTimeout = timeout * 1000;

   const TCHAR *login = config.getSubEntryValue(_T("Login"));
   if ((login != nullptr) && (*login != 0))
   {
      tchar_to_utf8(login, -1, target->m_login, sizeof(target->m_login));
      TCHAR password[256];
      DecryptPassword(login, config.getSubEntryValue(_T("Password"), 0, _T("")), password, 256);
      tchar_to_utf8(password, -1, target->m_password, sizeof(target->m_password));
   }

   const TCHAR *token = config.getSubEntryValue(_T("BearerToken"));
   if ((token != nullptr) && (*token != 0))
      target->m_bearerToken = UTF8StringFromTString(token);

   target->m_verifyPeer = config.getSubEntryValueAsBoolean(_T("VerifyCertificate"), 0, true);
   target->m_verifyHost = config.getSubEntryValueAsBoolean(_T("VerifyHost"), 0, true);
   const TCHAR *caCertificate = config.getSubEntryValue(_T("CACertificate"));
   if (caCertificate != nullptr)
      tchar_to_utf8(caCertificate, -1, target->m_caCertificate, sizeof(target->m_caCertificate));

   return target;
}

/**
 * Start scraper thread
 */
void ScrapeTarget::start()
{
   m_thread = ThreadCreateEx(this, &ScrapeTarget::scraperThread);
}

/**
 * Stop scraper thread
 */
void ScrapeTarget::stop()
{
   m_stopCondition.set();
   ThreadJoin(m_thread);
   m_thread = INVALID_THREAD_HANDLE;
}

/**
 * Scraper thread
 */
void ScrapeTarget::scraperThread()
{
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Scraper thread for target %s started (URL=%hs, interval=%u sec)"), m_name, m_url, m_interval / 1000);
   uint32_t sleepTime;
   do
   {
      int64_t startTime = GetCurrentTimeMs();
      scrape();
      int64_t elapsed = GetCurrentTimeMs() - startTime;
      sleepTime = (elapsed >= m_interval) ? 1000 : static_cast<uint32_t>(m_interval - elapsed);
   } while(!m_stopCondition.wait(sleepTime));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Scraper thread for target %s stopped"), m_name);
}

/**
 * Do single scrape of the target endpoint
 */
bool ScrapeTarget::scrape()
{
   CURL *curl = curl_easy_init();
   if (curl == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("%s: call to curl_easy_init() failed"), m_name);
      return false;
   }

   char errorText[CURL_ERROR_SIZE];
   errorText[0] = 0;
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorText);
#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, static_cast<long>(1));
#endif
#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(m_requestTimeout));
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, static_cast<long>(1));
   curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(8));
   curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Agent/" NETXMS_VERSION_STRING_A);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, m_verifyPeer ? static_cast<long>(1) : static_cast<long>(0));
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, m_verifyHost ? static_cast<long>(2) : static_cast<long>(0));
   if (m_caCertificate[0] != 0)
      curl_easy_setopt(curl, CURLOPT_CAINFO, m_caCertificate);
   EnableLibCURLUnexpectedEOFWorkaround(curl);

   struct curl_slist *headers = curl_slist_append(nullptr, "Accept: application/openmetrics-text; version=1.0.0; q=0.5, text/plain; version=0.0.4");
   if (m_bearerToken != nullptr)
   {
      char *authHeader = MemAllocStringA(strlen(m_bearerToken) + 32);
      strcpy(authHeader, "Authorization: Bearer ");
      strcat(authHeader, m_bearerToken);
      headers = curl_slist_append(headers, authHeader);
      MemFree(authHeader);
   }
   else if (m_login[0] != 0)
   {
      curl_easy_setopt(curl, CURLOPT_USERNAME, m_login);
      curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password);
      curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
   }
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);

   bool success = false;
   if (curl_easy_setopt(curl, CURLOPT_URL, m_url) == CURLE_OK)
   {
      CURLcode rc = curl_easy_perform(curl);
      if (rc == CURLE_OK)
      {
         long httpCode = 0;
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
         if ((httpCode >= 200) && (httpCode <= 299))
         {
            responseData.write('\0');
            ObjectArray<MetricSample> *samples = ParsePrometheusText(reinterpret_cast<const char*>(responseData.buffer()));
            nxlog_debug_tag(DEBUG_TAG, 6, _T("%s: scrape completed (%d samples)"), m_name, samples->size());

            if (!GetMetricMappings().isEmpty())
            {
               for(int i = 0; i < samples->size(); i++)
               {
                  MetricSample *sample = samples->get(i);
                  ProcessSample(sample->getName(), sample->getValue(),
                     [sample] (const char *labelName) { return sample->getLabelValue(labelName); });
               }
            }

            m_lock.lock();
            delete m_samples;
            m_samples = samples;
            m_lock.unlock();
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("%s: HTTP response code %03ld"), m_name, httpCode);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("%s: call to curl_easy_perform() failed (%hs)"), m_name, errorText);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("%s: call to curl_easy_setopt(CURLOPT_URL) failed"), m_name);
   }
   curl_slist_free_all(headers);
   curl_easy_cleanup(curl);

   m_lock.lock();
   m_lastScrapeTime = time(nullptr);
   m_lastScrapeSuccess = success;
   m_lock.unlock();
   return success;
}

/**
 * Get value of first sample matching given metric name and label filters
 */
LONG ScrapeTarget::getValue(const char *metric, const ObjectArray<SampleLabel>& filters, TCHAR *value) const
{
   LockGuard lockGuard(m_lock);
   if (m_samples == nullptr)
      return SYSINFO_RC_ERROR;

   for(int i = 0; i < m_samples->size(); i++)
   {
      MetricSample *sample = m_samples->get(i);
      if (strcmp(sample->getName(), metric) != 0)
         continue;

      bool match = true;
      for(int j = 0; (j < filters.size()) && match; j++)
      {
         const char *v = sample->getLabelValue(filters.get(j)->name);
         match = (v != nullptr) && (strcmp(v, filters.get(j)->value) == 0);
      }
      if (match)
      {
         ret_double(value, sample->getValue());
         return SYSINFO_RC_SUCCESS;
      }
   }
   return SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Get unique values of given label across all samples of given metric
 */
LONG ScrapeTarget::getLabelValues(const char *metric, const char *label, StringList *output) const
{
   LockGuard lockGuard(m_lock);
   if (m_samples == nullptr)
      return SYSINFO_RC_ERROR;

   StringSet uniqueValues;
   for(int i = 0; i < m_samples->size(); i++)
   {
      MetricSample *sample = m_samples->get(i);
      if (strcmp(sample->getName(), metric) != 0)
         continue;

      const char *v = sample->getLabelValue(label);
      if (v != nullptr)
      {
         TCHAR *labelValue = TStringFromUTF8String(v);
         if (!uniqueValues.contains(labelValue))
         {
            output->add(labelValue);
            uniqueValues.addPreallocated(labelValue);
         }
         else
         {
            MemFree(labelValue);
         }
      }
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Build canonical labelset representation (label=value,label=value with values quoted as needed)
 */
static String BuildLabelsetString(const MetricSample& sample)
{
   StringBuffer result;
   const ObjectArray<SampleLabel>& labels = sample.getLabels();
   for(int i = 0; i < labels.size(); i++)
   {
      if (i > 0)
         result.append(_T(','));
      SampleLabel *label = labels.get(i);
      TCHAR *name = TStringFromUTF8String(label->name);
      result.append(name);
      MemFree(name);
      result.append(_T('='));
      TCHAR *value = TStringFromUTF8String(label->value);
      AppendEscapedValue(result, value);
      MemFree(value);
   }
   return result;
}

/**
 * Fill table with all samples of given metric, one row per labelset
 */
LONG ScrapeTarget::fillSeriesTable(const char *metric, Table *table) const
{
   LockGuard lockGuard(m_lock);
   if (m_samples == nullptr)
      return SYSINFO_RC_ERROR;

   table->addColumn(_T("LABELS"), DCI_DT_STRING, _T("Labels"), true);
   table->addColumn(_T("VALUE"), DCI_DT_FLOAT, _T("Value"));

   // Add column for each label name seen in matching samples
   for(int i = 0; i < m_samples->size(); i++)
   {
      MetricSample *sample = m_samples->get(i);
      if (strcmp(sample->getName(), metric) != 0)
         continue;

      const ObjectArray<SampleLabel>& labels = sample->getLabels();
      for(int j = 0; j < labels.size(); j++)
      {
         TCHAR columnName[MAX_COLUMN_NAME];
         utf8_to_tchar(labels.get(j)->name, -1, columnName, MAX_COLUMN_NAME);
         if (table->getColumnIndex(columnName) == -1)
            table->addColumn(columnName, DCI_DT_STRING, columnName);
      }
   }

   for(int i = 0; i < m_samples->size(); i++)
   {
      MetricSample *sample = m_samples->get(i);
      if (strcmp(sample->getName(), metric) != 0)
         continue;

      table->addRow();
      table->set(0, BuildLabelsetString(*sample));
      table->set(1, sample->getValue());

      const ObjectArray<SampleLabel>& labels = sample->getLabels();
      for(int j = 0; j < labels.size(); j++)
      {
         TCHAR columnName[MAX_COLUMN_NAME];
         utf8_to_tchar(labels.get(j)->name, -1, columnName, MAX_COLUMN_NAME);
         table->setPreallocated(table->getColumnIndex(columnName), TStringFromUTF8String(labels.get(j)->value));
      }
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Load scrape targets from configuration
 */
void LoadScrapeTargets(Config *config)
{
   unique_ptr<ObjectArray<ConfigEntry>> targets = config->getSubEntries(_T("/Prometheus/Targets"), _T("*"));
   if (targets == nullptr)
      return;

   for(int i = 0; i < targets->size(); i++)
   {
      ScrapeTarget *target = ScrapeTarget::createFromConfig(*targets->get(i));
      if (target != nullptr)
      {
         s_targets.add(target);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Loaded scrape target %s (URL=%hs)"), target->getName(), target->getUrl());
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Loaded %d scrape targets"), s_targets.size());
}

/**
 * Start scraper threads
 */
void StartScrapers()
{
   if (s_targets.isEmpty())
      return;

   if (!InitializeLibCURL())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("cURL initialization failed, endpoint scraping will not be available"));
      return;
   }

   for(int i = 0; i < s_targets.size(); i++)
      s_targets.get(i)->start();
}

/**
 * Stop scraper threads
 */
void StopScrapers()
{
   for(int i = 0; i < s_targets.size(); i++)
      s_targets.get(i)->stop();
}

/**
 * Find scrape target by name
 */
static ScrapeTarget *FindTarget(const TCHAR *name)
{
   for(int i = 0; i < s_targets.size(); i++)
   {
      if (_tcsicmp(s_targets.get(i)->getName(), name) == 0)
         return s_targets.get(i);
   }
   return nullptr;
}

/**
 * Handler for Prometheus.Target.* parameters
 */
LONG H_TargetInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR targetName[64];
   if (!AgentGetParameterArg(param, 1, targetName, 64) || (targetName[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   ScrapeTarget *target = FindTarget(targetName);
   if (target == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   switch(*arg)
   {
      case 'C':
         ret_int(value, target->getSampleCount());
         break;
      case 'S':
         ret_boolean(value, target->isLastScrapeSuccessful());
         break;
      case 'T':
         ret_uint64(value, static_cast<uint64_t>(target->getLastScrapeTime()));
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Prometheus.Value parameter
 */
LONG H_TargetValue(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR targetName[64];
   if (!AgentGetParameterArg(param, 1, targetName, 64) || (targetName[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   char metric[256];
   if (!AgentGetParameterArgA(param, 2, metric, 256) || (metric[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   ScrapeTarget *target = FindTarget(targetName);
   if (target == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   ObjectArray<SampleLabel> filters(0, 8, Ownership::True);
   for(int i = 3; ; i++)
   {
      char buffer[1024];
      if (!AgentGetParameterArgA(param, i, buffer, 1024))
         return SYSINFO_RC_UNSUPPORTED;
      if (buffer[0] == 0)
         break;
      char *separator = strchr(buffer, '=');
      if (separator == nullptr)
         return SYSINFO_RC_UNSUPPORTED;
      *separator = 0;
      filters.add(new SampleLabel(buffer, strlen(buffer), MemCopyStringA(separator + 1)));
   }

   return target->getValue(metric, filters, value);
}

/**
 * Handler for Prometheus.Targets list
 */
LONG H_TargetList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   for(int i = 0; i < s_targets.size(); i++)
      value->add(s_targets.get(i)->getName());
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Prometheus.LabelValues list
 */
LONG H_LabelValues(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR targetName[64];
   if (!AgentGetParameterArg(param, 1, targetName, 64) || (targetName[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   char metric[256], label[256];
   if (!AgentGetParameterArgA(param, 2, metric, 256) || (metric[0] == 0) ||
       !AgentGetParameterArgA(param, 3, label, 256) || (label[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   ScrapeTarget *target = FindTarget(targetName);
   if (target == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   return target->getLabelValues(metric, label, value);
}

/**
 * Handler for Prometheus.Targets table
 */
LONG H_TargetsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
   value->addColumn(_T("INTERVAL"), DCI_DT_UINT, _T("Scrape interval"));
   value->addColumn(_T("STATUS"), DCI_DT_INT, _T("Last scrape status"));
   value->addColumn(_T("LAST_SCRAPE"), DCI_DT_UINT64, _T("Last scrape timestamp"));
   value->addColumn(_T("SAMPLES"), DCI_DT_UINT, _T("Sample count"));

   for(int i = 0; i < s_targets.size(); i++)
   {
      ScrapeTarget *target = s_targets.get(i);
      value->addRow();
      value->set(0, target->getName());
      value->set(1, target->getUrl());
      value->set(2, target->getScrapeInterval());
      value->set(3, target->isLastScrapeSuccessful() ? 1 : 0);
      value->set(4, static_cast<uint64_t>(target->getLastScrapeTime()));
      value->set(5, static_cast<uint32_t>(target->getSampleCount()));
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for Prometheus.Series table
 */
LONG H_SeriesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR targetName[64];
   if (!AgentGetParameterArg(param, 1, targetName, 64) || (targetName[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   char metric[256];
   if (!AgentGetParameterArgA(param, 2, metric, 256) || (metric[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   ScrapeTarget *target = FindTarget(targetName);
   if (target == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;

   return target->fillSeriesTable(metric, value);
}
