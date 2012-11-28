/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: email.cpp
**
**/

#include "nxcore.h"


//
// Receive buffer size
//

#define SMTP_BUFFER_SIZE            1024


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
   int nRetryCount;
} MAIL_ENVELOPE;


//
// Static data
//

static TCHAR m_szSmtpServer[MAX_PATH] = _T("localhost");
static WORD m_wSmtpPort = 25;
static char m_szFromAddr[MAX_PATH] = "netxms@localhost";
static char m_szFromName[MAX_PATH] = "NetXMS Server";
static Queue *m_pMailerQueue = NULL;
static THREAD m_hThread = INVALID_THREAD_HANDLE;


//
// Find end-of-line character
//

static char *FindEOL(char *pszBuffer, int nLen)
{
   int i;

   for(i = 0; i < nLen; i++)
      if (pszBuffer[i] == '\n')
         return &pszBuffer[i];
   return NULL;
}


//
// Read line from socket
//

static BOOL ReadLineFromSocket(SOCKET hSocket, char *pszBuffer, int *pnBufPos, char *pszLine)
{
   char *ptr;
   int nRet;

   do
   {
      ptr = FindEOL(pszBuffer, *pnBufPos);
      if (ptr == NULL)
      {
         nRet = RecvEx(hSocket, &pszBuffer[*pnBufPos], SMTP_BUFFER_SIZE - *pnBufPos, 0, 30000);
         if (nRet <= 0)
            return FALSE;
         *pnBufPos += nRet;
      }
   } while(ptr == NULL);
   *ptr = 0;
   strcpy(pszLine, pszBuffer);
   *pnBufPos -= (int)(ptr - pszBuffer + 1);
   memmove(pszBuffer, ptr + 1, *pnBufPos);
   return TRUE;
}


//
// Read SMTP response code from socket
//

static int GetSMTPResponse(SOCKET hSocket, char *pszBuffer, int *pnBufPos)
{
   char szLine[SMTP_BUFFER_SIZE];

   while(1)
   {
      if (!ReadLineFromSocket(hSocket, pszBuffer, pnBufPos, szLine))
         return -1;
      if (strlen(szLine) < 4)
         return -1;
      if (szLine[3] == ' ')
      {
         szLine[3] = 0;
         break;
      }
   }
   return atoi(szLine);
}


//
// Send e-mail
//

