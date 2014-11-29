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
** File: acpi.cpp
**
**/

#include "wmi.h"

/**
 * Handler for ACPI.ThermalZone.CurrentTemp(*)
 */
LONG H_ACPITZCurrTemp(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = NULL;
	IWbemClassObject *pClassObject = NULL;
	TCHAR instance[256];
	ULONG uRet;
	LONG rc = SYSINFO_RC_ERROR;

	if (*arg != '*')
		if (!AgentGetParameterArg(cmd, 1, instance, 256))
			return SYSINFO_RC_UNSUPPORTED;

	pEnumObject = DoWMIQuery(L"root\\WMI", L"SELECT InstanceName,CurrentTemperature FROM MSAcpi_ThermalZoneTemperature", &ctx);
	if (pEnumObject != NULL)
	{
		while((pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK) &&
			   (rc != SYSINFO_RC_SUCCESS))
		{
			VARIANT v;

			if (pClassObject->Get(L"InstanceName", 0, &v, 0, 0) == S_OK)
			{
				TCHAR *str = VariantToString(&v);
				VariantClear(&v);
				if (str != NULL)
				{
					if (!_tcsicmp(str, instance) || (*arg == _T('*')))
					{
						if (pClassObject->Get(L"CurrentTemperature", 0, &v, 0, 0) == S_OK)
						{
							ret_int(value, (VariantToInt(&v) - 2732) / 10);
							VariantClear(&v);
							rc = SYSINFO_RC_SUCCESS;
						}
					}
               free(str);
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
 * Handler for ACPI.ThermalZones list
 */
LONG H_ACPIThermalZones(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
{
	WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = NULL;
	IWbemClassObject *pClassObject = NULL;
	ULONG uRet;
	LONG rc = SYSINFO_RC_ERROR;

	pEnumObject = DoWMIQuery(L"root\\WMI", L"SELECT InstanceName FROM MSAcpi_ThermalZoneTemperature", &ctx);
	if (pEnumObject != NULL)
	{
		while(pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
		{
			VARIANT v;

			if (pClassObject->Get(L"InstanceName", 0, &v, 0, 0) == S_OK)
			{
				TCHAR *str;

				str = VariantToString(&v);
				VariantClear(&v);
				value->addPreallocated(str);
			}
			pClassObject->Release();
		}
		pEnumObject->Release();
		CloseWMIQuery(&ctx);
		rc = SYSINFO_RC_SUCCESS;
	}
   return rc;
}
