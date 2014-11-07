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
** File: config.cpp
**
**/

#include "nxagentd.h"

/**
 * Externals
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);

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
   UINT32 dwAddr;
   SOCKET hSocket;
   struct sockaddr_in sa;
   BOOL bRet = FALSE;
   TCHAR szBuffer[MAX_RESULT_LENGTH], *pszConfig;
   CSCPMessage msg, *pResponse;
   CSCP_MESSAGE *pRawMsg;
   CSCP_BUFFER *pBuffer;
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

   dwAddr = ResolveHostName(pszServer);
   if (dwAddr == INADDR_NONE)
   {
      _tprintf(_T("ERROR: Unable to resolve name of management server\n"));
      return FALSE;
   }

   hSocket = socket(AF_INET, SOCK_STREAM, 0);
   if (hSocket != INVALID_SOCKET)
   {
      // Fill in address structure
      memset(&sa, 0, sizeof(sa));
      sa.sin_addr.s_addr = dwAddr;
      sa.sin_family = AF_INET;
      sa.sin_port = htons(4701);
      if (connect(hSocket, (struct sockaddr *)&sa, sizeof(sa)) != -1)
      {
         // Prepare request
         msg.SetCode(CMD_GET_MY_CONFIG);
         msg.SetId(1);
         if (H_PlatformName(NULL, NULL, szBuffer) != SYSINFO_RC_SUCCESS)
            _tcscpy(szBuffer, _T("error"));
         msg.SetVariable(VID_PLATFORM_NAME, szBuffer);
         msg.SetVariable(VID_VERSION_MAJOR, (WORD)NETXMS_VERSION_MAJOR);
         msg.SetVariable(VID_VERSION_MINOR, (WORD)NETXMS_VERSION_MINOR);
         msg.SetVariable(VID_VERSION_RELEASE, (WORD)NETXMS_VERSION_BUILD);
         msg.SetVariable(VID_VERSION, NETXMS_VERSION_STRING);

         // Send request
         pRawMsg = msg.createMessage();
         nLen = ntohl(pRawMsg->dwSize);
         if (SendEx(hSocket, pRawMsg, nLen, 0, NULL) == nLen)
         {
            pRawMsg = (CSCP_MESSAGE *)realloc(pRawMsg, MAX_MSG_SIZE);
            pBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
            RecvNXCPMessage(0, NULL, pBuffer, 0, NULL, NULL, 0);

            nLen = RecvNXCPMessage(hSocket, pRawMsg, pBuffer, MAX_MSG_SIZE,
                                   &pDummyCtx, NULL, 30000);
            if (nLen >= 16)
            {
               pResponse = new CSCPMessage(pRawMsg);
               if ((pResponse->GetCode() == CMD_REQUEST_COMPLETED) &&
                   (pResponse->GetId() == 1) &&
                   (pResponse->GetVariableLong(VID_RCC) == 0))
               {
                  pszConfig = pResponse->GetVariableStr(VID_CONFIG_FILE);
                  if (pszConfig != NULL)
                  {
                     bRet = SaveConfig(pszConfig);
                     free(pszConfig);
                  }
               }
               delete pResponse;
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
