/*
** nxminfo - NetXMS module info tool
** Copyright (C) 2016-2024 Raden Solutions
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
#include <nms_util.h>
#include <netxms-version.h>
#include <nms_core.h>
#include <nxmodule.h>

NETXMS_EXECUTABLE_HEADER(nxminfo)

/**
 * Module metadata version 1
 */
struct NXMODULE_METADATA_V1
{
   uint32_t size;   // structure size in bytes
   int unicode;  // unicode flag
   char name[MAX_OBJECT_NAME];
   char vendor[128];
   char coreVersion[16];
   char coreBuildTag[32];
   char moduleVersion[16];
   char moduleBuildTag[32];
   char compiler[256];
};

/**
 * Check if entry point is present
 */
#define CHECK_ENTRY_POINT(p) WriteToTerminalEx(L"   %-20hs : %s\n", p, (DLGetSymbolAddr(handle, p, nullptr) != nullptr) ? L"yes" : L"no")

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   WriteToTerminal(L"NetXMS Module Info Tool Version " NETXMS_VERSION_STRING L" Build " NETXMS_BUILD_TAG L"\n\n");
   if (argc != 2)
   {
      WriteToTerminal(L"Usage: nxminfo <module>\n");
      return 1;
   }

   wchar_t *fname = WideStringFromMBStringSysLocale(argv[1]);

   TCHAR modname[MAX_PATH];
#if !defined(_WIN32)
   if (wcschr(fname, L'/') == nullptr)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      TCHAR libdir[MAX_PATH];
      GetNetXMSDirectory(nxDirLib, libdir);
      _sntprintf(modname, MAX_PATH, _T("%s/%s"), libdir, fname);
   }
   else
   {
      wcslcpy(modname, fname, MAX_PATH);
   }
#else
   wcslcpy(modname, fname, MAX_PATH);
#endif

   size_t len = wcslen(modname);
   if ((len < 4) || (wcsicmp(&modname[len - 4], L".nxm") && wcsicmp(&modname[len - wcslen(SHLIB_SUFFIX)], SHLIB_SUFFIX)))
      wcslcat(modname, L".nxm", MAX_PATH);

   wchar_t errorText[1024];
   HMODULE handle = DLOpen(modname, errorText);
   if (handle == nullptr)
   {
      WriteToTerminalEx(L"Cannot load module (%s)\n", errorText);
      return 2;
   }

   auto metadata = static_cast<NXMODULE_METADATA*>(DLGetSymbolAddr(handle, "NXM_metadata", errorText));
   if (metadata == nullptr)
   {
      WriteToTerminalEx(L"Cannot locate module metadata (%s)\n", errorText);
      return 3;
   }

   if (sizeof(NXMODULE_METADATA) != metadata->size)
   {
      WriteToTerminalEx(L"WARNING: module metadata size is different from what is expected by server (%d/%d)\n\n", metadata->size, (int)sizeof(NXMODULE_METADATA));
   }

   NXMODULE_METADATA module;
   memset(&module, 0, sizeof(NXMODULE_METADATA));
   memcpy(&module, metadata, std::min(static_cast<size_t>(metadata->size), sizeof(NXMODULE_METADATA)));

   if (strcmp(module.tagBegin, "$$$NXMINFO>$$$"))
   {
      memcpy(module.name, ((NXMODULE_METADATA_V1 *)metadata)->name, std::min(static_cast<size_t>(metadata->size) - 8, sizeof(NXMODULE_METADATA) - 24));
   }

   WriteToTerminalEx(L"Module:         %s\n", modname);
   WriteToTerminalEx(L"Name:           %hs\n", module.name);
   WriteToTerminalEx(L"Vendor:         %hs\n", module.vendor);
   WriteToTerminalEx(L"Version:        %hs\n", module.moduleVersion);
   WriteToTerminalEx(L"Build tag:      %hs\n", module.moduleBuildTag);
   WriteToTerminalEx(L"Core version:   %hs\n", module.coreVersion);
   WriteToTerminalEx(L"Core build tag: %hs\n", module.coreBuildTag);
   WriteToTerminalEx(L"Compiler:       %hs\n", module.compiler);
   WriteToTerminalEx(L"UNICODE build:  %s\n", module.unicode ? L"yes" : L"no");

   WriteToTerminal(L"\nEntry points:\n");
   CHECK_ENTRY_POINT("NXM_Register");
   CHECK_ENTRY_POINT("NXM_CheckDB");
   CHECK_ENTRY_POINT("NXM_CheckDBVersion");
   CHECK_ENTRY_POINT("NXM_UpgradeDB");
   CHECK_ENTRY_POINT("NXM_GetTables");
   CHECK_ENTRY_POINT("NXM_GetSchemaPrefix");

   DLClose(handle);
   MemFree(fname);
   return 0;
}
