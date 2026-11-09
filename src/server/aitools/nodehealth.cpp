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
** File: nodehealth.cpp
**
**/

#include "aitools.h"
#include <map>

/**
 * Minimal time between two samples of cumulative counters (milliseconds)
 */
#define COUNTER_SAMPLE_INTERVAL  3000

/**
 * Maximum number of interfaces sampled for error counters
 */
#define MAX_SAMPLED_INTERFACES   32

/**
 * Maximum number of filesystems checked for inode usage
 */
#define MAX_INODE_CHECKS         32

/**
 * Maximum number of individual findings produced from interface error DCIs (the rest is summarized)
 */
#define MAX_DCI_INTERFACE_FINDINGS  10

/**
 * Pre-evaluated findings collector
 */
class Findings
{
private:
   json_t *m_array;

public:
   Findings(json_t *array) { m_array = array; }

   void add(const char *severity, const char *section, const char *format, ...)
   {
      char message[1024];
      va_list args;
      va_start(args, format);
      vsnprintf(message, sizeof(message), format, args);
      va_end(args);

      json_t *finding = json_object();
      json_object_set_new(finding, "severity", json_string(severity));
      json_object_set_new(finding, "section", json_string(section));
      json_object_set_new(finding, "message", json_string(message));
      json_array_append_new(m_array, finding);
   }
};

/**
 * Convert wide string to UTF-8 into fixed buffer
 */
static const char *ToUtf8(const wchar_t *s, char *buffer, size_t size)
{
   size_t bytes = wchar_to_utf8(CHECK_NULL_EX(s), -1, buffer, size - 1);
   buffer[bytes] = 0;
   return buffer;
}

/**
 * Round value to one decimal digit for compact output
 */
static inline json_t *JsonReal1(double v)
{
   return json_real(round(v * 10) / 10);
}

/**
 * Create section object marked as not readable
 */
static json_t *SectionError(DataCollectionError e, const char *reason = nullptr)
{
   json_t *section = json_object();
   json_object_set_new(section, "error", json_string(DataCollectionErrorName(e)));
   if (reason != nullptr)
      json_object_set_new(section, "reason", json_string(reason));
   return section;
}

/**
 * Convert SNMP error code to data collection error
 */
static DataCollectionError SNMPErrorToDCE(uint32_t rc)
{
   switch(rc)
   {
      case SNMP_ERR_SUCCESS:
         return DCE_SUCCESS;
      case SNMP_ERR_TIMEOUT:
         return DCE_COMM_ERROR;
      case SNMP_ERR_NO_OBJECT:
         return DCE_NOT_SUPPORTED;
      default:
         return DCE_COLLECTION_ERROR;
   }
}

/**
 * Read numeric agent metric
 */
static DataCollectionError ReadAgentNumber(Node *node, const wchar_t *metric, double *value)
{
   wchar_t buffer[MAX_RESULT_LENGTH];
   DataCollectionError e = node->getMetricFromAgent(metric, buffer, MAX_RESULT_LENGTH);
   if (e != DCE_SUCCESS)
      return e;
   wchar_t *eptr;
   *value = wcstod(buffer, &eptr);
   return ((eptr != buffer) && (*eptr == 0)) ? DCE_SUCCESS : DCE_INVALID_DATA;
}

/**
 * Read optional numeric agent metric into JSON object (silently skipped if not available)
 */
static bool ReadOptionalAgentNumber(Node *node, const wchar_t *metric, json_t *section, const char *key, double *value)
{
   if (ReadAgentNumber(node, metric, value) != DCE_SUCCESS)
      return false;
   json_object_set_new(section, key, JsonReal1(*value));
   return true;
}

/**
 * Filesystem types that are always full or mirror another filesystem and should not be reported
 */
static bool IsIgnoredFileSystemType(const wchar_t *fsType)
{
   static const wchar_t *ignored[] = { L"squashfs", L"iso9660", L"udf", L"overlay", L"nsfs", L"devtmpfs", L"proc", L"sysfs", L"cgroup", L"cgroup2", L"autofs", L"fuse.lxcfs", L"efivarfs", L"securityfs", L"pstore", L"bpf", L"tracefs", L"debugfs", L"configfs", nullptr };
   if (fsType == nullptr)
      return false;
   for(int i = 0; ignored[i] != nullptr; i++)
      if (!wcsicmp(fsType, ignored[i]))
         return true;
   return false;
}

/**
 * Evaluate disk usage and add finding if needed
 */
static void EvaluateDiskUsage(Findings *findings, const char *name, double usedPerc, uint64_t freeBytes)
{
   if (usedPerc >= 95)
      findings->add("critical", "disks", "%s %.1f%% used (" UINT64_FMTA " MB available)", name, usedPerc, freeBytes / _ULL(1048576));
   else if (usedPerc >= 90)
      findings->add("warning", "disks", "%s %.1f%% used (" UINT64_FMTA " MB available)", name, usedPerc, freeBytes / _ULL(1048576));
}

/**
 * Evaluate uptime values (host and agent) and add findings if needed
 */
static void EvaluateUptime(Findings *findings, double hostUptime, double agentUptime, const char *agentName)
{
   if ((hostUptime >= 0) && (hostUptime < 3600))
   {
      findings->add("warning", "uptime", "host rebooted %d minutes ago", static_cast<int>(hostUptime / 60));
   }
   else if ((agentUptime >= 0) && (agentUptime < 3600) && ((hostUptime < 0) || (hostUptime > agentUptime + 300)))
   {
      findings->add("info", "uptime", "%s restarted %d minutes ago without host reboot", agentName, static_cast<int>(agentUptime / 60));
   }
}

/**
 * Evaluate CPU usage
 */
static void EvaluateCPUUsage(Findings *findings, double usage)
{
   if (usage >= 95)
      findings->add("critical", "cpu", "CPU usage %.1f%%", usage);
   else if (usage >= 90)
      findings->add("warning", "cpu", "CPU usage %.1f%%", usage);
}

/**
 * Evaluate swap usage
 */
static void EvaluateSwapUsage(Findings *findings, double usedPerc)
{
   if (usedPerc >= 80)
      findings->add("critical", "memory", "swap %.1f%% used", usedPerc);
   else if (usedPerc >= 50)
      findings->add("warning", "memory", "swap %.1f%% used", usedPerc);
}

/**
 * Evaluate available physical memory
 */
static void EvaluateAvailableMemory(Findings *findings, double availablePerc)
{
   if (availablePerc <= 5)
      findings->add("critical", "memory", "only %.1f%% of physical memory available", availablePerc);
   else if (availablePerc <= 10)
      findings->add("warning", "memory", "only %.1f%% of physical memory available", availablePerc);
}

/**
 * Interface selected for error counter sampling
 */
struct SampledInterface
{
   shared_ptr<Interface> iface;
   uint64_t counters[2][4];   // [sample][inErrors, outErrors, inDrops, outDrops]
   bool valid[2];
};

