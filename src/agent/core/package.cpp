/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2025 Raden Solutions
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
** File: package.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("installer")

#ifdef _WIN32

#include <Msi.h>

/**
 * Install MSI package
 */
static uint32_t InstallMsiMspPackage(const wchar_t *package, const wchar_t *userOptions, bool msp)
{
   MsiSetInternalUI(INSTALLUILEVEL_NONE, nullptr);

   StringBuffer options(msp ? L"REINSTALL=ALL REINSTALLMODE=ecmus REBOOT=ReallySuppress" : L"REBOOT=ReallySuppress");
   if (userOptions[0] != 0)
   {
      options.append(L' ');
      options.append(userOptions);
   }

   UINT rc = msp ?
      MsiApplyPatchW(package, nullptr, INSTALLTYPE_DEFAULT, options) :
      MsiInstallProductW(package, options);
   if ((rc == ERROR_SUCCESS) || (rc == ERROR_SUCCESS_REBOOT_REQUIRED))
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"%s package \"%s\" with options \"%s\" installed successfully", msp ? L"MSP" : L"MSI", package, options.cstr());
      if (rc == ERROR_SUCCESS_REBOOT_REQUIRED)
         nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"System restart is required for completing the installation");
      return ERR_SUCCESS;
   }

   wchar_t buffer[1024];
   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"%s package \"%s\" with options \"%s\" installation failed (%s)", msp ? L"MSP" : L"MSI", package, options.cstr(), GetSystemErrorText(rc, buffer, 1024));
   return ERR_EXEC_FAILED;
}

#endif

/**
 * Process executor for package installer
 */
class PackageInstallerProcessExecutor : public ProcessExecutor
{
protected:
   virtual void onOutput(const char *text, size_t length) override
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Installer output: %hs"), text);
   }

public:
   PackageInstallerProcessExecutor(const TCHAR *command) : ProcessExecutor(command)
   {
      m_sendOutput = true;
   }
};

/**
 * Install package
 */
uint32_t InstallSoftwarePackage(CommSession *session, const char *packageType, const TCHAR *packageFile, const TCHAR *command)
{
#ifdef _WIN32
   if (!stricmp(packageType, "msi"))
      return InstallMsiMspPackage(packageFile, command, false);
   if (!stricmp(packageType, "msp"))
      return InstallMsiMspPackage(packageFile, command, true);
#endif

   const TCHAR *workingDirectory = nullptr;
   StringBuffer commandLine;
   if (!stricmp(packageType, "executable"))
   {
      if (command[0] != 0)
      {
         commandLine.append(command);
         commandLine.replace(_T("${file}"), packageFile);
      }
      else
      {
         commandLine.append(_T("\""));
         commandLine.append(packageFile);
         commandLine.append(_T("\""));
      }
   }
#ifdef _WIN32
   else if (!stricmp(packageType, "msu"))
   {
      commandLine.append(_T("wusa.exe \""));
      commandLine.append(packageFile);
      commandLine.append(_T("\" /quiet"));
      if (command[0] != 0)
      {
         commandLine.append(_T(" "));
         commandLine.append(command);
      }
   }
#else
   else if (!stricmp(packageType, "deb"))
   {
      commandLine.append(_T("/usr/bin/dpkg -i"));
      if (command[0] != 0)
      {
         commandLine.append(_T(" "));
         commandLine.append(command);
      }
      commandLine.append(_T(" '"));
      commandLine.append(packageFile);
      commandLine.append(_T("'"));
   }
   else if (!stricmp(packageType, "rpm"))
   {
      commandLine.append(_T("/usr/bin/rpm -i"));
      if (command[0] != 0)
      {
         commandLine.append(_T(" "));
         commandLine.append(command);
      }
      commandLine.append(_T(" '"));
      commandLine.append(packageFile);
      commandLine.append(_T("'"));
   }
   else if (!stricmp(packageType, "tgz"))
   {
      commandLine.append(_T("tar"));
      if (command[0] != 0)
      {
         workingDirectory = command;
      }
      commandLine.append(_T(" zxf '"));
      commandLine.append(packageFile);
      commandLine.append(_T("'"));
   }
#endif
   else if (!stricmp(packageType, "zip"))
   {
      commandLine.append(_T("unzip"));
      commandLine.append(_T(" '"));
      commandLine.append(packageFile);
      commandLine.append(_T("'"));
      if (command[0] != 0)
      {
         commandLine.append(_T(" -d '"));
         commandLine.append(command);
         commandLine.append(_T("'"));
      }
   }

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Starting package installation using command line %s"), commandLine.cstr());
   PackageInstallerProcessExecutor executor(commandLine);
   if (workingDirectory != nullptr)
      executor.setWorkingDirectory(workingDirectory);

   if (!executor.execute())
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Package installer execution failed, command = %s"), commandLine.cstr());
      return ERR_EXEC_FAILED;
   }

   if (executor.waitForCompletion(600000))  // FIXME: what timeout to use?
   {
      nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, _T("Package installer successfully completed, command = %s"), commandLine.cstr());
      return ERR_SUCCESS;
   }

   nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Package installer did not finished within timeout, command = %s"), commandLine.cstr());
   return ERR_REQUEST_TIMEOUT;
}
