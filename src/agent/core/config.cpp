/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
#include <nxstat.h>

#ifdef _WIN32
#include <shlobj.h>
#endif

/**
 * Externals
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

extern TCHAR g_dataDirRecoveryPath[];

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

/**
* Create configuration file
*/
int CreateConfig(const char *pszServer, const char *pszLogFile, const char *pszFileStore,
      const char *configIncludeDir, int iNumSubAgents, char **ppszSubAgentList)
{
   FILE *fp;
   time_t currTime;
   int i;

   if (_taccess(g_szConfigFile, 0) == 0)
      return 0;  // File already exist, we shouldn't overwrite it

   fp = _tfopen(g_szConfigFile, _T("w"));
   if (fp != NULL)
   {
      currTime = time(NULL);
      _ftprintf(fp, _T("#\n# NetXMS agent configuration file\n# Created by agent installer at %s#\n\n"),
         _tctime(&currTime));
      _ftprintf(fp, _T("MasterServers = %hs\nConfigIncludeDir = %hs\nLogFile = %hs\nFileStore = %hs\n"),
         pszServer, configIncludeDir, pszLogFile, pszFileStore);

      Array extSubAgents;
      for (i = 0; i < iNumSubAgents; i++)
      {
         if (!strnicmp(ppszSubAgentList[i], "EXT:", 4))
         {
            extSubAgents.add(ppszSubAgentList[i] + 4);
         }
         else
         {
            _ftprintf(fp, _T("SubAgent = %hs\n"), ppszSubAgentList[i]);
         }
      }

      for (i = 0; i < extSubAgents.size(); i++)
      {
         char section[MAX_PATH];
         strncpy(section, (const char *)extSubAgents.get(i), MAX_PATH);
         char *eptr = strrchr(section, '.');
         if (eptr != NULL)
            *eptr = 0;
         strupr(section);
         _ftprintf(fp, _T("\n[EXT:%hs]\n"), section);
         _ftprintf(fp, _T("SubAgent = %hs\n"), (const char *)extSubAgents.get(i));
      }

      fclose(fp);
   }
   return (fp != NULL) ? 0 : 2;
}

/**
* Init config
*/
void InitConfig(const TCHAR *configSection)
{
   g_config = new Config();
   g_config->setTopLevelTag(_T("config"));
   g_config->setAlias(_T("agent"), configSection);

   // Set default data directory
   if (!_tcscmp(g_szDataDirectory, _T("{default}")))
   {
#ifdef _WIN32
      if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, g_szDataDirectory) == S_OK)
      {
         _tcslcat(g_szDataDirectory, _T("\\nxagentd"), MAX_PATH);
      }
      else
      {
         GetNetXMSDirectory(nxDirData, g_szDataDirectory);
      }
#else
      GetNetXMSDirectory(nxDirData, g_szDataDirectory);
#endif
   }
}

#ifdef _WIN32

/**
* Recover agent's data directory.
* During Windows 10 upgrades local service application data directory
* can be moved to Windows.old.
*/
static void RecoverDataDirectory()
{
   if (_taccess(g_szDataDirectory, 0) == 0)
      return;  // Data directory exist

   TCHAR appDataDir[MAX_PATH];
   if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataDir) != S_OK)
      return;

   _tcslcat(appDataDir, _T("\\nxagentd"), MAX_PATH);
   if (_tcsicmp(appDataDir, g_szDataDirectory))
      return;  // Data directory is not in AppData

   String oldAppDataDir(appDataDir);
   oldAppDataDir.toUppercase();
   oldAppDataDir.replace(_T("\\WINDOWS\\"), _T("\\WINDOWS.OLD\\"));
   if (_taccess(oldAppDataDir, 0) != 0)
      return;  // Old directory missing

   if (CopyFileOrDirectory(oldAppDataDir, g_szDataDirectory))
   {
      _tcslcpy(g_dataDirRecoveryPath, oldAppDataDir, MAX_PATH);
   }
}

#endif

void RecoverConfigPolicyDirectory()
{
   TCHAR legacyConfigPolicyDir[MAX_PATH] = AGENT_DEFAULT_DATA_DIR;
   if (!_tcscmp(legacyConfigPolicyDir, _T("{default}")))
   {
      GetNetXMSDirectory(nxDirData, legacyConfigPolicyDir);
   }

   if (legacyConfigPolicyDir[_tcslen(legacyConfigPolicyDir) - 1] != FS_PATH_SEPARATOR_CHAR)
      _tcscat(legacyConfigPolicyDir, FS_PATH_SEPARATOR);
   _tcscat(legacyConfigPolicyDir, CONFIG_AP_FOLDER);

   if (!_tcsicmp(legacyConfigPolicyDir, g_szConfigPolicyDir))
      return;

   if (_taccess(legacyConfigPolicyDir, 0) != 0)
      return;

   MoveFileOrDirectory(legacyConfigPolicyDir, g_szConfigPolicyDir);
}

/**
 * Load config
 */
bool LoadConfig()
{
   bool validConfig = g_config->loadConfig(g_szConfigFile, DEFAULT_CONFIG_SECTION, false);
   if (validConfig)
   {
      const TCHAR *dir = g_config->getValue(_T("/%agent/DataDirectory"));
      if (dir != NULL)
         _tcslcpy(g_szDataDirectory, dir, MAX_PATH);

      dir = g_config->getValue(_T("/%agent/ConfigIncludeDir"));
      if (dir != NULL)
      {
         validConfig = g_config->loadConfigDirectory(dir, DEFAULT_CONFIG_SECTION, false);
         if (!validConfig)
         {
            ConsolePrintf(_T("Error reading additional configuration files from \"%s\"\n"), dir);
         }
      }

#ifdef _WIN32
      RecoverDataDirectory();
#endif

      _tcslcpy(g_szConfigPolicyDir, g_szDataDirectory, MAX_PATH - 16);
      if (g_szConfigPolicyDir[_tcslen(g_szConfigPolicyDir) - 1] != FS_PATH_SEPARATOR_CHAR)
         _tcscat(g_szConfigPolicyDir, FS_PATH_SEPARATOR);
      _tcscat(g_szConfigPolicyDir, CONFIG_AP_FOLDER);
      if (_taccess(g_szConfigPolicyDir, 0) != 0)
      {
         // Check if configuration policies stored at old location
         RecoverConfigPolicyDirectory();
         CreateFolder(g_szConfigPolicyDir);
      }
      _tcscat(g_szConfigPolicyDir, FS_PATH_SEPARATOR);

      validConfig = g_config->loadConfigDirectory(g_szConfigPolicyDir, DEFAULT_CONFIG_SECTION);
      if (!validConfig)
      {
         ConsolePrintf(_T("Error reading additional configuration files from \"%s\"\n"), g_szConfigPolicyDir);
      }
   }
   else
   {
      ConsolePrintf(_T("Error loading configuration from \"%s\" section ") DEFAULT_CONFIG_SECTION _T("\n"), g_szConfigFile);
   }
   return validConfig;
}
