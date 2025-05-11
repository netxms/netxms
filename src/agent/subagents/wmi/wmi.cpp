/*
** WMI NetXMS subagent
** Copyright (C) 2008-2025 Victor Kirhenshtein
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
#include <netxms-version.h>

/**
 * Externals
 */
LONG H_ACPIThermalZones(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session);
LONG H_ACPITZCurrTemp(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetworkAdapterProperty(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_NetworkAdaptersList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session);
LONG H_NetworkAdaptersTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session);
LONG H_SecurityCenterDisplayName(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);
LONG H_SecurityCenterProductState(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session);

/**
 * Set table column value from variant
 */
void VariantToTableCell(VARIANT *v, Table *t, int column)
{
   switch (v->vt)
   {
      case VT_BOOL:
         t->set(column, v->boolVal ? _T("TRUE") : _T("FALSE"));
         break;
      case VT_UI1:
         t->set(column, v->bVal);
         break;
      case VT_I2:
         t->set(column, v->uiVal);
         break;
      case VT_I4:
         t->set(column, v->lVal);
         break;
      case VT_R4:
         t->set(column, v->fltVal);
         break;
      case VT_R8:
         t->set(column, v->dblVal);
         break;
      case VT_BSTR:
#ifdef UNICODE
         t->set(column, v->bstrVal);
#else
         t->setPreallocated(column, MBStringFromWideString(pValue->bstrVal));
#endif
         break;
      default:
         break;
   }
}

/**
 * Convert variant to string value
 */
