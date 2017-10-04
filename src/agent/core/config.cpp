/*
** NetXMS multiplatform core agent
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
** File: config.cpp
**
**/

#include "nxagentd.h"

/**
 * Externals
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Max NXCP message size
 */
#define MAX_MSG_SIZE    262144

/**
 * Save config to file
 */
static BOOL SaveConfig(TCHAR *pszConfig)
{
   FILE *fp;
   BOOL bRet = FALSE;

   fp = _tfopen(g_szConfigFile, _T("w"));
   if (fp != NULL)
   {
      bRet = (_fputts(pszConfig, fp) >= 0);
      fclose(fp);
   }
   return bRet;
}

/**
 * Download agent's config from management server
 */
BOOL DownloadConfig(TCHAR *pszServer)
{
   BOOL bRet = FALSE;
   TCHAR szBuffer[MAX_RESULT_LENGTH], *pszConfig;
   NXCPMessage msg, *pResponse;
   NXCP_MESSAGE *pRawMsg;
   NXCP_BUFFER *pBuffer;
   NXCPEncryptionContext *pDummyCtx = NULL;
   int nLen;

#ifdef _WIN32
   WSADATA wsaData;

   if (WSAStartup(0x0202, &wsaData) != 0)
   {
      _tprintf(_T("ERROR: Unable to initialize Windows Sockets\n"));
      return FALSE;
   }
#endif

   InetAddress addr = InetAddress::resolveHostName(pszServer);
   if (!addr.isValidUnicast())
   {
      _tprintf(_T("ERROR: Unable to resolve name of management server\n"));
      return FALSE;
   }

   SOCKET hSocket = socket(addr.getFamily(), SOCK_STREAM, 0);
   if (hSocket != INVALID_SOCKET)
   {
      SockAddrBuffer sa;
      addr.fillSockAddr(&sa, 4701);
      if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
      {
         // Prepare request
         msg.setCode(CMD_GET_MY_CONFIG);
         msg.setId(1);
         if (H_PlatformName(NULL, NULL, szBuffer, NULL) != SYSINFO_RC_SUCCESS)
            _tcscpy(szBuffer, _T("error"));
         msg.setField(VID_PLATFORM_NAME, szBuffer);
         msg.setField(VID_VERSION_MAJOR, (WORD)NETXMS_VERSION_MAJOR);
         msg.setField(VID_VERSION_MINOR, (WORD)NETXMS_VERSION_MINOR);
         msg.setField(VID_VERSION_RELEASE, (WORD)NETXMS_VERSION_BUILD);
         msg.setField(VID_VERSION, NETXMS_VERSION_STRING);

         // Send request
         pRawMsg = msg.serialize();
         nLen = ntohl(pRawMsg->size);
         if (SendEx(hSocket, pRawMsg, nLen, 0, NULL) == nLen)
         {
            pRawMsg = (NXCP_MESSAGE *)realloc(pRawMsg, MAX_MSG_SIZE);
            pBuffer = (NXCP_BUFFER *)malloc(sizeof(NXCP_BUFFER));
            NXCPInitBuffer(pBuffer);

            nLen = RecvNXCPMessage(hSocket, pRawMsg, pBuffer, MAX_MSG_SIZE,
                                   &pDummyCtx, NULL, 30000);
            if (nLen >= 16)
            {
               pResponse = NXCPMessage::deserialize(pRawMsg);
               if (pResponse != NULL)
               {
                  if ((pResponse->getCode() == CMD_REQUEST_COMPLETED) &&
                      (pResponse->getId() == 1) &&
                      (pResponse->getFieldAsUInt32(VID_RCC) == 0))
                  {
                     pszConfig = pResponse->getFieldAsString(VID_CONFIG_FILE);
                     if (pszConfig != NULL)
                     {
                        bRet = SaveConfig(pszConfig);
                        free(pszConfig);
                     }
                  }
                  delete pResponse;
               }
            }
            free(pBuffer);
         }
         free(pRawMsg);
      }
      else
      {
         printf("ERROR: Cannot connect to management server\n");
      }
      closesocket(hSocket);
   }

#ifdef _WIN32
   WSACleanup();
#endif
   return bRet;
}
