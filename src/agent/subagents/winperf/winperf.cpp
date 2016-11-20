/*
** Windows Performance NetXMS subagent
** Copyright (C) 2004-2016 Victor Kirhenshtein
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

/**
 * Constants
 */
#define WPF_ENABLE_DEFAULT_COUNTERS    0x0001

/**
 * Static variables
 */
static DWORD m_dwFlags = WPF_ENABLE_DEFAULT_COUNTERS;
static MUTEX s_autoCountersLock = MutexCreate();
static StringObjectMap<WINPERF_COUNTER> *s_autoCounters = new StringObjectMap<WINPERF_COUNTER>(false);

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
   { _T("System.IO.DiskQueue"), _T("\\PhysicalDisk(_Total)\\Avg. Disk Queue Length"), 0, 60, COUNTER_TYPE_FLOAT, DCIDESC_SYSTEM_IO_DISKQUEUE, DCI_DT_FLOAT },
   { _T("System.IO.DiskTime"), _T("\\PhysicalDisk(_Total)\\% Disk Time"), 0, 60, COUNTER_TYPE_FLOAT, _T("Average disk busy time for last minute"), DCI_DT_FLOAT },
   { NULL, NULL, 0, 0, 0, NULL, 0 }
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

	if (pArg == NULL)		// Normal call
	{
		if (!AgentGetParameterArg(pszParam, 1, szCounter, MAX_PATH))
			return SYSINFO_RC_UNSUPPORTED;

		// required number of samples
		TCHAR buffer[MAX_PATH] = _T("");
		AgentGetParameterArg(pszParam, 2, buffer, MAX_PATH);
		if (buffer[0] != 0)   // sample count given
		{
			int samples = _tcstol(buffer, NULL, 0);
			if (samples > 1)
			{
				_sntprintf(buffer, MAX_PATH, _T("%d#%s"), samples, szCounter);
				MutexLock(s_autoCountersLock);
				WINPERF_COUNTER *counter = s_autoCounters->get(buffer);
				if (counter == NULL)
				{
					counter = AddCounter(szCounter, 0, samples, COUNTER_TYPE_AUTO);
					if (counter != NULL)
					{
						AgentWriteDebugLog(5, _T("WINPERF: new automatic counter created: \"%s\""), buffer);
						s_autoCounters->set(buffer, counter);
					}
				}
				MutexUnlock(s_autoCountersLock);
				return (counter != NULL) ? H_CollectedCounterData(pszParam, (const TCHAR *)counter, pValue, session) : SYSINFO_RC_UNSUPPORTED;
			}
		}
	}
	else	// Call from H_CounterAlias
	{
		nx_strncpy(szCounter, pArg, MAX_PATH);
	}

   if ((rc = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhOpenQuery"), rc);
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
				AgentWriteDebugLog(2, _T("WINPERF: Counter translated: %s ==> %s"), szCounter, newName);
				rc = PdhAddCounter(hQuery, newName, 0, &hCounter);
			}
			free(newName);
		}
	   if (rc != ERROR_SUCCESS)
		{
			ReportPdhError(szFName, _T("PdhAddCounter"), rc);
			PdhCloseQuery(hQuery);
	      return SYSINFO_RC_UNSUPPORTED;
		}
   }

   // Get first sample
   if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhCollectQueryData"), rc);
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
				ReportPdhError(szFName, _T("PdhCollectQueryData"), rc);
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
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LARGE,
                                         &rawData2, &rawData1, &counterValue);
      else
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LARGE,
                                         &rawData1, NULL, &counterValue);
      ret_int64(pValue, counterValue.largeValue);
   }
   else
   {
      if (bUseTwoSamples)
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LONG,
                                         &rawData2, &rawData1, &counterValue);
      else
         PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_LONG,
                                         &rawData1, NULL, &counterValue);
      ret_int(pValue, counterValue.longValue);
   }
   PdhCloseQuery(hQuery);
   return SYSINFO_RC_SUCCESS;
}

/**
 * List of available performance objects
 */
static LONG H_PdhObjects(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   TCHAR *pszObject, *pszObjList, szHostName[256];
   LONG iResult = SYSINFO_RC_ERROR;
   PDH_STATUS rc;
   DWORD dwSize;

   dwSize = 256;
   if (GetComputerName(szHostName, &dwSize))
   {
      dwSize = 256000;
      pszObjList = (TCHAR *)malloc(sizeof(TCHAR) * dwSize);
      if ((rc = PdhEnumObjects(NULL, szHostName, pszObjList, &dwSize,
                               PERF_DETAIL_WIZARD, TRUE)) == ERROR_SUCCESS)
      {
         for(pszObject = pszObjList; *pszObject != 0; pszObject += _tcslen(pszObject) + 1)
				value->add(pszObject);
         iResult = SYSINFO_RC_SUCCESS;
      }
      else
      {
         ReportPdhError(_T("H_PdhObjects"), _T("PdhEnumObjects"), rc);
      }
      free(pszObjList);
   }
   return iResult;
}

