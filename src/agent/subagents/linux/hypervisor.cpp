/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004-2018 Raden Solutions
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

/**
 * Check for VMWare host
 */
static bool IsVMWare()
{
   DIR *d = opendir("/sys/bus/pci/devices");
   if (d == NULL)
      return false;

   bool result = false;

   struct dirent *e;
   while(!result && ((e = readdir(d)) != NULL))
   {
      if (e->d_name[0] == '.')
         continue;

      char path[1024];
      snprintf(path, 1024, "/sys/bus/pci/devices/%s/vendor", e->d_name);

      UINT32 size;
      BYTE *content = LoadFileA(path, &size);
      if (content != NULL)
      {
         if (!strncasecmp((char *)content, "0x15ad", MIN(size, 6)))
            result = true;
         free(content);
      }
   }
   closedir(d);

   return result;
}

/**
 * Get VMWare host version
 */
static bool GetVMWareVersionString(TCHAR *value)
{
   KeyValueOutputProcessExecutor pe(_T("vmware-toolbox-cmd stat raw text session"));
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
 * Check for XEN host
 */
static bool IsXEN()
{
   UINT32 size;
   BYTE *content = LoadFileA("/sys/hypervisor/type", &size);
   if (content == NULL)
      return false;
   
   bool result = (strncasecmp((char *)content, "xen", MIN(size, 3)) == 0);
   free(content);
   return result;
}

/**
 * Get XEN host version
 */
static bool GetXENVersionString(TCHAR *value)
{
   UINT32 size;
   BYTE *content = LoadFileA("/sys/hypervisor/version/major", &size);
   if (content == NULL)
      return false;
   int major = strtol((char *)content, NULL, 10);
   free(content);

   content = LoadFileA("/sys/hypervisor/version/minor", &size);
   if (content == NULL)
      return false;
   int minor = strtol((char *)content, NULL, 10);
   free(content);

   const char *extra = "";
   content = LoadFileA("/sys/hypervisor/version/extra", &size);
   if (content != NULL)
   {
      char *ptr = strchr((char *)content, '\n');
      if (ptr != NULL)
         *ptr = 0;
      extra = (const char *)content;
   }

   _sntprintf(value, MAX_RESULT_LENGTH, _T("%d.%d%hs"), major, minor, extra);
   return true;
}

/**
 * Handler for Hypervisor.Type parameter
 */
LONG H_HypervisorType(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (IsXEN())
   {
      ret_mbstring(value, "XEN");
      return SYSINFO_RC_SUCCESS;
   }
   if (IsVMWare())
   {
      ret_mbstring(value, "VMWare");
      return SYSINFO_RC_SUCCESS;
   }
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Handler for Hypervisor.Version parameter
 */
LONG H_HypervisorVersion(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   if (IsXEN() && GetXENVersionString(value))
      return SYSINFO_RC_SUCCESS;
   if (IsVMWare() && GetVMWareVersionString(value))
      return SYSINFO_RC_SUCCESS;
   return SYSINFO_RC_UNSUPPORTED;
}
