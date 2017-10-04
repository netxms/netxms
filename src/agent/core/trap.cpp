/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
static UINT64 s_genTrapCount = 0;	// Number of generated traps
static UINT64 s_sentTrapCount = 0;	// Number of sent traps
static UINT64 s_trapIdBase = 0;
static VolatileCounter s_trapIdCounter = 0;
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
	s_trapIdBase = (QWORD)time(NULL) << 32;
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
	DebugPrintf(1, _T("Trap sender thread terminated"));
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
   if (nxlog_get_debug_level() >= 5)
   {
      String argsText;
      for(int i = 0; i < iNumArgs; i++)
      {
         argsText.append(_T(", arg["));
         argsText.append(i);
         argsText.append(_T("]=\""));
         argsText.append(CHECK_NULL(ppArgList[i]));
         argsText.append(_T('"'));
      }
      nxlog_debug(5, _T("SendTrap(): event_code=%d, event_name=%s, num_args=%d%s"),
                  dwEventCode, CHECK_NULL(eventName), iNumArgs, (const TCHAR *)argsText);
   }

   NXCPMessage msg(CMD_TRAP, 0);
	msg.setField(VID_TRAP_ID, s_trapIdBase | (UINT64)InterlockedIncrement(&s_trapIdCounter));
   msg.setField(VID_EVENT_CODE, dwEventCode);
	if (eventName != NULL)
		msg.setField(VID_EVENT_NAME, eventName);
   msg.setField(VID_NUM_ARGS, (WORD)iNumArgs);
   for(int i = 0; i < iNumArgs; i++)
      msg.setField(VID_EVENT_ARG_BASE + i, ppArgList[i]);
   if (s_trapQueue != NULL)
	{
		s_genTrapCount++;
		s_lastTrapTime = time(NULL);
      s_trapQueue->put(msg.serialize());
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
 *        m - Multibyte (non-UNICODE) string
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
            break;
         case 'm':
#ifdef UNICODE
            ppArgList[i] = WideStringFromMBString(va_arg(args, char *));
#else
            ppArgList[i] = va_arg(args, char *);
#endif
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
          (pszFormat[i] == 'i') || (pszFormat[i] == 'a')
#ifdef UNICODE
          || (pszFormat[i] == 'm')
#endif
         )
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
	msg->setField(VID_TRAP_ID, s_trapIdBase | (UINT64)InterlockedIncrement(&s_trapIdCounter));
   if (s_trapQueue != NULL)
	{
		s_genTrapCount++;
		s_lastTrapTime = time(NULL);
      s_trapQueue->put(msg->serialize());
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
