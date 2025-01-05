/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005-2025 Victor Kirhenshtein
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
** File: nxmibc.cpp
**
**/

#include "nxmibc.h"
#include <netxms_getopt.h>
#include <netxms-version.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <nxstat.h>
#endif

NETXMS_EXECUTABLE_HEADER(nxmibc)

/**
 * Severity codes
 */
#define MIBC_INFO      0
#define MIBC_WARNING   1
#define MIBC_ERROR     2

/**
 * Externals
 */
int ParseMIBFiles(StringList *fileList, SNMP_MIBObject **rootObject);

/**
 * Terminal output indicator (if set, output will be colorized)
 */
bool g_terminalOutput = true;

/**
 * machine parseable output indicator
 */
bool g_machineParseableOutput = false;

/**
 * Static data
 */
static char s_outputFileName[MAX_PATH] = "netxms.cmib";
static StringList s_fileList;
static bool s_pauseBeforeExit = false;
static bool s_continueAfterError = false;

/**
 * Pause if needed
 */
static void Pause()
{
   if (s_pauseBeforeExit)
   {
#ifdef _WIN32
      _tprintf(_T("Press any key to continue...\n"));
      _getch();
#else
      _tprintf(_T("Press ENTER to continue...\n"));
      char temp[256];
      fgets(temp, 255, stdin);
#endif
   }
}

/**
 * Errors
 */
static struct
{
   int severity;
   const TCHAR *text;
} s_errors[] =
{
   { MIBC_INFO, _T("Operation completed successfully") },
   { MIBC_ERROR, _T("Import symbol \"%hs\" unresolved") },
   { MIBC_ERROR, _T("Import module \"%hs\" unresolved") },
   { MIBC_ERROR, _T("Parser error - %hs in line %d") },
   { MIBC_ERROR, _T("Cannot open input file (%hs)") },
   { MIBC_ERROR, _T("Cannot resolve symbol %hs") },
   { MIBC_WARNING, _T("Cannot resolve data type \"%hs\" for object \"%hs\"") },
   { MIBC_WARNING, _T("Object identifier started with uppercase letter in line %d") },
   { MIBC_WARNING, _T("Dangling comma character in line %d") }
};

/**
 * Display error text and abort compilation
 */
void ReportError(int error, const char *module, ...)
{
   static const TCHAR *severityText[] = { _T("\x1b[32;1minfo   "), _T("\x1b[33;1mwarning"), _T("\x1b[31;1merror  ") };
   static TCHAR severityPrefix[] = { _T('I'), _T('W'), _T('E') };

   va_list args;
   va_start(args, module);
   TCHAR message[256];
   _vsntprintf(message, 256, s_errors[error].text, args);
   va_end(args);

   if (g_machineParseableOutput)
      _tprintf(_T("%c:%d:{%hs}:%s\n"), severityPrefix[s_errors[error].severity], error, module, message);
   else
      WriteToTerminalEx(_T("[%s\x1b[0m] \x1b[36m%hs\x1b[0m : %s\n"), severityText[s_errors[error].severity], module, message);

   if (!s_continueAfterError && (s_errors[error].severity == MIBC_ERROR))
	{
		Pause();
      exit(1);
	}
}

/**
 * Display help and exit
 */
static void Help()
{
   _tprintf(_T("Usage:\n\n")
		      _T("nxmibc [options] source1 ... sourceN\n\n")
		      _T("Valid options:\n")
		      _T("   -a        : Compile all input files (continue after file parsing errors)\n")
		      _T("   -d <dir>  : Include all MIB files from given directory to compilation\n")
            _T("   -e <ext>  : Specify file extensions (default extension: \"mib\")\n")
            _T("   -m        : Produce machine-readable output\n")
		      _T("   -o <file> : Set output file name (default is netxms.mib)\n")
		      _T("   -P        : Pause before exit\n")
            _T("   -r        : Scan sub-directories\n")
		      _T("   -s        : Strip descriptions from MIB objects\n")
            _T("   -u        : Do not compress output file\n")
		      _T("   -z        : Compress output file\n")
		      _T("Note: compression is ON by default, so option -z effectively does nothing\n")
		      _T("      and left only for backward compatibility.\n")
		      _T("\n"));
   exit(0);
}

/**
 * Check if given file entry is a directory
 */
static bool IsDirectory(struct _tdirent *d, const TCHAR *filePath)
{
#if HAVE_DIRENT_D_TYPE
   return d->d_type == DT_DIR;
#else
   NX_STAT_STRUCT st;
   if (CALL_STAT(filePath, &st) == -1)
      return false;
   return S_ISDIR(st.st_mode);
#endif
}

/**
 * Scan directory for MIB files
 */