static DWORD SendMail(char *pszRcpt, char *pszSubject, char *pszText)
{
   SOCKET hSocket;
   struct sockaddr_in sa;
   char szBuffer[SMTP_BUFFER_SIZE];
   int iResp, iState = STATE_INITIAL, nBufPos = 0;
   DWORD dwRetCode;
	char szEncoding[128];

	// get mail encoding from DB
	ConfigReadStrA(_T("MailEncoding"), szEncoding, sizeof(szEncoding) / sizeof(TCHAR), "iso-8859-1");
   BOOL encodeSubject = ConfigReadInt(_T("MailBase64Subjects"), 0) != 0;

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = htons(m_wSmtpPort);

   // Resolve hostname
	sa.sin_addr.s_addr = ResolveHostName(m_szSmtpServer);
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
         iResp = GetSMTPResponse(hSocket, szBuffer, &nBufPos);
			DbgPrintf(8, _T("SMTP RESPONSE: %03d (state=%d)"), iResp, iState);
         if (iResp > 0)
         {
            switch(iState)
            {
               case STATE_INITIAL:
                  // Server should send 220 text after connect
                  if (iResp == 220)
                  {
                     iState = STATE_HELLO;
                     SendEx(hSocket, "HELO netxms\r\n", 13, 0, NULL);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_HELLO:
                  // Server should respond with 250 text to our HELO command
                  if (iResp == 250)
                  {
                     iState = STATE_FROM;
                     snprintf(szBuffer, SMTP_BUFFER_SIZE, "MAIL FROM: <%s>\r\n", m_szFromAddr);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_FROM:
                  // Server should respond with 250 text to our MAIL FROM command
                  if (iResp == 250)
                  {
                     iState = STATE_RCPT;
                     snprintf(szBuffer, SMTP_BUFFER_SIZE, "RCPT TO: <%s>\r\n", pszRcpt);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_RCPT:
                  // Server should respond with 250 text to our RCPT TO command
                  if (iResp == 250)
                  {
                     iState = STATE_DATA;
                     SendEx(hSocket, "DATA\r\n", 6, 0, NULL);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_DATA:
                  // Server should respond with 354 text to our DATA command
                  if (iResp == 354)
                  {
                     iState = STATE_MAIL_BODY;

                     // Mail headers
                     // from
                     snprintf(szBuffer, SMTP_BUFFER_SIZE, "From: %s <%s>\r\n", m_szFromName, m_szFromAddr);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);
                     // to
                     snprintf(szBuffer, SMTP_BUFFER_SIZE, "To: <%s>\r\n", pszRcpt);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);
                     // subject
                     if (encodeSubject)
                     {
                        char *encodedSubject = NULL;
                        int encodedSubjectLen = base64_encode_alloc(pszSubject, strlen(pszSubject), &encodedSubject);
                        if (encodedSubject != NULL)
                        {
                           snprintf(szBuffer, SMTP_BUFFER_SIZE, "Subject: =?%s?B?%s?=\r\n", szEncoding, encodedSubject);
                           free(encodedSubject);
                        }
                        else
                        {
                           // fallback
                           snprintf(szBuffer, SMTP_BUFFER_SIZE, "Subject: %s\r\n", pszSubject);
                        }
                     }
                     else
                     {
                        snprintf(szBuffer, SMTP_BUFFER_SIZE, "Subject: %s\r\n", pszSubject);
                     }
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);
                     
							// date
                     time_t currentTime;
							struct tm *pCurrentTM;
                     time(&currentTime);
#ifdef HAVE_LOCALTIME_R
                     struct tm currentTM;
                     localtime_r(&currentTime, &currentTM);
                     pCurrentTM = &currentTM;
#else
                     pCurrentTM = localtime(&currentTime);
#endif
#ifdef _WIN32
                     strftime(szBuffer, sizeof(szBuffer), "Date: %a, %d %b %Y %H:%M:%S ", pCurrentTM);

							TIME_ZONE_INFORMATION tzi;
							DWORD tzType = GetTimeZoneInformation(&tzi);
							LONG effectiveBias;
							switch(tzType)
							{
								case TIME_ZONE_ID_STANDARD:
									effectiveBias = tzi.Bias + tzi.StandardBias;
									break;
								case TIME_ZONE_ID_DAYLIGHT:
									effectiveBias = tzi.Bias + tzi.DaylightBias;
									break;
								case TIME_ZONE_ID_UNKNOWN:
									effectiveBias = tzi.Bias;
									break;
								default:		// error
									effectiveBias = 0;
									DbgPrintf(4, _T("GetTimeZoneInformation() call failed"));
									break;
							}
							int offset = abs(effectiveBias);
							sprintf(&szBuffer[strlen(szBuffer)], "%c%02d%02d\r\n", effectiveBias < 0 ? '+' : '-',
							        offset / 60, offset % 60);
#else
                     strftime(szBuffer, sizeof(szBuffer), "Date: %a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);
#endif

                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);
                     // content-type
                     snprintf(szBuffer, SMTP_BUFFER_SIZE,
                                       "Content-Type: text/plain; charset=%s\r\n"
                                       "Content-Transfer-Encoding: 8bit\r\n\r\n", szEncoding);
                     SendEx(hSocket, szBuffer, strlen(szBuffer), 0, NULL);

                     // Mail body
                     SendEx(hSocket, pszText, strlen(pszText), 0, NULL);
                     SendEx(hSocket, "\r\n.\r\n", 5, 0, NULL);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_MAIL_BODY:
                  // Server should respond with 250 to our mail body
                  if (iResp == 250)
                  {
                     iState = STATE_QUIT;
                     SendEx(hSocket, "QUIT\r\n", 6, 0, NULL);
                  }
                  else
                  {
                     iState = STATE_ERROR;
                  }
                  break;
               case STATE_QUIT:
                  // Server should respond with 221 text to our QUIT command
                  if (iResp == 221)
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
   static const TCHAR *m_szErrorText[] =
   {
      _T("Sent successfully"),
      _T("Unable to resolve SMTP server name"),
      _T("Communication failure"),
      _T("SMTP conversation failure")
   };

	DbgPrintf(1, _T("SMTP mailer thread started"));
   while(1)
   {
      MAIL_ENVELOPE *pEnvelope = (MAIL_ENVELOPE *)m_pMailerQueue->GetOrBlock();
      if (pEnvelope == INVALID_POINTER_VALUE)
         break;

		DbgPrintf(6, _T("SMTP(%p): new envelope, rcpt=%hs"), pEnvelope, pEnvelope->szRcptAddr);

      ConfigReadStr(_T("SMTPServer"), m_szSmtpServer, MAX_PATH, _T("localhost"));
      ConfigReadStrA(_T("SMTPFromAddr"), m_szFromAddr, MAX_PATH, "netxms@localhost");
      ConfigReadStrA(_T("SMTPFromName"), m_szFromName, MAX_PATH, "NetXMS Server");
      m_wSmtpPort = (WORD)ConfigReadInt(_T("SMTPPort"), 25);

      DWORD dwResult = SendMail(pEnvelope->szRcptAddr, pEnvelope->szSubject, pEnvelope->pszText);
      if (dwResult != SMTP_ERR_SUCCESS)
		{
			pEnvelope->nRetryCount--;
			DbgPrintf(6, _T("SMTP(%p): Failed to send e-mail, remaining retries: %d"), pEnvelope, pEnvelope->nRetryCount);
			if (pEnvelope->nRetryCount > 0)
			{
				// Try posting again
				m_pMailerQueue->Put(pEnvelope);
			}
			else
			{
				PostEvent(EVENT_SMTP_FAILURE, g_dwMgmtNode, "dsmm", dwResult, 
							 m_szErrorText[dwResult], pEnvelope->szRcptAddr, pEnvelope->szSubject);
				free(pEnvelope->pszText);
				free(pEnvelope);
			}
		}
		else
		{
			free(pEnvelope->pszText);
			free(pEnvelope);
			DbgPrintf(6, _T("SMTP(%p): mail sent successfully"), pEnvelope);
		}
   }
   return THREAD_OK;
}


//
// Initialize mailer subsystem
//

void InitMailer()
{
   m_pMailerQueue = new Queue;
   m_hThread = ThreadCreateEx(MailerThread, 0, NULL);
}


//
// Shutdown mailer
//

void ShutdownMailer()
{
   m_pMailerQueue->Clear();
   m_pMailerQueue->Put(INVALID_POINTER_VALUE);
   if (m_hThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_hThread);
   delete m_pMailerQueue;
}


//
// Post e-mail to queue
//

void NXCORE_EXPORTABLE PostMail(const TCHAR *pszRcpt, const TCHAR *pszSubject, const TCHAR *pszText)
{
   MAIL_ENVELOPE *pEnvelope;

   pEnvelope = (MAIL_ENVELOPE *)malloc(sizeof(MAIL_ENVELOPE));
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszRcpt, -1, pEnvelope->szRcptAddr, MAX_RCPT_ADDR_LEN, NULL, NULL);
	pEnvelope->szRcptAddr[MAX_RCPT_ADDR_LEN - 1] = 0;
	WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszSubject, -1, pEnvelope->szSubject, MAX_EMAIL_SUBJECT_LEN, NULL, NULL);
	pEnvelope->szSubject[MAX_EMAIL_SUBJECT_LEN - 1] = 0;
	pEnvelope->pszText = MBStringFromWideString(pszText);
#else
   nx_strncpy(pEnvelope->szRcptAddr, pszRcpt, MAX_RCPT_ADDR_LEN);
   nx_strncpy(pEnvelope->szSubject, pszSubject, MAX_EMAIL_SUBJECT_LEN);
   pEnvelope->pszText = _tcsdup(pszText);
#endif
	pEnvelope->nRetryCount = ConfigReadInt(_T("SMTPRetryCount"), 1);
   m_pMailerQueue->Put(pEnvelope);
}
