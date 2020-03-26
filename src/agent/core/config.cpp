/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
   if (fp != nullptr)
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

   SOCKET hSocket = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (hSocket != INVALID_SOCKET)
   {
      SockAddrBuffer sa;
      addr.fillSockAddr(&sa, 4701);
      if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
      {
         // Prepare request
         NXCPMessage msg(2);  // Server version is not known, use protocol version 2
         msg.setCode(CMD_GET_MY_CONFIG);
         msg.setId(1);
         if (H_PlatformName(nullptr, nullptr, szBuffer, nullptr) != SYSINFO_RC_SUCCESS)
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
                                   &pDummyCtx, nullptr, 30000);
            if (nLen >= 16)
            {
               NXCPMessage *pResponse = NXCPMessage::deserialize(pRawMsg);
               if (pResponse != nullptr)
               {
                  if ((pResponse->getCode() == CMD_REQUEST_COMPLETED) &&
                      (pResponse->getId() == 1) &&
                      (pResponse->getFieldAsUInt32(VID_RCC) == 0))
                  {
                     pszConfig = pResponse->getFieldAsString(VID_CONFIG_FILE);
                     if (pszConfig != nullptr)
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
int CreateConfig(bool forceCreate, const char *masterServers, const char *logFile, const char *fileStore,
      const char *configIncludeDir, int numSubAgents, char **subAgentList, const char *extraValues)
{
   FILE *fp;
   time_t currTime;
   int i;

   if ((_taccess(g_szConfigFile, 0) == 0) && !forceCreate)
      return 0;  // File already exist, we shouldn't overwrite it

   fp = _tfopen(g_szConfigFile, _T("w"));
   if (fp != nullptr)
   {
      currTime = time(nullptr);
      _ftprintf(fp, _T("#\n# NetXMS agent configuration file\n# Created by agent installer at %s#\n\n"),
         _tctime(&currTime));
      if (*masterServers == _T('~'))
      {
         // Setup agent-server tunnel(s)
         _ftprintf(fp, _T("MasterServers = %hs\n"), &masterServers[1]);
         const char *curr = &masterServers[1];
         while (true)
         {
            const char *next = strchr(curr, ',');
            if (next == nullptr)
            {
               _ftprintf(fp, _T("ServerConnection = %hs\n"), curr);
               break;
            }
            size_t len = next - curr;
            char temp[256];
            strlcpy(temp, curr, std::min(len + 1, sizeof(temp)));
            StrStripA(temp);
            if (temp[0] != 0)
               _ftprintf(fp, _T("ServerConnection = %hs\n"), temp);
            curr = next + 1;
         }
      }
      else
      {
         _ftprintf(fp, _T("MasterServers = %hs\n"), masterServers);
      }
      _ftprintf(fp, _T("ConfigIncludeDir = %hs\nLogFile = %hs\nFileStore = %hs\n"), configIncludeDir, logFile, fileStore);

      Array extSubAgents;
      for (i = 0; i < numSubAgents; i++)
      {
         if (!strnicmp(subAgentList[i], "EXT:", 4))
         {
            extSubAgents.add(subAgentList[i] + 4);
         }
         else
         {
            _ftprintf(fp, _T("SubAgent = %hs\n"), subAgentList[i]);
         }
      }

      for (i = 0; i < extSubAgents.size(); i++)
      {
         char section[MAX_PATH];
         strlcpy(section, (const char *)extSubAgents.get(i), MAX_PATH);
         char *eptr = strrchr(section, '.');
         if (eptr != nullptr)
            *eptr = 0;
         strupr(section);
         _ftprintf(fp, _T("\n[EXT:%hs]\n"), section);
         _ftprintf(fp, _T("SubAgent = %hs\n"), (const char *)extSubAgents.get(i));
      }

      if (extraValues != nullptr)
      {
         char *temp = MemCopyStringA(extraValues);
         char *curr = temp;
         while (*curr == '~')
            curr++;

         char *next;
         do
         {
            next = strstr(curr, "~~");
            if (next != nullptr)
               *next = 0;
            StrStripA(curr);
            if ((*curr == '[') || (*curr == '*') || (strchr(curr, '=') != nullptr))
            {
               _ftprintf(fp, _T("%hs\n"), curr);
            }
            curr = next + 2;
         } while (next != nullptr);

         MemFree(temp);
      }

      fclose(fp);
   }
   return (fp != nullptr) ? 0 : 2;
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
 * Recovery path for data directory
 */
extern TCHAR g_dataDirRecoveryPath[];

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
   if (SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataDir) != S_OK)
      return;

   _tcslcat(appDataDir, _T("\\nxagentd"), MAX_PATH);
   if (_tcsicmp(appDataDir, g_szDataDirectory))
      return;  // Data directory is not in AppData

   StringBuffer oldAppDataDir(appDataDir);
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
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != nullptr)
   {
      ConsolePrintf(_T("<%s> "), tag);
   }
   ConsolePrintf2(format, args);
   ConsolePrintf(_T("\n"));
}

/**
 * Load config
 */
bool LoadConfig()
{
   nxlog_set_debug_writer(DebugWriter);
   nxlog_set_debug_level(1);
   bool validConfig = g_config->loadConfig(g_szConfigFile, DEFAULT_CONFIG_SECTION, NULL, true);
   if (validConfig)
   {
      const TCHAR *dir = g_config->getValue(_T("/%agent/DataDirectory"));
      if (dir != NULL)
         _tcslcpy(g_szDataDirectory, dir, MAX_PATH);

      dir = g_config->getValue(_T("/%agent/ConfigIncludeDir"));
      if (dir != NULL)
         _tcslcpy(g_szConfigIncludeDir, dir, MAX_PATH);

      validConfig = g_config->loadConfigDirectory(g_szConfigIncludeDir, DEFAULT_CONFIG_SECTION, NULL, false);
      if (!validConfig)
      {
         ConsolePrintf(_T("Error reading additional configuration files from \"%s\"\n"), g_szConfigIncludeDir);
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
   nxlog_set_debug_writer(NULL);
   nxlog_set_debug_level(0);
   return validConfig;
}