static const char *s_counterNames[4] = { "inErrors", "outErrors", "inDrops", "outDrops" };

/**
 * Select interfaces for error counter sampling (operationally up, not loopback, not pseudo)
 */
static void SelectInterfaces(Node *node, std::vector<SampledInterface> *interfaces)
{
   unique_ptr<SharedObjectArray<NetObj>> children = node->getChildren(OBJECT_INTERFACE);
   for(int i = 0; (i < children->size()) && (interfaces->size() < MAX_SAMPLED_INTERFACES); i++)
   {
      shared_ptr<Interface> iface = static_pointer_cast<Interface>(children->getShared(i));
      if (iface->isLoopback() || iface->isFake() || (iface->getOperState() != IF_OPER_STATE_UP) || (iface->getIfIndex() == 0))
         continue;
      SampledInterface s;
      s.iface = iface;
      memset(s.counters, 0, sizeof(s.counters));
      s.valid[0] = s.valid[1] = false;
      interfaces->push_back(s);
   }
}

/**
 * Sample interface error counters via agent
 */
static void SampleInterfaceCountersAgent(Node *node, std::vector<SampledInterface> *interfaces, int sample)
{
   static const wchar_t *metrics[4] = { L"Net.Interface.InErrors", L"Net.Interface.OutErrors", L"Net.Interface.InDrops", L"Net.Interface.OutDrops" };
   for(SampledInterface& s : *interfaces)
   {
      s.valid[sample] = true;
      for(int c = 0; c < 4; c++)
      {
         wchar_t metric[128];
         nx_swprintf(metric, 128, L"%s(%u)", metrics[c], s.iface->getIfIndex());
         double value;
         if (ReadAgentNumber(node, metric, &value) == DCE_SUCCESS)
         {
            s.counters[sample][c] = static_cast<uint64_t>(value);
         }
         else
         {
            s.valid[sample] = false;
            break;
         }
      }
   }
}

/**
 * Sample interface error counters via SNMP (IF-MIB ifInErrors, ifOutErrors, ifInDiscards, ifOutDiscards)
 */
static void SampleInterfaceCountersSNMP(SNMP_Transport *snmp, std::vector<SampledInterface> *interfaces, int sample)
{
   static const uint32_t columns[4] = { 14, 20, 13, 19 };
   for(SampledInterface& s : *interfaces)
   {
      SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
      for(int c = 0; c < 4; c++)
      {
         uint32_t oid[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, columns[c], s.iface->getIfIndex() };
         request.bindVariable(new SNMP_Variable(oid, sizeof(oid) / sizeof(uint32_t)));
      }
      SNMP_PDU *response;
      s.valid[sample] = false;
      if (snmp->doRequest(&request, &response) != SNMP_ERR_SUCCESS)
         continue;
      if ((response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) && (response->getNumVariables() == 4))
      {
         s.valid[sample] = true;
         for(int c = 0; c < 4; c++)
         {
            SNMP_Variable *v = response->getVariable(c);
            if ((v->getType() == ASN_NO_SUCH_OBJECT) || (v->getType() == ASN_NO_SUCH_INSTANCE) || (v->getType() == ASN_NULL))
            {
               s.valid[sample] = false;
               break;
            }
            s.counters[sample][c] = v->getValueAsUInt64();
         }
      }
      delete response;
   }
}

/**
 * Build interface error section from two samples
 */
static json_t *BuildInterfaceSection(const std::vector<SampledInterface>& interfaces, int64_t elapsed, Findings *findings)
{
   json_t *section = json_object();
   json_object_set_new(section, "source", json_string("live"));
   json_object_set_new(section, "sampleIntervalMs", json_integer(elapsed));
   json_object_set_new(section, "note", json_string("'total' values are cumulative since interface counter reset (usually boot); 'delta' is the change during the sample interval; interfaces with all counters zero are omitted"));
   json_t *list = json_array();
   int failed = 0;
   for(const SampledInterface& s : interfaces)
   {
      if (!s.valid[0] || !s.valid[1])
      {
         failed++;
         continue;
      }

      json_t *total = json_object();
      json_t *delta = json_object();
      uint64_t deltas[4];
      for(int c = 0; c < 4; c++)
      {
         json_object_set_new(total, s_counterNames[c], json_integer(s.counters[1][c]));
         if (s.counters[1][c] >= s.counters[0][c])
         {
            deltas[c] = s.counters[1][c] - s.counters[0][c];
            json_object_set_new(delta, s_counterNames[c], json_integer(deltas[c]));
         }
         else
         {
            deltas[c] = 0;   // counter wrap or reset
            json_object_set_new(delta, s_counterNames[c], json_null());
         }
      }
      bool hasErrors = (deltas[0] > 0) || (deltas[1] > 0);
      bool hasDrops = (deltas[2] > 0) || (deltas[3] > 0);

      bool hasTotals = false;
      for(int c = 0; c < 4; c++)
         if (s.counters[1][c] != 0)
            hasTotals = true;
      if (!hasTotals)
      {
         json_decref(total);
         json_decref(delta);
         continue;
      }

      char name[256];
      ToUtf8(s.iface->getName(), name, sizeof(name));
      if (hasErrors || hasDrops)
      {
         // Dropped packets alone are common on busy interfaces (unknown protocols, tagged frames, etc.), so they are only informational
         findings->add(hasErrors ? "warning" : "info", "interfaces", "interface %s: " UINT64_FMTA " input errors, " UINT64_FMTA " output errors, " UINT64_FMTA " input drops, " UINT64_FMTA " output drops during %d s sample",
            name, deltas[0], deltas[1], deltas[2], deltas[3], static_cast<int>(elapsed / 1000));
      }

      json_t *entry = json_object();
      json_object_set_new(entry, "name", json_string(name));
      json_object_set_new(entry, "ifIndex", json_integer(s.iface->getIfIndex()));
      json_object_set_new(entry, "total", total);
      json_object_set_new(entry, "delta", delta);
      json_array_append_new(list, entry);
   }
   json_object_set_new(section, "sampledInterfaces", json_integer(static_cast<int64_t>(interfaces.size()) - failed));
   json_object_set_new(section, "interfaces", list);
   if (failed > 0)
      json_object_set_new(section, "unreadableInterfaces", json_integer(failed));
   return section;
}

/**
 * Build interface error section from existing DCIs. Returns nullptr if there are no suitable DCIs.
 */
