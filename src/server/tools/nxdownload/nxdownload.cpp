/* 
** nxdownload - command line tool used to download files from NetXMS agent
** Copyright (C) 2021-2023 Raden Solutions
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

NETXMS_EXECUTABLE_HEADER(nxdownload)

/**
 * Static fields
 */
static bool s_fingerprintMode = false;
static bool s_verbose = true;
static NXCPStreamCompressionMethod s_compression = NXCP_STREAM_COMPRESSION_NONE;

/**
 * Download progress callback
 */
static void ProgressCallback(size_t bytesTransferred)
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
      case 'F':   // Fingerprint mode
         s_fingerprintMode = true;
         break;
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
   return currentCount < 2;
}

/**
 * Get file fingerprints
 */
static int GetFileFingerprints(AgentConnection *conn, const TCHAR *file)
{
   RemoteFileFingerprint fp;
   uint32_t rcc = conn->getFileFingerprint(file, &fp);
   if (rcc != ERR_SUCCESS)
   {
      _tprintf(_T("%u: %s\n"), rcc, AgentErrorCodeToText(rcc));
      return 1;
   }

   TCHAR hashText[128];
   _tprintf(_T("Size.....: ") UINT64_FMT _T(" bytes\n"), fp.size);
   _tprintf(_T("CRC32....: %u (0x%08X)\n"), fp.crc32, fp.crc32);
   _tprintf(_T("MD5......: %s\n"), BinToStr(fp.md5, MD5_DIGEST_SIZE, hashText));
   _tprintf(_T("SHA256...: %s\n"), BinToStr(fp.sha256, SHA256_DIGEST_SIZE, hashText));

   if (fp.dataLength > 0)
   {
      _tprintf(_T("\nFirst %u bytes of file:\n"), static_cast<uint32_t>(fp.dataLength));
      for(size_t offset = 0; offset < fp.dataLength;)
      {
         _tprintf(_T("   %04X | "), static_cast<unsigned int>(offset));

         TCHAR text[17];
         int b;
         for(b = 0; (b < 16) && (offset < fp.dataLength); b++, offset++)
         {
            BYTE ch = fp.data[offset];
            _tprintf(_T("%02X "), ch);
            text[b] = ((ch < ' ') || (ch >= 127)) ? '.' : ch;
         }

         for(; b < 16; b++)
         {
            _tprintf(_T("   "));
            text[b] = ' ';
         }
         text[16] = 0;

         _tprintf(_T("| %s\n"), text);
      }
   }

   return 0;
}

/**
 * Execute command callback
 */
static int ExecuteCommandCb(AgentConnection *conn, int argc, TCHAR **argv, int optind, RSA_KEY serverKey)
{
   if (s_fingerprintMode)
      return GetFileFingerprints(conn, argv[optind + 1]);

   const TCHAR *localFile;
   if (argc - optind > 2)
   {
      localFile = argv[optind + 2];
   }
   else
   {
      TCHAR *remoteFile = argv[optind + 1];
      TCHAR *p = remoteFile + _tcslen(remoteFile);
      while((p > remoteFile) && (*p != '/') && (*p != '\\'))
         p--;
      if ((*p == '/') || (*p == '\\'))
         p++;
      localFile = p;
   }
   if (s_verbose)
      _tprintf(_T("Downloading to %s\n"), localFile);

   int64_t elapsedTime = GetMonotonicClockTime();
   if (s_verbose)
      _tprintf(_T("<Download>:                 "));
   uint32_t rcc = conn->downloadFile(argv[optind + 1], localFile, false, s_verbose ? ProgressCallback : nullptr, s_compression);
   if (s_verbose)
      _tprintf(_T("\r                        \r"));
   elapsedTime = GetMonotonicClockTime() - elapsedTime;
   if (s_verbose)
   {
      if (rcc == ERR_SUCCESS)
      {
         uint64_t fileSize = FileSize(argv[optind + 1]);
         _tprintf(_T("File transferred successfully\n") UINT64_FMT _T(" bytes in ") INT64_FMT _T(".%03d seconds (%.2f KB/sec)\n"),
                  fileSize,
                  elapsedTime / 1000,
                  (int32_t)(elapsedTime % 1000),
                  ((double)((int64_t)fileSize / 1024) / (double)elapsedTime) * 1000);
      }
      else
      {
         _tprintf(_T("%u: %s\n"), rcc, AgentErrorCodeToText(rcc));
      }
   }

   return rcc == ERR_SUCCESS ? 0 : 1;
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
   tool.displayName = _T("Agent File Download Tool");
   tool.mainHelpText = _T("Usage: nxdownload [<options>] <host> <path to source file> [<path to destination file>]\n")
                       _T("Tool specific options are:\n")
                       _T("   -F           : Get file fingerprints instead of download.\n")
                       _T("   -q           : Quiet mode.\n")
                       _T("   -z           : Compress data stream with LZ4.\n")
                       _T("   -Z           : Compress data stream with DEFLATE.\n");
   tool.additionalOptions = "FqzZ";
   tool.executeCommandCb = &ExecuteCommandCb;
   tool.parseAdditionalOptionCb = &ParseAdditionalOptionCb;
   tool.isArgMissingCb = &IsArgMissingCb;

   return ExecuteServerCommandLineTool(&tool);
}
