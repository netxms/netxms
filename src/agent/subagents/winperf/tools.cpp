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
** File: tools.cpp
**
**/

#include "winperf.h"

/**
 * Max length of counter element
 */
#define MAX_ELEMENT_LENGTH		1024

/**
 * List of configured counters
 */
static StructArray<COUNTER_INDEX> m_counterIndexes;

/**
 * Get error text for PDH functions
 */
TCHAR *GetPdhErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufferSize)
{
   TCHAR *pszMsg;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_HMODULE | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     GetModuleHandle(_T("PDH.DLL")), dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR)&pszMsg, 0, NULL)>0)
   {
      TranslateStr(pszMsg, _T("\r\n"), _T(""));
      nx_strncpy(pszBuffer, pszMsg, iBufferSize);
      LocalFree(pszMsg);
   }
   else
   {
      GetSystemErrorText(dwError, pszBuffer, iBufferSize);
   }
   return pszBuffer;
}

/**
 * Report PDH error to master agent's log
 */
void ReportPdhError(TCHAR *pszFunction, TCHAR *pszPdhCall, PDH_STATUS dwError)
{
   if (dwError == PDH_NO_DATA)
      return;

   TCHAR szBuffer[1024];

   AgentWriteLog(EVENTLOG_WARNING_TYPE, _T("%s: PDH Error %08X in call to %s (%s)"), 
                   pszFunction, dwError, pszPdhCall, GetPdhErrorText(dwError, szBuffer, 1024));
}

/**
 * Create index of counters
 */
void CreateCounterIndex(TCHAR *englishCounters, TCHAR *localCounters)
{
	for(TCHAR *curr = englishCounters; *curr != 0; )
	{
      COUNTER_INDEX ci;
		ci.index = _tcstoul(curr, NULL, 10);
		curr += _tcslen(curr) + 1;
		ci.englishName = _tcsdup(curr);
		curr += _tcslen(curr) + 1;
      ci.localName = NULL;
      m_counterIndexes.add(&ci);
	}
   AgentWriteDebugLog(2, _T("WinPerf: %d counter indexes read"), m_counterIndexes.size());

   int translations = 0;
	for(TCHAR *curr = localCounters; *curr != 0; )
	{
		DWORD index = _tcstoul(curr, NULL, 10);
		curr += _tcslen(curr) + 1;
      for(int i = 0; i < m_counterIndexes.size(); i++)
      {
         COUNTER_INDEX *ci = m_counterIndexes.get(i);
         if (ci->index == index)
         {
            ci->localName = _tcsdup(curr);
            translations++;
            break;
         }
      }
		curr += _tcslen(curr) + 1;
	}
   AgentWriteDebugLog(2, _T("WinPerf: %d counter translations read"), translations);

}

/**
 * Translate single counter name's element
 */
static bool TranslateElement(TCHAR *name)
{
   for(int i = 0; i < m_counterIndexes.size(); i++)
	{
      COUNTER_INDEX *ci = m_counterIndexes.get(i);
      if (!_tcsicmp(ci->englishName, name))
		{
         if (ci->localName != NULL)
         {
            _tcscpy(name, ci->localName);
            return true;
         }
			
         DWORD size = MAX_ELEMENT_LENGTH * sizeof(TCHAR);
			return PdhLookupPerfNameByIndex(NULL, ci->index, name, &size) == ERROR_SUCCESS;
		}
	}
	return false;
}

/**
 * Translate counter name from English to localized
 */
BOOL TranslateCounterName(const TCHAR *pszName, TCHAR *pszOut)
{
	const TCHAR *pCurr = pszName;
	const TCHAR *pSlash, *pBrace, *pNext;
	TCHAR szTemp[MAX_ELEMENT_LENGTH];
	bool bs1, bs2;
	int nLen;

	// Generic counter name looks like following:
	// \\machine\object(parent/instance#index)\counter
	// where machine, parent, instance, and index parts may be omited.
	// We should translate object and counter parts if possible.

	if (*pCurr != _T('\\'))
		return FALSE;	// Counter name should always start with "\"  or "\\"
	pCurr++;

	// Skip machine name
	if (*pCurr == _T('\\'))
	{
		pCurr++;
		pCurr = _tcschr(pCurr, _T('\\'));
		if (pCurr == NULL)
			return FALSE;	// Object name missing
		pCurr++;
	}
	memcpy(pszOut, pszName, (pCurr - pszName) * sizeof(TCHAR));
	pszOut[pCurr - pszName] = 0;

	// Object name ends by \ or (
	pSlash = _tcschr(pCurr, _T('\\'));
	pBrace = _tcschr(pCurr, _T('('));
	if (pSlash == NULL)
		return FALSE;
	if ((pSlash < pBrace) || (pBrace == NULL))
	{
		if (pSlash - pCurr >= MAX_ELEMENT_LENGTH)
			return FALSE;
		memcpy(szTemp, pCurr, (pSlash - pCurr) * sizeof(TCHAR));
		szTemp[pSlash - pCurr] = 0;
		pCurr = pSlash;
		pNext = pSlash + 1;
	}
	else
	{
		if (pBrace - pCurr >= MAX_ELEMENT_LENGTH)
			return FALSE;
		memcpy(szTemp, pCurr, (pBrace - pCurr) * sizeof(TCHAR));
		szTemp[pBrace - pCurr] = 0;
		pCurr = pBrace;
		pNext = _tcschr(pCurr, _T(')'));
		if (pNext == NULL)
			return FALSE;
		pNext++;
		if (*pNext != _T('\\'))
			return FALSE;
		pNext++;
	}
	bs1 = TranslateElement(szTemp);
	_tcscat(pszOut, szTemp);
	nLen = (int)_tcslen(pszOut);
	memcpy(&pszOut[nLen], pCurr, (pNext - pCurr) * sizeof(TCHAR));
	pszOut[nLen + (pNext - pCurr)] = 0;
	pCurr = pNext;

	// Translate counter name
	nx_strncpy(szTemp, pCurr, MAX_ELEMENT_LENGTH);
	bs2 = TranslateElement(szTemp);
	_tcscat(pszOut, szTemp);

	return bs1 || bs2;
}