static json_t *BuildInterfaceSectionFromDCI(Node *node, uint32_t userId, Findings *findings)
{
   // Agent metrics Net.Interface.{In,Out}{Errors,Drops} and IF-MIB ifInDiscards(13), ifInErrors(14), ifOutDiscards(19), ifOutErrors(20)
   unique_ptr<SharedObjectArray<DCObject>> dcis = node->getDCObjectsByRegex(
      L"^(Net\\.Interface\\.(In|Out)(Errors|Drops)(64)?\\(|\\.?1\\.3\\.6\\.1\\.2\\.1\\.2\\.2\\.1\\.(13|14|19|20)\\.)", true, userId);
   if (dcis == nullptr)
      return nullptr;

   json_t *list = json_array();
   int nonZeroErrors = 0, nonZeroDrops = 0, reported = 0;
   for(int i = 0; i < dcis->size(); i++)
   {
      DCObject *dco = dcis->get(i);
      if ((dco->getType() != DCO_TYPE_ITEM) || (dco->getStatus() != ITEM_STATUS_ACTIVE))
         continue;
      DCItem *dci = static_cast<DCItem*>(dco);
      int method = dci->getDeltaCalculationMethod();
      if ((method == DCM_ORIGINAL_VALUE) || dci->getLastValueTimestamp().isNull())
         continue;

      const char *methodName = (method == DCM_AVERAGE_PER_SECOND) ? "per second" : ((method == DCM_AVERAGE_PER_MINUTE) ? "per minute" : "per poll interval");
      String value = dci->getLastValue();
      char name[256], description[256], valueText[64];
      ToUtf8(dci->getName(), name, sizeof(name));
      ToUtf8(dci->getDescription(), description, sizeof(description));
      ToUtf8(value, valueText, sizeof(valueText));

      json_t *entry = json_object();
      json_object_set_new(entry, "dciId", json_integer(dci->getId()));
      json_object_set_new(entry, "name", json_string(name));
      json_object_set_new(entry, "description", json_string(description));
      json_object_set_new(entry, "value", json_string(valueText));
      json_object_set_new(entry, "unit", json_string(methodName));
      json_object_set_new(entry, "timestamp", dci->getLastValueTimestamp().asJson());
      json_array_append_new(list, entry);

      if (wcstod(value.cstr(), nullptr) > 0)
      {
         // Errors are reported as warnings, dropped/discarded packets alone only as information
         bool isError = (wcsistr(dci->getName(), L"Errors") != nullptr) || (wcsstr(dci->getName(), L"1.3.6.1.2.1.2.2.1.14.") != nullptr) ||
            (wcsstr(dci->getName(), L"1.3.6.1.2.1.2.2.1.20.") != nullptr);
         if (isError)
            nonZeroErrors++;
         else
            nonZeroDrops++;
         if (reported < MAX_DCI_INTERFACE_FINDINGS)
         {
            reported++;
            findings->add(isError ? "warning" : "info", "interfaces", "%s = %s %s (DCI \"%s\")", description, valueText, methodName, name);
         }
      }
   }

   if (json_array_size(list) == 0)
   {
      json_decref(list);
      return nullptr;
   }

   if (nonZeroErrors + nonZeroDrops > reported)
      findings->add((nonZeroErrors > 0) ? "warning" : "info", "interfaces", "%d interface error and %d discard DCIs have non-zero last values in total (first %d listed above, full list in 'interfaces' section)",
         nonZeroErrors, nonZeroDrops, reported);

   json_t *section = json_object();
   json_object_set_new(section, "source", json_string("dci"));
   json_object_set_new(section, "note", json_string("values are last collected delta values of existing error/discard DCIs"));
   json_object_set_new(section, "dcis", list);
   return section;
}

/**
 * Process sample from System.Processes table
 */
struct ProcessSample
{
   uint64_t cpuTime;
   bool seen;
};

/**
 * Build top processes section (by RSS from latest sample, by CPU from delta between two samples)
 */
static json_t *BuildProcessSection(const shared_ptr<Table>& first, const shared_ptr<Table>& second, int64_t elapsed, int top)
{
   const Table *t = second.get();
   int cPid = t->getColumnIndex(L"PID");
   int cName = t->getColumnIndex(L"NAME");
   int cUser = t->getColumnIndex(L"USER");
   int cThreads = t->getColumnIndex(L"THREADS");
   int cHandles = t->getColumnIndex(L"HANDLES");
   int cKTime = t->getColumnIndex(L"KTIME");
   int cUTime = t->getColumnIndex(L"UTIME");
   int cRSS = t->getColumnIndex(L"RSS");
   int cMemUsage = t->getColumnIndex(L"MEMORY_USAGE");
   int cCmdLine = t->getColumnIndex(L"CMDLINE");
   if ((cPid == -1) || (cName == -1))
      return SectionError(DCE_INVALID_DATA, "unexpected structure of System.Processes table");

   // CPU time per PID from first sample
   std::map<uint32_t, uint64_t> cpuTime;
   if ((first != nullptr) && (cKTime != -1) && (cUTime != -1))
   {
      int fPid = first->getColumnIndex(L"PID");
      int fKTime = first->getColumnIndex(L"KTIME");
      int fUTime = first->getColumnIndex(L"UTIME");
      if ((fPid != -1) && (fKTime != -1) && (fUTime != -1))
      {
         for(int i = 0; i < first->getNumRows(); i++)
            cpuTime[first->getAsUInt(i, fPid)] = first->getAsUInt64(i, fKTime) + first->getAsUInt64(i, fUTime);
      }
   }

   struct Row
   {
      int row;
      uint64_t rss;
      double cpu;   // -1 if unknown
   };
   std::vector<Row> rows;
   for(int i = 0; i < t->getNumRows(); i++)
   {
      Row r;
      r.row = i;
      r.rss = (cRSS != -1) ? t->getAsUInt64(i, cRSS) : 0;
      r.cpu = -1;
      if (!cpuTime.empty() && (elapsed > 0))
      {
         auto it = cpuTime.find(t->getAsUInt(i, cPid));
         if (it != cpuTime.end())
         {
            uint64_t now = t->getAsUInt64(i, cKTime) + t->getAsUInt64(i, cUTime);
            if (now >= it->second)
               r.cpu = static_cast<double>(now - it->second) * 100.0 / static_cast<double>(elapsed);
         }
      }
      rows.push_back(r);
   }

   auto rowToJson = [&] (const Row& r) -> json_t*
   {
      json_t *p = json_object();
      json_object_set_new(p, "pid", json_integer(t->getAsUInt(r.row, cPid)));
      json_object_set_new(p, "name", json_string_w(t->getAsString(r.row, cName, L"")));
      if (cUser != -1)
         json_object_set_new(p, "user", json_string_w(t->getAsString(r.row, cUser, L"")));
      if (cRSS != -1)
         json_object_set_new(p, "rssBytes", json_integer(r.rss));
      if (cMemUsage != -1)
         json_object_set_new(p, "memoryPerc", JsonReal1(t->getAsDouble(r.row, cMemUsage)));
      if (r.cpu >= 0)
         json_object_set_new(p, "cpuPercOfOneCore", JsonReal1(r.cpu));
      if (cThreads != -1)
         json_object_set_new(p, "threads", json_integer(t->getAsUInt(r.row, cThreads)));
      if (cHandles != -1)
         json_object_set_new(p, "handles", json_integer(t->getAsUInt(r.row, cHandles)));
      if (cCmdLine != -1)
      {
         wchar_t buffer[204];
         wcslcpy(buffer, t->getAsString(r.row, cCmdLine, L""), 201);
         TrimW(buffer);
         if (wcslen(buffer) == 200)
            wcscat(buffer, L"...");
         if (buffer[0] != 0)
            json_object_set_new(p, "cmdLine", json_string_w(buffer));
      }
      return p;
   };

   json_t *section = json_object();
   json_object_set_new(section, "processCount", json_integer(t->getNumRows()));

   if (cRSS != -1)
   {
      std::sort(rows.begin(), rows.end(), [] (const Row& a, const Row& b) { return a.rss > b.rss; });
      json_t *list = json_array();
      for(int i = 0; (i < top) && (i < static_cast<int>(rows.size())); i++)
         json_array_append_new(list, rowToJson(rows[i]));
      json_object_set_new(section, "byMemory", list);
   }

   if (!cpuTime.empty())
   {
      std::sort(rows.begin(), rows.end(), [] (const Row& a, const Row& b) { return a.cpu > b.cpu; });
      json_t *list = json_array();
      for(int i = 0; (i < top) && (i < static_cast<int>(rows.size())) && (rows[i].cpu >= 0); i++)
         json_array_append_new(list, rowToJson(rows[i]));
      json_object_set_new(section, "byCpu", list);
      json_object_set_new(section, "cpuSampleIntervalMs", json_integer(elapsed));
   }
   else
   {
      json_object_set_new(section, "byCpu", SectionError(DCE_NOT_SUPPORTED, "process CPU times not available"));
   }
   return section;
}

