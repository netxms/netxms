/* 
** NetXMS - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: email.cpp
**
**/

#include "nxcore.h"


//
// Sender errors
//

#define SMTP_ERR_SUCCESS            0
#define SMTP_ERR_BAD_SERVER_NAME    1
#define SMTP_ERR_COMM_FAILURE       2
#define SMTP_ERR_PROTOCOL_FAILURE   3


//
// Mail sender states
//

#define STATE_INITIAL      0
#define STATE_HELLO        1
#define STATE_FROM         2
#define STATE_RCPT         3
#define STATE_DATA         4
#define STATE_MAIL_BODY    5
#define STATE_QUIT         6
#define STATE_FINISHED     7
#define STATE_ERROR        8


//
// Mail envelope structure
//

typedef struct
{
   char szRcptAddr[MAX_RCPT_ADDR_LEN];
   char szSubject[MAX_EMAIL_SUBJECT_LEN];
   char *pszText;
} MAIL_ENVELOPE;


//
// Static data
//

static char m_szSmtpServer[MAX_PATH] = "localhost";
static WORD m_wSmtpPort = 25;
static char m_szFromAddr[MAX_PATH] = "netxms@localhost";
static Queue *m_pMailerQueue = NULL;


//
// Receive bytes from network or fail on timeout
//

int RecvWithTimeout(SOCKET hSocket, char *pszBuffer, int iBufSize)
{
   fd_set rdfs;
   struct timeval timeout;

   // Wait for data
   FD_ZERO(&rdfs);
   FD_SET(hSocket, &rdfs);
   timeout.tv_sec = 10;
   timeout.tv_usec = 0;
	if (select(hSocket + 1, &rdfs, NULL, NULL, &timeout) == 0)
      return 0;     // Timeout

   return recv(hSocket, pszBuffer, iBufSize, 0);
}


//
// Send e-mail
//

