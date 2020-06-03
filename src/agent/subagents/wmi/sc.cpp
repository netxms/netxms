/*
** WMI NetXMS subagent
** Copyright (C) 2008-2020 Victor Kirhenshtein
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
** File: sc.cpp
**
**/

#include "wmi.h"
#include <netfw.h>

/**
 * Get state of Windows Firewall
 */
static int GetWindowsFirewallState()
{
   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
   if ((hr != S_OK) && (hr != S_FALSE))
      return -1;

   INetFwPolicy2 *fwPolicy;
   hr = CoCreateInstance(__uuidof(NetFwPolicy2), NULL, CLSCTX_INPROC_SERVER, __uuidof(INetFwPolicy2), (void**)&fwPolicy);
   if (hr != S_OK)
   {
      AgentWriteDebugLog(6, _T("WMI: call to CoCreateInstance(NetFwPolicy2) failed"));
      CoUninitialize();
      return -1;
   }

   long profiles;
   fwPolicy->get_CurrentProfileTypes(&profiles);

   bool enabled = false;
   for (int i = 1; i < 8; i = i << 1)
   {
      if ((profiles & i) == 0)
         continue;

      VARIANT_BOOL v = FALSE;
      fwPolicy->get_FirewallEnabled((NET_FW_PROFILE_TYPE2)i, &v);
      if (v)
      {
         enabled = true;
         break;
      }
   }

   fwPolicy->Release();
   CoUninitialize();
   return enabled ? 1 : 0;
}

/**
 * Handler for System.*Product.DisplayName parameters
 */
LONG H_SecurityCenterDisplayName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG rc = SYSINFO_RC_ERROR;

   WCHAR query[256];
#ifdef UNICODE
   snwprintf(query, 256, L"SELECT displayName FROM %s", arg);
#else
   snwprintf(query, 256, L"SELECT displayName FROM %hs", arg);
#endif

   WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = DoWMIQuery(L"root\\SecurityCenter2", query, &ctx);
	if (pEnumObject != NULL)
	{
	   IWbemClassObject *pClassObject = NULL;
      ULONG uRet;
		if (pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
		{
			VARIANT v;

			if (pClassObject->Get(L"displayName", 0, &v, 0, 0) == S_OK)
			{
				TCHAR *str = VariantToString(&v);
				VariantClear(&v);
            if (str != NULL)
            {
               ret_string(value, str);
               MemFree(str);
				   rc = SYSINFO_RC_SUCCESS;
            }
			}
			else
			{
            AgentWriteDebugLog(6, _T("WMI: H_SecurityCenterDisplayName: property \"displayName\" not found"));
				rc = SYSINFO_RC_UNSUPPORTED;
			}
			pClassObject->Release();
		}
      else
      {
         AgentWriteDebugLog(6, _T("WMI: H_SecurityCenterProductState: no objects of class \"%s\""), arg);
         rc = SYSINFO_RC_UNSUPPORTED;
      }
		pEnumObject->Release();
		CloseWMIQuery(&ctx);
	}

   if ((rc != SYSINFO_RC_SUCCESS) && !_tcscmp(arg, _T("FirewallProduct")))
   {
      // WMI only provides information about 3rd party firewall products
      // If none available assume that Windows Firewall is used
      ret_string(value, _T("Windows Firewall"));
      rc = SYSINFO_RC_SUCCESS;
   }

   return rc;
}

/**
 * Handler for System.*Product.Running and System.*Product.UpToDate parameters
 */
LONG H_SecurityCenterProductState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG rc = SYSINFO_RC_ERROR;

   WCHAR query[256];
#ifdef UNICODE
   snwprintf(query, 256, L"SELECT productState FROM %s", &arg[1]);
#else
   snwprintf(query, 256, L"SELECT productState FROM %hs", &arg[1]);
#endif

   WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = DoWMIQuery(L"root\\SecurityCenter2", query, &ctx);
	if (pEnumObject != NULL)
	{
	   IWbemClassObject *pClassObject = NULL;
      ULONG uRet;
		if (pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
		{
			VARIANT v;

			if (pClassObject->Get(L"productState", 0, &v, 0, 0) == S_OK)
			{
            LONG state = VariantToInt(&v);
            ret_int(value, (arg[0] == 'U') ? (((state & 0x000010) != 0) ? 0 : 1) : (((state & 0x001000) != 0) ? 1 : 0));
			   rc = SYSINFO_RC_SUCCESS;
			}
			else
			{
            AgentWriteDebugLog(6, _T("WMI: H_SecurityCenterProductState: property \"productState\" not found"));
				rc = SYSINFO_RC_UNSUPPORTED;
			}
			pClassObject->Release();
		}
      else
      {
         AgentWriteDebugLog(6, _T("WMI: H_SecurityCenterProductState: no objects of class \"%s\""), &arg[1]);
         rc = SYSINFO_RC_UNSUPPORTED;
      }
		pEnumObject->Release();
		CloseWMIQuery(&ctx);
	}

   if ((rc != SYSINFO_RC_SUCCESS) && !_tcscmp(&arg[1], _T("FirewallProduct")))
   {
      // WMI only provides information about 3rd party firewall products
      // If none available check Windows Firewall state
      if (arg[0] == 'R')
      {
         int fwState = GetWindowsFirewallState();
         if (fwState >= 0)
         {
            ret_int(value, fwState);
            rc = SYSINFO_RC_SUCCESS;
         }
         else
         {
            rc = SYSINFO_RC_ERROR;
         }
      }
      else
      {
         rc = SYSINFO_RC_UNSUPPORTED;
      }
   }

   return rc;
}