/**
 * Build CPU section via agent
 */
static json_t *AgentCPUSection(Node *node, Findings *findings)
{
   double usage;
   DataCollectionError e = ReadAgentNumber(node, L"System.CPU.Usage", &usage);
   json_t *section = (e == DCE_SUCCESS) ? json_object() : SectionError(e);
   if (e == DCE_SUCCESS)
   {
      json_object_set_new(section, "usagePerc", JsonReal1(usage));
      EvaluateCPUUsage(findings, usage);
   }

   double count = 0, value;
   if (ReadAgentNumber(node, L"System.CPU.Count", &count) == DCE_SUCCESS)
      json_object_set_new(section, "count", json_integer(static_cast<int64_t>(count)));
   else
      count = 0;
   ReadOptionalAgentNumber(node, L"System.CPU.LoadAvg", section, "loadAvg1", &value);
   if (ReadOptionalAgentNumber(node, L"System.CPU.LoadAvg5", section, "loadAvg5", &value) && (count > 0) && (value / count >= 2))
      findings->add("warning", "cpu", "5 minute load average %.2f on %d CPUs", value, static_cast<int>(count));
   ReadOptionalAgentNumber(node, L"System.CPU.LoadAvg15", section, "loadAvg15", &value);
   if (ReadOptionalAgentNumber(node, L"System.CPU.Usage.IoWait", section, "ioWaitPerc", &value) && (value >= 20))
      findings->add("warning", "cpu", "CPU I/O wait %.1f%%", value);
   if (ReadOptionalAgentNumber(node, L"System.CPU.Usage.Steal", section, "stealPerc", &value) && (value >= 10))
      findings->add("warning", "cpu", "CPU steal time %.1f%% (hypervisor contention)", value);
   return section;
}

/**
 * Build memory section via agent
 */
static json_t *AgentMemorySection(Node *node, Findings *findings)
{
   double total;
   DataCollectionError e = ReadAgentNumber(node, L"System.Memory.Physical.Total", &total);
   if (e != DCE_SUCCESS)
      return SectionError(e);

   json_t *section = json_object();
   json_object_set_new(section, "physicalTotalBytes", json_integer(static_cast<uint64_t>(total)));
   double usedPerc, availablePerc;
   bool hasUsed = ReadOptionalAgentNumber(node, L"System.Memory.Physical.UsedPerc", section, "physicalUsedPerc", &usedPerc);
   if (ReadOptionalAgentNumber(node, L"System.Memory.Physical.AvailablePerc", section, "physicalAvailablePerc", &availablePerc))
      EvaluateAvailableMemory(findings, availablePerc);
   else if (hasUsed)
      EvaluateAvailableMemory(findings, 100 - usedPerc);

   double swapTotal, swapUsedPerc;
   if (ReadAgentNumber(node, L"System.Memory.Swap.Total", &swapTotal) == DCE_SUCCESS)
   {
      json_object_set_new(section, "swapTotalBytes", json_integer(static_cast<uint64_t>(swapTotal)));
      if ((swapTotal > 0) && ReadOptionalAgentNumber(node, L"System.Memory.Swap.UsedPerc", section, "swapUsedPerc", &swapUsedPerc))
         EvaluateSwapUsage(findings, swapUsedPerc);
   }
   return section;
}

/**
 * Build disk section via agent
 */
