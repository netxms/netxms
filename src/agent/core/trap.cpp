/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: trap.cpp
**
**/

#include "nxagentd.h"
#include <stdarg.h>


//
// Static data
//

static Queue *m_pTrapQueue = NULL;


//
// Trap sender
//

THREAD_RESULT THREAD_CALL TrapSender(void *pArg)
{
   CSCP_MESSAGE *pMsg;
   DWORD i;
   BOOL bTrapSent;

   m_pTrapQueue = new Queue;
   while(1)
   {
      pMsg = (CSCP_MESSAGE *)m_pTrapQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)
         break;

      bTrapSent = FALSE;

      MutexLock(g_hSessionListAccess, INFINITE);
      for(i = 0; i < g_dwMaxSessions; i++)
         if (g_pSessionList[i] != NULL)
            if (g_pSessionList[i]->AcceptTraps())
            {
               g_pSessionList[i]->SendRawMessage(pMsg);
               bTrapSent = TRUE;
            }
      MutexUnlock(g_hSessionListAccess);

      if (bTrapSent)
		{
	      free(pMsg);
		}
		else
		{
         m_pTrapQueue->Insert(pMsg);	// Re-queue trap
			ThreadSleep(1);
		}
   }
   delete m_pTrapQueue;
   m_pTrapQueue = NULL;
	DebugPrintf(INVALID_INDEX, _T("Trap sender thread terminated"));
   return THREAD_OK;
}


//
// Shutdown trap sender
//

void ShutdownTrapSender()
{
	m_pTrapQueue->SetShutdownMode();
}


//
// Send trap to server
//

void SendTrap(DWORD dwEventCode, int iNumArgs, TCHAR **ppArgList)
{
   int i;
   CSCPMessage msg;

   msg.SetCode(CMD_TRAP);
   msg.SetId(0);
   msg.SetVariable(VID_EVENT_CODE, dwEventCode);
   msg.SetVariable(VID_NUM_ARGS, (WORD)iNumArgs);
   for(i = 0; i < iNumArgs; i++)
      msg.SetVariable(VID_EVENT_ARG_BASE + i, ppArgList[i]);
   if (m_pTrapQueue != NULL)
      m_pTrapQueue->Put(msg.CreateMessage());
}


//
// Send trap - variant 2
// Arguments:
// dwEventCode - Event code
// pszFormat   - Parameter format string, each parameter represented by one character.
//    The following format characters can be used:
//        s - String
//        d - Decimal integer
//        x - Hex integer
//        a - IP address
//        i - Object ID
//        D - 64-bit decimal integer
//        X - 64-bit hex integer
//

void SendTrap(DWORD dwEventCode, const char *pszFormat, va_list args)
{
   int i, iNumArgs;
   TCHAR *ppArgList[64];
   static TCHAR badFormat[] = _T("BAD FORMAT");

   iNumArgs = (pszFormat == NULL) ? 0 : (int)strlen(pszFormat);
   for(i = 0; i < iNumArgs; i++)
   {
      switch(pszFormat[i])
      {
         case 's':
            ppArgList[i] = va_arg(args, TCHAR *);
				if (ppArgList[i] == NULL)
					ppArgList[i] = (TCHAR *)_T("");
            break;
         case 'd':
            ppArgList[i] = (char *)malloc(16);
            sprintf(ppArgList[i], "%d", va_arg(args, LONG));
            break;
         case 'D':
            ppArgList[i] = (char *)malloc(32);
            sprintf(ppArgList[i], INT64_FMT, va_arg(args, INT64));
            break;
         case 'x':
         case 'i':
            ppArgList[i] = (char *)malloc(16);
            sprintf(ppArgList[i], "0x%08X", va_arg(args, DWORD));
            break;
         case 'X':
            ppArgList[i] = (char *)malloc(32);
#ifdef _WIN32
            sprintf(ppArgList[i], "0x%016I64X", va_arg(args, QWORD));
#else
            sprintf(ppArgList[i], "0x%016llX", va_arg(args, QWORD));
#endif
            break;
         case 'a':
            ppArgList[i] = (char *)malloc(16);
            IpToStr(va_arg(args, DWORD), ppArgList[i]);
            break;
         default:
            ppArgList[i] = badFormat;
            break;
      }
   }

   SendTrap(dwEventCode, iNumArgs, ppArgList);

   for(i = 0; i < iNumArgs; i++)
      if ((pszFormat[i] == 'd') || (pszFormat[i] == 'x') ||
          (pszFormat[i] == 'D') || (pszFormat[i] == 'X') ||
          (pszFormat[i] == 'i') || (pszFormat[i] == 'a'))
         free(ppArgList[i]);
}


//
// Send trap - variant 3
// Same as variant 2, but uses argument list instead of va_list
//

void SendTrap(DWORD dwEventCode, const char *pszFormat, ...)
{
   va_list args;

   va_start(args, pszFormat);
   SendTrap(dwEventCode, pszFormat, args);
   va_end(args);
}
