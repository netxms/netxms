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

#define CFG_BUFFER_SIZE    256000


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
// Value of given performance counter
//

static LONG H_PdhCounterValue(TCHAR *pszParam, TCHAR *pArg, TCHAR *pValue)
{
   HQUERY hQuery;
   HCOUNTER hCounter;
   PDH_RAW_COUNTER rawData1, rawData2;
   PDH_FMT_COUNTERVALUE counterValue;
   PDH_STATUS rc;
   PDH_COUNTER_INFO ci;
   TCHAR szCounter[MAX_PATH], szBuffer[16];
   static TCHAR szFName[] = _T("H_PdhCounterValue");
   DWORD dwSize;
   BOOL bUseTwoSamples = FALSE;

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
   PdhGetRawCounterValue(hCounter, NULL, &rawData1);

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

   dwSize = sizeof(ci);
   if ((rc = PdhGetCounterInfo(hCounter, FALSE, &dwSize, &ci)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhGetCounterInfo"), rc);
      PdhCloseQuery(hQuery);
      return SYSINFO_RC_ERROR;
   }

   if (ci.dwType & PERF_SIZE_LARGE)
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
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("PDH.CounterValue(*)"), H_PdhCounterValue, NULL },
   { _T("PDH.Version"), H_PdhVersion, NULL }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { _T("PDH.ObjectCounters(*)"), H_PdhObjectItems, _T("C") },
   { _T("PDH.ObjectInstances(*)"), H_PdhObjectItems, _T("I") },
   { _T("PDH.Objects"), H_PdhObjects, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	"WinPerf", 0x01000000, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};


//
// Configuration file template
//

static NX_CFG_TEMPLATE cfgTemplate[] =
{
   { "Counter", CT_STRING_LIST, ',', 0, CFG_BUFFER_SIZE, 0, NULL },
   { "", CT_END_OF_LIST, 0, 0, 0, 0, NULL }
};


//
// Entry point for NetXMS agent
//

extern "C" BOOL __declspec(dllexport) __cdecl 
   NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
{
   DWORD dwResult;

   // Load configuration
   cfgTemplate[0].pBuffer = malloc(CFG_BUFFER_SIZE);
   dwResult = NxLoadConfig(pszConfigFile, _T("WinPerf"), cfgTemplate, FALSE);
   if (dwResult == NXCFG_ERR_OK)
   {
   }
   free(cfgTemplate[0].pBuffer);
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
