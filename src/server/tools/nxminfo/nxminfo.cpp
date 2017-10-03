/*
** nxminfo - NetXMS module info tool
** Copyright (C) 2016-2017 Raden Solutions
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

/**
 * Module metadata version 1
 */
struct NXMODULE_METADATA_V1
{
   UINT32 size;   // structure size in bytes
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
#define CHECK_ENTRY_POINT(p) _tprintf(_T("   %-16hs : %s\n"), p, (DLGetSymbolAddr(handle, p, NULL) != NULL) ? _T("yes") : _T("no"))

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   _tprintf(_T("NetXMS Module Info Tool Version ") NETXMS_VERSION_STRING _T(" Build ") NETXMS_BUILD_TAG IS_UNICODE_BUILD_STRING _T("\n\n"));
   if (argc != 2)
   {
      _tprintf(_T("Usage: nxminfo <module>\n"));
      return 1;
   }

#ifdef UNICODE
   WCHAR *fname = WideStringFromMBStringSysLocale(argv[1]);
#else
   char *fname = argv[1];
#endif

#if !defined(_WIN32)
   TCHAR modname[MAX_PATH];
   if (_tcschr(fname, _T('/')) == NULL)
   {
      // Assume that module name without path given
      // Try to load it from pkglibdir
      const TCHAR *homeDir = _tgetenv(_T("NETXMS_HOME"));
      if (homeDir != NULL)
      {
         _sntprintf(modname, MAX_PATH, _T("%s/lib/netxms/%s"), homeDir, fname);
      }
      else
      {
         _sntprintf(modname, MAX_PATH, _T("%s/%s"), PKGLIBDIR, fname);
      }
   }
   else
   {
      nx_strncpy(modname, fname, MAX_PATH);
   }
#else
#define modname fname
#endif

   TCHAR errorText[1024];
   HMODULE handle = DLOpen(modname, errorText);
   if (handle == NULL)
   {
      _tprintf(_T("Cannot load module (%s)\n"), errorText);
      return 2;
   }

   NXMODULE_METADATA *metadata = (NXMODULE_METADATA *)DLGetSymbolAddr(handle, "NXM_metadata", errorText);
   if (metadata == NULL)
   {
      _tprintf(_T("Cannot locate module metadata (%s)\n"), errorText);
      return 3;
   }

   if (sizeof(NXMODULE_METADATA) != metadata->size)
   {
      _tprintf(_T("WARNING: module metadata size is different from what is expected by server (%d/%d)\n\n"), metadata->size, (int)sizeof(NXMODULE_METADATA));
   }

   NXMODULE_METADATA module;
   memset(&module, 0, sizeof(NXMODULE_METADATA));
   memcpy(&module, metadata, std::min(static_cast<size_t>(metadata->size), sizeof(NXMODULE_METADATA)));

   if (strcmp(module.tagBegin, "$$$NXMINFO>$$$"))
   {
      memcpy(module.name, ((NXMODULE_METADATA_V1 *)metadata)->name, std::min(static_cast<size_t>(metadata->size) - 8, sizeof(NXMODULE_METADATA) - 24));
   }

   _tprintf(_T("Module:         %s\n"), modname);
   _tprintf(_T("Name:           %hs\n"), module.name);
   _tprintf(_T("Vendor:         %hs\n"), module.vendor);
   _tprintf(_T("Version:        %hs\n"), module.moduleVersion);
   _tprintf(_T("Build tag:      %hs\n"), module.moduleBuildTag);
   _tprintf(_T("Core version:   %hs\n"), module.coreVersion);
   _tprintf(_T("Core build tag: %hs\n"), module.coreBuildTag);
   _tprintf(_T("Compiler:       %hs\n"), module.compiler);
   _tprintf(_T("UNICODE build:  %s\n"), module.unicode ? _T("yes") : _T("no"));

   _tprintf(_T("\nEntry points:\n"));
   CHECK_ENTRY_POINT("NXM_Init");
   CHECK_ENTRY_POINT("NXM_CheckDB");
   CHECK_ENTRY_POINT("NXM_UpgradeDB");
   CHECK_ENTRY_POINT("NXM_GetTables");

   DLClose(handle);
#ifdef UNICODE
   free(fname);
#endif
   return 0;
}
