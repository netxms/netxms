/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2013-2024 Victor Kirhenshtein
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

#include "linux_subagent.h"
#include <sys/utsname.h>

/**
 * Parse pacman output
 */
static void PacmanParser(const StringList& input, Table *output, const TCHAR *arch)
{
   for(int i = 0; i < input.size(); i++)
   {
      TCHAR line[1024];
      _tcslcpy(line, input.get(i), 1024);

      TCHAR *p = _tcschr(line, _T(':'));
      if (p == nullptr)
         continue;

      *p = 0;
      p++;
      Trim(line);
      Trim(p);

      if (!_tcsicmp(line, _T("Name")))
      {
         output->addRow();
         output->set(0, p);
         output->set(6, p);
      }
      else if (!_tcsicmp(line, _T("Description")))
      {
         output->set(5, p);
      }
      else if (!_tcsicmp(line, _T("Install Date")))
      {
         if (p[0] != 0)
         {
            struct tm localTime;
#ifdef UNICODE
            char text[256];
            wchar_to_mb(p, -1, text, 256);
            if (strptime(text, "%a %d %b %Y %H:%M:%S %Z", &localTime) != nullptr)
#else
            if (strptime(p, "%a %d %b %Y %H:%M:%S %Z", &localTime) != nullptr)
#endif
            {
               output->set(3, static_cast<int64_t>(mktime(&localTime)));
            }
         }
      }
      else if (!_tcsicmp(line, _T("Packager")))
      {
         output->set(2, p);
      }
      else if (!_tcsicmp(line, _T("URL")))
      {
         output->set(4, p);
      }
      else if (!_tcsicmp(line, _T("Version")))
      {
         output->set(1, p);
      }
   }
}

/**
 * Default parser (used for rpm, dpkg, and opkg)
 */
static void DefaultParser(const StringList& input, Table *output, const TCHAR *arch)
{
   for(int i = 0; i < input.size(); i++)
   {
      TCHAR line[1024];
      _tcslcpy(line, input.get(i), 1024);

      if (_tcsncmp(line, _T("@@@"), 3))
         continue;

      output->addRow();
      TCHAR *curr = _tcschr(&line[3], _T('#'));
      if (curr != nullptr)
         curr++;
      else
         curr = &line[3];
		for(int i = 0; i < 6; i++)
		{
			TCHAR *ptr = _tcschr(curr, _T('|'));
			if (ptr != nullptr)
				*ptr = 0;

			// Remove architecture from package name if it is the same as
			// OS architecture or package is architecture-independent
			if (i == 0)
			{
	         output->set(6, curr); // set uninstall key to name:arch
			   TCHAR *pa = _tcsrchr(curr, _T(':'));
			   if (pa != nullptr)
			   {
			      if (!_tcscmp(pa, _T(":all")) || !_tcscmp(pa, _T(":noarch")) || !_tcscmp(pa, _T(":(none)")) || (_tcsstr(arch, pa) != nullptr))
			         *pa = 0;
			   }
			}

			output->set(i, curr);

			if (ptr == nullptr)
				break;
         curr = ptr + 1;
      }
   }
}

/**
 * Package manager type
 */
enum class PackageManager
{
   DPKG, RPM, PACMAN, UNKNOWN
};

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
#if _OPENWRT
   const TCHAR *command = _T("opkg list-installed | awk -e '{ print \"@@@ #\" $1 \"|\" $3 \"||||\" }'");
   bool shellExec = true;
   void (*parser)(const StringList&, Table*, const TCHAR*) = DefaultParser;