/**
 * List of available performance items for given object
 */
static LONG H_PdhObjectItems(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
   TCHAR *pszElement, *pszCounterList, *pszInstanceList, szHostName[256], szObject[256];
   LONG iResult = SYSINFO_RC_ERROR;
   DWORD dwSize1, dwSize2;
   PDH_STATUS rc;

   AgentGetParameterArg(pszParam, 1, szObject, 256);
   if (szObject[0] != 0)
   {
      dwSize1 = 256;
      if (GetComputerName(szHostName, &dwSize1))
      {
         dwSize1 = dwSize2 = 256000;
         pszCounterList = (TCHAR *)malloc(sizeof(TCHAR) * dwSize1);
         pszInstanceList = (TCHAR *)malloc(sizeof(TCHAR) * dwSize2);
         rc = PdhEnumObjectItems(NULL, szHostName, szObject,
                                 pszCounterList, &dwSize1, pszInstanceList, &dwSize2,
                                 PERF_DETAIL_WIZARD, 0);
         if ((rc == ERROR_SUCCESS) || (rc == PDH_MORE_DATA))
         {
            for(pszElement = (pArg[0] == _T('C')) ? pszCounterList : pszInstanceList;
                *pszElement != 0; pszElement += _tcslen(pszElement) + 1)
               value->add(pszElement);
            iResult = SYSINFO_RC_SUCCESS;
         }
         else
         {
            ReportPdhError(_T("H_PdhObjectItems"), _T("PdhEnumObjectItems"), rc);
         }
         free(pszCounterList);
         free(pszInstanceList);
      }
   }
   else
   {
      iResult = SYSINFO_RC_UNSUPPORTED;
   }
   return iResult;
}

/**
 * Value of specific performance parameter, which is mapped one-to-one to
 * performance counter. Actually, it's an alias for PDH.CounterValue(xxx) parameter.
 */
static LONG H_CounterAlias(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   return H_PdhCounterValue(NULL, pArg, pValue, session);
}

/**
 * Handler for System.Memory.Physical.FreePerc parameter
 */
