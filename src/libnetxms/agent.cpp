/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: agent.cpp
**
**/

#include "libnetxms.h"


//
// Static data
//

static void (* s_fpWriteLog)(int, const TCHAR *) = NULL;
static void (* s_fpSendTrap1)(DWORD, const char *, va_list) = NULL;
static void (* s_fpSendTrap2)(DWORD, int, TCHAR **) = NULL;
static BOOL (* s_fpSendFile)(void *, DWORD, const char *, long) = NULL;


//
// Initialize subagent API
//

void LIBNETXMS_EXPORTABLE InitSubAgentAPI(void (* writeLog)(int, const TCHAR *),
														void (* sendTrap1)(DWORD, const char *, va_list),
														void (* sendTrap2)(DWORD, int, TCHAR **),
														BOOL (* sendFile)(void *, DWORD, const char *, long))
{
   s_fpWriteLog = writeLog;
	s_fpSendTrap1 = sendTrap1;
	s_fpSendTrap2 = sendTrap2;
	s_fpSendFile = sendFile;
}


//
// Write message to agent's log
//

void LIBNETXMS_EXPORTABLE AgentWriteLog(int iLevel, const TCHAR *pszFormat, ...)
{
   TCHAR szBuffer[4096];
   va_list args;

   if (s_fpWriteLog != NULL)
   {
      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 4096, pszFormat, args);
      va_end(args);
      s_fpWriteLog(iLevel, szBuffer);
   }
}

void LIBNETXMS_EXPORTABLE AgentWriteLog2(int iLevel, const TCHAR *pszFormat, va_list args)
{
   TCHAR szBuffer[4096];

   if (s_fpWriteLog != NULL)
   {
      _vsntprintf(szBuffer, 4096, pszFormat, args);
      s_fpWriteLog(iLevel, szBuffer);
   }
}


//
// Send trap from agent to server
//

void LIBNETXMS_EXPORTABLE AgentSendTrap(DWORD dwEvent, const char *pszFormat, ...)
{
   va_list args;

   if (s_fpSendTrap1 != NULL)
   {
      va_start(args, pszFormat);
      s_fpSendTrap1(dwEvent, pszFormat, args);
      va_end(args);
   }
}

void LIBNETXMS_EXPORTABLE AgentSendTrap2(DWORD dwEvent, int nCount, TCHAR **ppszArgList)
{
   if (s_fpSendTrap2 != NULL)
      s_fpSendTrap2(dwEvent, nCount, ppszArgList);
}


//
// Get arguments for parameters like name(arg1,...)
// Returns FALSE on processing error
//

BOOL LIBNETXMS_EXPORTABLE AgentGetParameterArg(const TCHAR *param, int index, TCHAR *arg, int maxSize)
{
   const TCHAR *ptr1, *ptr2;
   int state, currIndex, pos;
   BOOL bResult = TRUE;

   arg[0] = 0;    // Default is empty string
   ptr1 = _tcschr(param, _T('('));
   if (ptr1 == NULL)
      return TRUE;  // No arguments at all
   for(ptr2 = ptr1 + 1, currIndex = 1, state = 0, pos = 0; state != -1; ptr2++)
   {
      switch(state)
      {
         case 0:  // Normal
            switch(*ptr2)
            {
               case _T(')'):
                  if (currIndex == index)
                     arg[pos] = 0;
                  state = -1;    // Finish processing
                  break;
               case _T('"'):
                  state = 1;     // String
                  break;
               case _T('\''):        // String, type 2
                  state = 2;
                  break;
               case _T(','):
                  if (currIndex == index)
                  {
                     arg[pos] = 0;
                     state = -1;
                  }
                  else
                  {
                     currIndex++;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 1:  // String in ""
            switch(*ptr2)
            {
               case _T('"'):
                  state = 0;     // Normal
                  break;
               case 0:
                  state = -1;    // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 2:  // String in ''
            switch(*ptr2)
            {
               case _T('\''):
                  state = 0;     // Normal
                  break;
               case 0:
                  state = -1;    // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
      }
   }

   if (bResult)
      StrStrip(arg);
   return bResult;
}


//
// Send file to server
//

BOOL LIBNETXMS_EXPORTABLE AgentSendFileToServer(void *session, DWORD requestId, const TCHAR *file, long offset)
{
	if ((s_fpSendFile == NULL) || (session == NULL) || (file == NULL))
		return FALSE;
	return s_fpSendFile(session, requestId, file, offset);
}