static json_t *AgentDiskSection(Node *node, Findings *findings)
{
   shared_ptr<Table> table;
   DataCollectionError e = node->getTableFromAgent(L"FileSystem.Volumes", &table);
   if (e != DCE_SUCCESS)
      return SectionError(e);

   int cMountPoint = table->getColumnIndex(L"MOUNTPOINT");
   int cVolume = table->getColumnIndex(L"VOLUME");
   int cType = table->getColumnIndex(L"FSTYPE");
   int cTotal = table->getColumnIndex(L"SIZE.TOTAL");
   int cAvail = table->getColumnIndex(L"SIZE.AVAIL");
   int cAvailPerc = table->getColumnIndex(L"SIZE.AVAIL.PCT");
   if ((cMountPoint == -1) || (cTotal == -1) || (cAvail == -1) || (cAvailPerc == -1))
      return SectionError(DCE_INVALID_DATA, "unexpected structure of FileSystem.Volumes table");

   json_t *section = json_object();
   json_object_set_new(section, "note", json_string("usedPerc is computed from space available to unprivileged users"));
   json_t *list = json_array();
   int inodeChecks = 0, skippedTmpfs = 0;
   bool inodesSupported = true;
   for(int i = 0; i < table->getNumRows(); i++)
   {
      const wchar_t *fsType = (cType != -1) ? table->getAsString(i, cType) : nullptr;
      uint64_t total = table->getAsUInt64(i, cTotal);
      if ((total == 0) || IsIgnoredFileSystemType(fsType))
         continue;

      const wchar_t *mountPoint = table->getAsString(i, cMountPoint, L"");
      uint64_t avail = table->getAsUInt64(i, cAvail);
      double usedPerc = 100.0 - table->getAsDouble(i, cAvailPerc);
      if ((fsType != nullptr) && !wcsicmp(fsType, L"tmpfs") && (usedPerc < 50))
      {
         skippedTmpfs++;
         continue;
      }

      json_t *entry = json_object();
      json_object_set_new(entry, "mountPoint", json_string_w(mountPoint));
      if (cVolume != -1)
         json_object_set_new(entry, "volume", json_string_w(table->getAsString(i, cVolume, L"")));
      if (fsType != nullptr)
         json_object_set_new(entry, "fsType", json_string_w(fsType));
      json_object_set_new(entry, "totalBytes", json_integer(total));
      json_object_set_new(entry, "availableBytes", json_integer(avail));
      json_object_set_new(entry, "usedPerc", JsonReal1(usedPerc));

      char name[256];
      ToUtf8(mountPoint, name, sizeof(name));
      EvaluateDiskUsage(findings, name, usedPerc, avail);

      if (inodesSupported && (inodeChecks < MAX_INODE_CHECKS))
      {
         inodeChecks++;
         wchar_t metric[MAX_PARAM_NAME];
         nx_swprintf(metric, MAX_PARAM_NAME, L"FileSystem.UsedInodesPerc(%s)", mountPoint);
         double inodesUsed;
         DataCollectionError ie = ReadAgentNumber(node, metric, &inodesUsed);
         if (ie == DCE_SUCCESS)
         {
            json_object_set_new(entry, "inodesUsedPerc", JsonReal1(inodesUsed));
            if (inodesUsed >= 95)
               findings->add("critical", "disks", "%s: %.1f%% of inodes used", name, inodesUsed);
            else if (inodesUsed >= 90)
               findings->add("warning", "disks", "%s: %.1f%% of inodes used", name, inodesUsed);
         }
         else if (ie == DCE_NOT_SUPPORTED)
         {
            inodesSupported = false;
         }
      }

      json_array_append_new(list, entry);
   }
   json_object_set_new(section, "filesystems", list);
   if (skippedTmpfs > 0)
      json_object_set_new(section, "omittedTmpfsBelow50Perc", json_integer(skippedTmpfs));
   return section;
}

/**
 * Build uptime section via agent
 */
static json_t *AgentUptimeSection(double systemUptime, Node *node, Findings *findings)
{
   json_t *section = json_object();
   json_object_set_new(section, "systemUptimeSec", json_integer(static_cast<int64_t>(systemUptime)));
   json_object_set_new(section, "bootTime", json_time_string(time(nullptr) - static_cast<time_t>(systemUptime)));
   double agentUptime = -1;
   if (ReadAgentNumber(node, L"Agent.Uptime", &agentUptime) == DCE_SUCCESS)
      json_object_set_new(section, "agentUptimeSec", json_integer(static_cast<int64_t>(agentUptime)));
   else
      agentUptime = -1;
   EvaluateUptime(findings, systemUptime, agentUptime, "NetXMS agent");
   return section;
}

/**
 * Build service section via agent
 */
static json_t *AgentServiceSection(Node *node, Findings *findings)
{
   shared_ptr<Table> table;
   DataCollectionError e = node->getTableFromAgent(L"System.Services", &table);
   if (e != DCE_SUCCESS)
      return SectionError(e);

   int cName = table->getColumnIndex(L"NAME");
   int cDispName = table->getColumnIndex(L"DISPNAME");
   int cState = table->getColumnIndex(L"STATE");
   int cStartup = table->getColumnIndex(L"STARTUP");
   if ((cName == -1) || (cState == -1) || (cStartup == -1))
      return SectionError(DCE_INVALID_DATA, "unexpected structure of System.Services table");

   // On systemd hosts, oneshot units that completed successfully are reported as "Stopped" as well,
   // so automatic+stopped is only reported as a finding on other platforms
   const wchar_t *platform = node->getPlatformName();
   bool systemd = (platform != nullptr) && !wcsnicmp(platform, L"Linux", 5);

   json_t *section = json_object();
   json_object_set_new(section, "serviceCount", json_integer(table->getNumRows()));
   json_t *list = json_array();
   int enabledStopped = 0;
   for(int i = 0; i < table->getNumRows(); i++)
   {
      const wchar_t *state = table->getAsString(i, cState, L"");
      const wchar_t *startup = table->getAsString(i, cStartup, L"");
      bool failed = !wcsicmp(state, L"Failed");
      bool automatic = !wcsnicmp(startup, L"Auto", 4) && (wcsstr(startup, L"Trigger") == nullptr);
      bool stopped = !wcsicmp(state, L"Stopped");
      if (!failed && !(automatic && stopped))
         continue;

      if (systemd && !failed)
      {
         enabledStopped++;
         continue;
      }

      char name[256];
      ToUtf8(table->getAsString(i, cName, L""), name, sizeof(name));
      if (failed)
         findings->add("warning", "services", "service %s failed", name);
      else
         findings->add("warning", "services", "service %s has automatic startup but is stopped", name);

      if (json_array_size(list) < 50)
      {
         json_t *entry = json_object();
         json_object_set_new(entry, "name", json_string(name));
         if (cDispName != -1)
            json_object_set_new(entry, "displayName", json_string_w(table->getAsString(i, cDispName, L"")));
         json_object_set_new(entry, "state", json_string_w(state));
         json_object_set_new(entry, "startup", json_string_w(startup));
         json_array_append_new(list, entry);
      }
   }
   json_object_set_new(section, "notRunning", list);
   if (systemd)
   {
      json_object_set_new(section, "enabledButInactive", json_integer(enabledStopped));
      json_object_set_new(section, "note", json_string("systemd reports enabled oneshot units that already finished the same way as stopped services, "
         "so only failed units are listed; enabledButInactive counts the rest (full list is in agent table System.Services)"));
   }
   return section;
}

/**
 * Build clock section via agent
 */
static json_t *AgentClockSection(Node *node, Findings *findings)
{
   int64_t t0 = GetCurrentTimeMs();
   double agentTime;
   DataCollectionError e = ReadAgentNumber(node, L"System.CurrentTime", &agentTime);
   int64_t t1 = GetCurrentTimeMs();
   if (e != DCE_SUCCESS)
      return SectionError(e);

   // Agent reports whole seconds, so offset precision is about one second
   int64_t offset = static_cast<int64_t>(agentTime) - (t0 + t1) / 2000;
   json_t *section = json_object();
   json_object_set_new(section, "offsetSec", json_integer(offset));
   json_object_set_new(section, "roundTripMs", json_integer(t1 - t0));
   json_object_set_new(section, "note", json_string("positive offset means node clock is ahead of server clock; precision is about 1 second"));
   int64_t absOffset = (offset < 0) ? -offset : offset;
   if (absOffset >= 300)
      findings->add("critical", "clock", "clock offset %d s from server", static_cast<int>(offset));
   else if (absOffset >= 60)
      findings->add("warning", "clock", "clock offset %d s from server", static_cast<int>(offset));
   return section;
}

/**
 * Build agent problems section
 */
