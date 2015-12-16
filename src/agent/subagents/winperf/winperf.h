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
** $module: winperf.h
**
**/

#ifndef _winperf_h_
#define _winperf_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <pdh.h>
#include <pdhmsg.h>

#ifdef _DEBUG
#define DEBUG_SUFFIX    "-debug"
#else
#define DEBUG_SUFFIX    ""
#endif


//
// Counter types
//

#define COUNTER_TYPE_AUTO     0
#define COUNTER_TYPE_INT32    1
#define COUNTER_TYPE_INT64    2
#define COUNTER_TYPE_FLOAT    3

/**
 * Counter structure
 */
struct WINPERF_COUNTER
{
   TCHAR *pszName;
   WORD  wType;
   WORD  wNumSamples;
   union
   {
      LONG iLong;
      INT64 iLarge;
      double dFloat;
   } value;
   DWORD dwFormat;   // Format code for PDH functions
   PDH_RAW_COUNTER *pRawValues;
   DWORD dwBufferPos;
   HCOUNTER handle;
};

/**
 * Counter set
 */
class WinPerfCounterSet
{
private:
   DWORD m_interval;    // Interval beetween samples in milliseconds
	ObjectArray<WINPERF_COUNTER> *m_counters;
   TCHAR m_class;
	MUTEX m_mutex;
	CONDITION m_changeCondition;
   THREAD m_collectorThread;

	void collectorThread();
	static THREAD_RESULT THREAD_CALL collectorThreadStarter(void *);

public:
	WinPerfCounterSet(DWORD interval, TCHAR cls);
	~WinPerfCounterSet();

	void addCounter(WINPERF_COUNTER *c);

	void startCollectorThread();
   void joinCollectorThread();
};

/**
 * Counter index structure
 */
struct COUNTER_INDEX
{
	DWORD index;
	TCHAR *englishName;
	TCHAR *localName;
};

/**
 * Functions
 */
void CreateCounterIndex(TCHAR *engilshCounters, TCHAR *localCounters);
BOOL TranslateCounterName(const TCHAR *pszName, TCHAR *pszOut);
int CheckCounter(const TCHAR *pszName, TCHAR **ppszNewName);
void StartCollectorThreads();
void JoinCollectorThreads();
TCHAR *GetPdhErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufferSize);
void ReportPdhError(TCHAR *pszFunction, TCHAR *pszPdhCall, PDH_STATUS dwError);
WINPERF_COUNTER *AddCounter(TCHAR *pszName, int iClass, int iNumSamples, int iDataType);
BOOL AddCounterFromConfig(TCHAR *pszStr);
BOOL AddParameter(TCHAR *pszName, LONG (* fpHandler)(const TCHAR *, const TCHAR *, TCHAR *, AbstractCommSession *),
                  TCHAR *pArg, int iDataType, TCHAR *pszDescription);
LONG H_CollectedCounterData(const TCHAR *pszParam, const TCHAR *pArg, TCHAR *pValue, AbstractCommSession *session);

#endif
