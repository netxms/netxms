/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
** File: sms.cpp
**
**/

#include "nxcore.h"

/**
 * Server config
 */
extern Config g_serverConfig;

/**
 * SMS structure
 */
typedef struct
{
   TCHAR szRcpt[MAX_RCPT_ADDR_LEN];
   TCHAR *szText;
} SMS;

/**
 * Static data
 */
static Queue s_smsQueue;
static bool (* s_fpDrvSendMsg)(const TCHAR *, const TCHAR *);
static void (* s_fpDrvUnload)();
static THREAD s_senderThread = INVALID_THREAD_HANDLE;

/**
 * Sender thread
 */
static THREAD_RESULT THREAD_CALL SenderThread(void *pArg)
{
   SMS *pMsg;

   while(1)
   {
      pMsg = (SMS *)s_smsQueue.getOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)
         break;

      DbgPrintf(4, _T("SMS sender: rcpt=%s text=\"%s\""), pMsg->szRcpt, pMsg->szText);
		int tries = 3;
		do
		{
			if (s_fpDrvSendMsg(pMsg->szRcpt, pMsg->szText))
				break;
			DbgPrintf(3, _T("Failed to send SMS (will%s retry)"), (tries > 1) ? _T("") : _T(" not"));
		}
		while(--tries > 0);

		if (tries == 0)
		{
			DbgPrintf(3, _T("Failed to send SMS (complete failure)"));
			PostEvent(EVENT_SMS_FAILURE, g_dwMgmtNode, "s", pMsg->szRcpt);
		}
		free(pMsg->szText);
      free(pMsg);
   }
   return THREAD_OK;
}

/**
 * Initialize SMS subsystem
 */
void InitSMSSender()
{
   TCHAR szDriver[MAX_PATH], szDrvConfig[MAX_CONFIG_VALUE];

   ConfigReadStr(_T("SMSDriver"), szDriver, MAX_PATH, _T("<none>"));
   ConfigReadStr(_T("SMSDrvConfig"), szDrvConfig, MAX_CONFIG_VALUE, _T(""));
   if (_tcsicmp(szDriver, _T("<none>")))
   {
      TCHAR szErrorText[256];
      HMODULE hModule;

      hModule = DLOpen(szDriver, szErrorText);
      if (hModule != NULL)
      {
         bool (* DrvInit)(const TCHAR *, Config *);

         DrvInit = (bool (*)(const TCHAR *, Config *))DLGetSymbolAddr(hModule, "SMSDriverInit", szErrorText);
         s_fpDrvSendMsg = (bool (*)(const TCHAR *, const TCHAR *))DLGetSymbolAddr(hModule, "SMSDriverSend", szErrorText);
         s_fpDrvUnload = (void (*)())DLGetSymbolAddr(hModule, "SMSDriverUnload", szErrorText);
         if ((DrvInit != NULL) && (s_fpDrvSendMsg != NULL) && (s_fpDrvUnload != NULL))
         {
            if (DrvInit(szDrvConfig, &g_serverConfig))
            {
               s_senderThread = ThreadCreateEx(SenderThread, 0, NULL);
            }
            else
            {
               nxlog_write(MSG_SMSDRV_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", szDriver);
               DLClose(hModule);
            }
         }
         else
         {
            nxlog_write(MSG_SMSDRV_NO_ENTRY_POINTS, EVENTLOG_ERROR_TYPE, "s", szDriver);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_write(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, "ss", szDriver, szErrorText);
      }
   }
}

/**
 * Shutdown SMS sender
 */
void ShutdownSMSSender()
{
   s_smsQueue.clear();
   s_smsQueue.put(INVALID_POINTER_VALUE);
   if (s_senderThread != INVALID_THREAD_HANDLE)
      ThreadJoin(s_senderThread);
}

/**
 * Post SMS to queue
 */
void NXCORE_EXPORTABLE PostSMS(const TCHAR *pszRcpt, const TCHAR *pszText)
{
	SMS *pMsg = (SMS *)malloc(sizeof(SMS));
	nx_strncpy(pMsg->szRcpt, pszRcpt, MAX_RCPT_ADDR_LEN);
	pMsg->szText = _tcsdup(pszText);
	s_smsQueue.put(pMsg);
}
