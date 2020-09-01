/*
** NetXMS - Network Management System
** Notification driver for SMTP protocol
** Copyright (C) 2019-2020 Raden Solutions
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
** File: smtp.cpp
**
**/

#include <ncdrv.h>
#include <nms_util.h>
#include <nxcldefs.h>

#define DEBUG_TAG _T("ncd.smtp")

static const NCConfigurationTemplate s_config(true, true);

/**
 * Receive buffer size
 */
#define SMTP_BUFFER_SIZE            1024

/**
 * Sender errors
 */
#define SMTP_ERR_SUCCESS            0
#define SMTP_ERR_BAD_SERVER_NAME    1
#define SMTP_ERR_COMM_FAILURE       2
#define SMTP_ERR_PROTOCOL_FAILURE   3

/**
 * Mail sender states
 */
#define STATE_INITIAL      0
#define STATE_HELLO        1
#define STATE_FROM         2
#define STATE_RCPT         3
#define STATE_DATA         4
#define STATE_MAIL_BODY    5
#define STATE_QUIT         6
#define STATE_FINISHED     7
#define STATE_ERROR        8

/**
 * Mail envelope structure
 */
struct MAIL_ENVELOPE
{
   char rcptAddr[MAX_RCPT_ADDR_LEN];
   char subject[MAX_EMAIL_SUBJECT_LEN];
   char *text;
   char encoding[64];
   bool isHtml;
   bool isUtf8;
   int retryCount;
};

/**
 * Mail sending error text
 */
static const TCHAR *s_szErrorText[] =
{
   _T("Sent successfully"),
   _T("Unable to resolve SMTP server name"),
   _T("Communication failure"),
   _T("SMTP conversation failure")
};

/**
 * SMTP driver class
 */
class SmtpDriver : public NCDriver
{
private:
   TCHAR m_server[MAX_STRING_VALUE];
   uint32_t m_retryCount;
   uint16_t m_port;
   char m_localHostName[MAX_STRING_VALUE];
   char m_fromName[MAX_STRING_VALUE];
   char m_fromAddr[MAX_STRING_VALUE];
   char m_encoding[64];
   int m_isHtml;

   SmtpDriver();
   UINT32 sendMail(const char *pszRcpt, const char *pszSubject, const char *pszText, const char *encoding, bool isHtml, bool isUtf8);

public:
   virtual bool send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body) override;
   MAIL_ENVELOPE *prepareMail(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body);
   static SmtpDriver *createInstance(Config *config);
};



/**
 * Create driver instance
 */
SmtpDriver *SmtpDriver::createInstance(Config *config)
{
   SmtpDriver *driver = new SmtpDriver();

   NX_CFG_TEMPLATE configTemplate[] =
   {
      { _T("Server"), CT_STRING, 0, 0, sizeof(driver->m_server)/sizeof(TCHAR), 0, driver->m_server },
      { _T("RetryCount"), CT_LONG, 0, 0, 0, 0, &(driver->m_retryCount) }, //1
      { _T("Port"), CT_LONG, 0, 0, 0, 0, &(driver->m_port) }, //25
      { _T("LocalHostName"), CT_MB_STRING, 0, 0, sizeof(driver->m_localHostName)/sizeof(TCHAR), 0, driver->m_localHostName },
      { _T("FromName"), CT_MB_STRING, 0, 0, sizeof(driver->m_fromName)/sizeof(TCHAR), 0, driver->m_fromName }, //NetXMS Server
      { _T("FromAddr"), CT_MB_STRING, 0, 0, sizeof(driver->m_fromAddr)/sizeof(TCHAR), 0, driver->m_fromAddr }, //netxms@localhost
      { _T("MailEncoding"), CT_MB_STRING, 0, 0, sizeof(driver->m_encoding)/sizeof(TCHAR), 0, driver->m_encoding }, //utf8
      { _T("IsHTML"), CT_BOOLEAN, 0, 0, 1, 0, &(driver->m_isHtml) },
      { _T(""), CT_END_OF_LIST, 0, 0, 0, 0, NULL }
   };

   if (!config->parseTemplate(_T("SMTP"), configTemplate))
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Error parsing driver configuration"));
      delete driver;
      return NULL;
   }

   return driver;
}

/**
 * Constructor for SMTP diver
 */
SmtpDriver::SmtpDriver()
{
   _tcscmp(m_server, _T("localhost"));
   m_retryCount = 1;
   m_port = 25;
   m_localHostName[0] = 0;
   strcmp(m_fromName, "NetXMS Server");
   strcmp(m_fromAddr, "netxms@localhost");
   strcmp(m_encoding, "utf8");
   m_isHtml = false;

}

