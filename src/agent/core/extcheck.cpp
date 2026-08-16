/*
** NetXMS multiplatform core agent
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
** File: extcheck.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("extcheck")

/**
 * External check execution timeout in milliseconds (0 = use external metric timeout)
 */
uint32_t g_externalCheckTimeout = 0;

/**
 * External check result cache timeout in seconds
 */
uint32_t g_externalCheckCacheTimeout = 5;

/**
 * Skip numeric part of perfdata value. Returns pointer to first character after the number
 * (start of UOM) or start of the string if it does not begin with a valid number.
 */
static const TCHAR *SkipNumericValue(const TCHAR *s)
{
   const TCHAR *p = s;
   if ((*p == _T('+')) || (*p == _T('-')))
      p++;
   int digits = 0;
   while (_istdigit(*p))
   {
      p++;
      digits++;
   }
   if (*p == _T('.'))
   {
      p++;
      while (_istdigit(*p))
      {
         p++;
         digits++;
      }
   }
   if (digits == 0)
      return s;
   if ((*p == _T('e')) || (*p == _T('E')))
   {
      const TCHAR *e = p + 1;
      if ((*e == _T('+')) || (*e == _T('-')))
         e++;
      if (_istdigit(*e))
      {
         p = e;
         while (_istdigit(*p))
            p++;
      }
   }
   return p;
}

/**
 * Parse performance data string in Nagios plugin format - space-separated entries of the form
 * 'label'=value[UOM];[warn];[crit];[min];[max]. Malformed entries are skipped. Returns number
 * of successfully parsed entries.
 */
int ParseNagiosPerfData(const TCHAR *text, ObjectArray<PerfDataValue> *values)
{
   int count = 0;
   const TCHAR *p = text;
   while (true)
   {
      while (_istspace(*p))
         p++;
      if (*p == 0)
         break;

      // Extract label (may be quoted with single quotes, doubled quote is an escaped quote)
      StringBuffer label;
      if (*p == _T('\''))
      {
         p++;
         while (*p != 0)
         {
            if (*p == _T('\''))
            {
               if (p[1] == _T('\''))
               {
                  label.append(_T('\''));
                  p += 2;
               }
               else
               {
                  p++;
                  break;
               }
            }
            else
            {
               label.append(*p++);
            }
         }
      }
      else
      {
         while ((*p != 0) && (*p != _T('=')) && !_istspace(*p))
            label.append(*p++);
      }

      if ((*p != _T('=')) || label.isEmpty())
      {
         // Malformed entry, skip to next whitespace
         while ((*p != 0) && !_istspace(*p))
            p++;
         continue;
      }
      p++;

      // Collect semicolon-separated fields (value+UOM, warn, crit, min, max) up to whitespace
      StringBuffer fields[5];
      int f = 0;
      while ((*p != 0) && !_istspace(*p))
      {
         if (*p == _T(';'))
         {
            if (f < 4)
               f++;
         }
         else
         {
            fields[f].append(*p);
         }
         p++;
      }

      // Split value and UOM; entries with non-numeric value (including "U" for unknown) are skipped
      const TCHAR *v = fields[0].cstr();
      const TCHAR *uom = SkipNumericValue(v);
      if (uom == v)
         continue;

      StringBuffer value;
      value.append(v, uom - v);
      values->add(new PerfDataValue(label, value, uom, fields[1], fields[2], fields[3], fields[4]));
      count++;
   }
   return count;
}

/**
 * Parse Nagios plugin output. Status text is the part of the first line before "|". Performance
 * data starts after "|" on the first line; on subsequent lines everything after the next "|"
 * (including all following lines) is treated as performance data continuation, as defined by
 * the Nagios plugin API for multi-line output. Long text output is ignored.
 */
