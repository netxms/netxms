/* 
** nxupload - command line tool used to upload files to NetXMS agent
** Copyright (C) 2004-2023 Raden Solutions
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
** File: nxupload.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsrvapi.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxupload)

/**
 * Static fields
 */
static bool s_verbose = true;
static bool s_install = false;
static bool s_upgrade = false;
static TCHAR s_destinationFile[MAX_PATH] = _T("");
static TCHAR s_packageType[16] = _T("executable");
static TCHAR s_packageOptions[256] = _T("");
static NXCPStreamCompressionMethod s_compression = NXCP_STREAM_COMPRESSION_NONE;

/**
 * Do agent upgrade
 */
static int UpgradeAgent(AgentConnection *conn, const TCHAR *pszPkgName, RSA_KEY serverKey)
{
   bool connected = false;
   uint32_t rcc = conn->startUpgrade(pszPkgName);
   if (rcc == ERR_SUCCESS)
   {
      conn->disconnect();

      if (s_verbose)
      {
         _tprintf(_T("Agent upgrade started, waiting for completion...\n")
                  _T("[............................................................]\r["));
         fflush(stdout);
         for(int i = 0; i < 120; i += 2)
         {
            ThreadSleep(2);
            _puttc(_T('*'), stdout);
            fflush(stdout);
            if ((i % 10 == 0) && (i > 10))
            {
               if (conn->connect(serverKey))
               {
                  connected = true;
                  break;   // Connected successfully
               }
            }
         }
         _puttc(_T('\n'), stdout);
      }
      else
      {
         ThreadSleep(20);
         for(int i = 20; i < 120; i += 20)
         {
            ThreadSleep(20);
            if (conn->connect(serverKey))
            {
               connected = true;
               break;   // Connected successfully
            }
         }
      }

      // Last attempt to reconnect
      if (!connected)
         connected = conn->connect(serverKey);

      if (connected && s_verbose)
      {
         _tprintf(_T("Successfully established connection to agent after upgrade\n"));
      }
      else
      {
         _ftprintf(stderr, _T("Failed to establish connection to the agent after upgrade\n"));
      }
   }
   else
   {
      if (s_verbose)
         _ftprintf(stderr, _T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
   }

   return connected ? 0 : 1;
}

/**
 * Do package installation
 */
static int InstallPackage(AgentConnection *conn, const TCHAR *pszPkgName)
{
   uint32_t rcc = conn->installPackage(pszPkgName, s_packageType, s_packageOptions);
   if (s_verbose)
   {
      if (rcc == ERR_SUCCESS)
         _tprintf(_T("Package installation completed successfully\n"));
      else
         _ftprintf(stderr, _T("Cannot install package (%d: %s)\n"), rcc, AgentErrorCodeToText(rcc));
   }
   return (rcc == ERR_SUCCESS) ? 0 : 1;
}

/**
 * Upload progress callback
 */
static void ProgressCallback(size_t bytesTransferred)
{
	_tprintf(_T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b") UINT64_FMT_ARGS(_T("-16")), static_cast<uint64_t>(bytesTransferred));
}

/**
 * Process nxupload specific parameters
 */
static bool ParseAdditionalOptionCb(const char ch, const TCHAR *optarg)
{
   switch(ch)
   {
      case 'C':
        _tcslcpy(s_packageOptions, optarg, 256);
        break;
      case 'd':
        _tcslcpy(s_destinationFile, optarg, MAX_PATH);
        break;
      case 'i':   // Install package
         s_install = true;
         s_upgrade = false;
         break;
      case 'q':   // Quiet mode
         s_verbose = false;
         break;
      case 't':
        _tcslcpy(s_packageType, optarg, 16);
        break;
      case 'u':   // Upgrade agent
         s_upgrade = true;
         s_install = false;
         break;
      case 'z':
         s_compression = NXCP_STREAM_COMPRESSION_LZ4;
         break;
      case 'Z':
         s_compression = NXCP_STREAM_COMPRESSION_DEFLATE;
         break;
      default:
         break;
   }
   return true;
}

/**
 * Validate parameters count
 */
static bool IsArgMissingCb(int currentCount)
{
   return currentCount < 2;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, TCHAR **argv, int optind, RSA_KEY serverKey)
{
   int64_t elapsedTime = GetCurrentTimeMs();
   if (s_verbose)
      _tprintf(_T("Upload:                 "));
   uint32_t rcc = conn->uploadFile(argv[optind + 1], s_destinationFile[0] != 0 ? s_destinationFile : nullptr, false, s_verbose ? ProgressCallback : nullptr, s_compression);
   if (s_verbose)
      _tprintf(_T("\r                        \r"));
   elapsedTime = GetCurrentTimeMs() - elapsedTime;
   if (s_verbose)
   {
      if (rcc == ERR_SUCCESS)
      {
         uint64_t bytes = FileSize(argv[optind + 1]);
         _tprintf(_T("File transferred successfully\n") UINT64_FMT _T(" bytes in %d.%03d seconds (%.2f KB/sec)\n"),
               bytes, static_cast<int32_t>(elapsedTime / 1000), static_cast<int32_t>(elapsedTime % 1000),
               (static_cast<double>(static_cast<int64_t>(bytes) / 1024) / static_cast<double>(elapsedTime)) * 1000);
      }
      else
      {
         _tprintf(_T("%d: %s\n"), rcc, AgentErrorCodeToText(rcc));
      }
   }

   if (rcc != ERR_SUCCESS)
      return 1;

   if (s_upgrade)
      return UpgradeAgent(conn, argv[optind + 1], serverKey);

   if (s_install)
      return InstallPackage(conn, argv[optind + 1]);

   return 0;
}

/**
 * Startup
 */
#ifdef _WIN32
int _tmain(int argc, TCHAR *argv[])
#else
int main(int argc, char *argv[])
#endif
{
   ServerCommandLineTool tool;
   tool.argc = argc;
   tool.argv = argv;
   tool.displayName = _T("Agent File Upload Tool");
   tool.mainHelpText = _T("Usage: nxupload [<options>] <host> <file>\n")
                       _T("Tool specific options are:\n")
                       _T("   -C <options> : Set package deployment options or command line (depending on package type)\n")
                       _T("   -d <file>    : Fully qualified destination file name\n")
                       _T("   -i           : Start installation of uploaded package.\n")
                       _T("   -q           : Quiet mode.\n")
                       _T("   -t <type>    : Set package type (default is \"executable\").\n")
                       _T("   -u           : Start agent upgrade from uploaded package.\n")
                       _T("   -z           : Compress data stream with LZ4.\n")
                       _T("   -Z           : Compress data stream with DEFLATE.\n");
   tool.additionalOptions = "C:d:inqt:uzZ";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