MAIL_ENVELOPE *SmtpDriver::prepareMail(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   auto envelope = MemAllocStruct<MAIL_ENVELOPE>();
   strncpy(envelope->encoding, m_encoding, 64);
   envelope->isUtf8 = m_isHtml || !stricmp(envelope->encoding, "utf-8") || !stricmp(envelope->encoding, "utf8");

#ifdef UNICODE
   WideCharToMultiByte(envelope->isUtf8 ? CP_UTF8 : CP_ACP, envelope->isUtf8 ? 0 : WC_DEFAULTCHAR | WC_COMPOSITECHECK, recipient, -1, envelope->rcptAddr, MAX_RCPT_ADDR_LEN, NULL, NULL);
   envelope->rcptAddr[MAX_RCPT_ADDR_LEN - 1] = 0;
   WideCharToMultiByte(envelope->isUtf8 ? CP_UTF8 : CP_ACP, envelope->isUtf8 ? 0 : WC_DEFAULTCHAR | WC_COMPOSITECHECK, subject, -1, envelope->subject, MAX_EMAIL_SUBJECT_LEN, NULL, NULL);
   envelope->subject[MAX_EMAIL_SUBJECT_LEN - 1] = 0;
   envelope->text = envelope->isUtf8 ? UTF8StringFromWideString(body) : MBStringFromWideString(body);
#else
   if (envelope->isUtf8)
   {
      mb_to_utf8(recipient, -1, envelope->rcptAddr, MAX_RCPT_ADDR_LEN);
      envelope->rcptAddr[MAX_RCPT_ADDR_LEN - 1] = 0;
      mb_to_utf8(subject, -1, envelope->subject, MAX_EMAIL_SUBJECT_LEN);
      envelope->subject[MAX_EMAIL_SUBJECT_LEN - 1] = 0;
      envelope->text = UTF8StringFromMBString(body);
   }
   else
   {
      nx_strncpy(envelope->rcptAddr, recipient, MAX_RCPT_ADDR_LEN);
      nx_strncpy(envelope->subject, subject, MAX_EMAIL_SUBJECT_LEN);
      envelope->text = strdup(body);
   }
#endif
   envelope->retryCount = m_retryCount;
   envelope->isHtml = m_isHtml;
   return envelope;
}
/**
 * Find end-of-line character
 */
static char *FindEOL(char *pszBuffer, size_t len)
{
   for(size_t i = 0; i < len; i++)
      if (pszBuffer[i] == '\n')
         return &pszBuffer[i];
   return nullptr;
}

/**
 * Read line from socket
 */
static BOOL ReadLineFromSocket(SOCKET hSocket, char *pszBuffer, size_t *pnBufPos, char *pszLine)
{
   char *ptr;
   do
   {
      ptr = FindEOL(pszBuffer, *pnBufPos);
      if (ptr == NULL)
      {
         ssize_t bytes = RecvEx(hSocket, &pszBuffer[*pnBufPos], SMTP_BUFFER_SIZE - *pnBufPos, 0, 30000);
         if (bytes <= 0)
            return FALSE;
         *pnBufPos += bytes;
      }
   } while(ptr == NULL);
   *ptr = 0;
   strcpy(pszLine, pszBuffer);
   *pnBufPos -= (int)(ptr - pszBuffer + 1);
   memmove(pszBuffer, ptr + 1, *pnBufPos);
   return TRUE;
}

/**
 * Read SMTP response code from socket
 */
static int GetSMTPResponse(SOCKET hSocket, char *pszBuffer, size_t *pnBufPos)
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

/**
 * Encode SMTP header
 */
static char *EncodeHeader(const char *header, const char *encoding, const char *data, char *buffer, size_t bufferSize)
{
   bool encode = false;
   for(const char *p = data; *p != 0; p++)
      if (*p & 0x80)
      {
         encode = true;
         break;
      }
   if (encode)
   {
      char *encodedData = NULL;
      base64_encode_alloc(data, strlen(data), &encodedData);
      if (encodedData != NULL)
      {
         if (header != NULL)
            snprintf(buffer, bufferSize, "%s: =?%s?B?%s?=\r\n", header, encoding, encodedData);
         else
            snprintf(buffer, bufferSize, "=?%s?B?%s?=", encoding, encodedData);
         free(encodedData);
      }
      else
      {
         // fallback
         if (header != NULL)
            snprintf(buffer, bufferSize, "%s: %s\r\n", header, data);
         else
            strlcpy(buffer, data, bufferSize);
      }
   }
   else
   {
      if (header != NULL)
         snprintf(buffer, bufferSize, "%s: %s\r\n", header, data);
      else
         strlcpy(buffer, data, bufferSize);
   }
   return buffer;
}

/**
 * Send e-mail
 */