static json_t *AgentProblemsSection(Node *node, Findings *findings)
{
   shared_ptr<Table> table;
   DataCollectionError e = node->getTableFromAgent(L"Agent.Problems", &table);
   if (e != DCE_SUCCESS)
      return SectionError(e);

   int cKey = table->getColumnIndex(L"KEY");
   int cSeverity = table->getColumnIndex(L"SEVERITY");
   int cMessage = table->getColumnIndex(L"MESSAGE");
   json_t *list = json_array();
   for(int i = 0; i < table->getNumRows(); i++)
   {
      int severity = (cSeverity != -1) ? table->getAsInt(i, cSeverity) : SEVERITY_WARNING;
      char message[512];
      ToUtf8((cMessage != -1) ? table->getAsString(i, cMessage, L"") : L"", message, sizeof(message));
      json_t *entry = json_object();
      if (cKey != -1)
         json_object_set_new(entry, "key", json_string_w(table->getAsString(i, cKey, L"")));
      json_object_set_new(entry, "severity", json_string_w(GetEventSeverityName(severity)));
      json_object_set_new(entry, "message", json_string(message));
      json_array_append_new(list, entry);
      if (i < 10)
         findings->add((severity >= SEVERITY_MAJOR) ? "critical" : "warning", "agentProblems", "agent problem: %s", message);
   }
   if (table->getNumRows() > 10)
      findings->add("warning", "agentProblems", "%d agent problems registered in total", table->getNumRows());

   json_t *section = json_object();
   json_object_set_new(section, "problems", list);
   return section;
}

/**
 * Collect health data via NetXMS agent. Returns false if agent is not reachable.
 */
static bool CollectViaAgent(Node *node, uint32_t userId, int top, json_t *output, Findings *findings)
{
   double systemUptime;
   DataCollectionError e = ReadAgentNumber(node, L"System.Uptime", &systemUptime);
   if (e == DCE_COMM_ERROR)
      return false;

   // First sample of cumulative counters (sample times are taken after each read completes, so the
   // interval between the two samples is not inflated by the duration of the first read)
   int64_t sampleStart = GetCurrentTimeMs();
   shared_ptr<Table> processes1;
   node->getTableFromAgent(L"System.Processes", &processes1);
   int64_t processSampleStart = GetCurrentTimeMs();

   json_t *interfaceSection = BuildInterfaceSectionFromDCI(node, userId, findings);
   std::vector<SampledInterface> interfaces;
   int64_t interfaceSampleStart = 0;
   if (interfaceSection == nullptr)
   {
      SelectInterfaces(node, &interfaces);
      SampleInterfaceCountersAgent(node, &interfaces, 0);
      interfaceSampleStart = GetCurrentTimeMs();
   }

   // Non-cumulative sections are read between the two samples
   json_object_set_new(output, "uptime", (e == DCE_SUCCESS) ? AgentUptimeSection(systemUptime, node, findings) : SectionError(e));
   json_object_set_new(output, "cpu", AgentCPUSection(node, findings));
   json_object_set_new(output, "memory", AgentMemorySection(node, findings));
   json_object_set_new(output, "disks", AgentDiskSection(node, findings));
   json_object_set_new(output, "services", AgentServiceSection(node, findings));
   json_object_set_new(output, "clock", AgentClockSection(node, findings));
   json_object_set_new(output, "agentProblems", AgentProblemsSection(node, findings));

   int64_t elapsed = GetCurrentTimeMs() - sampleStart;
   if (elapsed < COUNTER_SAMPLE_INTERVAL)
   {
      ThreadSleepMs(static_cast<uint32_t>(COUNTER_SAMPLE_INTERVAL - elapsed));
   }

   // Second sample
   shared_ptr<Table> processes2;
   DataCollectionError pe = node->getTableFromAgent(L"System.Processes", &processes2);
   int64_t processSampleTime = GetCurrentTimeMs() - processSampleStart;
   if (pe == DCE_SUCCESS)
      json_object_set_new(output, "topProcesses", BuildProcessSection(processes1, processes2, processSampleTime, top));
   else
      json_object_set_new(output, "topProcesses", SectionError(pe));

   if (interfaceSection == nullptr)
   {
      SampleInterfaceCountersAgent(node, &interfaces, 1);
      interfaceSection = BuildInterfaceSection(interfaces, GetCurrentTimeMs() - interfaceSampleStart, findings);
   }
   json_object_set_new(output, "interfaces", interfaceSection);
   return true;
}

/**
 * hrStorageTable entry
 */
struct StorageEntry
{
   int type;
   wchar_t descr[128];
   uint64_t unitSize;
   uint64_t size;
   uint64_t used;

   StorageEntry() { type = 0; descr[0] = 0; unitSize = 0; size = 0; used = 0; }
};

/**
 * Build memory and disk sections from HOST-RESOURCES-MIB hrStorageTable
 */
