/*
** NetXMS platform subagent for Windows
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
#include <intrin.h>

/**
 * CPU vendor ID
 */
static char s_cpuVendorId[16] = "UNKNOWN";

/**
 * Read CPU vendor ID
 */
void ReadCPUVendorId()
{
#if _M_ARM64
   // FIXME: read vendor ID from "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0" in registry
#else
   int cpuInfo[4];
   __cpuid(cpuInfo, 0);

   memcpy(s_cpuVendorId, &cpuInfo[1], 4);
   memcpy(&s_cpuVendorId[4], &cpuInfo[3], 4);
   memcpy(&s_cpuVendorId[8], &cpuInfo[2], 4);
   s_cpuVendorId[12] = 0;
#endif
}

/**
 * Check for VMware host
 */
static bool IsVMware()
{
   return !strcmp(SMBIOS_GetHardwareProduct(), "VMware Virtual Platform") ||
          !strncmp(s_cpuVendorId, "VMware", 6);
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

   const TCHAR *v = pe.getData().get(_T("version"));
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
   return !strcmp(SMBIOS_GetHardwareProduct(), "VirtualBox");
}

/**
 * Get VirtualBox host version
 */
static bool GetVirtualBoxVersionString(TCHAR *value)
{
   const char * const *oemStrings = SMBIOS_GetOEMStrings();
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
   const char *mf = SMBIOS_GetHardwareManufacturer();
   const char *prod = SMBIOS_GetHardwareProduct();

   if ((!strcmp(mf, "Xen") && !strcmp(prod, "HVM domU")) ||
       !strncmp(s_cpuVendorId, "XenVMM", 6))
   {
      ret_mbstring(value, "XEN");
      return SYSINFO_RC_SUCCESS;
   }

   if (IsVMware())
   {
      ret_mbstring(value, "VMware");
      return SYSINFO_RC_SUCCESS;
   }

   if ((!strcmp(mf, "Microsoft Corporation") && !strcmp(prod, "Virtual Machine")) ||
       !strcmp(s_cpuVendorId, "Microsoft Hv"))
   {
      ret_mbstring(value, "Hyper-V");
      return SYSINFO_RC_SUCCESS;
   }

   if ((!strcmp(mf, "Red Hat") && !strcmp(prod, "KVM")) ||
       !strncmp(s_cpuVendorId, "KVM", 3))
   {
      ret_mbstring(value, "KVM");
      return SYSINFO_RC_SUCCESS;
   }

   if (!strcmp(mf, "QEMU"))
   {
      ret_mbstring(value, "QEMU");
      return SYSINFO_RC_SUCCESS;
   }

   if (!strcmp(mf, "Amazon EC2"))
   {
      ret_mbstring(value, "Amazon EC2");
      return SYSINFO_RC_SUCCESS;
   }

   if (IsVirtualBox())
   {
      ret_mbstring(value, "VirtualBox");
      return SYSINFO_RC_SUCCESS;
   }

   if (!strncmp(s_cpuVendorId, "bhyve", 5))
   {
      ret_mbstring(value, "bhyve");
      return SYSINFO_RC_SUCCESS;
   }

   if (!strcmp(s_cpuVendorId, " lrpepyh vr"))
   {
      ret_mbstring(value, "Parallels");
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
   if (!strcmp(SMBIOS_GetHardwareManufacturer(), "Amazon EC2"))
   {
      ret_mbstring(value, SMBIOS_GetHardwareProduct());
      return SYSINFO_RC_SUCCESS;
   }
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for System.CPU.VendorId parameter
 */
LONG H_CpuVendorId(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_mbstring(value, s_cpuVendorId);
   return SYSINFO_RC_SUCCESS;
}
