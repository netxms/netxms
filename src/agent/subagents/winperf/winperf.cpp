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
// CPU utilization
//

static LONG H_PdhhCounterValue(char *pszParam, char *pArg, char *pValue)
{
   HQUERY hQuery;
   HhCounter hhCounter;
   PDH_RAW_COUNTER rawData;
   PDH_FMT_COUNTERVALUE hCounterValue;
   PDH_STATUS rc;
   PDH_hCounter_INFO ci;
   TCHAR szhCounter[MAX_PATH], szBuffer[1024];
   static TCHAR szFName[] = _T("H_PdhhCounterValue");

   NxGetParameterArg(pszParam, 1, szhCounter, MAX_PATH);

   if ((rc = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhOpenQuery"), rc);
      return SYSINFO_RC_ERROR;
   }

   if ((rc = PdhAddhCounter(hQuery, szhCounter, 0, &hCounter)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhAddhCounter"), rc);
      PdhCloseQuery(hQuery);
      return SYSINFO_RC_UNSUPPORTED;
   }

   if ((rc = PdhGethCounterInfo(hCounter, FALSE, &dwSize, &ci)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhGetCounterInfo"), rc);
      PdhCloseQuery(hQuery);
      return SYSINFO_RC_ERROR;
   }

   if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhCollectQueryData"), rc);
      PdhCloseQuery(hQuery);
      return SYSINFO_RC_ERROR;
   }

   PdhGetRawCounterValue(hCounter, NULL, &rawData);
   PdhCalculateCounterFromRawValue(hCounter, PDH_FMT_DOUBLE,
                                   &rawData, NULL, &hCounterValue);
   PdhCloseQuery(hQuery);
   return SYSINFO_RC_SUCCESS;
}


//
// List of available performance objects
//

static LONG H_PdhObjects(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
   TCHAR *pszObject, *pszObjList, szHostName[256];
   LONG iResult = SYSINFO_RC_ERROR;
   DWORD dwSize;

   dwSize = 256;
   if (GetComputerName(szHostName, &dwSize))
   {
      dwSize = 256000;
      pszObjList = (TCHAR *)malloc(sizeof(TCHAR) * dwSize);
      if (PdhEnumObjects(NULL, szHostName, pszObjList, &dwSize, PERF_DETAIL_WIZARD, TRUE) == ERROR_SUCCESS)
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

static LONG H_PdhObjectItems(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
   TCHAR *pszElement, *pszhCounterList, *pszInstanceList, szHostName[256], szObject[256];
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
         pszhCounterList = (TCHAR *)malloc(sizeof(TCHAR) * dwSize1);
         pszInstanceList = (TCHAR *)malloc(sizeof(TCHAR) * dwSize2);
         rc = PdhEnumObjectItems(NULL, szHostName, szObject,
                                 pszhCounterList, &dwSize1, pszInstanceList, &dwSize2, 
                                 PERF_DETAIL_WIZARD, 0);
         if ((rc == ERROR_SUCCESS) || (rc == PDH_MORE_DATA))
         {
            for(pszElement = (pArg[0] == _T('C')) ? pszhCounterList : pszInstanceList;
                *pszElement != 0; pszElement += _tcslen(pszElement) + 1)
               NxAddResultString(pValue, pszElement);
            iResult = SYSINFO_RC_SUCCESS;
         }
         else
         {
            ReportPdhError(_T("H_PdhObjectItems"), _T("PdhEnumObjectItems"), rc);
         }
         free(pszhCounterList);
         free(pszInstanceLIst)
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
   { "PDH.hCounterValue(*)", H_PdhhCounterValue, NULL }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "PDH.ObjecthCounters(*)", H_PdhObjectItems, _T("C") },
   { "PDH.ObjectInstances(*)", H_PdhObjectItems, _T("I") },
   { "PDH.Objects", H_PdhObjects, NULL }
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
// Entry point for NetXMS agent
//

extern "C" BOOL __declspec(dllexport) __cdecl 
   NxSubAgentInit(NETXMS_SUBAGENT_INFO **ppInfo, TCHAR *pszConfigFile)
{
   *ppInfo = &m_info;
   return TRUE;
}


//
// DLL entry point
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   return TRUE;
}