static void SNMPStorageSections(SNMP_Transport *snmp, json_t *output, Findings *findings)
{
   std::map<uint32_t, StorageEntry> storage;
   int count = 0;
   uint32_t rc = SnmpWalk(snmp, SNMP_ObjectId({ 1, 3, 6, 1, 2, 1, 25, 2, 3, 1 }),
      [&storage, &count] (SNMP_Variable *v) -> uint32_t
      {
         if (count++ > 2048)
            return SNMP_ERR_SNAPSHOT_TOO_BIG;
         const SNMP_ObjectId& oid = v->getName();
         if (oid.length() != 12)
            return SNMP_ERR_SUCCESS;
         StorageEntry& e = storage[oid.getElement(11)];
         switch(oid.getElement(10))
         {
            case 2:  // hrStorageType
               {
                  SNMP_ObjectId type = v->getValueAsObjectId();
                  if ((type.length() == 10) && (type.compare({ 1, 3, 6, 1, 2, 1, 25, 2, 1 }) == OID_LONGER))
                     e.type = type.getElement(9);
               }
               break;
            case 3:  // hrStorageDescr
               v->getValueAsString(e.descr, 128);
               break;
            case 4:  // hrStorageAllocationUnits
               e.unitSize = v->getValueAsUInt();
               break;
            case 5:  // hrStorageSize
               e.size = v->getValueAsUInt();
               break;
            case 6:  // hrStorageUsed
               e.used = v->getValueAsUInt();
               break;
         }
         return SNMP_ERR_SUCCESS;
      });
   if ((rc != SNMP_ERR_SUCCESS) || storage.empty())
   {
      DataCollectionError e = (rc == SNMP_ERR_SUCCESS) ? DCE_NOT_SUPPORTED : SNMPErrorToDCE(rc);
      json_object_set_new(output, "memory", SectionError(e, "HOST-RESOURCES-MIB hrStorageTable not readable"));
      json_object_set_new(output, "disks", SectionError(e, "HOST-RESOURCES-MIB hrStorageTable not readable"));
      return;
   }

   json_t *memory = json_object();
   json_t *virtualMemory = json_array();
   json_t *filesystems = json_array();
   uint64_t buffers = 0, cached = 0;
   const StorageEntry *ram = nullptr;
   for(const auto& it : storage)
   {
      const StorageEntry& e = it.second;
      uint64_t total = e.size * e.unitSize;
      uint64_t used = e.used * e.unitSize;
      if (total == 0)
         continue;
      double usedPerc = static_cast<double>(used) * 100.0 / static_cast<double>(total);
      char name[256];
      ToUtf8(e.descr, name, sizeof(name));

      switch(e.type)
      {
         case hrStorageRam:
            if (ram == nullptr)
               ram = &e;
            break;
         case hrStorageVirtualMemory:
            {
               json_t *entry = json_object();
               json_object_set_new(entry, "description", json_string(name));
               json_object_set_new(entry, "totalBytes", json_integer(total));
               json_object_set_new(entry, "usedPerc", JsonReal1(usedPerc));
               json_array_append_new(virtualMemory, entry);
               if (wcsistr(e.descr, L"swap") != nullptr)
                  EvaluateSwapUsage(findings, usedPerc);
            }
            break;
         case hrStorageOther:
            // NET-SNMP reports buffers and cache as separate entries of type "other"
            if (!wcsicmp(e.descr, L"Memory buffers"))
               buffers = used;
            else if (!wcsicmp(e.descr, L"Cached memory"))
               cached = used;
            break;
         case hrStorageFixedDisk:
         case hrStorageNetworkDisk:
            {
               json_t *entry = json_object();
               json_object_set_new(entry, "description", json_string(name));
               json_object_set_new(entry, "totalBytes", json_integer(total));
               json_object_set_new(entry, "availableBytes", json_integer(total - std::min(used, total)));
               json_object_set_new(entry, "usedPerc", JsonReal1(usedPerc));
               json_array_append_new(filesystems, entry);
               EvaluateDiskUsage(findings, name, usedPerc, total - std::min(used, total));
            }
            break;
      }
   }

   if (ram != nullptr)
   {
      uint64_t total = ram->size * ram->unitSize;
      uint64_t used = ram->used * ram->unitSize;
      json_object_set_new(memory, "physicalTotalBytes", json_integer(total));
      json_object_set_new(memory, "physicalUsedPerc", JsonReal1(static_cast<double>(used) * 100.0 / static_cast<double>(total)));
      if ((buffers > 0) || (cached > 0))
      {
         // hrStorageUsed for RAM includes buffers and cache on NET-SNMP
         uint64_t available = total - std::min(used, total) + buffers + cached;
         double availablePerc = std::min(static_cast<double>(available) * 100.0 / static_cast<double>(total), 100.0);
         json_object_set_new(memory, "physicalAvailablePerc", JsonReal1(availablePerc));
         EvaluateAvailableMemory(findings, availablePerc);
      }
      else
      {
         json_object_set_new(memory, "note", json_string("physicalUsedPerc may include file system cache depending on SNMP agent"));
      }
   }
   else
   {
      json_object_set_new(memory, "error", json_string(DataCollectionErrorName(DCE_NOT_SUPPORTED)));
   }
   json_object_set_new(memory, "virtualMemory", virtualMemory);
   json_object_set_new(output, "memory", memory);

   json_t *disks = json_object();
   json_object_set_new(disks, "filesystems", filesystems);
   json_object_set_new(output, "disks", disks);
}

/**
 * Build CPU section from HOST-RESOURCES-MIB hrProcessorLoad
 */
static json_t *SNMPCPUSection(SNMP_Transport *snmp, Findings *findings)
{
   int count = 0;
   double total = 0;
   uint32_t rc = SnmpWalk(snmp, SNMP_ObjectId({ 1, 3, 6, 1, 2, 1, 25, 3, 3, 1, 2 }),
      [&count, &total] (SNMP_Variable *v) -> uint32_t
      {
         if (count >= 4096)
            return SNMP_ERR_SNAPSHOT_TOO_BIG;
         total += v->getValueAsInt();
         count++;
         return SNMP_ERR_SUCCESS;
      });
   if ((rc != SNMP_ERR_SUCCESS) || (count == 0))
      return SectionError((rc == SNMP_ERR_SUCCESS) ? DCE_NOT_SUPPORTED : SNMPErrorToDCE(rc), "HOST-RESOURCES-MIB hrProcessorLoad not readable");

   double usage = total / count;
   json_t *section = json_object();
   json_object_set_new(section, "usagePerc", JsonReal1(usage));
   json_object_set_new(section, "count", json_integer(count));
   json_object_set_new(section, "note", json_string("average of hrProcessorLoad (1 minute average per processor)"));
   EvaluateCPUUsage(findings, usage);
   return section;
}

/**
 * Collect health data via SNMP. Returns false if SNMP agent is not reachable.
 */
static bool CollectViaSNMP(Node *node, json_t *output, Findings *findings)
{
   unique_ptr<SNMP_Transport> snmp(node->createSnmpTransport());
   if (snmp == nullptr)
      return false;

   // hrSystemUptime (host) and sysUpTime (SNMP agent)
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 25, 1, 1, 0 }));
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 1, 3, 0 }));
   SNMP_PDU *response;
   uint32_t rc = snmp->doRequest(&request, &response);
   if (rc == SNMP_ERR_TIMEOUT)
      return false;
   if (rc == SNMP_ERR_SUCCESS)
   {
      double hostUptime = -1, agentUptime = -1;
      if ((response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) && (response->getNumVariables() == 2))
      {
         SNMP_Variable *v = response->getVariable(0);
         if ((v->getType() != ASN_NO_SUCH_OBJECT) && (v->getType() != ASN_NO_SUCH_INSTANCE) && (v->getType() != ASN_NULL))
            hostUptime = static_cast<double>(v->getValueAsUInt() / 100);
         v = response->getVariable(1);
         if ((v->getType() != ASN_NO_SUCH_OBJECT) && (v->getType() != ASN_NO_SUCH_INSTANCE) && (v->getType() != ASN_NULL))
            agentUptime = static_cast<double>(v->getValueAsUInt() / 100);
      }
      delete response;

      if ((hostUptime >= 0) || (agentUptime >= 0))
      {
         json_t *section = json_object();
         if (hostUptime >= 0)
         {
            json_object_set_new(section, "systemUptimeSec", json_integer(static_cast<int64_t>(hostUptime)));
            json_object_set_new(section, "bootTime", json_time_string(time(nullptr) - static_cast<time_t>(hostUptime)));
         }
         if (agentUptime >= 0)
            json_object_set_new(section, "snmpAgentUptimeSec", json_integer(static_cast<int64_t>(agentUptime)));
         json_object_set_new(section, "note", json_string("SNMP TimeTicks wrap after 497 days"));
         json_object_set_new(output, "uptime", section);
         EvaluateUptime(findings, hostUptime, agentUptime, "SNMP agent");
      }
      else
      {
         json_object_set_new(output, "uptime", SectionError(DCE_NOT_SUPPORTED));
      }
   }
   else
   {
      json_object_set_new(output, "uptime", SectionError(SNMPErrorToDCE(rc)));
   }

   int64_t sampleStart = GetCurrentTimeMs();
   std::vector<SampledInterface> interfaces;
   SelectInterfaces(node, &interfaces);
   SampleInterfaceCountersSNMP(snmp.get(), &interfaces, 0);
   int64_t interfaceSampleStart = GetCurrentTimeMs();

   json_object_set_new(output, "cpu", SNMPCPUSection(snmp.get(), findings));
   SNMPStorageSections(snmp.get(), output, findings);

   json_object_set_new(output, "services", SectionError(DCE_NOT_SUPPORTED, "requires NetXMS agent"));
   json_object_set_new(output, "topProcesses", SectionError(DCE_NOT_SUPPORTED, "requires NetXMS agent"));
   json_object_set_new(output, "clock", SectionError(DCE_NOT_SUPPORTED, "requires NetXMS agent"));
   json_object_set_new(output, "agentProblems", SectionError(DCE_NOT_SUPPORTED, "requires NetXMS agent"));

   int64_t elapsed = GetCurrentTimeMs() - sampleStart;
   if (elapsed < COUNTER_SAMPLE_INTERVAL)
      ThreadSleepMs(static_cast<uint32_t>(COUNTER_SAMPLE_INTERVAL - elapsed));
   SampleInterfaceCountersSNMP(snmp.get(), &interfaces, 1);
   json_object_set_new(output, "interfaces", BuildInterfaceSection(interfaces, GetCurrentTimeMs() - interfaceSampleStart, findings));
   return true;
}