static DWORD SendMail(char *pszRcpt, char *pszSubject, char *pszText)
{
   SOCKET hSocket;
   struct hostent *hs;
   struct sockaddr_in sa;
   char szBuffer[256];
   int iErr, iState = STATE_INITIAL;
   DWORD dwRetCode;

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = htons(m_wSmtpPort);

   // Resolve hostname
   hs = gethostbyname(m_szSmtpServer);
   if (hs != NULL)
   {
      memcpy(&sa.sin_addr.s_addr, hs->h_addr, sizeof(DWORD));
   }
   else
   {
      sa.sin_addr.s_addr = inet_addr(m_szSmtpServer);
   }

   if ((sa.sin_addr.s_addr == INADDR_ANY) || (sa.sin_addr.s_addr == INADDR_NONE))
      return SMTP_ERR_BAD_SERVER_NAME;

   // Create socket
   hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (hSocket == -1)
      return SMTP_ERR_COMM_FAILURE;

   // Connect to server
   if (connect(hSocket, (struct sockaddr *)&sa, sizeof(sa)) == 0)
   {
      while((iState != STATE_FINISHED) && (iState != STATE_ERROR))
      {
         iErr = RecvWithTimeout(hSocket, szBuffer, 255);
         if (iErr > 0)
         {
            szBuffer[iErr] = 0;
            DbgPrintf(AF_DEBUG_ACTIONS, "SMTP: %s", szBuffer);
         
            switch(iState)
            {
               case STATE_INITIAL:
                  // Server should send 220 text after connect
                  if (!memcmp(szBuffer,"220",3))
                  {
                     iState = STATE_HELLO;
                     SendEx(hSocket, "HELO netxms\r\n", 13, 0);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_HELLO:
                  // Server should respond with 250 text to our HELO command
                  if (!memcmp(szBuffer,"250",3))
                  {
                     iState = STATE_FROM;
                     sprintf(szBuffer, "MAIL FROM: <%s>\r\n", m_szFromAddr);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_FROM:
                  // Server should respond with 250 text to our MAIL FROM command
                  if (!memcmp(szBuffer,"250",3))
                  {
                     iState = STATE_RCPT;
                     sprintf(szBuffer, "RCPT TO: <%s>\r\n", pszRcpt);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_RCPT:
                  // Server should respond with 250 text to our RCPT TO command
                  if (!memcmp(szBuffer,"250",3))
                  {
                     iState = STATE_DATA;
                     SendEx(hSocket, "DATA\r\n", 6, 0);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_DATA:
                  // Server should respond with 354 text to our DATA command
                  if (!memcmp(szBuffer,"354",3))
                  {
                     iState = STATE_MAIL_BODY;

                     // Mail header
                     sprintf(szBuffer, "From: NetXMS Server <%s>\r\n", m_szFromAddr);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0);
                     sprintf(szBuffer, "To: <%s>\r\n", pszRcpt);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0);
                     sprintf(szBuffer, "Subject: <%s>\r\n\r\n", pszSubject);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0);

                     // Mail body
                     SendEx(hSocket, pszText, strlen(pszText), 0);
                     SendEx(hSocket, "\r\n.\r\n", 5, 0);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_MAIL_BODY:
                  // Server should respond with 250 to our mail body
                  if (!memcmp(szBuffer,"250",3))
                  {
                     iState = STATE_QUIT;
                     SendEx(hSocket, "QUIT\r\n", 6, 0);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_QUIT:
                  // Server should respond with 221 text to our QUIT command
                  if (!memcmp(szBuffer,"221",3))
                  {
                     iState = STATE_FINISHED;
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               default:
                  iState = STATE_ERROR;
                  break;
            }
         }
         else
         {
            iState = STATE_ERROR;
         }
      }

      // Shutdown communication channel
      shutdown(hSocket, SHUT_RDWR);
      closesocket(hSocket);

      dwRetCode = (iState == STATE_FINISHED) ? SMTP_ERR_SUCCESS : SMTP_ERR_PROTOCOL_FAILURE;
   }
   else
   {
      dwRetCode = SMTP_ERR_COMM_FAILURE;
   }

   return dwRetCode;
}


//
// Mailer thread
//

static THREAD_RESULT THREAD_CALL MailerThread(void *pArg)
{
   MAIL_ENVELOPE *pEnvelope;
   DWORD dwResult;
   static char *m_szErrorText[] =
   {
      "Sended successfully",
      "Unable to resolve SMTP server name",
      "Communication failure",
      "SMTP conversation failure"
   };

   while(1)
   {
      pEnvelope = (MAIL_ENVELOPE *)m_pMailerQueue->GetOrBlock();
      if (pEnvelope == INVALID_POINTER_VALUE)
         break;

      dwResult = SendMail(pEnvelope->szRcptAddr, pEnvelope->szSubject, pEnvelope->pszText);
      if (dwResult != SMTP_ERR_SUCCESS)
         PostEvent(EVENT_SMTP_FAILURE, g_dwMgmtNode, "dsss", dwResult, 
                   m_szErrorText[dwResult], pEnvelope->szRcptAddr, pEnvelope->szSubject);

      free(pEnvelope->pszText);
      free(pEnvelope);
   }
   return THREAD_OK;
}


//
// Initialize mailer subsystem
//

void InitMailer(void)
{
   ConfigReadStr("SMTPServer", m_szSmtpServer, MAX_PATH, "localhost");
   ConfigReadStr("SMTPFromAddr", m_szFromAddr, MAX_PATH, "netxms@localhost");
   m_wSmtpPort = (WORD)ConfigReadInt("SMTPPort", 25);

   m_pMailerQueue = new Queue;

   ThreadCreate(MailerThread, 0, NULL);
}


//
// Shutdown mailer
//

void ShutdownMailer(void)
{
   m_pMailerQueue->Clear();
   m_pMailerQueue->Put(INVALID_POINTER_VALUE);
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
