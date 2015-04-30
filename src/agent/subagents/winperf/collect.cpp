/*
** Windows Performance NetXMS subagent
** Copyright (C) 2004-2013 Victor Kirhenshtein
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
** File: collect.cpp
**
**/

#include "winperf.h"

/**
 * Counter sets
 */
WinPerfCounterSet *m_cntSet[3] =
{
	new WinPerfCounterSet(1000, _T('A')),
	new WinPerfCounterSet(5000, _T('B')),
	new WinPerfCounterSet(6000, _T('C'))
};

/**
 * Counter set constructor
 */
WinPerfCounterSet::WinPerfCounterSet(DWORD interval, TCHAR cls)
{
	m_mutex = MutexCreate();
	m_changeCondition = ConditionCreate(TRUE);
	m_interval = interval;
	m_class = cls;
	m_counters = new ObjectArray<WINPERF_COUNTER>(32, 32, true);
}

/**
 * Counter set destructor
 */
WinPerfCounterSet::~WinPerfCounterSet()
{
	MutexDestroy(m_mutex);
	ConditionDestroy(m_changeCondition);
	delete m_counters;
}

/**
 * Add counter to set
 */
void WinPerfCounterSet::addCounter(WINPERF_COUNTER *c)
{
	MutexLock(m_mutex);
	m_counters->add(c);
	MutexUnlock(m_mutex);
	ConditionSet(m_changeCondition);
}

/**
 * Collector thread
 */
void WinPerfCounterSet::collectorThread()
{
   HQUERY hQuery;
   PDH_STATUS rc;
   PDH_STATISTICS statData;
   TCHAR szFName[] = _T("CollectorThread_X");

   szFName[16] = m_class;

	// Outer loop - rebuild query after set changes
	while(1)
	{
		AgentWriteDebugLog(2, _T("WINPERF: %s waiting for set change"), szFName);

		HANDLE handles[2];
		handles[0] = g_hCondShutdown;
		handles[1] = m_changeCondition;
      DWORD waitStatus = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (waitStatus == WAIT_OBJECT_0)
         break;   // Shutdown condition
		if (waitStatus != WAIT_OBJECT_0 + 1)
			continue;	// set not changed, continue waiting

		ConditionReset(m_changeCondition);

		AgentWriteDebugLog(2, _T("WINPERF: %s: set changed"), szFName);
		if ((rc = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
		{
			ReportPdhError(szFName, _T("PdhOpenQuery"), rc);
			continue;
		}

		// Add counters to query
	   DWORD dwOKCounters = 0;
		MutexLock(m_mutex);
		for(int i = 0; i < m_counters->size(); i++)
		{
			WINPERF_COUNTER *counter = m_counters->get(i);
			if ((rc = PdhAddCounter(hQuery, counter->pszName, 0, &counter->handle)) != ERROR_SUCCESS)
			{
				TCHAR szBuffer[1024];

				AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("WINPERF: %s: Unable to add counter \"%s\" to query (%s)"), 
				              szFName, counter->pszName, GetPdhErrorText(rc, szBuffer, 1024));
				counter->handle = NULL;
			}
			else
			{
            AgentWriteDebugLog(4, _T("WINPERF: %s: Counter \"%s\" added to query"), szFName, counter->pszName);
				dwOKCounters++;
			}
		}
		MutexUnlock(m_mutex);

			// Check if we was able to add at least one counter
		if (dwOKCounters == 0)
		{
			PdhCloseQuery(hQuery);
			AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("WINPERF: Failed to add any counter to query for counter set %c"), m_class);
			continue;
		}

		// Collection loop (until shutdown or set change)
		AgentWriteDebugLog(2, _T("WINPERF: %s entered data collection loop"), szFName);
		while(1)
		{
			HANDLE handles[2];
			handles[0] = g_hCondShutdown;
			handles[1] = m_changeCondition;
			waitStatus = WaitForMultipleObjects(2, handles, FALSE, m_interval);
			if (waitStatus == WAIT_OBJECT_0)
				goto stop;	// shutdown
			if (waitStatus == WAIT_OBJECT_0 + 1)
				break;   // Set change, break collection loop

			// Collect data
			if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
				ReportPdhError(szFName, _T("PdhCollectQueryData"), rc);

			// Get raw values for each counter and compute average value
			for(int i = 0; i < m_counters->size(); i++)
			{
				WINPERF_COUNTER *counter = m_counters->get(i);
				if (counter->handle != NULL)
				{
					// Get raw value into buffer
					PdhGetRawCounterValue(counter->handle, NULL, &counter->pRawValues[counter->dwBufferPos]);
					counter->dwBufferPos++;
					if (counter->dwBufferPos == (DWORD)counter->wNumSamples)
						counter->dwBufferPos = 0;

					// Calculate mean value
					if ((rc = PdhComputeCounterStatistics(counter->handle,
								counter->dwFormat,
								counter->dwBufferPos, 
								counter->wNumSamples,
								counter->pRawValues,
								&statData)) != ERROR_SUCCESS)
					{
						ReportPdhError(szFName, _T("PdhComputeCounterStatistics"), rc);
					}

					// Update mean value in counter set
					switch(counter->wType)
					{
						case COUNTER_TYPE_INT32:
							counter->value.iLong = statData.mean.longValue;
							break;
						case COUNTER_TYPE_INT64:
							counter->value.iLarge = statData.mean.largeValue;
							break;
						case COUNTER_TYPE_FLOAT:
							counter->value.dFloat = statData.mean.doubleValue;
							break;
						default:
							break;
					}
				}
			}
		}

		AgentWriteDebugLog(2, _T("WINPERF: %s: data collection stopped"), szFName);
		PdhCloseQuery(hQuery);
	}

stop:
	AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("WINPERF: Collector thread for counter set %c terminated"), m_class);
}

