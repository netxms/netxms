/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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

#include "portcheck.h"
#include <tls_conn.h>

/**
 * Check SMTP/SMTPS service - parameter handler
 */
LONG H_CheckSMTP(const TCHAR* param, const TCHAR* arg, TCHAR* value, AbstractCommSession* session)
{
   char host[256], recipient[256], portAsChar[256];
   uint16_t port;

   AgentGetParameterArgA(param, 1, host, sizeof(host));
   AgentGetParameterArgA(param, 2, recipient, sizeof(recipient));
   uint32_t timeout = GetTimeoutFromArgs(param, 3);
   AgentGetParameterArgA(param, 4, portAsChar, sizeof(portAsChar));

   if ((host[0] == 0) || (recipient[0] == 0))
      return SYSINFO_RC_UNSUPPORTED;

   if (portAsChar[0] == 0)
   {
      port = (arg[1] == 'S') ? 465 : 25;
   }
   else
   {
      port = ParsePort(portAsChar);
      if (port == 0)
         return SYSINFO_RC_UNSUPPORTED;
   }

   LONG rc = SYSINFO_RC_SUCCESS;
   int64_t start = GetCurrentTimeMs();
   int result = CheckSMTP(arg[1] == 'S', InetAddress::resolveHostName(host), port, recipient, timeout);
   if (*arg == 'R')
   {
      if (result == PC_ERR_NONE)
         ret_int64(value, GetCurrentTimeMs() - start);
      else if (g_serviceCheckFlags & SCF_NEGATIVE_TIME_ON_ERROR)
         ret_int(value, -result);
      else
         rc = SYSINFO_RC_ERROR;
   }
   else
   {
      ret_int(value, result);
   }
   return rc;
}

/**
 * Check SMTP/SMTPS service
 */
int CheckSMTP(bool enableTLS, const InetAddress& addr, uint16_t port, const char* to, uint32_t timeout)
{
   int status = PC_ERR_NONE;
   int err = 0;

   TLSConnection tc(SUBAGENT_DEBUG_TAG, false, timeout);

   if (tc.connect(addr, port, enableTLS, timeout))
   {
      char buff[2048];
      char tmp[128];
      char hostname[128];

      status = PC_ERR_HANDSHAKE;

#define CHECK_OK(x)                        \
   err = 1;                                \
   while (1)                               \
   {                                       \
      if (tc.recv(buff, sizeof(buff)) > 3) \
      {                                    \
         if (buff[3] == '-')               \
            continue;                      \
         if (strncmp(buff, x " ", 4) == 0) \
            err = 0;                       \
         break;                            \
      }                                    \
      else                                 \
         break;                            \
   }                                       \
   if (err == 0)

      CHECK_OK("220")
      {
         strlcpy(hostname, g_hostName, sizeof(hostname));
         if (hostname[0] == 0)
         {
#ifdef UNICODE
            WCHAR wname[128] = L"";
            GetLocalHostName(wname, 128, true);
            wchar_to_utf8(wname, -1, hostname, sizeof(hostname));
#else
            GetLocalHostName(szHostname, sizeof(szHostname), true);
#endif
            if (hostname[0] == 0)
            {
               strcpy(hostname, "netxms-portcheck");
            }
         }

         snprintf(tmp, sizeof(tmp), "HELO %s\r\n", hostname);
         if (tc.send(tmp, strlen(tmp)))
         {
            CHECK_OK("250")
            {
               snprintf(tmp, sizeof(tmp), "MAIL FROM: noreply@%s\r\n", g_szDomainName);
               if (tc.send(tmp, strlen(tmp)))
               {
                  CHECK_OK("250")
                  {
                     snprintf(tmp, sizeof(tmp), "RCPT TO: %s\r\n", to);
                     if (tc.send(tmp, strlen(tmp)))
                     {
                        CHECK_OK("250")
                        {
                           if (tc.send("DATA\r\n", 6))
                           {
                              CHECK_OK("354")
                              {
                                 // date
                                 time_t currentTime;
                                 struct tm* pCurrentTM;
                                 char timeAsChar[64];

                                 time(&currentTime);
#ifdef HAVE_LOCALTIME_R
                                 struct tm currentTM;
                                 localtime_r(&currentTime, &currentTM);
                                 pCurrentTM = &currentTM;
#else
                                 pCurrentTM = localtime(&currentTime);
#endif
                                 strftime(timeAsChar, sizeof(timeAsChar), "%a, %d %b %Y %H:%M:%S %z\r\n", pCurrentTM);

                                 snprintf(buff, sizeof(buff), "From: <noreply@%s>\r\nTo: <%s>\r\nSubject: NetXMS test mail\r\nDate: %s\r\n\r\nNetXMS test mail\r\n.\r\n",
                                          hostname, to, timeAsChar);

                                 if (tc.send(buff, strlen(buff)))
                                 {
                                    CHECK_OK("250")
                                    {
                                       if (tc.send("QUIT\r\n", 6))
                                       {
                                          CHECK_OK("221")
                                          {
                                             status = PC_ERR_NONE;
                                          }
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   else
   {
      status = PC_ERR_CONNECT;
   }

   return status;
}