void ParseNagiosPluginOutput(const StringList& output, StringBuffer *statusText, ObjectArray<PerfDataValue> *perfData)
{
   if (output.isEmpty())
      return;

   const TCHAR *line = output.get(0);
   const TCHAR *pipe = _tcschr(line, _T('|'));
   if (pipe != nullptr)
   {
      statusText->append(line, pipe - line);
      ParseNagiosPerfData(pipe + 1, perfData);
   }
   else
   {
      statusText->append(line);
   }
   statusText->trim();

   bool inPerfData = false;
   for(int i = 1; i < output.size(); i++)
   {
      line = output.get(i);
      if (inPerfData)
      {
         ParseNagiosPerfData(line, perfData);
      }
      else
      {
         pipe = _tcschr(line, _T('|'));
         if (pipe != nullptr)
         {
            inPerfData = true;
            ParseNagiosPerfData(pipe + 1, perfData);
         }
      }
   }
}

/**
 * External check defined by Nagios-compatible monitoring plugin
 */
class ExternalCheck
{
private:
   TCHAR *m_name;
   TCHAR *m_command;
   Mutex m_mutex;
   int32_t m_status;
   TCHAR *m_statusText;
   ObjectArray<PerfDataValue> m_perfData;
   time_t m_timestamp;  // Time of last execution attempt (0 = never executed)
   bool m_success;      // Last execution attempt was successful

   bool refresh();

public:
   ExternalCheck(const TCHAR *name, const TCHAR *command) : m_mutex(MutexType::FAST), m_perfData(0, 16, Ownership::True)
   {
      m_name = MemCopyString(name);
      m_command = MemCopyString(command);
      m_status = 0;
      m_statusText = nullptr;
      m_timestamp = 0;
      m_success = false;
   }
   ~ExternalCheck()
   {
      MemFree(m_name);
      MemFree(m_command);
      MemFree(m_statusText);
   }

   LONG getStatus(TCHAR *value);
   LONG getStatusText(TCHAR *value);
   LONG getPerfDataValue(const TCHAR *label, TCHAR *value);
   LONG getPerfDataTable(Table *table);
};

/**
 * Execute plugin and refresh cached result if it is older than cache timeout. Both successful
 * and failed execution attempts are cached to avoid re-running the plugin for each DCI collected
 * in the same polling cycle. Must be called with mutex held.
 */
bool ExternalCheck::refresh()
{
   time_t now = time(nullptr);
   if ((m_timestamp != 0) && (now - m_timestamp < static_cast<time_t>(g_externalCheckCacheTimeout)))
      return m_success;

   m_timestamp = now;
   m_success = false;

   nxlog_debug_tag(DEBUG_TAG, 6, _T("Executing external check \"%s\" (command \"%s\")"), m_name, m_command);
   LineOutputProcessExecutor executor(m_command);
   if (!executor.execute())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot start external check \"%s\" (command \"%s\")"), m_name, m_command);
      return false;
   }
   if (!executor.waitForCompletion(g_externalCheckTimeout))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("External check \"%s\" execution timeout (command \"%s\")"), m_name, m_command);
      return false;
   }

   m_status = static_cast<int32_t>(executor.getExitCode());
   m_perfData.clear();
   StringBuffer statusText;
   ParseNagiosPluginOutput(executor.getData(), &statusText, &m_perfData);
   MemFree(m_statusText);
   m_statusText = MemCopyString(statusText.cstr());
   m_success = true;
   nxlog_debug_tag(DEBUG_TAG, 6, _T("External check \"%s\" completed (status=%d, %d performance data values)"), m_name, m_status, m_perfData.size());
   return true;
}

/**
 * Get plugin exit code
 */
