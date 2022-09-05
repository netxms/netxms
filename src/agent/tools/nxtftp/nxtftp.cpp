/* 
** nxtftp - simple TFTP client
** Copyright (C) 2006-2022 Raden Solutions
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
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>
#include <netxms_getopt.h>
#include <nxstat.h>
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxtftp)

static const TCHAR *s_usage =
   _T("Usage:\n")
   _T("   nxtftp [-D] [-p port] [-r remote_file ] command server file\n\n")
   _T("Valid commands are:\n")
   _T("   get - get file from server\n")
   _T("   put - put file to server\n\n");

/**
 * Debug writer
 */
static void DebugWriter(const TCHAR *tag, const TCHAR *format, va_list args)
{
   if (tag != nullptr)
      _tprintf(_T("%s: "), tag);
   _vtprintf(format, args);
   _tprintf(_T("\n"));
}

/**
 * Upload file to server
 */
static int UploadFile(const InetAddress& addr, uint16_t port, const char *localFile, const TCHAR *remoteFile)
{
#ifdef UNICODE
   WCHAR sourceFile[MAX_PATH];
   MultiByteToWideCharSysLocale(localFile, sourceFile, MAX_PATH);
#else
   const char *sourceFile = localFile;
#endif
   TFTPError rc = TFTPWrite(sourceFile, remoteFile, addr, port,
      [] (size_t bytesTotal) -> void
      {
         WriteToTerminalEx(_T("\rWriting: %6u bytes"), static_cast<uint32_t>(bytesTotal));
      });
   WriteToTerminal(_T("\r                                                \r"));

   if (rc == TFTP_SUCCESS)
   {
      WriteToTerminal(_T("File transfer completed\n"));
      return 0;
   }

   WriteToTerminalEx(_T("File transfer failed (%s)\n"), TFTPErrorMessage(rc));
   return 3;
}

/**
 * Download file from server
 */
static int DownloadFile(const InetAddress& addr, uint16_t port, const char *localFile, const TCHAR *remoteFile)
{
#ifdef UNICODE
   WCHAR sourceFile[MAX_PATH];
   MultiByteToWideCharSysLocale(localFile, sourceFile, MAX_PATH);
#else
   const char *sourceFile = localFile;
#endif
   TFTPError rc = TFTPRead(sourceFile, remoteFile, addr, port,
      [] (size_t bytesTotal) -> void
      {
         WriteToTerminalEx(_T("\rReading: %6u bytes"), static_cast<uint32_t>(bytesTotal));
      });
   WriteToTerminal(_T("\r                                                \r"));

   if (rc == TFTP_SUCCESS)
   {
      WriteToTerminal(_T("File transfer completed\n"));
      return 0;
   }

   WriteToTerminalEx(_T("File transfer failed (%s)\n"), TFTPErrorMessage(rc));
   return 3;
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   uint16_t port = 69;
   TCHAR *remoteFile = nullptr;

   InitNetXMSProcess(true);

   int ch;
   while((ch = getopt(argc, argv, "Dhp:r:")) != -1)
   {
      switch(ch)
      {
         case 'D':
            nxlog_set_debug_level(9);
            nxlog_set_debug_writer(DebugWriter);
            break;
         case 'p':
            port = static_cast<uint16_t>(strtol(optarg, nullptr, 10));
            break;
         case 'r':
#ifdef UNICODE
            remoteFile = WideStringFromMBStringSysLocale(optarg);
#else
            remoteFile = optarg;
#endif
            break;
         default:
            _fputts(s_usage, stdout);
            return 1;
      }
   }

   if (argc - optind < 3)
   {
      _fputts(s_usage, stdout);
      return 1;
   }

#ifdef _WIN32
   WSADATA wsaData;
   int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
   if (wrc != 0)
   {
      TCHAR buffer[1024];
      _tprintf(_T("Call to WSAStartup() failed (%s)"), GetLastSocketErrorText(buffer, 1024));
      return FALSE;
   }
#endif

   InetAddress addr = InetAddress::resolveHostName(argv[optind + 1]);
   if (!addr.isValidUnicast() && !addr.isLoopback())
   {
      _tprintf(_T("Host name %hs cannot be resolved or invalid\n"), argv[optind + 1]);
      return 2;
   }

   if (!stricmp(argv[optind], "put"))
   {
      return UploadFile(addr, port, argv[optind + 2], remoteFile);
   }
   else if (!stricmp(argv[optind], "get"))
   {
      return DownloadFile(addr, port, argv[optind + 2], remoteFile);
   }

   _tprintf(_T("Invalid command %hs\n"), argv[optind]);
   return 1;
}