static LONG H_FreeMemoryPct(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session)
{
   TCHAR buffer[MAX_RESULT_LENGTH];
   LONG rc = H_PdhCounterValue(NULL, pArg, buffer, session);
   if (rc != SYSINFO_RC_SUCCESS)
      return rc;

   UINT64 free = _tcstoull(buffer, NULL, 10);

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
static BOOL SubAgentInit(Config *config)
{
   StartCollectorThreads();
	return TRUE;
}

/**
 * Handler for subagent unload
 */
static void SubAgentShutdown()
{
   JoinCollectorThreads();
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("PDH.CounterValue(*)"), H_PdhCounterValue, NULL, DCI_DT_INT, _T("") },
   { _T("PDH.Version"), H_PdhVersion, NULL, DCI_DT_UINT, _T("Version of PDH.DLL") },
	{ _T("System.ThreadCount"), H_CounterAlias, _T("\\System\\Threads"), DCI_DT_UINT, DCIDESC_SYSTEM_THREADCOUNT },
	{ _T("System.Uptime"), H_CounterAlias, _T("\\System\\System Up Time"), DCI_DT_UINT, DCIDESC_SYSTEM_UPTIME },
   { _T("WinPerf.Features"), H_WinPerfFeatures, NULL, DCI_DT_UINT, _T("Features supported by this WinPerf version") },
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("PDH.ObjectCounters(*)"), H_PdhObjectItems, _T("C") },
   { _T("PDH.ObjectInstances(*)"), H_PdhObjectItems, _T("I") },
   { _T("PDH.Objects"), H_PdhObjects, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WinPerf"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, NULL,      // handlers
   0, NULL,             // parameters
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Add new parameter to list
 */
BOOL AddParameter(TCHAR *pszName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *), TCHAR *pArg, int iDataType, TCHAR *pszDescription)
{
   DWORD i;

   for(i = 0; i < m_info.numParameters; i++)
      if (!_tcsicmp(pszName, m_info.parameters[i].name))
         break;

   if (i == m_info.numParameters)
   {
      // Extend list
      m_info.numParameters++;
      m_info.parameters =
         (NETXMS_SUBAGENT_PARAM *)realloc(m_info.parameters,
                  sizeof(NETXMS_SUBAGENT_PARAM) * m_info.numParameters);
   }

   nx_strncpy(m_info.parameters[i].name, pszName, MAX_PARAM_NAME);
   m_info.parameters[i].handler = fpHandler;
   m_info.parameters[i].arg = pArg;
   m_info.parameters[i].dataType = iDataType;
   nx_strncpy(m_info.parameters[i].description, pszDescription, MAX_DB_STRING);

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
static TCHAR *m_pszCounterList = NULL;
static NX_CFG_TEMPLATE m_cfgTemplate[] =
{
   { _T("Counter"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszCounterList },
   { _T("EnableDefaultCounters"), CT_BOOLEAN, 0, 0, WPF_ENABLE_DEFAULT_COUNTERS, 0, &m_dwFlags },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WINPERF)
{
	if (m_info.parameters != NULL)
		return FALSE;	// Most likely another instance of WINPERF subagent already loaded

	// Read performance counter indexes
	TCHAR *counters = NULL;
	size_t countersBufferSize = 0;
   DWORD status;
	do
	{
		countersBufferSize += 8192;
		counters = (TCHAR *)realloc(counters, countersBufferSize);
		DWORD bytes = (DWORD)countersBufferSize;
      DWORD type;
		status = RegQueryValueEx(HKEY_PERFORMANCE_DATA, _T("Counter 009"), NULL, &type, (BYTE *)counters, &bytes);
	} while(status == ERROR_MORE_DATA);
   if ((status != ERROR_SUCCESS) || (counters[0] == 0))
   {
      AgentWriteDebugLog(1, _T("WinPerf: failed to read counters from HKEY_PERFORMANCE_DATA"));

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
		      counters = (TCHAR *)realloc(counters, countersBufferSize);
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
	      status = RegQueryValueEx(hKey, _T("Counter"), NULL, &type, (BYTE *)localCounters, &bytes);
	      while(status == ERROR_MORE_DATA)
	      {
		      countersBufferSize += 8192;
		      localCounters = (TCHAR *)realloc(localCounters, countersBufferSize);
		      bytes = (DWORD)countersBufferSize;
   	      status = RegQueryValueEx(hKey, _T("Counter"), NULL, &type, (BYTE *)localCounters, &bytes);
	      }
         RegCloseKey(hKey);
      }

		CreateCounterIndex(counters, localCounters);
      free(localCounters);
	}
	free(counters);

   // Init parameters list
   m_info.numParameters = sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM);
   m_info.parameters = (NETXMS_SUBAGENT_PARAM *)nx_memdup(m_parameters, sizeof(m_parameters));

	// Check counter names for H_CounterAlias
   TCHAR *newName;
	for(UINT32 i = 0; i < m_info.numParameters; i++)
	{
		if (m_info.parameters[i].handler == H_CounterAlias)
		{
			CheckCounter(m_info.parameters[i].arg, &newName);
			if (newName != NULL)
				m_info.parameters[i].arg = newName;
		}
	}

   // Add System.Memory.Free* handlers if "\Memory\Free & Zero Page List Bytes" counter is available
   const TCHAR *counter = _T("\\Memory\\Free & Zero Page List Bytes");
	CheckCounter(counter, &newName);
   if (newName != NULL)
      counter = newName;
   TCHAR value[MAX_RESULT_LENGTH];
   if (H_PdhCounterValue(NULL, counter, value, NULL) == SYSINFO_RC_SUCCESS)
   {
      AgentWriteDebugLog(4, _T("WinPerf: \"\\Memory\\Free & Zero Page List Bytes\" is supported"));
      AddParameter(_T("System.Memory.Physical.Free"), H_CounterAlias, (TCHAR *)counter, DCI_DT_UINT64, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE);
      AddParameter(_T("System.Memory.Physical.FreePerc"), H_FreeMemoryPct, (TCHAR *)counter, DCI_DT_FLOAT, DCIDESC_SYSTEM_MEMORY_PHYSICAL_FREE_PCT);
   }
   else
   {
      AgentWriteDebugLog(4, _T("WinPerf: \"\\Memory\\Free & Zero Page List Bytes\" is not supported"));
      safe_free(newName);
   }

   // Load configuration
	bool success = config->parseTemplate(_T("WinPerf"), m_cfgTemplate);
	if (success)
   {
      TCHAR *pItem, *pEnd;

      // Parse counter list
      if (m_pszCounterList != NULL)
      {
         for(pItem = m_pszCounterList; *pItem != 0; pItem = pEnd + 1)
         {
            pEnd = _tcschr(pItem, _T('\n'));
            if (pEnd != NULL)
               *pEnd = 0;
            StrStrip(pItem);
            if (!AddCounterFromConfig(pItem))
               AgentWriteLog(EVENTLOG_WARNING_TYPE,
                               _T("Unable to add counter from configuration file. ")
                               _T("Original configuration record: %s"), pItem);
         }
         free(m_pszCounterList);
      }

      if (m_dwFlags & WPF_ENABLE_DEFAULT_COUNTERS)
         AddPredefinedCounters();
   }
   else
   {
      safe_free(m_pszCounterList);
   }
   *ppInfo = &m_info;
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
