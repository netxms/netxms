/*
** Windows Performance NetXMS subagent
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
** File: winperf.cpp
**
**/

#include "winperf.h"
#include <netxms-version.h>

/**
 * Constants
 */
#define WPF_ENABLE_DEFAULT_COUNTERS    0x0001

/**
 * Subagent shutdown condition
 */
HANDLE g_winperfShutdownCondition = nullptr;

/**
 * Static variables
 */
static uint32_t s_flags = WPF_ENABLE_DEFAULT_COUNTERS;
static Mutex s_autoCountersLock(MutexType::FAST);
static StringObjectMap<WINPERF_COUNTER> *s_autoCounters = new StringObjectMap<WINPERF_COUNTER>(Ownership::False);

/**
 * List of predefined performance counters
 */
static struct
{
   TCHAR *pszParamName;
   TCHAR *pszCounterName;
   int iClass;
   int iNumSamples;
   int iDataType;
   TCHAR *pszDescription;
   int iDCIDataType;
} m_counterList[] =
{
   { _T("System.CPU.LoadAvg"), _T("\\System\\Processor Queue Length"), 0, 60, COUNTER_TYPE_FLOAT, _T("Average CPU load for last minute"), DCI_DT_FLOAT },
   { _T("System.CPU.LoadAvg5"), _T("\\System\\Processor Queue Length"), 0, 300, COUNTER_TYPE_FLOAT, _T("Average CPU load for last 5 minutes"), DCI_DT_FLOAT },
   { _T("System.CPU.LoadAvg15"), _T("\\System\\Processor Queue Length"), 0, 900, COUNTER_TYPE_FLOAT, _T("Average CPU load for last 15 minutes"), DCI_DT_FLOAT },
   { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
};

/**
 * Handler for PDH.Version parameter
 */
static LONG H_PdhVersion(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   DWORD dwVersion;
   if (PdhGetDllVersion(&dwVersion) != ERROR_SUCCESS)
      return SYSINFO_RC_ERROR;
   ret_uint(pValue, dwVersion);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for WinPerf.Features parameter
 */
static LONG H_WinPerfFeatures(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   ret_uint(pValue, WINPERF_AUTOMATIC_SAMPLE_COUNT | WINPERF_REMOTE_COUNTER_CONFIG);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Value of given counter collected by one of the collector threads
 */
LONG H_CollectedCounterData(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   switch(((WINPERF_COUNTER *)pArg)->wType)
   {
      case COUNTER_TYPE_INT32:
         ret_int(pValue, ((WINPERF_COUNTER *)pArg)->value.iLong);
         break;
      case COUNTER_TYPE_INT64:
         ret_int64(pValue, ((WINPERF_COUNTER *)pArg)->value.iLarge);
         break;
      case COUNTER_TYPE_FLOAT:
         ret_double(pValue, ((WINPERF_COUNTER *)pArg)->value.dFloat);
         break;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Value of given performance counter
 */
static LONG H_PdhCounterValue(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   HQUERY hQuery;
   HCOUNTER hCounter;
   PDH_RAW_COUNTER rawData1, rawData2;
   PDH_FMT_COUNTERVALUE counterValue;
   PDH_STATUS rc;
   TCHAR szCounter[MAX_PATH];
   DWORD dwType;
   BOOL bUseTwoSamples = FALSE;
   static TCHAR szFName[] = _T("H_PdhCounterValue");

	if (pArg == nullptr)		// Normal call
	{
		if (!AgentGetParameterArg(pszParam, 1, szCounter, MAX_PATH))
			return SYSINFO_RC_UNSUPPORTED;

		// required number of samples
		TCHAR buffer[MAX_PATH] = _T("");
		AgentGetParameterArg(pszParam, 2, buffer, MAX_PATH);
		if (buffer[0] != 0)   // sample count given
		{
			int samples = _tcstol(buffer, nullptr, 0);
			if (samples > 1)
			{
				_sntprintf(buffer, MAX_PATH, _T("%d#%s"), samples, szCounter);
				s_autoCountersLock.lock();
				WINPERF_COUNTER *counter = s_autoCounters->get(buffer);
				if (counter == nullptr)
				{
					counter = AddCounter(szCounter, 0, samples, COUNTER_TYPE_AUTO);
					if (counter != nullptr)
					{
						nxlog_debug_tag(WINPERF_DEBUG_TAG, 5, _T("New automatic counter created: \"%s\""), buffer);
						s_autoCounters->set(buffer, counter);
					}
				}
				s_autoCountersLock.unlock();
				return (counter != nullptr) ? H_CollectedCounterData(pszParam, (const TCHAR *)counter, pValue, session) : SYSINFO_RC_UNSUPPORTED;
			}
		}
	}
	else	// Call from H_CounterAlias
	{
		_tcslcpy(szCounter, pArg, MAX_PATH);
	}

   if ((rc = PdhOpenQuery(nullptr, 0, &hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhOpenQuery"), nullptr, rc);
      return SYSINFO_RC_ERROR;
   }

   if ((rc = PdhAddCounter(hQuery, szCounter, 0, &hCounter)) != ERROR_SUCCESS)
   {
		// Attempt to translate counter name
		if ((rc == PDH_CSTATUS_NO_COUNTER) || (rc == PDH_CSTATUS_NO_OBJECT))
		{
			TCHAR *newName = (TCHAR *)malloc(_tcslen(szCounter) * sizeof(TCHAR) * 4);
			if (TranslateCounterName(szCounter, newName))
			{
				nxlog_debug_tag(WINPERF_DEBUG_TAG, 2, _T("Counter translated: %s ==> %s"), szCounter, newName);
				rc = PdhAddCounter(hQuery, newName, 0, &hCounter);
			}
			MemFree(newName);
		}
	   if (rc != ERROR_SUCCESS)
		{
			ReportPdhError(szFName, _T("PdhAddCounter"), szCounter, rc);
			PdhCloseQuery(hQuery);
	      return SYSINFO_RC_UNSUPPORTED;
		}
   }

   // Get first sample
   if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhCollectQueryData"), nullptr, rc);
      PdhCloseQuery(hQuery);
      return SYSINFO_RC_ERROR;
   }
   PdhGetRawCounterValue(hCounter, &dwType, &rawData1);

   // Get second sample if required
   if ((dwType & 0x00000C00) == PERF_TYPE_COUNTER)
   {
		DWORD subType = dwType & 0x000F0000;
		if ((subType == PERF_COUNTER_RATE) || (subType == PERF_COUNTER_PRECISION))
		{
			Sleep(1000);   // We will take second sample after one second
			if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
			{
				ReportPdhError(szFName, _T("PdhCollectQueryData"), nullptr,  rc);
				PdhCloseQuery(hQuery);
				return SYSINFO_RC_ERROR;
			}
			PdhGetRawCounterValue(hCounter, NULL, &rawData2);
			bUseTwoSamples = TRUE;
		}
   }

   if ((dwType & 0x00000300) == PERF_SIZE_LARGE)
   {
      if (bUseTwoSamples)
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LARGE, &rawData2, &rawData1, &counterValue);
      else
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LARGE, &rawData1, nullptr, &counterValue);
      ret_int64(pValue, counterValue.largeValue);
   }
   else
   {
      if (bUseTwoSamples)
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LONG, &rawData2, &rawData1, &counterValue);
      else
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LONG, &rawData1, nullptr, &counterValue);
      ret_int(pValue, counterValue.longValue);
   }
   PdhCloseQuery(hQuery);
   return SYSINFO_RC_SUCCESS;
}

/**
 * List of available performance objects
 */
static LONG H_PdhObjects(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR hostName[256];
   LONG iResult = SYSINFO_RC_ERROR;
   DWORD size = 256;
   if (GetComputerName(hostName, &size))
   {
      size = 256000;
      TCHAR *objectList = MemAllocString(size);
      PDH_STATUS rc = PdhEnumObjects(nullptr, hostName, objectList, &size, PERF_DETAIL_WIZARD, TRUE);
      if (rc == ERROR_SUCCESS)
      {
         for(TCHAR *object = objectList; *object != 0; object += _tcslen(object) + 1)
				value->add(object);
         iResult = SYSINFO_RC_SUCCESS;
      }
      else
      {
         ReportPdhError(_T("H_PdhObjects"), _T("PdhEnumObjects"), nullptr, rc);
      }
      MemFree(objectList);
   }
   return iResult;
}

/**
 * List of available performance items for given object
 */
static LONG H_PdhObjectItems(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   TCHAR szObject[256];
   if (!AgentGetParameterArg(pszParam, 1, szObject, 256))
      return SYSINFO_RC_UNSUPPORTED;

   if (szObject[0] == 0)
      return SYSINFO_RC_UNSUPPORTED;

   DWORD dwSize1 = 256;
   TCHAR hostName[256];
   if (!GetComputerName(hostName, &dwSize1))
      return SYSINFO_RC_ERROR;

   LONG result = SYSINFO_RC_ERROR;
   dwSize1 = 256000;
   DWORD dwSize2 = 256000;
   TCHAR *counterList = MemAllocString(dwSize1);
   TCHAR *instanceList = MemAllocString(dwSize2);
   PDH_STATUS rc = PdhEnumObjectItems(nullptr, hostName, szObject, counterList, &dwSize1, instanceList, &dwSize2, PERF_DETAIL_WIZARD, 0);
   if ((rc == ERROR_SUCCESS) || (rc == PDH_MORE_DATA))
   {
      for(TCHAR *pszElement = (pArg[0] == _T('C')) ? counterList : instanceList; *pszElement != 0; pszElement += _tcslen(pszElement) + 1)
         value->add(pszElement);
      result = SYSINFO_RC_SUCCESS;
   }
   else
   {
      ReportPdhError(_T("H_PdhObjectItems"), _T("PdhEnumObjectItems"), szObject, rc);
   }
   MemFree(counterList);
   MemFree(instanceList);
   return result;
}

/**
 * Value of specific performance parameter, which is mapped one-to-one to
 * performance counter. Actually, it's an alias for PDH.CounterValue(xxx) parameter.
 */
static LONG H_CounterAlias(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   return H_PdhCounterValue(nullptr, arg, value, session);
}

/**
 * Handler for System.Memory.Physical.FreePerc parameter
 */
static LONG H_FreeMemoryPct(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   LONG rc = H_PdhCounterValue(nullptr, pArg, buffer, session);
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   uint64_t free = _tcstoull(buffer, nullptr, 10);

   MEMORYSTATUSEX mse;
   mse.dwLength = sizeof(MEMORYSTATUSEX);
   if (!GlobalMemoryStatusEx(&mse))
      return SYSINFO_RC_ERROR;

   ret_double(pValue, ((double)free * 100.0 / mse.ullTotalPhys), 2);
   return SYSINFO_RC_SUCCESS;
}

/**
 * Initialize subagent
 */
static bool SubAgentInit(Config *config)
{
   g_winperfShutdownCondition = CreateEvent(nullptr, TRUE, FALSE, nullptr);
   StartCollectorThreads();
   return true;
}

/**
 * Handler for subagent unload
 */
static void SubAgentShutdown()
{
   SetEvent(g_winperfShutdownCondition);
   JoinCollectorThreads();
   CloseHandle(g_winperfShutdownCondition);
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("PDH.CounterValue(*)"), H_PdhCounterValue, nullptr, DCI_DT_INT, _T("") },
   { _T("PDH.Version"), H_PdhVersion, nullptr, DCI_DT_UINT, _T("Version of PDH.DLL") },
	{ _T("System.ThreadCount"), H_CounterAlias, _T("\\System\\Threads"), DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT },
   { _T("WinPerf.Features"), H_WinPerfFeatures, nullptr, DCI_DT_UINT, _T("Features supported by this WinPerf version") },
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("PDH.ObjectCounters(*)"), H_PdhObjectItems, _T("C") },
   { _T("PDH.ObjectInstances(*)"), H_PdhObjectItems, _T("I") },
   { _T("PDH.Objects"), H_PdhObjects, nullptr }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("WinPerf"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, nullptr, nullptr, nullptr,    // callbacks
   0, nullptr,             // parameters
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   s_lists,
   0, nullptr,	// tables
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Add new parameter to list
 */
BOOL AddParameter(TCHAR *pszName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *), TCHAR *pArg, int iDataType, TCHAR *pszDescription)
{
   DWORD i;

   for(i = 0; i < s_info.numParameters; i++)
      if (!_tcsicmp(pszName, s_info.parameters[i].name))
         break;

   if (i == s_info.numParameters)
   {
      // Extend list
      s_info.numParameters++;
      s_info.parameters = MemReallocArray(s_info.parameters, s_info.numParameters);
   }

   _tcslcpy(s_info.parameters[i].name, pszName, MAX_PARAM_NAME);
   s_info.parameters[i].handler = fpHandler;
   s_info.parameters[i].arg = pArg;
   s_info.parameters[i].dataType = iDataType;
   _tcslcpy(s_info.parameters[i].description, pszDescription, MAX_DB_STRING);

   return TRUE;
}

/**
 * Add predefined counters
 */
static void AddPredefinedCounters()
{
   for(int i = 0; m_counterList[i].pszParamName != NULL; i++)
   {
      WINPERF_COUNTER *pCnt = AddCounter(m_counterList[i].pszCounterName, m_counterList[i].iClass,
                        m_counterList[i].iNumSamples, m_counterList[i].iDataType);
      if (pCnt != NULL)
         AddParameter(m_counterList[i].pszParamName, H_CollectedCounterData,
                      (TCHAR *)pCnt, m_counterList[i].iDCIDataType,
                      m_counterList[i].pszDescription);
   }
}

/**
 * Configuration file template
 */
static TCHAR *s_counterList = nullptr;
static NX_CFG_TEMPLATE s_cfgTemplate[] =
{
   { _T("Counter"), CT_STRING_CONCAT, _T('\n'), 0, 0, 0, &s_counterList },
   { _T("EnableDefaultCounters"), CT_BOOLEAN_FLAG_32, 0, 0, WPF_ENABLE_DEFAULT_COUNTERS, 0, &s_flags },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, nullptr }
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WINPERF)
{
	if (s_info.parameters != nullptr)
		return false;	// Most likely another instance of WINPERF subagent already loaded

	// Read performance counter indexes
	TCHAR *counters = nullptr;
	size_t countersBufferSize = 0;
   DWORD status;
	do
	{
		countersBufferSize += 8192;
		counters = MemRealloc(counters, countersBufferSize);
		DWORD bytes = (DWORD)countersBufferSize;
      DWORD type;
		status = RegQueryValueEx(HKEY_PERFORMANCE_DATA, _T("Counter 009"), nullptr, &type, (BYTE *)counters, &bytes);
	} while(status == ERROR_MORE_DATA);
   if ((status != ERROR_SUCCESS) || (counters[0] == 0))
   {
      nxlog_debug_tag(WINPERF_DEBUG_TAG, 1, _T("Failed to read counters from HKEY_PERFORMANCE_DATA"));

      HKEY hKey;
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"), 0, KEY_READ, &hKey);
      if (status == ERROR_SUCCESS)
      {
	      DWORD bytes = (DWORD)countersBufferSize;
         DWORD type;
	      status = RegQueryValueEx(hKey, _T("Counter"), NULL, &type, (BYTE *)counters, &bytes);
	      while(status == ERROR_MORE_DATA)
	      {
		      countersBufferSize += 8192;
		      counters = MemRealloc(counters, countersBufferSize);
		      bytes = (DWORD)countersBufferSize;
   	      status = RegQueryValueEx(hKey, _T("Counter"), NULL, &type, (BYTE *)counters, &bytes);
	      }
         RegCloseKey(hKey);
      }
   }
	if (status == ERROR_SUCCESS)
	{
	   countersBufferSize = 8192;
	   TCHAR *localCounters = (TCHAR *)malloc(countersBufferSize);
      localCounters[0] = 0;

      HKEY hKey;
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\CurrentLanguage"), 0, KEY_READ, &hKey);
      if (status == ERROR_SUCCESS)
      {
	      DWORD bytes = (DWORD)countersBufferSize;
         DWORD type;
	      status = RegQueryValueEx(hKey, _T("Counter"), nullptr, &type, (BYTE *)localCounters, &bytes);
	      while(status == ERROR_MORE_DATA)
	      {
		      countersBufferSize += 8192;
		      localCounters = MemRealloc(localCounters, countersBufferSize);
		      bytes = (DWORD)countersBufferSize;
   	      status = RegQueryValueEx(hKey, _T("Counter"), NULL, &type, (BYTE *)localCounters, &bytes);
	      }
         RegCloseKey(hKey);
      }

		CreateCounterIndex(counters, localCounters);
      MemFree(localCounters);
	}
	MemFree(counters);

   // Init parameters list
   s_info.numParameters = sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM);
   s_info.parameters = MemCopyBlock(s_parameters, sizeof(s_parameters));

	// Check counter names for H_CounterAlias
   TCHAR *newName;
	for(size_t i = 0; i < s_info.numParameters; i++)
	{
		if (s_info.parameters[i].handler == H_CounterAlias)
		{
			CheckCounter(s_info.parameters[i].arg, &newName);
			if (newName != NULL)
				s_info.parameters[i].arg = newName;
		}
	}

   // Add System.Memory.Free* handlers if "\Memory\Free & Zero Page List Bytes" counter is available
   const TCHAR *counter = _T("\\Memory\\Free & Zero Page List Bytes");
	CheckCounter(counter, &newName);
   if (newName != NULL)
      counter = newName;
   TCHAR value[MAX_RESULT_LENGTH];
   if (H_PdhCounterValue(nullptr, counter, value, nullptr) == SYSINFO_RC_SUCCESS)
   {
      nxlog_debug_tag(WINPERF_DEBUG_TAG, 4, _T("\"\\Memory\\Free & Zero Page List Bytes\" is supported"));
      AddParameter(_T("System.Memory.Physical.Free"), H_CounterAlias, (TCHAR *)counter, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE);
      AddParameter(_T("System.Memory.Physical.FreePerc"), H_FreeMemoryPct, (TCHAR *)counter, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT);
   }
   else
   {
      nxlog_debug_tag(WINPERF_DEBUG_TAG, 4, _T("\"\\Memory\\Free & Zero Page List Bytes\" is not supported"));
      MemFree(newName);
   }

   // Load configuration
	bool success = config->parseTemplate(_T("WinPerf"), s_cfgTemplate);
	if (success)
   {
      // Parse counter list
      if (s_counterList != nullptr)
      {
         TCHAR *curr, *next;
         for(curr = s_counterList; *curr != 0; curr = next + 1)
         {
            next = _tcschr(curr, _T('\n'));
            if (next != nullptr)
               *next = 0;
            Trim(curr);
            if (!AddCounterFromConfig(curr))
            {
               nxlog_write_tag(NXLOG_WARNING, WINPERF_DEBUG_TAG, _T("Unable to add counter from configuration file. Original configuration record: %s"), curr);
            }
         }
         MemFree(s_counterList);
      }

      if (s_flags & WPF_ENABLE_DEFAULT_COUNTERS)
         AddPredefinedCounters();
   }
   else
   {
      MemFree(s_counterList);
   }
   *ppInfo = &s_info;
   return success;
}

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}
