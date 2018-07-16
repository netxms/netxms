/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: hypervisor.cpp
**
**/

#include "winnt_subagent.h"

/**
 * Check for VMware host
 */
static bool IsVMware()
{
   return !strcmp(GetHardwareProduct(), "VMware Virtual Platform");
}

/**
 * Get VMware host version
 */
static bool GetVMwareVersionString(TCHAR *value)
{
   KeyValueOutputProcessExecutor pe(_T("\"C:\\Program Files\\VMware\\VMware Tools\\VMwareToolboxCmd.exe\" stat raw text session"));
   if (!pe.execute())
      return false;
   if (!pe.waitForCompletion(1000))
      return false;

   const TCHAR *v = pe.getData()->get(_T("version"));
   if (v != NULL)
   {
      _tcslcpy(value, v, MAX_RESULT_LENGTH);
      return true;
   }
   return false;
}

/**
 * Check for VirtualBox host
 */
static bool IsVirtualBox()
{
   return !strcmp(GetHardwareProduct(), "VirtualBox");
}

/**
 * Get VirtualBox host version
 */
static bool GetVirtualBoxVersionString(TCHAR *value)
{
   const char * const *oemStrings = GetOEMStrings();
   for(int i = 0; oemStrings[i] != NULL; i++)
   {
      if (!strncmp(oemStrings[i], "vboxVer_", 8))
      {
         _sntprintf(value, MAX_RESULT_LENGTH, _T("VirtualBox %hs"), oemStrings[i] + 8);
         return true;
      }
   }
   return false;
}

/**
 * Handler for Hypervisor.Type parameter
 */
LONG H_HypervisorType(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   const char *mf = GetHardwareManufacturer();
   const char *prod = GetHardwareProduct();

   if (!strcmp(mf, "Xen") && !strcmp(prod, "HVM domU"))
   {
      ret_mbstring(value, "XEN");
      return SYSINFO_RC_SUCCESS;
   }

   if (IsVMware())
   {
      ret_mbstring(value, "VMware");
      return SYSINFO_RC_SUCCESS;
   }

   if (!strcmp(mf, "Microsoft Corporation") && !strcmp(prod, "Virtual Machine"))
   {
      ret_mbstring(value, "Hyper-V");
      return SYSINFO_RC_SUCCESS;
   }

   if (!strcmp(mf, "Red Hat") && !strcmp(prod, "KVM"))
   {
      ret_mbstring(value, "KVM");
      return SYSINFO_RC_SUCCESS;
   }

   if (IsVirtualBox())
   {
      ret_mbstring(value, "VirtualBox");
      return SYSINFO_RC_SUCCESS;
   }

   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for Hypervisor.Version parameter
 */
LONG H_HypervisorVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (IsVMware() && GetVMwareVersionString(value))
      return SYSINFO_RC_SUCCESS;
   if (IsVirtualBox() && GetVirtualBoxVersionString(value))
      return SYSINFO_RC_SUCCESS;
   return SYSINFO_RC_UNSUPPORTED;
}
