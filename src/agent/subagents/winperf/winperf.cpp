/*
** Windows Performance NetXMS subagent
** Copyright (C) 2004 Victor Kirhenshtein
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
** $module: winperf.cpp
**
**/

#include "winperf.h"


//
// Constants
//

#define WPF_ENABLE_DEFAULT_COUNTERS    0x0001


//
// Global variables
//

HANDLE g_hCondShutdown = NULL;


//
// Static variables
//

static DWORD m_dwFlags = WPF_ENABLE_DEFAULT_COUNTERS;


//
// List of predefined performance counters
//
static struct
{
   TCHAR *pszParamName;
   TCHAR *pszCounterName;
   int iClass;
   int iNumSamples;
   int iDataType;
} m_counterList[] =
{
   { _T("System.CPU.LoadAvg"), _T("\\System\\Processor Queue Length"), 0, 60, COUNTER_TYPE_FLOAT },
   { _T("System.CPU.LoadAvg5"), _T("\\System\\Processor Queue Length"), 0, 300, COUNTER_TYPE_FLOAT },
   { _T("System.CPU.LoadAvg15"), _T("\\System\\Processor Queue Length"), 0, 900, COUNTER_TYPE_FLOAT },
   { _T("System.CPU.Usage"), _T("\\Processor(_Total)\\% Processor Time"), 0, 60, COUNTER_TYPE_INT32 },
   { _T("System.CPU.Usage5"), _T("\\Processor(_Total)\\% Processor Time"), 0, 300, COUNTER_TYPE_INT32 },
   { _T("System.CPU.Usage15"), _T("\\Processor(_Total)\\% Processor Time"), 0, 900, COUNTER_TYPE_INT32 },
   { NULL, NULL, 0, 0, 0 }
};


//
// Value of given performance counter
//

static LONG H_PdhVersion(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
   DWORD dwVersion;

   if (PdhGetDllVersion(&dwVersion) != ERROR_SUCCESS)
      return SYSINFO_RC_ERROR;
   ret_uint(pValue, dwVersion);
   return SYSINFO_RC_SUCCESS;
}


//
// Value of given counter collected by one of the collector threads
//