UINT32 SmtpDriver::sendMail(const char *pszRcpt, const char *pszSubject, const char *pszText, const char *encoding, bool isHtml, bool isUtf8)
{
   char fromName[256];
   if (isUtf8)
   {
      mb_to_utf8(m_fromName, -1, fromName, 256);
   }
   if (m_localHostName[0] == 0)
   {
#ifdef UNICODE
      WCHAR localHostNameW[256] = L"";
      GetLocalHostName(localHostNameW, 256, true);
      wchar_to_utf8(localHostNameW, -1, m_localHostName, 256);
#else
      GetLocalHostName(m_localHostName, 256, true);
#endif
      if (m_localHostName[0] == 0)
         strcpy(m_localHostName, "localhost");
   }

   // Resolve hostname
   InetAddress addr = InetAddress::resolveHostName(m_server);
   if (!addr.isValid() || addr.isBroadcast() || addr.isMulticast())
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Cannot resolve server name %s"), m_server);
      return SMTP_ERR_BAD_SERVER_NAME;
   }

   // Create socket and connect to server
   SOCKET hSocket = ConnectToHost(addr, m_port, 3000);
   if (hSocket == INVALID_SOCKET)
      return SMTP_ERR_COMM_FAILURE;

   char szBuffer[SMTP_BUFFER_SIZE];
   size_t nBufPos = 0;
   int iState = STATE_INITIAL;
   while((iState != STATE_FINISHED) && (iState != STATE_ERROR))
   {
      int iResp = GetSMTPResponse(hSocket, szBuffer, &nBufPos);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("SMTP RESPONSE: %03d (state=%d)"), iResp, iState);
      if (iResp > 0)
      {
         switch(iState)
         {
            case STATE_INITIAL:
               // Server should send 220 text after connect
               if (iResp == 220)
               {
                  iState = STATE_HELLO;
                  char command[280];
                  snprintf(command, 280, "HELO %s\r\n", m_localHostName);
                  SendEx(hSocket, command, strlen(command), 0, nullptr);
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
                  snprintf(szBuffer, SMTP_BUFFER_SIZE, "MAIL FROM: <%s>\r\n", m_fromAddr);
                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);
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
                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);
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
                  SendEx(hSocket, "DATA\r\n", 6, 0, nullptr);
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
                  char from[512];
                  snprintf(szBuffer, SMTP_BUFFER_SIZE, "From: \"%s\" <%s>\r\n", EncodeHeader(nullptr, encoding, fromName, from, 512), m_fromAddr);
                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);
                  // to
                  snprintf(szBuffer, SMTP_BUFFER_SIZE, "To: <%s>\r\n", pszRcpt);
                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);
                  // subject
                  EncodeHeader("Subject", encoding, pszSubject, szBuffer, SMTP_BUFFER_SIZE);
                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);

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
                  UINT32 tzType = GetTimeZoneInformation(&tzi);
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
                     default:    // error
                        effectiveBias = 0;
                        nxlog_debug_tag(DEBUG_TAG, 4, _T("GetTimeZoneInformation() call failed"));
                        break;
                  }
                  int offset = abs(effectiveBias);
                  sprintf(&szBuffer[strlen(szBuffer)], "%c%02d%02d\r\n", effectiveBias <= 0 ? '+' : '-', offset / 60, offset % 60);
#else
                  strftime(szBuffer, sizeof(szBuffer), "Date: %a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);
#endif

                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);
                  // content-type
                  snprintf(szBuffer, SMTP_BUFFER_SIZE,
                                    "Content-Type: text/%s; charset=%s\r\n"
                                    "Content-Transfer-Encoding: 8bit\r\n\r\n", isHtml ? "html" : "plain", encoding);
                  SendEx(hSocket, szBuffer, strlen(szBuffer), 0, nullptr);

                  // Mail body
                  SendEx(hSocket, pszText, strlen(pszText), 0, nullptr);
                  SendEx(hSocket, "\r\n.\r\n", 5, 0, nullptr);
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
                  SendEx(hSocket, "QUIT\r\n", 6, 0, nullptr);
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

   return (iState == STATE_FINISHED) ? SMTP_ERR_SUCCESS : SMTP_ERR_PROTOCOL_FAILURE;
}


/**
 * Driver send method
 */
bool SmtpDriver::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   bool success = false;

   MAIL_ENVELOPE *pEnvelope = prepareMail(recipient, subject, body);
   nxlog_debug(6, _T("SMTP(%p): new envelope, rcpt=%hs"), pEnvelope, pEnvelope->rcptAddr);
   while (pEnvelope->retryCount > 0)
   {
      uint32_t result = sendMail(pEnvelope->rcptAddr, pEnvelope->subject, pEnvelope->text, pEnvelope->encoding, pEnvelope->isHtml, pEnvelope->isUtf8);
      if (result != SMTP_ERR_SUCCESS)
      {
         pEnvelope->retryCount--;
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SMTP(%p): Failed to send e-mail with error \"%s\", remaining retries: %d"), pEnvelope, s_szErrorText[result], pEnvelope->retryCount);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("SMTP(%p): mail sent successfully"), pEnvelope);
         success = true;
         break;
      }

      if (pEnvelope->retryCount > 0)
         ThreadSleep(1);
   }
   MemFree(pEnvelope->text);
   MemFree(pEnvelope);
   return success;
}

/**
 * Driver entry point
 */
DECLARE_NCD_ENTRY_POINT(SMTP, &s_config)
{
   return SmtpDriver::createInstance(config);
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
