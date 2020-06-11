/* 
** nxupload - command line tool used to upload files to NetXMS agent
** Copyright (C) 2004-2020 Raden Solutions
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

#ifndef _WIN32
#include <netdb.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxupload)

/**
 * Static fields
 */
static bool s_verbose = true, s_upgrade = false;
static TCHAR s_destinationFile[MAX_PATH] = {0};
static NXCPStreamCompressionMethod s_compression = NXCP_STREAM_COMPRESSION_NONE;

/**
 * Do agent upgrade
 */
static int UpgradeAgent(AgentConnection *conn, TCHAR *pszPkgName, BOOL bVerbose, RSA *serverKey)
{
   UINT32 dwError;
   int i;
   BOOL bConnected = FALSE;

   dwError = conn->startUpgrade(pszPkgName);
   if (dwError == ERR_SUCCESS)
   {
      conn->disconnect();

      if (bVerbose)
      {
         _tprintf(_T("Agent upgrade started, waiting for completion...\n")
                  _T("[............................................................]\r["));
         fflush(stdout);
         for(i = 0; i < 120; i += 2)
         {
            ThreadSleep(2);
            _puttc(_T('*'), stdout);
            fflush(stdout);
            if ((i % 20 == 0) && (i > 30))
            {
               if (conn->connect(serverKey, FALSE))
               {
                  bConnected = TRUE;
                  break;   // Connected successfully
               }
            }
         }
         _puttc(_T('\n'), stdout);
      }
      else
      {
         ThreadSleep(20);
         for(i = 20; i < 120; i += 20)
         {
            ThreadSleep(20);
            if (conn->connect(serverKey, FALSE))
            {
               bConnected = TRUE;
               break;   // Connected successfully
            }
         }
      }

      // Last attempt to reconnect
      if (!bConnected)
         bConnected = conn->connect(serverKey, FALSE);

      if (bConnected && bVerbose)
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
      if (bVerbose)
         _ftprintf(stderr, _T("%d: %s\n"), dwError, AgentErrorCodeToText(dwError));
   }

   return bConnected ? 0 : 1;
}

/**
 * Upload progress callback
 */
static void ProgressCallback(INT64 bytesTransferred, void *cbArg)
{
#ifdef _WIN32
	_tprintf(_T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%-16I64d"), bytesTransferred);
#else
	_tprintf(_T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b%-16lld"), bytesTransferred);
#endif
}

/**
 * Process nxuload specifuc parameters
 */
static bool ParseAdditionalOptionCb(const char ch, const char *optarg)
{
   switch(ch)
   {
      case 'd':
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, optarg, -1, s_destinationFile, MAX_PATH);
        s_destinationFile[MAX_PATH - 1] = 0;
#else
        strlcpy(s_destinationFile, optarg, MAX_PATH);
#endif
        break;
      case 'q':   // Quiet mode
         s_verbose = FALSE;
         break;
      case 'u':   // Upgrade agent
         s_upgrade = TRUE;
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
 * Validate parametercount
 */
static bool IsArgMissingCb(int currentCount)
{
   return currentCount < 2;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, char *argv[], RSA *pServerKey)
{
   UINT32 dwError;
   int exitCode;
   INT64 nElapsedTime;

#ifdef UNICODE
   WCHAR fname[MAX_PATH];
   MultiByteToWideCharSysLocale(argv[optind + 1], fname, MAX_PATH);
   fname[MAX_PATH - 1] = 0;
#else
#define fname argv[optind + 1]
#endif
   nElapsedTime = GetCurrentTimeMs();
   if (s_verbose)
      _tprintf(_T("Upload:                 "));
   dwError = conn->uploadFile(fname, s_destinationFile[0] != 0 ? s_destinationFile : NULL, false, s_verbose ? ProgressCallback : NULL, NULL, s_compression);
   if (s_verbose)
      _tprintf(_T("\r                        \r"));
   nElapsedTime = GetCurrentTimeMs() - nElapsedTime;
   if (s_verbose)
   {
      if (dwError == ERR_SUCCESS)
      {
         QWORD qwBytes;

         qwBytes = FileSize(fname);
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
      exitCode = UpgradeAgent(conn, fname, s_verbose, pServerKey);
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
int main(int argc, char *argv[])
{
   ServerCommandLineTool tool;
   tool.argc = argc;
   tool.argv = argv;
   tool.mainHelpText = _T("Usage: nxupload [<options>] <host> <file>\n")
                       _T("Tool specific options are:\n")
                       _T("   -d <file>    : Fully qualified destination file name\n")
                       _T("   -q           : Quiet mode.\n")
                       _T("   -u           : Start agent upgrade from uploaded package.\n")
                       _T("   -z           : Compress data stream with LZ4.\n")
                       _T("   -Z           : Compress data stream with DEFLATE.\n");
   tool.additionalOptions = "d:quzZ";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
