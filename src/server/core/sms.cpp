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
// Static data
//

static Queue *m_pMsgQueue;


//
// Initialize SMS subsystem
//

void InitSMSSender(void)
{
   TCHAR szDriver[MAX_PATH], szDrvConfig[MAX_PATH];

   ConfigReadStr(_T("SMSDriver"), szDriver, MAX_PATH, _T("none"));
   ConfigReadStr(_T("SMSDrvConfig"), szDrvConfig, MAX_DB_STRING, _T(""));
   if (_tcsicmp(szDriver, _T("none")))
   {
      TCHAR szErrorText[256];
      HMODULE hModule;

      hModule = DLOpen(szDriver, szErrorText);
      if (hModule != NULL)
      {
         BOOL (* ModuleInit)(NXMODULE *);

         ModuleInit = (BOOL (*)(NXMODULE *))DLGetSymbolAddr(hModule, "NetXMSModuleInit", szErrorText);
         if (ModuleInit != NULL)
         {
            m_pMsgQueue = new Queue;

            ThreadCreate(SenderThread, 0, NULL);
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
// Shutdown mailer
//

void ShutdownSMSSender(void)
{
   m_pMsgQueue->Clear();
   m_pMsgQueue->Put(INVALID_POINTER_VALUE);
}


//
// Post e-mail to queue
//

void NXCORE_EXPORTABLE PostMail(char *pszRcpt, char *pszSubject, char *pszText)
{
   MAIL_ENVELOPE *pEnvelope;

   pEnvelope = (MAIL_ENVELOPE *)malloc(sizeof(MAIL_ENVELOPE));
   strncpy(pEnvelope->szRcptAddr, pszRcpt, MAX_RCPT_ADDR_LEN);
   strncpy(pEnvelope->szSubject, pszSubject, MAX_EMAIL_SUBJECT_LEN);
   pEnvelope->pszText = strdup(pszText);
   m_pMailerQueue->Put(pEnvelope);
}
