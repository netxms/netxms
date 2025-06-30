/* 
** nxhwid - diplay local system hardware ID
** Copyright (C) 2006-2025 Raden Solutions
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
#include <netxms-version.h>

NETXMS_EXECUTABLE_HEADER(nxhwid)

#if defined(__FreeBSD__)
#include "../../smbios/freebsd.cpp"
#elif defined(__linux__)
#include "../../smbios/linux.cpp"
#elif defined(__sun)
#include "../../smbios/solaris.cpp"
#elif defined(_WIN32)
#include "../../smbios/windows.cpp"
#else
static BYTE *SMBIOS_Reader(size_t *size)
{
   return nullptr;
}
#endif

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
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   int ch;
   while((ch = getopt(argc, argv, "Dh")) != -1)
   {
      if (ch == 'D')
      {
         nxlog_set_debug_level(9);
         nxlog_set_debug_writer(DebugWriter);
      }
      else
      {
         _tprintf(_T("Usage: nxhwid [-D]\n"));
         return 1;
      }
   }

   SMBIOS_Parse(SMBIOS_Reader);

   int rc = 0;
   BYTE hwid[HARDWARE_ID_LENGTH];
   if (GetSystemHardwareId(hwid))
   {
      TCHAR buffer[128];
      _tprintf(_T("Hardware ID: %s\n"), BinToStr(hwid, sizeof(hwid), buffer));
   }
   else
   {
      _tprintf(_T("Hardware ID: n/a\n"));
      rc = 1;
   }

   BYTE sysid[SYSTEM_ID_LENGTH];
   if (GetUniqueSystemId(sysid))
   {
      TCHAR buffer[128];
      _tprintf(_T("System ID:   %s\n"), BinToStr(sysid, sizeof(sysid), buffer));
   }
   else
   {
      _tprintf(_T("System ID:   n/a\n"));
      rc = 1;
   }
   
   return rc;
}