TCHAR *VariantToString(VARIANT *v, TCHAR *buffer, size_t size)
{
   TCHAR *b = buffer;
   switch(v->vt) 
   {
		case VT_NULL:
         if (b == NULL)
            b = MemCopyString(_T(""));
         else
            *b = 0;
         break;
	   case VT_BOOL:
         if (b == NULL)
            b = MemCopyString(v->boolVal ? _T("TRUE") : _T("FALSE"));
         else
            _tcslcpy(b, v->boolVal ? _T("TRUE") : _T("FALSE"), size);
         break;
	   case VT_UI1:
         if (b == NULL)
         {
            size = 32;
            b = MemAllocArray<TCHAR>(size);
         }
			_sntprintf(b, size, _T("%d"), v->bVal);
			break;
		case VT_I2:
         if (b == NULL)
         {
            size = 32;
            b = MemAllocArray<TCHAR>(size);
         }
			_sntprintf(b, 32, _T("%d"), v->uiVal);
			break;
		case VT_I4:
         if (b == NULL)
         {
            size = 32;
            b = MemAllocArray<TCHAR>(size);
         }
         _sntprintf(b, 32, _T("%d"), v->lVal);
			break;
		case VT_R4:
         if (b == NULL)
         {
            size = 32;
            b = MemAllocArray<TCHAR>(size);
         }
         _sntprintf(b, 32, _T("%f"), v->fltVal);
			break;
		case VT_R8:
         if (b == NULL)
         {
            size = 32;
            b = MemAllocArray<TCHAR>(size);
         }
         _sntprintf(b, 32, _T("%f"), v->dblVal);
			break;
		case VT_BSTR:
         if (b == NULL)
         {
#ifdef UNICODE
            b = MemCopyString(v->bstrVal);
#else
            b = MBStringFromWideString(v->bstrVal);
#endif
         }
         else
         {
#ifdef UNICODE
            _tcslcpy(b, v->bstrVal, size);
#else
            WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, v->bstrVal, -1, b, size, NULL, NULL);
#endif
         }
			break;
	   default:
         if (b != NULL)
            *b = 0;
			break;
	}

   return b;
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

   HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if ((hr != S_OK) && (hr != S_FALSE))
		return NULL;

	if (CoInitializeSecurity(NULL, -1, NULL, NULL,
	                         RPC_C_AUTHN_LEVEL_PKT,
	                         RPC_C_IMP_LEVEL_IMPERSONATE,
	                         NULL, EOAC_NONE,	0) != S_OK)
	{
		CoUninitialize();
      AgentWriteDebugLog(6, _T("WMI: call to CoInitializeSecurity failed"));
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
         else
         {
            AgentWriteDebugLog(6, _T("WMI: call to pWbemServices->ExecQuery failed"));
         }
		}
      else
      {
         AgentWriteDebugLog(6, _T("WMI: call to pWbemLocator->ConnectServer failed"));
      }
	}
   else
   {
      AgentWriteDebugLog(6, _T("WMI: call to CoCreateInstance failed"));
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
static LONG H_WMIQuery(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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
					TCHAR *str = VariantToString(&v);
					if (str != NULL)
					{
						ret_string(value, str);
						MemFree(str);
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
  * Handler for generic WMI query that return list of values
  * Format: WMI.Query(namespace, query, property)
  */
static LONG H_WMIListQuery(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
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
      while (pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
      {
         VARIANT v;
         if (pClassObject->Get(prop, 0, &v, 0, 0) == S_OK)
         {
            TCHAR *str = VariantToString(&v);
            if (str != NULL)
            {
               value->add(str);
               MemFree(str);
            }
            else
            {
               value->add(_T("<WMI result conversion error>"));
            }
            VariantClear(&v);
         }
         else
         {
            AgentWriteDebugLog(5, _T("WMI: cannot get property \"%s\" from query \"%s\" in namespace \"%s\""), prop, query, ns);
         }
         pClassObject->Release();
      }
      pEnumObject->Release();
      CloseWMIQuery(&ctx);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      AgentWriteDebugLog(5, _T("WMI: query \"%s\" in namespace \"%s\" failed"), query, ns);
   }
   return rc;
}

/**
  * Handler for generic WMI query that return table
  * Format: WMI.Query(namespace, query)
  */
static LONG H_WMITableQuery(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   WMI_QUERY_CONTEXT ctx;
   IEnumWbemClassObject *pEnumObject = NULL;
   IWbemClassObject *pClassObject = NULL;
   TCHAR ns[256], query[256];
   ULONG uRet;
   LONG rc = SYSINFO_RC_ERROR;

   if (!AgentGetParameterArg(cmd, 1, ns, 256) ||
       !AgentGetParameterArg(cmd, 2, query, 256))
      return SYSINFO_RC_UNSUPPORTED;

   pEnumObject = DoWMIQuery(ns, query, &ctx);
   if (pEnumObject != NULL)
   {
      bool headerCreated = false;
      StringList properties;
      while (pEnumObject->Next(WBEM_INFINITE, 1, &pClassObject, &uRet) == S_OK)
      {
         value->addRow();
         pClassObject->BeginEnumeration(0);
         VARIANT v;
         BSTR prop;
         long flavor;
         while (pClassObject->Next(0, &prop, &v, NULL, &flavor) == S_OK)
         {
            if ((flavor & WBEM_FLAVOR_MASK_ORIGIN) == WBEM_FLAVOR_ORIGIN_SYSTEM)
               continue;   // Ignore system properties

            int index = value->getColumnIndex(prop);
            if (index == -1)
            {
               index = value->addColumn(prop);
            }

            TCHAR *str = VariantToString(&v);
            if (str != NULL)
            {
               value->set(index, str);
               MemFree(str);
            }
            VariantClear(&v);
            SysFreeString(prop);
         }
         pClassObject->Release();
      }
      pEnumObject->Release();
      CloseWMIQuery(&ctx);
      rc = SYSINFO_RC_SUCCESS;
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
static LONG H_WMINameSpaces(const TCHAR *pszParam, const TCHAR *pArg, StringList *value, AbstractCommSession *session)
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
static LONG H_WMIClasses(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   TCHAR ns[256];
   if (!AgentGetParameterArg(param, 1, ns, 256))
      return SYSINFO_RC_UNSUPPORTED;

   LONG rc = SYSINFO_RC_ERROR;

   WMI_QUERY_CONTEXT ctx;
   IEnumWbemClassObject *enumObject = DoWMIQuery(ns, L"SELECT * FROM meta_class", &ctx);
   if (enumObject != NULL)
   {
      IWbemClassObject *classObject = NULL;
      ULONG uRet;
      while(enumObject->Next(WBEM_INFINITE, 1, &classObject, &uRet) == S_OK)
      {
         VARIANT v;
         if (classObject->Get(L"__CLASS", 0, &v, 0, 0) == S_OK)
         {
            TCHAR *str = VariantToString(&v);
            VariantClear(&v);
            value->addPreallocated(str);
         }
         classObject->Release();
      }
      enumObject->Release();
      CloseWMIQuery(&ctx);
      rc = SYSINFO_RC_SUCCESS;
   }

   return rc;
}

/**
 * Provided parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("ACPI.ThermalZone.CurrentTemp"), H_ACPITZCurrTemp, _T("*"), DCI_DT_INT, _T("Current temperature in ACPI thermal zone") },
   { _T("ACPI.ThermalZone.CurrentTemp(*)"), H_ACPITZCurrTemp, _T("%"), DCI_DT_INT, _T("Current temperature in ACPI thermal zone {instance}") },
   { _T("Hardware.NetworkAdapter.Availability(*)"), H_NetworkAdapterProperty, _T("Availability"), DCI_DT_UINT, DCIDESC_HARDWARE_NETWORKADAPTER_AVAILABILITY },
   { _T("Hardware.NetworkAdapter.Description(*)"), H_NetworkAdapterProperty, _T("Description"), DCI_DT_STRING, DCIDESC_HARDWARE_NETWORKADAPTER_DESCRIPTION },
   { _T("Hardware.NetworkAdapter.InterfaceIndex(*)"), H_NetworkAdapterProperty, _T("InterfaceIndex"), DCI_DT_UINT, DCIDESC_HARDWARE_NETWORKADAPTER_IFINDEX },
   { _T("Hardware.NetworkAdapter.MACAddress(*)"), H_NetworkAdapterProperty, _T("MACAddress"), DCI_DT_STRING, DCIDESC_HARDWARE_NETWORKADAPTER_MACADDR },
   { _T("Hardware.NetworkAdapter.Manufacturer(*)"), H_NetworkAdapterProperty, _T("Manufacturer"), DCI_DT_STRING, DCIDESC_HARDWARE_NETWORKADAPTER_MANUFACTURER },
   { _T("Hardware.NetworkAdapter.Product(*)"), H_NetworkAdapterProperty, _T("ProductName"), DCI_DT_STRING, DCIDESC_HARDWARE_NETWORKADAPTER_PRODUCT },
   { _T("Hardware.NetworkAdapter.Speed(*)"), H_NetworkAdapterProperty, _T("Speed"), DCI_DT_UINT64, DCIDESC_HARDWARE_NETWORKADAPTER_SPEED },
   { _T("Hardware.NetworkAdapter.Type(*)"), H_NetworkAdapterProperty, _T("AdapterType"), DCI_DT_STRING, DCIDESC_HARDWARE_NETWORKADAPTER_TYPE },
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
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("ACPI.ThermalZones"), H_ACPIThermalZones, nullptr },
   { _T("Hardware.NetworkAdapters"), H_NetworkAdaptersList, nullptr },
   { _T("WMI.Classes(*)"), H_WMIClasses, nullptr },
   { _T("WMI.NameSpaces"), H_WMINameSpaces, nullptr },
   { _T("WMI.Query(*)"), H_WMIListQuery, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("Hardware.NetworkAdapters"), H_NetworkAdaptersTable, nullptr, _T("INDEX"), DCTDESC_HARDWARE_NETWORK_ADAPTERS },
   { _T("WMI.Query(*)"), H_WMITableQuery, nullptr, _T(""), _T("Generic WMI query") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
   _T("WMI"), NETXMS_VERSION_STRING,
   nullptr, nullptr, nullptr, nullptr,  nullptr,    // callbacks
   sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
   s_parameters,
   sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
   s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   s_tables,
   0, nullptr,	// actions
   0, nullptr	// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(WMI)
{
   *ppInfo = &s_info;
   return true;
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