static void ScanDirectory(const TCHAR *path, const StringSet *extensions, bool recursive)
{
   _TDIR *dir = _topendir(path);
   if (dir == nullptr)
      return;

   while(true)
   {
      struct _tdirent *file = _treaddir(dir);
      if (file == nullptr)
         break;

      if (_tcscmp(file->d_name, _T(".")) && _tcscmp(file->d_name, _T("..")))
      {
         TCHAR filePath[MAX_PATH];
         _sntprintf(filePath, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("%s"), path, file->d_name);
         if (recursive && IsDirectory(file, filePath))
         {
            ScanDirectory(filePath, extensions, recursive);
         }
         else
         {
            TCHAR *extension = _tcsrchr(file->d_name, _T('.'));
            if (extension != nullptr)
            {
               _tcslwr(extension);
               if (extensions->contains(extension + 1))
                  s_fileList.add(filePath);
            }
         }
      }
   }
   _tclosedir(dir);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
   bool recursive = false;
   bool scanDir = false;
   uint32_t flags = SMT_COMPRESS_DATA;
   int rc = 0;

   InitNetXMSProcess(true);

   g_terminalOutput = IsOutputToTerminal();

   StringList paths;
   StringSet extensions;
   extensions.add(_T("mib"));

   // Parse command line
   opterr = 1;
   int ch;
   while((ch = getopt(argc, argv, "ad:e:hmo:Prsuz")) != -1)
   {
      switch(ch)
      {
         case 'a':
            s_continueAfterError = true;
            break;
         case 'e':
#ifdef UNICODE
            extensions.addPreallocated(wcslwr(WideStringFromMBStringSysLocale(optarg)));
#else
            extensions.add(strlwr(optarg));
#endif
            break;
         case 'h':   // Display help and exit
            WriteToTerminal(
               _T("NetXMS MIB Compiler  Version ") NETXMS_VERSION_STRING _T(" (") NETXMS_BUILD_TAG _T(")\n")
               _T("Copyright (c) 2005-2025 Raden Solutions\n\n"));
            Help();
            break;
         case 'd':
#ifdef UNICODE
            paths.addPreallocated(WideStringFromMBStringSysLocale(optarg));
#else
            paths.add(optarg);
#endif
            scanDir = true;
            break;
         case 'm':
            g_machineParseableOutput = true;
            g_terminalOutput = false;
            break;
         case 'o':
            strlcpy(s_outputFileName, optarg, MAX_PATH);
            break;
			case 'P':
            s_pauseBeforeExit = true;
            break;
         case 'r':
            recursive = true;
            break;
         case 's':
            flags |= SMT_SKIP_DESCRIPTIONS;
            break;
         case 'u':
            flags &= ~SMT_COMPRESS_DATA;
            break;
         case 'z':
            flags |= SMT_COMPRESS_DATA;
            break;
         case '?':
            return 255;
         default:
            break;
      }
   }

   if (!g_machineParseableOutput)
   {
      WriteToTerminal(
         _T("NetXMS MIB Compiler  Version ") NETXMS_VERSION_STRING _T(" (") NETXMS_BUILD_TAG _T(")\n")
         _T("Copyright (c) 2005-2025 Raden Solutions\n\n"));
   }

   if (scanDir)
   {
      for(int i = 0; i < paths.size(); i++)
      {
         ScanDirectory(paths.get(i), &extensions, recursive);
      }
   }

   for(int i = optind; i < argc; i++)
   {
#ifdef UNICODE
      s_fileList.addPreallocated(WideStringFromMBStringSysLocale(argv[i]));
#else
      s_fileList.add(argv[i]);
#endif
   }

   if (s_fileList.size() > 0)
   {
      SNMP_MIBObject *rootObject;
      ParseMIBFiles(&s_fileList, &rootObject);
      if (rootObject != nullptr)
      {
#ifdef UNICODE
			WCHAR *wname = WideStringFromMBStringSysLocale(s_outputFileName);
         uint32_t status = SnmpSaveMIBTree(wname, rootObject, flags);
			MemFree(wname);
#else
			uint32_t status = SnmpSaveMIBTree(s_outputFileName, rootObject, flags);
#endif
         delete rootObject;
         if (status == SNMP_ERR_SUCCESS)
         {
            if (g_machineParseableOutput)
               _tprintf(_T("S:0:Success\n"));
            else
               WriteToTerminalEx(_T("\x1b[32;1mCOMPLETED\x1b[0m\n"), s_outputFileName, SnmpGetErrorText(status));
         }
         else
         {
            if (g_machineParseableOutput)
               _tprintf(_T("S:%d:%s\n"), status, SnmpGetErrorText(status));
            else
               WriteToTerminalEx(_T("\x1b[31;1mERROR\x1b[0m: Cannot save output file %hs (%s)\n"), s_outputFileName, SnmpGetErrorText(status));
            rc = 1;
         }
      }
   }
   else
   {
      if (g_machineParseableOutput)
         _tprintf(_T("S:99:No source files\n"));
      else
         WriteToTerminalEx(_T("\x1b[31;1mERROR\x1b[0m: No source files given\n"));
      rc = 1;
   }

	Pause();
   return rc;
}
