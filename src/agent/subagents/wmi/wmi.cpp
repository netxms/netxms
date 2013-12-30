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
** File: wmi.cpp
**
**/

#include "wmi.h"

/**
 * Externals
 */
LONG H_ACPIThermalZones(const TCHAR *pszParam, const TCHAR *pArg, StringList *value);
LONG H_ACPITZCurrTemp(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_SecurityCenterDisplayName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);
LONG H_SecurityCenterProductState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value);

/**
 * Convert variant to string value
 */
TCHAR *VariantToString(VARIANT *pValue)
{
   TCHAR *buf = NULL;

   switch (pValue->vt) 
   {
		case VT_NULL: 
         buf = _tcsdup(_T("<null>"));
         break;
	   case VT_BOOL: 
			buf = _tcsdup(pValue->boolVal ? _T("TRUE") : _T("FALSE"));
         break;
	   case VT_UI1:
			buf = (TCHAR *)malloc(32 * sizeof(TCHAR));
			_sntprintf(buf, 32, _T("%d"), pValue->bVal);
			break;
		case VT_I2:
			buf = (TCHAR *)malloc(32 * sizeof(TCHAR));
			_sntprintf(buf, 32, _T("%d"), pValue->uiVal);
			break;
		case VT_I4:
			buf = (TCHAR *)malloc(32 * sizeof(TCHAR));
			_sntprintf(buf, 32, _T("%d"), pValue->lVal);
			break;
		case VT_R4:
			buf = (TCHAR *)malloc(32 * sizeof(TCHAR));
			_sntprintf(buf, 32, _T("%f"), pValue->fltVal);
			break;
		case VT_R8:
			buf = (TCHAR *)malloc(32 * sizeof(TCHAR));
			_sntprintf(buf, 32, _T("%f"), pValue->dblVal);
			break;
		case VT_BSTR:
#ifdef UNICODE
			buf = wcsdup(pValue->bstrVal);
#else
			buf = MBStringFromWideString(pValue->bstrVal);
#endif
			break;
	   default:
			break;
	}

   return buf;
}

/**
 * Convert variant to integer value
 */
LONG VariantToInt(VARIANT *pValue)
{
   LONG val;

   switch (pValue->vt) 
   {
	   case VT_BOOL: 
			val = pValue->boolVal ? 1 : 0;
         break;
	   case VT_UI1:
			val = pValue->bVal;
			break;
		case VT_I2:
			val = pValue->uiVal;
			break;
		case VT_I4:
			val = pValue->lVal;
			break;
		case VT_R4:
			val = (LONG)pValue->fltVal;
			break;
		case VT_R8:
			val = (LONG)pValue->dblVal;
			break;
		case VT_BSTR:
			val = wcstol(pValue->bstrVal, NULL, 0);
			break;
	   default:
			val = 0;
			break;
	}

   return val;
}

/**
 * Perform generic WMI query
 * Returns pointer to IEnumWbemClassObject ready for enumeration or NULL
 */
IEnumWbemClassObject *DoWMIQuery(WCHAR *ns, WCHAR *query, WMI_QUERY_CONTEXT *ctx)
{
	IWbemLocator *pWbemLocator = NULL;
	IWbemServices *pWbemServices = NULL;
	IEnumWbemClassObject *pEnumObject = NULL;

	memset(ctx, 0, sizeof(WMI_QUERY_CONTEXT));

	if (CoInitializeEx(0, COINIT_MULTITHREADED) != S_OK)
		return NULL;

	if (CoInitializeSecurity(NULL, -1, NULL, NULL,
	                         RPC_C_AUTHN_LEVEL_PKT,
	                         RPC_C_IMP_LEVEL_IMPERSONATE,
	                         NULL, EOAC_NONE,	0) != S_OK)
	{
		CoUninitialize();
		return NULL;
	}

	if (CoCreateInstance(CLSID_WbemAdministrativeLocator,
	                     NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
	                     IID_IUnknown, (void **)&pWbemLocator) == S_OK)
	{
		ctx->m_locator = pWbemLocator;
		if (pWbemLocator->ConnectServer(ns, NULL, NULL,
			                             NULL, 0, NULL, NULL, &pWbemServices) == S_OK)
		{
			ctx->m_services = pWbemServices;
			if (pWbemServices->ExecQuery(L"WQL", query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
					                       NULL, &pEnumObject) == S_OK)
			{
				//pEnumObject->Reset();
			}
		}
	}
	if (pEnumObject == NULL)
		CloseWMIQuery(ctx);
	return pEnumObject;
}