LONG ExternalCheck::getStatus(TCHAR *value)
{
   LockGuard lock(m_mutex);
   if (!refresh())
      return SYSINFO_RC_ERROR;
   ret_int(value, m_status);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get text portion of first line of plugin output
 */
LONG ExternalCheck::getStatusText(TCHAR *value)
{
   LockGuard lock(m_mutex);
   if (!refresh())
      return SYSINFO_RC_ERROR;
   ret_string(value, CHECK_NULL_EX(m_statusText));
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get value of named performance data label
 */
LONG ExternalCheck::getPerfDataValue(const TCHAR *label, TCHAR *value)
{
   LockGuard lock(m_mutex);
   if (!refresh())
      return SYSINFO_RC_ERROR;
   for(int i = 0; i < m_perfData.size(); i++)
   {
      PerfDataValue *v = m_perfData.get(i);
      if (!_tcsicmp(v->label, label))
      {
         ret_string(value, v->value);
         return SYSINFO_RC_SUCCESS;
      }
   }
   return SYSINFO_RC_NO_SUCH_INSTANCE;
}

/**
 * Get all performance data as table
 */
LONG ExternalCheck::getPerfDataTable(Table *table)
{
   LockGuard lock(m_mutex);
   if (!refresh())
      return SYSINFO_RC_ERROR;
   table->addColumn(_T("LABEL"), DCI_DT_STRING, _T("Label"), true);
   table->addColumn(_T("VALUE"), DCI_DT_FLOAT, _T("Value"));
   table->addColumn(_T("UOM"), DCI_DT_STRING, _T("Unit of measurement"));
   table->addColumn(_T("WARNING"), DCI_DT_STRING, _T("Warning threshold"));
   table->addColumn(_T("CRITICAL"), DCI_DT_STRING, _T("Critical threshold"));
   table->addColumn(_T("MIN"), DCI_DT_FLOAT, _T("Minimum value"));
   table->addColumn(_T("MAX"), DCI_DT_FLOAT, _T("Maximum value"));
   for(int i = 0; i < m_perfData.size(); i++)
   {
      PerfDataValue *v = m_perfData.get(i);
      table->addRow();
      table->set(0, v->label);
      table->set(1, v->value);
      table->set(2, v->uom);
      table->set(3, v->warning);
      table->set(4, v->critical);
      table->set(5, v->min);
      table->set(6, v->max);
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Configured external checks
 */
static StringObjectMap<ExternalCheck> s_checks(Ownership::True);

/**
 * Handler for ExternalCheck.Status(*)
 */
LONG H_ExternalCheckStatus(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;
   ExternalCheck *check = s_checks.get(name);
   if (check == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return check->getStatus(value);
}

/**
 * Handler for ExternalCheck.StatusText(*)
 */
LONG H_ExternalCheckStatusText(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;
   ExternalCheck *check = s_checks.get(name);
   if (check == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return check->getStatusText(value);
}

/**
 * Handler for ExternalCheck.Value(*)
 */
LONG H_ExternalCheckValue(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256], label[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256) || !AgentGetMetricArg(cmd, 2, label, 256) || (label[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;
   ExternalCheck *check = s_checks.get(name);
   if (check == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return check->getPerfDataValue(label, value);
}

/**
 * Handler for ExternalCheck.Checks list
 */
LONG H_ExternalCheckList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   StringList names = s_checks.keys();
   value->addAll(&names);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for ExternalCheck.PerfData(*) table
 */
LONG H_ExternalCheckPerfData(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetMetricArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;
   ExternalCheck *check = s_checks.get(name);
   if (check == nullptr)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   return check->getPerfDataTable(value);
}

/**
 * Add external check from configuration. Expected format is name:command.
 */
bool AddExternalCheck(TCHAR *config)
{
   TCHAR *cmdLine = _tcschr(config, _T(':'));
   if (cmdLine == nullptr)
      return false;

   *cmdLine = 0;
   cmdLine++;
   Trim(config);
   Trim(cmdLine);
   if ((*config == 0) || (*cmdLine == 0))
      return false;

   s_checks.set(config, new ExternalCheck(config, cmdLine));
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Added external check \"%s\" (command \"%s\")"), config, cmdLine);
   return true;
}