/**
 * Starter for collector thread
 */
THREAD_RESULT THREAD_CALL WinPerfCounterSet::collectorThreadStarter(void *arg)
{
	((WinPerfCounterSet *)arg)->collectorThread();
	return THREAD_OK;
}

/**
 * Start collector thread
 */
void WinPerfCounterSet::startCollectorThread()
{
	ThreadCreate(collectorThreadStarter, 0, this);
}

/**
 * Check that counter's name is valid and determine counter's type
 */
int CheckCounter(const TCHAR *pszName, TCHAR **ppszNewName)
{
   HQUERY hQuery;
   HCOUNTER hCounter;
   PDH_STATUS rc;
   PDH_COUNTER_INFO ci;
   DWORD dwSize;
   static TCHAR szFName[] = _T("CheckCounter");

	*ppszNewName = NULL;

   if ((rc = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhOpenQuery"), rc);
      return -1;
   }

	rc = PdhAddCounter(hQuery, pszName, 0, &hCounter);
   if (rc != ERROR_SUCCESS)
   {
		// Attempt to translate counter name
		if ((rc == PDH_CSTATUS_NO_COUNTER) || (rc == PDH_CSTATUS_NO_OBJECT))
		{
			*ppszNewName = (TCHAR *)malloc(_tcslen(pszName) * sizeof(TCHAR) * 4);
			if (TranslateCounterName(pszName, *ppszNewName))
			{
				AgentWriteDebugLog(2, _T("WINPERF: Counter translated: %s ==> %s"), pszName, *ppszNewName);
				rc = PdhAddCounter(hQuery, *ppszNewName, 0, &hCounter);
				if (rc != ERROR_SUCCESS)
				{
					free(*ppszNewName);
					*ppszNewName = NULL;
				}
			}
		}
	   if (rc != ERROR_SUCCESS)
		{
			ReportPdhError(szFName, _T("PdhAddCounter"), rc);
			PdhCloseQuery(hQuery);
			return -1;
		}
   }

   dwSize = sizeof(ci);
   if ((rc = PdhGetCounterInfo(hCounter, FALSE, &dwSize, &ci)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhGetCounterInfo"), rc);
      PdhCloseQuery(hQuery);
      return -1;
   }

   PdhCloseQuery(hQuery);
   return (ci.dwType & (PERF_COUNTER_RATE | PERF_COUNTER_FRACTION)) ? COUNTER_TYPE_FLOAT :
      ((ci.dwType & PERF_SIZE_LARGE) ? COUNTER_TYPE_INT64 : COUNTER_TYPE_INT32);
}

/**
 * Add counter to set
 */
WINPERF_COUNTER *AddCounter(TCHAR *pszName, int iClass, int iNumSamples, int iDataType)
{
   WINPERF_COUNTER *pCnt;
	TCHAR *pszNewName;
   int iType;

   // Check for valid class
   if ((iClass < 0) || (iClass > 2))
      return NULL;

   // Check counter's name and get it's type
   iType = CheckCounter(pszName, &pszNewName);
   if (iType == -1)
	{
		safe_free(pszNewName);
      return NULL;
	}

   // Create new counter
   pCnt = new WINPERF_COUNTER;
   memset(pCnt, 0, sizeof(WINPERF_COUNTER));
	if (pszNewName != NULL)
	{
		pCnt->pszName = (TCHAR *)realloc(pszNewName, (_tcslen(pszNewName) + 1) * sizeof(TCHAR));
	}
	else
	{
		pCnt->pszName = _tcsdup(pszName);
	}
   pCnt->wType = (iDataType == COUNTER_TYPE_AUTO) ? iType : iDataType;
   pCnt->wNumSamples = iNumSamples;
   pCnt->pRawValues = (PDH_RAW_COUNTER *)malloc(sizeof(PDH_RAW_COUNTER) * iNumSamples);
   memset(pCnt->pRawValues, 0, sizeof(PDH_RAW_COUNTER) * iNumSamples);
   switch(pCnt->wType)
   {
      case COUNTER_TYPE_INT32:
         pCnt->dwFormat = PDH_FMT_LONG;
         break;
      case COUNTER_TYPE_INT64:
         pCnt->dwFormat = PDH_FMT_LARGE;
         break;
      case COUNTER_TYPE_FLOAT:
         pCnt->dwFormat = PDH_FMT_DOUBLE;
         break;
   }

   // Add counter to set
	m_cntSet[iClass]->addCounter(pCnt);
   return pCnt;
}

/**
 * Add custom counter from configuration file
 * Should be in form 
 * <parameter name>:<counter path>:<number of samples>:<class>:<data type>:<description>
 * Class can be A (poll every second), B (poll every 5 seconds) or C (poll every 30 seconds)
 */
BOOL AddCounterFromConfig(TCHAR *pszStr)
{
   TCHAR *ptr, *eptr, *pszCurrField;
   TCHAR szParamName[MAX_PARAM_NAME], szCounterPath[MAX_PATH];
   TCHAR szDescription[MAX_DB_STRING] = _T("");
   int iState, iField, iNumSamples, iClass, iPos, iDataType = DCI_DT_INT;
   WINPERF_COUNTER *pCnt;

   // Parse line
   pszCurrField = (TCHAR *)malloc(sizeof(TCHAR) * (_tcslen(pszStr) + 1));
   for(ptr = pszStr, iState = 0, iField = 0, iPos = 0; (iState != -1) && (iState != 255); ptr++)
   {
      switch(iState)
      {
         case 0:  // Normal text
            switch(*ptr)
            {
               case '\'':
                  iState = 1;
                  break;
               case '"':
                  iState = 2;
                  break;
               case ':':   // New field
               case 0:
                  pszCurrField[iPos] = 0;
                  switch(iField)
                  {
                     case 0:  // Parameter name
                        nx_strncpy(szParamName, pszCurrField, MAX_PARAM_NAME);
                        break;
                     case 1:  // Counter path
                        nx_strncpy(szCounterPath, pszCurrField, MAX_PATH);
                        break;
                     case 2:  // Number of samples
                        iNumSamples = _tcstol(pszCurrField, &eptr, 0);
                        if (*eptr != 0)
                           iState = 255;  // Error
                        break;
                     case 3:  // Class
                        _tcsupr(pszCurrField);
                        iClass = *pszCurrField - _T('A');
                        if ((iClass < 0) || (iClass > 2))
                           iState = 255;  // Error
                        break;
                     case 4:  // Data type
                        iDataType = NxDCIDataTypeFromText(pszCurrField);
                        if (iDataType == -1)    // Invalid data type specified
                           iState = 255;
                        break;
                     case 5:  // Description
                        nx_strncpy(szDescription, pszCurrField, MAX_DB_STRING);
                        break;
                     default:
                        iState = 255;  // Error
                        break;
                  }
                  iField++;
                  iPos = 0;
                  if ((iState != 255) && (*ptr == 0))
                     iState = -1;   // Finish
                  break;
               default:
                  pszCurrField[iPos++] = *ptr;
                  break;
            }
            break;
         case 1:  // Single quotes
            switch(*ptr)
            {
               case '\'':
                  iState = 0;
                  break;
               case 0:  // Unexpected end of line
                  iState = 255;
                  break;
               default:
                  pszCurrField[iPos++] = *ptr;
                  break;
            }
            break;
         case 2:  // Double quotes
            switch(*ptr)
            {
               case '"':
                  iState = 0;
                  break;
               case 0:  // Unexpected end of line
                  iState = 255;
                  break;
               default:
                  pszCurrField[iPos++] = *ptr;
                  break;
            }
            break;
         default:
            break;
      }
   }
   free(pszCurrField);

   // Add new counter if parsing was successful
   if ((iState == -1) && (iField >= 4) && (iField <= 6))
   {
      pCnt = AddCounter(szCounterPath, iClass, iNumSamples, (iDataType == DCI_DT_FLOAT) ? COUNTER_TYPE_FLOAT : COUNTER_TYPE_AUTO);
      if (pCnt != NULL)
         AddParameter(szParamName, H_CollectedCounterData, (TCHAR *)pCnt,
                      iDataType, szDescription);
   }

   return ((iState == -1) && (iField >= 4) && (iField <= 6));
}

/**
 * Start collector threads
 */
void StartCollectorThreads()
{
   for(int i = 0; i < 3; i++)
		m_cntSet[i]->startCollectorThread();
}