LONG H_CollectedCounterData(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
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


//
// Value of given performance counter
//

static LONG H_PdhCounterValue(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
   HQUERY hQuery;
   HCOUNTER hCounter;
   PDH_RAW_COUNTER rawData1, rawData2;
   PDH_FMT_COUNTERVALUE counterValue;
   PDH_STATUS rc;
   TCHAR szCounter[MAX_PATH], szBuffer[16];
   DWORD dwType;
   BOOL bUseTwoSamples = FALSE;
   static TCHAR szFName[] = _T("H_PdhCounterValue");

   if ((!NxGetParameterArg(pszParam, 1, szCounter, MAX_PATH)) ||
       (!NxGetParameterArg(pszParam, 2, szBuffer, 16)))
      return SYSINFO_RC_UNSUPPORTED;

   bUseTwoSamples = _tcstol(szBuffer, NULL, 0) ? TRUE : FALSE;

   if ((rc = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhOpenQuery"), rc);
      return SYSINFO_RC_ERROR;
   }

   if ((rc = PdhAddCounter(hQuery, szCounter, 0, &hCounter)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhAddCounter"), rc);
      PdhCloseQuery(hQuery);
      return SYSINFO_RC_UNSUPPORTED;
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
   if (bUseTwoSamples)
   {
      Sleep(1000);   // We will take second sample after one second
      if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
      {
         ReportPdhError(szFName, _T("PdhCollectQueryData"), rc);
         PdhCloseQuery(hQuery);
         return SYSINFO_RC_ERROR;
      }
      PdhGetRawCounterValue(hCounter, NULL, &rawData2);
   }

   if (dwType & PERF_SIZE_LARGE)
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


//
// List of available performance objects
//

static LONG H_PdhObjects(TCHAR *pszParam, TCHAR *pArg, NETXMS_VALUES_LIST *pValue)
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
            NxAddResultString(pValue, pszObject);
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


//
// List of available performance items for given object
//

static LONG H_PdhObjectItems(TCHAR *pszParam, TCHAR *pArg, NETXMS_VALUES_LIST *pValue)
{
   TCHAR *pszElement, *pszCounterList, *pszInstanceList, szHostName[256], szObject[256];
   LONG iResult = SYSINFO_RC_ERROR;
   DWORD dwSize1, dwSize2;
   PDH_STATUS rc;

   NxGetParameterArg(pszParam, 1, szObject, 256);
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
               NxAddResultString(pValue, pszElement);
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


//
// Value of specific performance parameter, which is mapped one-to-one to
// performance counter. Actually, it's an alias for PDH.CounterValue(xxx) parameter.
//

static LONG H_CounterAlias(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
   return H_PdhCounterValue(pArg, NULL, pValue);
}


//
// Handler for subagent unload
//

static void OnUnload(void)
{
   if (g_hCondShutdown != NULL)
      SetEvent(g_hCondShutdown);
   Sleep(500);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("PDH.CounterValue(*)"), H_PdhCounterValue, NULL },
   { _T("PDH.Version"), H_PdhVersion, NULL },
   { _T("System.Threads"), H_CounterAlias, _T("(\\System\\Threads)") },
   { _T("System.Uptime"), H_CounterAlias, _T("(\\System\\System Up Time)") }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { _T("PDH.ObjectCounters(*)"), H_PdhObjectItems, _T("C") },
   { _T("PDH.ObjectInstances(*)"), H_PdhObjectItems, _T("I") },
   { _T("PDH.Objects"), H_PdhObjects, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WinPerf"), _T(NETXMS_VERSION_STRING) _T(DEBUG_SUFFIX), OnUnload,
	0, NULL, NULL,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};


//
// Add new parameter to list
//

BOOL AddParameter(TCHAR *pszName, LONG (* fpHandler)(TCHAR *, TCHAR *, TCHAR *), TCHAR *pArg)
{
   DWORD i;

   for(i = 0; i < m_info.dwNumParameters; i++)
      if (!_tcsicmp(pszName, m_info.pParamList[i].szName))
         break;

   if (i == m_info.dwNumParameters)
   {
      // Extend list
      m_info.dwNumParameters++;
      m_info.pParamList = 
         (NETXMS_SUBAGENT_PARAM *)realloc(m_info.pParamList, 
                  sizeof(NETXMS_SUBAGENT_PARAM) * m_info.dwNumParameters);
   }

   _tcsncpy(m_info.pParamList[i].szName, pszName, MAX_PARAM_NAME);
   m_info.pParamList[i].fpHandler = fpHandler;
   m_info.pParamList[i].pArg = pArg;
   
   return TRUE;
}


//
// Add predefined counters
//

static void AddPredefinedCounters(void)
{
   int i;
   WINPERF_COUNTER *pCnt;

   for(i = 0; m_counterList[i].pszParamName != NULL; i++)
   {
      pCnt = AddCounter(m_counterList[i].pszCounterName, m_counterList[i].iClass,
                        m_counterList[i].iNumSamples, m_counterList[i].iDataType);
      if (pCnt != NULL)
         AddParameter(m_counterList[i].pszParamName, H_CollectedCounterData, (TCHAR *)pCnt);
   }
}


//
// Configuration file template
//

static TCHAR *m_pszCounterList = NULL;
static NX_CFG_TEMPLATE cfgTemplate[] =
{
   { _T("Counter"), CT_STRING_LIST, _T('\n'), 0, 0, 0, &m_pszCounterList },
   { _T("EnableDefaultCounters"), CT_BOOLEAN, 0, 0, WPF_ENABLE_DEFAULT_COUNTERS, 0, &m_dwFlags },
   { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Entry point for NetXMS agent
//

extern "C" BOOL __declspec(dllexport) __cdecl 
   NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
{
   DWORD dwResult;

   // Init parameters list
   m_info.dwNumParameters = sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM);
   m_info.pParamList = (NETXMS_SUBAGENT_PARAM *)nx_memdup(m_parameters, sizeof(m_parameters));

   // Load configuration
   dwResult = NxLoadConfig(pszConfigFile, _T("WinPerf"), cfgTemplate, FALSE);
   if (dwResult == NXCFG_ERR_OK)
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
               NxWriteAgentLog(EVENTLOG_WARNING_TYPE, 
                               _T("Unable to add counter from configuration file. "
                                  "Original configuration record: %s",), pItem);
         }
         free(m_pszCounterList);
      }

      if (m_dwFlags & WPF_ENABLE_DEFAULT_COUNTERS)
         AddPredefinedCounters();

      // Create shitdown condition object
      g_hCondShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);

      StartCollectorThreads();
   }
   else
   {
      safe_free(m_pszCounterList);
   }
   *ppInfo = &m_info;
   return dwResult == NXCFG_ERR_OK;
}


//
// DLL entry point
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}
