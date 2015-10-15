/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Static data
 */
static Queue *s_trapQueue = NULL;
static QWORD s_genTrapCount = 0;	// Number of generated traps
static QWORD s_sentTrapCount = 0;	// Number of sent traps
static QWORD s_trapId = 0;
static time_t s_lastTrapTime = 0;

/**
 * Trap sender
 */
THREAD_RESULT THREAD_CALL TrapSender(void *pArg)
{
   NXCP_MESSAGE *pMsg;
   UINT32 i;
   bool trapSent;

   s_trapQueue = new Queue;
	s_trapId = (QWORD)time(NULL) << 32;
   while(1)
   {
      pMsg = (NXCP_MESSAGE *)s_trapQueue->getOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)
         break;

      trapSent = false;

      if (g_dwFlags & AF_SUBAGENT_LOADER)
      {
         trapSent = SendRawMessageToMasterAgent(pMsg);
      }
      else
      {
         MutexLock(g_hSessionListAccess);
         for(i = 0; i < g_dwMaxSessions; i++)
            if (g_pSessionList[i] != NULL)
               if (g_pSessionList[i]->canAcceptTraps())
               {
                  g_pSessionList[i]->sendRawMessage(pMsg);
                  trapSent = true;
               }
         MutexUnlock(g_hSessionListAccess);
      }

      if (trapSent)
		{
	      free(pMsg);
			s_sentTrapCount++;
		}
		else
		{
         s_trapQueue->insert(pMsg);	// Re-queue trap
			ThreadSleep(1);
		}
   }
   delete s_trapQueue;
   s_trapQueue = NULL;
	DebugPrintf(INVALID_INDEX, 1, _T("Trap sender thread terminated"));
   return THREAD_OK;
}

/**
 * Shutdown trap sender
 */
void ShutdownTrapSender()
{
	s_trapQueue->setShutdownMode();
}

/**
 * Send trap to server
 */
void SendTrap(UINT32 dwEventCode, const TCHAR *eventName, int iNumArgs, TCHAR **ppArgList)
{
   int i;
   NXCPMessage msg;

	DebugPrintf(INVALID_INDEX, 5, _T("SendTrap(): event_code=%d, event_name=%s, num_args=%d, arg[0]=\"%s\" arg[1]=\"%s\" arg[2]=\"%s\""),
	            dwEventCode, CHECK_NULL(eventName), iNumArgs, 
					(iNumArgs > 0) ? ppArgList[0] : _T("(null)"),
					(iNumArgs > 1) ? ppArgList[1] : _T("(null)"),
					(iNumArgs > 2) ? ppArgList[2] : _T("(null)"));

   msg.setCode(CMD_TRAP);
   msg.setId(0);
	msg.setField(VID_TRAP_ID, s_trapId++);
   msg.setField(VID_EVENT_CODE, dwEventCode);
	if (eventName != NULL)
		msg.setField(VID_EVENT_NAME, eventName);
   msg.setField(VID_NUM_ARGS, (WORD)iNumArgs);
   for(i = 0; i < iNumArgs; i++)
      msg.setField(VID_EVENT_ARG_BASE + i, ppArgList[i]);
   if (s_trapQueue != NULL)
	{
		s_genTrapCount++;
		s_lastTrapTime = time(NULL);
      s_trapQueue->put(msg.createMessage());
	}
}

/**
 * Send trap - variant 2
 * Arguments:
 * dwEventCode - Event code
 * eventName   - event name; to send event by name, eventCode must be set to 0
 * pszFormat   - Parameter format string, each parameter represented by one character.
 *    The following format characters can be used:
 *        s - String
 *        d - Decimal integer
 *        x - Hex integer
 *        a - IP address
 *        i - Object ID
 *        D - 64-bit decimal integer
 *        X - 64-bit hex integer
 */
void SendTrap(UINT32 dwEventCode, const TCHAR *eventName, const char *pszFormat, va_list args)
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
            ppArgList[i] = (TCHAR *)malloc(16 * sizeof(TCHAR));   //
            _sntprintf(ppArgList[i], 16, _T("%d"), va_arg(args, LONG)); //
            break;
         case 'D':
            ppArgList[i] = (TCHAR *)malloc(32 * sizeof(TCHAR)); //
            _sntprintf(ppArgList[i], 32, INT64_FMT, va_arg(args, INT64)); //
            break;
         case 'x':
         case 'i':
            ppArgList[i] = (TCHAR *)malloc(16 * sizeof(TCHAR));  //
            _sntprintf(ppArgList[i], 16, _T("0x%08X"), va_arg(args, UINT32));  //
            break;
         case 'X':
            ppArgList[i] = (TCHAR *)malloc(32 * sizeof(TCHAR));
            _sntprintf(ppArgList[i], 32, UINT64X_FMT(_T("016")), va_arg(args, QWORD));
            break;
         case 'a':
            ppArgList[i] = (TCHAR *)malloc(16 * sizeof(TCHAR));
            IpToStr(va_arg(args, UINT32), ppArgList[i]);
            break;
         default:
            ppArgList[i] = badFormat;
            break;
      }
   }

   SendTrap(dwEventCode, eventName, iNumArgs, ppArgList);

   for(i = 0; i < iNumArgs; i++)
      if ((pszFormat[i] == 'd') || (pszFormat[i] == 'x') ||
          (pszFormat[i] == 'D') || (pszFormat[i] == 'X') ||
          (pszFormat[i] == 'i') || (pszFormat[i] == 'a'))
         free(ppArgList[i]);

}

/**
 * Send trap - variant 3
 * Same as variant 2, but uses argument list instead of va_list
 */
void SendTrap(UINT32 dwEventCode, const TCHAR *eventName, const char *pszFormat, ...)
{
   va_list args;

   va_start(args, pszFormat);
   SendTrap(dwEventCode, eventName, pszFormat, args);
   va_end(args);

}

/**
 * Forward trap from external subagent to server
 */
void ForwardTrap(NXCPMessage *msg)
{
	msg->setField(VID_TRAP_ID, s_trapId++);
   if (s_trapQueue != NULL)
	{
		s_genTrapCount++;
		s_lastTrapTime = time(NULL);
      s_trapQueue->put(msg->createMessage());
	}
}

/**
 * Handler for trap statistic DCIs
 */
LONG H_AgentTraps(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	switch(arg[0])
	{
		case 'G':
			ret_uint64(value, s_genTrapCount);
			break;
		case 'S':
			ret_uint64(value, s_sentTrapCount);
			break;
		case 'T':
			ret_uint64(value, (QWORD)s_lastTrapTime);
			break;
		default:
			return SYSINFO_RC_UNSUPPORTED;
	}
	return SYSINFO_RC_SUCCESS;
}