/**
 * Add last values of OS health DCIs (used when live data is not available)
 */
static void AddHealthDCIValues(Node *node, uint32_t userId, json_t *output)
{
   unique_ptr<SharedObjectArray<DCObject>> dcis = node->getDCObjectsByRegex(
      L"^(System\\.CPU\\.(Usage|LoadAvg)|System\\.Memory\\.(Physical|Swap|Virtual)\\.(Used|Available|Free)Perc|FileSystem\\.(Used|Free|Avail)Perc\\(|System\\.Uptime|Agent\\.Uptime|HostMib\\.Memory\\.)", true, userId);
   json_t *list = json_array();
   if (dcis != nullptr)
   {
      for(int i = 0; (i < dcis->size()) && (json_array_size(list) < 50); i++)
      {
         DCObject *dco = dcis->get(i);
         if ((dco->getType() != DCO_TYPE_ITEM) || dco->getLastValueTimestamp().isNull())
            continue;
         DCItem *dci = static_cast<DCItem*>(dco);
         String value = dci->getLastValue();
         if (value.isEmpty())
            continue;
         json_t *entry = json_object();
         json_object_set_new(entry, "dciId", json_integer(dci->getId()));
         json_object_set_new(entry, "name", json_string_w(dci->getName()));
         json_object_set_new(entry, "description", json_string_w(dci->getDescription()));
         json_object_set_new(entry, "value", json_string_w(value));
         json_object_set_new(entry, "timestamp", dci->getLastValueTimestamp().asJson());
         json_array_append_new(list, entry);
      }
   }
   json_object_set_new(output, "dciLastValues", list);
}

/**
 * Get one-call OS health snapshot of a node with pre-evaluated findings
 */
std::string F_GetNodeHealth(json_t *arguments, uint32_t userId)
{
   shared_ptr<NetObj> object = FindObjectByNameOrId(arguments, "object", OBJECT_NODE);
   if ((object == nullptr) || (object->getObjectClass() != OBJECT_NODE))
      return std::string("Node not found");
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
      return std::string("Access denied");

   Node *node = static_cast<Node*>(object.get());

   int top = json_object_get_int32(arguments, "top", 5);
   if (top < 1)
      top = 1;
   else if (top > 50)
      top = 50;

   json_t *output = json_object();
   json_object_set_new(output, "nodeId", json_integer(node->getId()));
   json_object_set_new(output, "nodeName", json_string_w(node->getName()));
   json_object_set_new(output, "platform", json_string_w(node->getPlatformName()));
   json_object_set_new(output, "timestamp", json_time_string(time(nullptr)));
   json_object_set_new(output, "source", json_null());   // placeholder to keep summary fields first in output

   json_t *findingList = json_array();
   json_object_set_new(output, "findings", findingList);
   Findings findings(findingList);
   // Each live source is skipped when the node does not support it, the user has no right to read it,
   // or it is not reachable; the next source is tried and the reasons are reported together
   const char *source = nullptr;
   const char *agentProblem = nullptr;
   const char *snmpProblem = nullptr;
   bool agentAccessDenied = false;
   if (node->isNativeAgent())
   {
      if (!node->checkAccessRights(userId, OBJECT_ACCESS_READ_AGENT))
      {
         agentProblem = "access to NetXMS agent data is denied";
         agentAccessDenied = true;
      }
      else if (CollectViaAgent(node, userId, top, output, &findings))
         source = "agent";
      else
         agentProblem = "NetXMS agent is not reachable";
   }
   if ((source == nullptr) && node->isSNMPSupported())
   {
      if (!node->checkAccessRights(userId, OBJECT_ACCESS_READ_SNMP))
         snmpProblem = "access to SNMP data is denied";
      else if (CollectViaSNMP(node, output, &findings))
         source = "snmp";
      else
         snmpProblem = "SNMP agent is not reachable";
   }

   char reason[256];
   if ((agentProblem != nullptr) && (snmpProblem != nullptr))
      snprintf(reason, sizeof(reason), "%s; %s", agentProblem, snmpProblem);
   else if (agentProblem != nullptr)
      strlcpy(reason, agentProblem, sizeof(reason));
   else if (snmpProblem != nullptr)
      strlcpy(reason, snmpProblem, sizeof(reason));
   else
      strlcpy(reason, "node has neither NetXMS agent nor SNMP", sizeof(reason));

   if (source == nullptr)
   {
      findings.add("critical", "general", "live OS data unavailable: %s; last collected DCI values are provided instead", reason);
      json_object_set_new(output, "source", json_string("dci"));
      json_object_set_new(output, "liveDataUnavailable", json_string(reason));
      AddHealthDCIValues(node, userId, output);
   }
   else
   {
      json_object_set_new(output, "source", json_string(source));
      // Data read via SNMP although node has NetXMS agent: a reachability problem is a finding in itself,
      // a permission problem only explains why agent-only sections are missing
      if (agentProblem != nullptr)
         findings.add(agentAccessDenied ? "info" : "critical", "general",
            "%s; data was read via SNMP instead (services, processes, clock and agent problems are not available)", agentProblem);
   }

   return JsonToString(output);
}
