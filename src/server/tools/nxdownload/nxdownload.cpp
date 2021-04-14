/* 
** nxdownload - command line tool used to download files from NetXMS agent
** Copyright (C) 2021 Raden Solutions
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
** File: nxdownload.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <nxsrvapi.h>
#include <netxms-version.h>

#ifndef _WIN32
#include <netdb.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxdownload)

/**
 * Static fields
 */
static bool s_verbose = true;
static NXCPStreamCompressionMethod s_compression = NXCP_STREAM_COMPRESSION_NONE;

/**
 * Download progress callback
 */
static void ProgressCallback(size_t bytesTransferred, void *cbArg)
{
	_tprintf(_T("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b") UINT64_FMT_ARGS(_T("-16")), bytesTransferred);
}

/**
 * Process nxuload specific parameters
 */
static bool ParseAdditionalOptionCb(const char ch, const TCHAR *optarg)
{
   switch(ch)
   {
      case 'q':   // Quiet mode
         s_verbose = false;
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
   return currentCount < 3;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, TCHAR **argv, int optind, RSA *pServerKey)
{
   int64_t nElapsedTime = GetCurrentTimeMs();
   if (s_verbose)
      _tprintf(_T("<Download>:                 "));
   uint32_t dwError = conn->downloadFile(argv[optind + 1], argv[optind + 2], false, s_verbose ? ProgressCallback : nullptr, nullptr, s_compression);
   if (s_verbose)
      _tprintf(_T("\r                        \r"));
   nElapsedTime = GetCurrentTimeMs() - nElapsedTime;
   if (s_verbose)
   {
      if (dwError == ERR_SUCCESS)
      {
         uint64_t fileSize = FileSize(argv[optind + 1]);
         _tprintf(_T("File transferred successfully\n") UINT64_FMT _T(" bytes in ") INT64_FMT _T(".%03d seconds (%.2f KB/sec)\n"),
                  fileSize,
                  nElapsedTime / 1000,
                  (int32_t)(nElapsedTime % 1000),
                  ((double)((int64_t)fileSize / 1024) / (double)nElapsedTime) * 1000);
      }
      else
      {
         _tprintf(_T("%u: %s\n"), dwError, AgentErrorCodeToText(dwError));
      }
   }

   return dwError == ERR_SUCCESS ? 0 : 1;
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
   tool.mainHelpText = _T("Usage: nxdownload [<options>] <host> <path to source file> <path to destination file>\n")
                       _T("Tool specific options are:\n")
                       _T("   -q           : Quiet mode.\n")
                       _T("   -z           : Compress data stream with LZ4.\n")
                       _T("   -Z           : Compress data stream with DEFLATE.\n");
   tool.additionalOptions = "qzZ";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}