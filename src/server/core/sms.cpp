/* 
** NetXMS - Network Management System
** Copyright (C) 2003,2004,2005 Victor Kirhenshtein
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
** $module: sms.cpp
**
**/

#include "nxcore.h"


//
// SMS structure
//

typedef struct
{
   char szRcpt[MAX_RCPT_ADDR_LEN];
   char szText[160];
} SMS;


//
// Static data
//

static Queue *m_pMsgQueue = NULL;
static BOOL (* m_DrvSendMsg)(TCHAR *, TCHAR *);
static void (* m_DrvUnload)(void);
static THREAD m_hThread = INVALID_THREAD_HANDLE;


//
// Sender thread
//

static THREAD_RESULT THREAD_CALL SenderThread(void *pArg)
{
   SMS *pMsg;

   while(1)
   {
      pMsg = (SMS *)m_pMsgQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)
         break;

      if (!m_DrvSendMsg(pMsg->szRcpt, pMsg->szText))
         PostEvent(EVENT_SMS_FAILURE, g_dwMgmtNode, "s", pMsg->szRcpt);

      free(pMsg);
   }
   return THREAD_OK;
}


//
// Initialize SMS subsystem
//

void InitSMSSender(void)
{
   TCHAR szDriver[MAX_PATH], szDrvConfig[MAX_PATH];

   ConfigReadStr(_T("SMSDriver"), szDriver, MAX_PATH, _T("<none>"));
   ConfigReadStr(_T("SMSDrvConfig"), szDrvConfig, MAX_DB_STRING, _T(""));
   if (_tcsicmp(szDriver, _T("<none>")))
   {
      TCHAR szErrorText[256];
      HMODULE hModule;

      hModule = DLOpen(szDriver, szErrorText);
      if (hModule != NULL)
      {
         BOOL (* DrvInit)(TCHAR *);

         DrvInit = (BOOL (*)(TCHAR *))DLGetSymbolAddr(hModule, "SMSDriverInit", szErrorText);
         m_DrvSendMsg = (BOOL (*)(TCHAR *, TCHAR *))DLGetSymbolAddr(hModule, "SMSDriverSend", szErrorText);
         m_DrvUnload = (void (*)(void))DLGetSymbolAddr(hModule, "SMSDriverUnload", szErrorText);
         if ((DrvInit != NULL) && (m_DrvSendMsg != NULL) && (m_DrvUnload != NULL))
         {
            if (DrvInit(szDrvConfig))
            {
               m_pMsgQueue = new Queue;

               m_hThread = ThreadCreateEx(SenderThread, 0, NULL);
            }
            else
            {
               WriteLog(MSG_SMSDRV_INIT_FAILED, EVENTLOG_ERROR_TYPE, "s", szDriver);
               DLClose(hModule);
            }
         }
         else
         {
            WriteLog(MSG_SMSDRV_NO_ENTRY_POINTS, EVENTLOG_ERROR_TYPE, "s", szDriver);
            DLClose(hModule);
         }
      }
      else
      {
         WriteLog(MSG_DLOPEN_FAILED, EVENTLOG_ERROR_TYPE, 
                  _T("ss"), szDriver, szErrorText);
      }
   }
}


//
// Shutdown SMS sender
//

void ShutdownSMSSender(void)
{
   if (m_pMsgQueue != NULL)
   {
      m_pMsgQueue->Clear();
      m_pMsgQueue->Put(INVALID_POINTER_VALUE);
      if (m_hThread != INVALID_THREAD_HANDLE)
         ThreadJoin(m_hThread);
      delete m_pMsgQueue;
   }
}


//
// Post SMS to queue
//

void NXCORE_EXPORTABLE PostSMS(TCHAR *pszRcpt, TCHAR *pszText)
{
   SMS *pMsg;

	if (m_pMsgQueue != NULL)
	{
		pMsg = (SMS *)malloc(sizeof(SMS));
		_tcsncpy(pMsg->szRcpt, pszRcpt, MAX_RCPT_ADDR_LEN);
		_tcsncpy(pMsg->szText, pszText, 160);
		m_pMsgQueue->Put(pMsg);
	}
}
