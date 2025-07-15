/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2023 Raden Solutions
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
#include <netxms-version.h>

#ifdef _WIN32
#include <shlobj.h>
#endif

/**
 * Global config instance
 */
shared_ptr_store<Config> g_config;

/**
 * Externals
 */
LONG H_PlatformName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Max NXCP message size
 */
#define MAX_MSG_SIZE    262144

/**
 * Download agent's config from management server
 */
bool DownloadConfig(const TCHAR *server)
{
#ifdef _WIN32
   WSADATA wsaData;
   if (WSAStartup(0x0202, &wsaData) != 0)
   {
      _tprintf(_T("ERROR: Unable to initialize Windows Sockets\n"));
      return false;
   }
#endif

   InetAddress addr = InetAddress::resolveHostName(server);
   if (!addr.isValidUnicast())
   {
      _tprintf(_T("ERROR: Unable to resolve name of management server\n"));
      return false;
   }

   bool success = false;
   SOCKET hSocket = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (hSocket != INVALID_SOCKET)
   {
      SockAddrBuffer sa;
      addr.fillSockAddr(&sa, 4701);
      if (connect(hSocket, (struct sockaddr *)&sa, SA_LEN((struct sockaddr *)&sa)) != -1)
      {
         // Prepare request
         NXCPMessage msg(CMD_GET_MY_CONFIG, 1, 2);  // Server version is not known, use protocol version 2

         TCHAR buffer[MAX_RESULT_LENGTH];
         if (H_PlatformName(nullptr, nullptr, buffer, nullptr) != SYSINFO_RC_SUCCESS)
            _tcscpy(buffer, _T("error"));
         msg.setField(VID_PLATFORM_NAME, buffer);
         msg.setField(VID_VERSION_MAJOR, (WORD)NETXMS_VERSION_MAJOR);
         msg.setField(VID_VERSION_MINOR, (WORD)NETXMS_VERSION_MINOR);
         msg.setField(VID_VERSION_RELEASE, (WORD)NETXMS_VERSION_RELEASE);
         msg.setField(VID_VERSION, NETXMS_VERSION_STRING);

         // Send request
         NXCP_MESSAGE *rawMsg = msg.serialize();
         ssize_t nLen = ntohl(rawMsg->size);
         if (SendEx(hSocket, rawMsg, nLen, 0, nullptr) == nLen)
         {
            SocketMessageReceiver receiver(hSocket, 4096, MAX_MSG_SIZE);
            MessageReceiverResult result;
            NXCPMessage *response = receiver.readMessage(30000, &result);
            if (response != nullptr)
            {
               if ((response->getCode() == CMD_REQUEST_COMPLETED) &&
                   (response->getId() == 1) &&
                   (response->getFieldAsUInt32(VID_RCC) == 0))
               {
                  char *config = response->getFieldAsUtf8String(VID_CONFIG_FILE);
                  if (config != nullptr)
                  {
                     SaveFileStatus status = SaveFile(g_szConfigFile, config, strlen(config), false, true);
                     if (status == SaveFileStatus::SUCCESS)
                     {
                        _tprintf(_T("INFO: Configuration file downloaded from management server\n"));
                        success = true;
                     }
                     else if ((status == SaveFileStatus::OPEN_ERROR) || (status == SaveFileStatus::RENAME_ERROR))
                     {
                        _tprintf(_T("ERROR: Error opening file %s for writing (%s)"), g_szConfigFile, _tcserror(errno));
                     }
                     else
                     {
                        _tprintf(_T("ERROR: Error writing configuration file (%s)"), _tcserror(errno));
                     }
                     MemFree(config);
                  }
               }
               else
               {
                  _tprintf(_T("ERROR: Invalid response from management server\n"));
               }
               delete response;
            }
            else
            {
               _tprintf(_T("ERROR: No response from management server\n"));
            }
         }
         MemFree(rawMsg);
      }
      else
      {
         _tprintf(_T("ERROR: Cannot connect to management server\n"));
      }
      closesocket(hSocket);
   }

#ifdef _WIN32
   WSACleanup();
#endif
   return success;
}

