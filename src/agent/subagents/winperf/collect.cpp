/*
** Windows Performance NetXMS subagent
** Copyright (C) 2004-2020 Victor Kirhenshtein
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
WinPerfCounterSet::WinPerfCounterSet(DWORD interval, TCHAR cls) : m_mutex(MutexType::FAST)
{
	m_changeCondition = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_interval = interval;
	m_class = cls;
	m_counters = new ObjectArray<WINPERF_COUNTER>(32, 32, Ownership::True);
   m_collectorThread = INVALID_THREAD_HANDLE;
}

/**
 * Counter set destructor
 */
WinPerfCounterSet::~WinPerfCounterSet()
{
	CloseHandle(m_changeCondition);
	delete m_counters;
}

/**
 * Add counter to set
 */
void WinPerfCounterSet::addCounter(WINPERF_COUNTER *c)
{
	m_mutex.lock();
	m_counters->add(c);
	m_mutex.unlock();
	SetEvent(m_changeCondition);
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
	while(true)
	{
		nxlog_debug_tag(WINPERF_DEBUG_TAG, 2, _T("%s waiting for set change"), szFName);

		HANDLE handles[2];
      handles[0] = g_winperfShutdownCondition;
		handles[1] = m_changeCondition;
      DWORD waitStatus = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
		if (waitStatus == WAIT_OBJECT_0)
         break;   // Shutdown condition
		if (waitStatus != WAIT_OBJECT_0 + 1)
			continue;	// set not changed, continue waiting

		ResetEvent(m_changeCondition);

		nxlog_debug_tag(WINPERF_DEBUG_TAG, 2, _T("%s: set changed"), szFName);
		if ((rc = PdhOpenQuery(NULL, 0, &hQuery)) != ERROR_SUCCESS)
		{
			ReportPdhError(szFName, _T("PdhOpenQuery"), nullptr, rc);
			continue;
		}

		// Add counters to query
	   DWORD dwOKCounters = 0;
		m_mutex.lock();
		for(int i = 0; i < m_counters->size(); i++)
		{
			WINPERF_COUNTER *counter = m_counters->get(i);
			if ((rc = PdhAddCounter(hQuery, counter->pszName, 0, &counter->handle)) != ERROR_SUCCESS)
			{
				TCHAR szBuffer[1024];
				nxlog_write_tag(NXLOG_WARNING, WINPERF_DEBUG_TAG, _T("%s: Unable to add counter \"%s\" to query (%s)"), 
                  szFName, counter->pszName, GetPdhErrorText(rc, szBuffer, 1024));
				counter->handle = nullptr;
			}
			else
			{
            nxlog_debug_tag(WINPERF_DEBUG_TAG, 4, _T("%s: Counter \"%s\" added to query"), szFName, counter->pszName);
				dwOKCounters++;
			}
		}
		m_mutex.unlock();

      // Check if we was able to add at least one counter
      if (dwOKCounters == 0)
      {
			PdhCloseQuery(hQuery);
			nxlog_write_tag(NXLOG_WARNING, WINPERF_DEBUG_TAG, _T("Failed to add any counter to query for counter set %c"), m_class);
			continue;
		}

		// Collection loop (until shutdown or set change)
		nxlog_debug_tag(WINPERF_DEBUG_TAG, 2, _T("%s entered data collection loop"), szFName);
		while(true)
		{
			HANDLE handles[2];
         handles[0] = g_winperfShutdownCondition;
			handles[1] = m_changeCondition;
			waitStatus = WaitForMultipleObjects(2, handles, FALSE, m_interval);
			if (waitStatus == WAIT_OBJECT_0)
				goto stop;	// shutdown
			if (waitStatus == WAIT_OBJECT_0 + 1)
				break;   // Set change, break collection loop

			// Collect data
			if ((rc = PdhCollectQueryData(hQuery)) != ERROR_SUCCESS)
				ReportPdhError(szFName, _T("PdhCollectQueryData"), nullptr, rc);

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
						ReportPdhError(szFName, _T("PdhComputeCounterStatistics"), counter->pszName, rc);
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

		nxlog_debug_tag(WINPERF_DEBUG_TAG, 2, _T("%s: data collection stopped"), szFName);
		PdhCloseQuery(hQuery);
	}

stop:
   nxlog_write_tag(NXLOG_INFO, WINPERF_DEBUG_TAG, _T("Collector thread for counter set %c terminated"), m_class);
}

/**
 * Start collector thread
 */
void WinPerfCounterSet::startCollectorThread()
{
	m_collectorThread = ThreadCreateEx(this, &WinPerfCounterSet::collectorThread);
}

/**
 * Join collector thread
 */
void WinPerfCounterSet::joinCollectorThread()
{
   ThreadJoin(m_collectorThread);
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

	*ppszNewName = nullptr;

   if ((rc = PdhOpenQuery(nullptr, 0, &hQuery)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhOpenQuery"), nullptr, rc);
      return -1;
   }

	rc = PdhAddCounter(hQuery, pszName, 0, &hCounter);
   if (rc != ERROR_SUCCESS)
   {
		// Attempt to translate counter name
		if ((rc == PDH_CSTATUS_NO_COUNTER) || (rc == PDH_CSTATUS_NO_OBJECT))
		{
			*ppszNewName = MemAllocString(_tcslen(pszName) * 4);
			if (TranslateCounterName(pszName, *ppszNewName))
			{
				nxlog_debug_tag(WINPERF_DEBUG_TAG, 2, _T("Counter translated: %s ==> %s"), pszName, *ppszNewName);
				rc = PdhAddCounter(hQuery, *ppszNewName, 0, &hCounter);
				if (rc != ERROR_SUCCESS)
				{
					MemFree(*ppszNewName);
					*ppszNewName = nullptr;
				}
			}
		}
	   if (rc != ERROR_SUCCESS)
		{
			ReportPdhError(szFName, _T("PdhAddCounter"), pszName, rc);
			PdhCloseQuery(hQuery);
			return -1;
		}
   }

   dwSize = sizeof(ci);
   if ((rc = PdhGetCounterInfo(hCounter, FALSE, &dwSize, &ci)) != ERROR_SUCCESS)
   {
      ReportPdhError(szFName, _T("PdhGetCounterInfo"), pszName, rc);
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
		MemFree(pszNewName);
      return NULL;
	}

   // Create new counter
   pCnt = new WINPERF_COUNTER;
   memset(pCnt, 0, sizeof(WINPERF_COUNTER));
	if (pszNewName != NULL)
	{
		pCnt->pszName = (TCHAR *)MemRealloc(pszNewName, (_tcslen(pszNewName) + 1) * sizeof(TCHAR));
	}
	else
	{
		pCnt->pszName = _tcsdup(pszName);
	}
   pCnt->wType = (iDataType == COUNTER_TYPE_AUTO) ? iType : iDataType;
   pCnt->wNumSamples = iNumSamples;
   pCnt->pRawValues = MemAllocArray<PDH_RAW_COUNTER>(iNumSamples);
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
   pszCurrField = (TCHAR *)MemAlloc(sizeof(TCHAR) * (_tcslen(pszStr) + 1));
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
                        _tcslcpy(szParamName, pszCurrField, MAX_PARAM_NAME);
                        break;
                     case 1:  // Counter path
                        _tcslcpy(szCounterPath, pszCurrField, MAX_PATH);
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
                        iDataType = TextToDataType(pszCurrField);
                        if (iDataType == -1)    // Invalid data type specified
                           iState = 255;
                        break;
                     case 5:  // Description
                        _tcslcpy(szDescription, pszCurrField, MAX_DB_STRING);
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

/**
 * Join collector threads
 */
void JoinCollectorThreads()
{
   for(int i = 0; i < 3; i++)
		m_cntSet[i]->joinCollectorThread();
}