#else
   PackageManager pm = PackageManager::UNKNOWN;

   // If alien is installed, try to determine native package manager (issue NX-2566)
   if (access("/bin/alien", X_OK) == 0)
   {
      if (ProcessExecutor::executeAndWait(_T("['/bin/rpm','-q','alien']"), 5000) == 0)
      {
         pm = PackageManager::RPM;
      }
      else if (ProcessExecutor::executeAndWait(_T("['/usr/bin/dpkg-query','-W','alien']"), 5000) == 0)
      {
         pm = PackageManager::DPKG;
      }
   }
   else if (access("/bin/rpm", X_OK) == 0)
   {
      pm = PackageManager::RPM;
   }
   else if (access("/usr/bin/dpkg-query", X_OK) == 0)
   {
      pm = PackageManager::DPKG;
   }
   else if (access("/usr/bin/pacman", X_OK) == 0)
   {
      pm = PackageManager::PACMAN;
   }

   bool shellExec;
   const TCHAR *command;
   void (*parser)(const StringList&, Table*, const TCHAR*) = DefaultParser;
   switch(pm)
   {
      case PackageManager::DPKG:
         command = _T("/usr/bin/dpkg-query -W -f '@@@${Status}#${package}:${Architecture}|${version}|||${homepage}|${description}\\n' | grep '@@@install.*installed.*#'");
         shellExec = true;
         break;
      case PackageManager::PACMAN:
         command = _T("/usr/bin/pacman -Qi");
         shellExec = false;
         parser = PacmanParser;
         break;
      case PackageManager::RPM:
         command = _T("/bin/rpm -qa --queryformat '@@@ #%{NAME}:%{ARCH}|%{VERSION}%|RELEASE?{-%{RELEASE}}:{}||%{VENDOR}|%{INSTALLTIME}|%{URL}|%{SUMMARY}\\n'");
         shellExec = false;
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }

#endif

   struct utsname un;
   const TCHAR *arch;
#ifdef UNICODE
   TCHAR machine[64];
#endif
   if (uname(&un) != -1)
   {
      if (!strcmp(un.machine, "i686") || !strcmp(un.machine, "i586") || !strcmp(un.machine, "i486") || !strcmp(un.machine, "i386"))
      {
         arch = _T(":i686:i586:i486:i386");
      }
      else if (!strcmp(un.machine, "amd64") || !strcmp(un.machine, "x86_64"))
      {
         arch = _T(":amd64:x86_64");
      }
      else
      {
#ifdef UNICODE
         machine[0] = 0;
         mb_to_wchar(un.machine, -1, &machine[1], 63);
         arch = machine;
#else
         memmove(&un.machine[1], un.machine, strlen(un.machine) + 1);
         un.machine[0] = ':';
         arch = un.machine;
#endif
      }
   }
   else
   {
      arch = _T(":i686:i586:i486:i386");
   }

   LineOutputProcessExecutor executor(command, shellExec);
   if (!executor.execute())
      return SYSINFO_RC_ERROR;

   if (!executor.waitForCompletion(5000))
      return SYSINFO_RC_ERROR;

   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"), true);
   value->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
   value->addColumn(_T("DATE"), DCI_DT_INT64, _T("Install Date"));
   value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
   value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));
   value->addColumn(_T("UNINSTALL_KEY"), DCI_DT_STRING, _T("Uninstall key"));

   parser(executor.getData(), value, arch);

   return SYSINFO_RC_SUCCESS;
}

/**
 * Agent action for uninstalling product
 */
uint32_t H_UninstallProduct(const shared_ptr<ActionExecutionContext>& context)
{
   if (!context->hasArgs())
      return ERR_BAD_ARGUMENTS;

   const TCHAR *uninstallKey = context->getArg(0);
   if (uninstallKey[0] == 0)
      return ERR_BAD_ARGUMENTS;

   StringBuffer command;
   if (access("/bin/rpm", X_OK) == 0)
   {
      command.append(_T("['/bin/rpm','-e','"));
      command.append(uninstallKey);
      command.append(_T("']"));
   }
   else if (access("/usr/bin/dpkg", X_OK) == 0)
   {
      command.append(_T("['/usr/bin/dpkg','-r','"));
      command.append(uninstallKey);
      command.append(_T("']"));
   }
   else if (access("/usr/bin/pacman", X_OK) == 0)
   {
      command.append(_T("['/usr/bin/pacman','-R','"));
      command.append(uninstallKey);
      command.append(_T("']"));
   }
   else
   {
      return ERR_FUNCTION_NOT_SUPPORTED;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Executing uninstall command \"%s\" for product key \"%s\""), command.cstr(), uninstallKey);
   ProcessExecutor executor(command);
   if (!executor.execute())
      return ERR_EXEC_FAILED;
   if (!executor.waitForCompletion(120000))
      return ERR_REQUEST_TIMEOUT;
   return (executor.getExitCode() == 0) ? ERR_SUCCESS : ERR_EXEC_FAILED;
}
