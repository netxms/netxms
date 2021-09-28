/* 
** nxupload - command line tool used to upload files to NetXMS agent
** Copyright (C) 2004-2021 Raden Solutions
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
static bool s_doNotSplit = false;
static bool s_verbose = true;
static bool s_upgrade = false;
static TCHAR s_destinationFile[MAX_PATH] = {0};
static NXCPStreamCompressionMethod s_compression = NXCP_STREAM_COMPRESSION_NONE;

/**
 * Do agent upgrade
 */
static int UpgradeAgent(AgentConnection *conn, const TCHAR *pszPkgName, RSA *serverKey)
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
            if ((i % 20 == 0) && (i > 30))
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
 * Upload progress callback
 */
static void ProgressCallback(size_t bytesTransferred, void *cbArg)
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
      case 'd':
        _tcslcpy(s_destinationFile, optarg, MAX_PATH);
        break;
      case 'n':   // Do not split file into chunks
         s_doNotSplit = true;
         break;
      case 'q':   // Quiet mode
         s_verbose = false;
         break;
      case 'u':   // Upgrade agent
         s_upgrade = true;
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
static int ExecuteCommandCb(AgentConnection *conn, int argc, TCHAR **argv, int optind, RSA *pServerKey)
{
   uint32_t dwError;
   int exitCode;
   int64_t nElapsedTime;

   nElapsedTime = GetCurrentTimeMs();
   if (s_verbose)
      _tprintf(_T("Upload:                 "));
   dwError = conn->uploadFile(argv[optind + 1], s_destinationFile[0] != 0 ? s_destinationFile : NULL, false, s_verbose ? ProgressCallback : NULL, NULL, s_compression, s_doNotSplit);
   if (s_verbose)
      _tprintf(_T("\r                        \r"));
   nElapsedTime = GetCurrentTimeMs() - nElapsedTime;
   if (s_verbose)
   {
      if (dwError == ERR_SUCCESS)
      {
         QWORD qwBytes;

         qwBytes = FileSize(argv[optind + 1]);
         _tprintf(_T("File transferred successfully\n") UINT64_FMT _T(" bytes in %d.%03d seconds (%.2f KB/sec)\n"),
                  qwBytes, (LONG)(nElapsedTime / 1000),
                  (LONG)(nElapsedTime % 1000),
                  ((double)((INT64)qwBytes / 1024) / (double)nElapsedTime) * 1000);
      }
      else
      {
         _tprintf(_T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
      }
   }

   if (s_upgrade && (dwError == RCC_SUCCESS))
   {
      exitCode = UpgradeAgent(conn, argv[optind + 1], pServerKey);
   }
   else
   {
      exitCode = (dwError == ERR_SUCCESS) ? 0 : 1;
   }
   return exitCode;
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
   tool.mainHelpText = _T("Usage: nxupload [<options>] <host> <file>\n")
                       _T("Tool specific options are:\n")
                       _T("   -d <file>    : Fully qualified destination file name\n")
                       _T("   -n           : Do not split file into chunks.\n")
                       _T("   -q           : Quiet mode.\n")
                       _T("   -u           : Start agent upgrade from uploaded package.\n")
                       _T("   -z           : Compress data stream with LZ4.\n")
                       _T("   -Z           : Compress data stream with DEFLATE.\n");
   tool.additionalOptions = "d:nquzZ";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