/**
 * Create configuration file
 */
int CreateConfig(bool forceCreate, const char *masterServers, const char *logFile, const char *fileStore,
      const char *configIncludeDir, int numSubAgents, char **subAgentList, const char *extraValues)
{
   if ((_taccess(g_szConfigFile, F_OK) == 0) && !forceCreate)
      return 0;  // File already exist, we shouldn't overwrite it

   FILE *fp = _tfopen(g_szConfigFile, _T("w"));
   if (fp == nullptr)
      return 2;   // Open error

   time_t currTime = time(nullptr);
   TCHAR timeText[32];
   _ftprintf(fp, _T("#\n# NetXMS agent configuration file\n# Created by agent installer at %s#\n\n"), FormatTimestamp(currTime, timeText));
   if (*masterServers == _T('~'))
   {
      // Setup agent-server tunnel(s)
      const char *curr = &masterServers[1];
      while (true)
      {
         const char *options = strchr(curr, ',');
         const char *next = strchr(curr, ';');

         char address[256];
         if ((options != nullptr)  && ((next == nullptr) || (options < next)))
         {
            size_t len = options - curr;
            strlcpy(address, curr, std::min(len + 1, sizeof(address)));
         }
         else if (next != nullptr)
         {
            size_t len = next - curr;
            strlcpy(address, curr, std::min(len + 1, sizeof(address)));
         }
         else
         {
            strlcpy(address, curr, sizeof(address));
         }
         TrimA(address);

         if (next == nullptr)
         {
            _ftprintf(fp, _T("ServerConnection = %hs\nMasterServers = %hs\n"), curr, address);
            break;
         }
         size_t len = next - curr;
         char temp[1024];
         strlcpy(temp, curr, std::min(len + 1, sizeof(temp)));
         TrimA(temp);
         if (temp[0] != 0)
            _ftprintf(fp, _T("ServerConnection = %hs\nMasterServers = %hs\n"), temp, address);
         curr = next + 1;
      }
   }
   else
   {
      // Replace ; with , in master server
      // ; are allowed as separators for for compatibility with tunnel mode
      char temp[4096];
      strlcpy(temp, masterServers, sizeof(temp));
      for (char *p = temp; *p != 0; p++)
         if (*p == ';')
            *p = ',';
      _ftprintf(fp, _T("MasterServers = %hs\n"), temp);
   }
   _ftprintf(fp, _T("ConfigIncludeDir = %hs\nLogFile = %hs\nFileStore = %hs\n"), configIncludeDir, logFile, fileStore);

   Array extSubAgents;
   for (int i = 0; i < numSubAgents; i++)
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

   for (int i = 0; i < extSubAgents.size(); i++)
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
         TrimA(curr);
         if ((*curr == '[') || (*curr == '*') || (strchr(curr, '=') != nullptr))
         {
            _ftprintf(fp, _T("%hs\n"), curr);
         }
         curr = next + 2;
      } while (next != nullptr);

      MemFree(temp);
   }

   fclose(fp);
   return 0;
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
   if (_taccess(g_szDataDirectory, F_OK) == 0)
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
   if (_taccess(oldAppDataDir, F_OK) != 0)
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
      _tcslcat(legacyConfigPolicyDir, FS_PATH_SEPARATOR, MAX_PATH);
   _tcslcat(legacyConfigPolicyDir, SUBDIR_CONFIG_POLICY, MAX_PATH);

   if (!_tcsicmp(legacyConfigPolicyDir, g_szConfigPolicyDir))
      return;

   if (_taccess(legacyConfigPolicyDir, F_OK) != 0)
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
bool LoadConfig(const TCHAR *configSection, const StringBuffer& cmdLineValues, bool firstStart, bool ignoreErrors)
{
   shared_ptr<Config> config = make_shared<Config>();
   config->setTopLevelTag(_T("config"));
   config->setAlias(_T("agent"), configSection);
   config->setLogErrors(!ignoreErrors);

   // Set default data directory
   if (!_tcscmp(g_szDataDirectory, _T("{default}")))
   {
#ifdef _WIN32
      if (SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, g_szDataDirectory) == S_OK)
      {
         _tcslcat(g_szDataDirectory, _T("\\nxagentd"), MAX_PATH);
         SetNetXMSDataDirectory(g_szDataDirectory);
      }
      else
      {
         GetNetXMSDirectory(nxDirData, g_szDataDirectory);
      }
#else
      GetNetXMSDirectory(nxDirData, g_szDataDirectory);
#endif
   }

   if (firstStart)
   {
      nxlog_set_debug_writer(DebugWriter);
      nxlog_set_debug_level(1);
   }
   bool validConfig = config->loadConfig(g_szConfigFile, DEFAULT_CONFIG_SECTION, nullptr, true);
   if (validConfig)
   {
      if (!cmdLineValues.isEmpty())
      {
         char *content = cmdLineValues.getUTF8String();
         config->loadIniConfigFromMemory(content, strlen(content), _T("<cmdline>"), DEFAULT_CONFIG_SECTION, true);
         MemFree(content);
      }

      const TCHAR *dir = config->getValue(_T("/%agent/DataDirectory"));
      if (dir != nullptr)
      {
         _tcslcpy(g_szDataDirectory, dir, MAX_PATH);
         SetNetXMSDataDirectory(g_szDataDirectory);
      }

      dir = config->getValue(_T("/%agent/ConfigIncludeDir"));
      if (dir != nullptr)
         _tcslcpy(g_szConfigIncludeDir, dir, MAX_PATH);

      validConfig = config->loadConfigDirectory(g_szConfigIncludeDir, DEFAULT_CONFIG_SECTION, nullptr, ignoreErrors);
      if (!validConfig && !ignoreErrors)
      {
         ConsolePrintf(_T("Error reading additional configuration files from \"%s\"\n"), g_szConfigIncludeDir);
      }

#ifdef _WIN32
      RecoverDataDirectory();
#endif

      _tcslcpy(g_szConfigPolicyDir, g_szDataDirectory, MAX_PATH - 16);
      if (g_szConfigPolicyDir[_tcslen(g_szConfigPolicyDir) - 1] != FS_PATH_SEPARATOR_CHAR)
         _tcslcat(g_szConfigPolicyDir, FS_PATH_SEPARATOR, MAX_PATH);
      _tcslcat(g_szConfigPolicyDir, SUBDIR_CONFIG_POLICY, MAX_PATH);
      if (_taccess(g_szConfigPolicyDir, F_OK) != 0)
      {
         // Check if configuration policies stored at old location
         RecoverConfigPolicyDirectory();
         CreateDirectoryTree(g_szConfigPolicyDir);
      }
      _tcscat(g_szConfigPolicyDir, FS_PATH_SEPARATOR);

      validConfig = config->loadConfigDirectory(g_szConfigPolicyDir, DEFAULT_CONFIG_SECTION);
      if (!validConfig && !ignoreErrors)
      {
         ConsolePrintf(_T("Error reading additional configuration files from \"%s\"\n"), g_szConfigPolicyDir);
      }
      if (ignoreErrors)
         validConfig = true;
   }
   else
   {
      ConsolePrintf(_T("Error loading configuration from \"%s\" section ") DEFAULT_CONFIG_SECTION _T("\n"), g_szConfigFile);
   }

   if (firstStart)
   {
      nxlog_set_debug_writer(nullptr);
      nxlog_set_debug_level(0);
   }

   g_config.set(config);
   return validConfig;
}
