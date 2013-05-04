/*
** WMI NetXMS subagent
** Copyright (C) 2008-2013 Victor Kirhenshtein
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

/**
 * Handler for System.*Product.DisplayName parameters
 */
LONG H_SecurityCenterDisplayName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
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
		while((pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK) &&
			   (rc != SYSINFO_RC_SUCCESS))
		{
			VARIANT v;

			if (pClassObject->Get(L"displayName", 0, &v, 0, 0) == S_OK)
			{
				TCHAR *str = VariantToString(&v);
				VariantClear(&v);
            if (str != NULL)
            {
               ret_string(value, str);
               free(str);
				   rc = SYSINFO_RC_SUCCESS;
            }
			}
			else
			{
				rc = SYSINFO_RC_UNSUPPORTED;
			}
			pClassObject->Release();
		}
		pEnumObject->Release();
		CloseWMIQuery(&ctx);
	}
   return rc;
}

/**
 * Handler for System.*Product.Running and System.*Product.UpToDate parameters
 */
LONG H_SecurityCenterProductState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
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
		while((pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK) &&
			   (rc != SYSINFO_RC_SUCCESS))
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
				rc = SYSINFO_RC_UNSUPPORTED;
			}
			pClassObject->Release();
		}
		pEnumObject->Release();
		CloseWMIQuery(&ctx);
	}
   return rc;
}
