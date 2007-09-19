/* $Id: tools.cpp,v 1.5 2007-09-19 16:57:40 victor Exp $ */
/* 
** NetXMS - Network Management System
** HTTP Server
** Copyright (C) 2006, 2007 Alex Kirhenshtein and Victor Kirhenshtein
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

#include "nxhttpd.h"


//
// Print debug messages
//

void DebugPrintf(DWORD dwSessionId, const char *pszFormat, ...)
{
   if (g_dwFlags & AF_DEBUG)
   {
      va_list args;
      char szBuffer[4096];

      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 4096, pszFormat, args);
      va_end(args);
      
      if (dwSessionId != INVALID_INDEX)
         WriteLog(MSG_DEBUG_SESSION, EVENTLOG_INFORMATION_TYPE, "ds", dwSessionId, szBuffer);
      else
         WriteLog(MSG_DEBUG, EVENTLOG_INFORMATION_TYPE, "s", szBuffer);
   }
}


//
// Translate given code to text
//

const TCHAR *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText)
{
   int i;

   for(i = 0; pTranslator[i].pszText != NULL; i++)
      if (pTranslator[i].iCode == iCode)
         return pTranslator[i].pszText;
   return pszDefaultText;
}


//
// Format time stamp
//

TCHAR *FormatTimeStamp(DWORD dwTimeStamp, TCHAR *pszBuffer, int iType)
{
   struct tm *pTime;
   static const TCHAR *pFormat[] = { _T("%d-%b-%Y %H:%M:%S"), _T("%H:%M:%S"), _T("%b/%d"), _T("%b") };

   pTime = localtime((const time_t *)&dwTimeStamp);
	if (pTime != NULL)
		_tcsftime(pszBuffer, 32, pFormat[iType], pTime);
	else
		_tcscpy(pszBuffer, _T("(null)"));
   return pszBuffer;
}


//
// Create ID list from string of form (id1)(id2)..(idN)
//

DWORD *IdListFromString(const TCHAR *pszStr, DWORD *pdwCount)
{
	DWORD i, *pdwList;
	TCHAR *curr, *start;

	*pdwCount = NumChars(pszStr, _T('('));
	if (*pdwCount > 0)
	{
		pdwList = (DWORD *)malloc(sizeof(DWORD) * (*pdwCount));
		memset(pdwList, 0, sizeof(DWORD) * (*pdwCount));
		for(i = 0, start = (TCHAR *)pszStr; i < *pdwCount; i++)
		{
			curr = _tcschr(start, _T('('));
			if (curr == NULL)
				break;
			curr++;
			pdwList[i] = _tcstoul(curr, &start, 10);
		}
	}
	else
	{
		pdwList = NULL;
	}
	return pdwList;
}


//
// Check if given ID is a member of list
//

BOOL IsListMember(DWORD dwId, DWORD dwCount, DWORD *pdwList)
{
	DWORD i;

	for(i = 0; i < dwCount; i++)
		if (pdwList[i] == dwId)
			return TRUE;
	return FALSE;
}