/**
 * Cleanup after WMI query
 */
void CloseWMIQuery(WMI_QUERY_CONTEXT *ctx)
{
	if (ctx->m_services != NULL)
		ctx->m_services->Release();
	if (ctx->m_locator != NULL)
		ctx->m_locator->Release();
	memset(ctx, 0, sizeof(WMI_QUERY_CONTEXT));
	CoUninitialize();
}

/**
 * Handler for generic WMI query
 * Format: WMI.Query(namespace, query, property)
 */
static LONG H_WMIQuery(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
	WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = NULL;
	IWbemClassObject *pClassObject = NULL;
	TCHAR ns[256], query[256], prop[256];
	ULONG uRet;
	LONG rc = SYSINFO_RC_ERROR;

	if (!AgentGetParameterArg(cmd, 1, ns, 256) ||
	    !AgentGetParameterArg(cmd, 2, query, 256) ||
	    !AgentGetParameterArg(cmd, 3, prop, 256))
		return SYSINFO_RC_UNSUPPORTED;

	pEnumObject = DoWMIQuery(ns, query, &ctx);
	if (pEnumObject != NULL)
	{
		if (!_tcsicmp(prop, _T("%%count%%")))
		{
			LONG count = 0;
			while(pEnumObject->Skip(WBEM_INFINITE, 1) == S_OK)
				count++;
			ret_int(value, count);
			rc = SYSINFO_RC_SUCCESS;
		}
		else
		{
			if (pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
			{
				VARIANT v;

				if (pClassObject->Get(prop, 0, &v, 0, 0) == S_OK)
				{
					TCHAR *str;

					str = VariantToString(&v);
					if (str != NULL)
					{
						ret_string(value, str);
						free(str);
					}
					else
					{
						ret_string(value, _T("<WMI result conversion error>"));
					}
					VariantClear(&v);

					rc = SYSINFO_RC_SUCCESS;
				}
				else
				{
					AgentWriteDebugLog(5, _T("WMI: cannot get property \"%s\" from query \"%s\" in namespace \"%s\""), prop, query, ns);
				}
				pClassObject->Release();
			}
			else
			{
				AgentWriteDebugLog(5, _T("WMI: no objects returned from query \"%s\" in namespace \"%s\""), query, ns);
			}
		}
		pEnumObject->Release();
		CloseWMIQuery(&ctx);
	}
	else
	{
		AgentWriteDebugLog(5, _T("WMI: query \"%s\" in namespace \"%s\" failed"), query, ns);
	}
   return rc;
}

/**
 * Handler for WMI.NameSpaces list
 */
static LONG H_WMINameSpaces(const TCHAR *pszParam, const TCHAR *pArg, StringList *value)
{
	WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = NULL;
	IWbemClassObject *pClassObject = NULL;
	ULONG uRet;
	LONG rc = SYSINFO_RC_ERROR;

	pEnumObject = DoWMIQuery(L"root", L"SELECT Name FROM __NAMESPACE", &ctx);
	if (pEnumObject != NULL)
	{
		while(pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
		{
			VARIANT v;

			if (pClassObject->Get(L"Name", 0, &v, 0, 0) == S_OK)
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

/**
 * Handler for WMI.Classes list
 */
static LONG H_WMIClasses(const TCHAR *pszParam, const TCHAR *pArg, StringList *value)
{
	WMI_QUERY_CONTEXT ctx;
	IEnumWbemClassObject *pEnumObject = NULL;
	IWbemClassObject *pClassObject = NULL;
	TCHAR ns[256];
	ULONG uRet;
	LONG rc = SYSINFO_RC_ERROR;

	if (!AgentGetParameterArg(pszParam, 1, ns, 256))
		return SYSINFO_RC_UNSUPPORTED;

	pEnumObject = DoWMIQuery(ns, L"SELECT * FROM meta_class", &ctx);
	if (pEnumObject != NULL)
	{
		while(pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
		{
			VARIANT v;

			if (pClassObject->Get(L"__CLASS", 0, &v, 0, 0) == S_OK)
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

/**
 * Initialize subagent
 */
static BOOL SubAgentInit(Config *config)
{
	return TRUE;
}

/**
 * Handler for subagent unload
 */
static void SubAgentShutdown()
{
}

/**
 * Provided parameters
 */
static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { _T("ACPI.ThermalZone.CurrentTemp"), H_ACPITZCurrTemp, _T("*"), DCI_DT_INT, _T("Current temperature in ACPI thermal zone") },
   { _T("ACPI.ThermalZone.CurrentTemp(*)"), H_ACPITZCurrTemp, _T("%"), DCI_DT_INT, _T("Current temperature in ACPI thermal zone {instance}") },
   { _T("System.AntiSpywareProduct.Active"), H_SecurityCenterProductState, _T("RAntiSpywareProduct"), DCI_DT_INT, _T("Anti-spyware product active") },
   { _T("System.AntiSpywareProduct.DisplayName"), H_SecurityCenterDisplayName, _T("AntispywareProduct"), DCI_DT_STRING, _T("Anti-spyware product display name") },
   { _T("System.AntiSpywareProduct.UpToDate"), H_SecurityCenterProductState, _T("UAntiSpywareProduct"), DCI_DT_INT, _T("Anti-spyware product up to date") },
   { _T("System.AntiVirusProduct.Active"), H_SecurityCenterProductState, _T("RAntiVirusProduct"), DCI_DT_INT, _T("Anti-virus product active") },
   { _T("System.AntiVirusProduct.DisplayName"), H_SecurityCenterDisplayName, _T("AntiVirusProduct"), DCI_DT_STRING, _T("Anti-virus product display name") },
   { _T("System.AntiVirusProduct.UpToDate"), H_SecurityCenterProductState, _T("UAntiVirusProduct"), DCI_DT_INT, _T("Anti-virus product up to date") },
   { _T("System.FirewallProduct.Active"), H_SecurityCenterProductState, _T("RFirewallProduct"), DCI_DT_INT, _T("Firewall active") },
   { _T("System.FirewallProduct.DisplayName"), H_SecurityCenterDisplayName, _T("FirewallProduct"), DCI_DT_STRING, _T("Firewall product display name") },
   { _T("System.FirewallProduct.UpToDate"), H_SecurityCenterProductState, _T("UFirewallProduct"), DCI_DT_INT, _T("Firewall product up to date") },
   { _T("WMI.Query(*)"), H_WMIQuery, NULL, DCI_DT_STRING, _T("Generic WMI query") }
};

/**
 * Provided lists
 */
static NETXMS_SUBAGENT_LIST m_lists[] =
{
   { _T("ACPI.ThermalZones"), H_ACPIThermalZones, NULL },
   { _T("WMI.Classes(*)"), H_WMIClasses, NULL },
   { _T("WMI.NameSpaces"), H_WMINameSpaces, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	_T("WMI"), NETXMS_VERSION_STRING,
   SubAgentInit, SubAgentShutdown, NULL,      // handlers
   sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	0, NULL,	// tables
   0, NULL,	// actions
	0, NULL	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WMI)
{
   *ppInfo = &m_info;
   return TRUE;
}

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}
