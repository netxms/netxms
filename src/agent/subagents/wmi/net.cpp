/*
** WMI NetXMS subagent
** Copyright (C) 2008-2019 Victor Kirhenshtein
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
** File: net.cpp
**
**/

#include "wmi.h"

/**
 * Set column value from property
 */
inline void SetColumnFromProperty(Table *table, int column, const WCHAR *property, IWbemClassObject *classObject)
{
   VARIANT v;
   if (classObject->Get(property, 0, &v, 0, 0) == S_OK)
   {
      VariantToTableCell(&v, table, column);
   }
}

/**
 * Query condition for reading physical adapters
 */
#define QUERY_CONDITION \
   L"WHERE NetConnectionStatus<>4" \
   L"   AND PhysicalAdapter=True" \
   L"   AND NOT Manufacturer LIKE 'TAP-Windows Provider%'" \
   L"   AND NOT (Manufacturer LIKE 'Fortinet%' AND ProductName LIKE 'PPP%')"

/**
 * Handler for Hardware.NetworkAdapters table
 */
LONG H_NetworkAdaptersTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_ERROR;

   WMI_QUERY_CONTEXT ctx;
   IEnumWbemClassObject *enumObject = DoWMIQuery(L"root\\cimv2", 
      L"SELECT Index,ProductName,Manufacturer,Description,AdapterType,MACAddress,InterfaceIndex,Speed,Availability "
      L"FROM Win32_NetworkAdapter " QUERY_CONDITION,
      &ctx);
   if (enumObject != NULL)
   {
      rc = SYSINFO_RC_SUCCESS;

      value->addColumn(_T("INDEX"), DCI_DT_UINT, _T("Index"), true);
      value->addColumn(_T("PRODUCT"), DCI_DT_STRING, _T("Product"));
      value->addColumn(_T("MANUFACTURER"), DCI_DT_STRING, _T("Manufacturer"));
      value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));
      value->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));
      value->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("MAC address"));
      value->addColumn(_T("IF_INDEX"), DCI_DT_UINT, _T("Interface index"));
      value->addColumn(_T("SPEED"), DCI_DT_UINT64, _T("Speed"));
      value->addColumn(_T("AVAILABILITY"), DCI_DT_UINT, _T("Availability"));

      IWbemClassObject *classObject = NULL;
      ULONG uRet;
      while(enumObject->Next(WBEM_INFINITE, 1, &classObject, &uRet) == S_OK)
      {
         value->addRow();

         SetColumnFromProperty(value, 0, L"Index", classObject);
         SetColumnFromProperty(value, 1, L"ProductName", classObject);
         SetColumnFromProperty(value, 2, L"Manufacturer", classObject);
         SetColumnFromProperty(value, 3, L"Description", classObject);
         SetColumnFromProperty(value, 4, L"AdapterType", classObject);
         SetColumnFromProperty(value, 5, L"MACAddress", classObject);
         SetColumnFromProperty(value, 6, L"InterfaceIndex", classObject);
         SetColumnFromProperty(value, 7, L"Speed", classObject);
         SetColumnFromProperty(value, 8, L"Availability", classObject);

         classObject->Release();
      }
      enumObject->Release();
      CloseWMIQuery(&ctx);
   }
   return rc;
}

/**
 * Handler for Hardware.NetworkAdapters list
 */
LONG H_NetworkAdaptersList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   LONG rc = SYSINFO_RC_ERROR;

   WMI_QUERY_CONTEXT ctx;
   IEnumWbemClassObject *enumObject = DoWMIQuery(L"root\\cimv2",
      L"SELECT Index FROM Win32_NetworkAdapter " QUERY_CONDITION,
      &ctx);
   if (enumObject != NULL)
   {
      rc = SYSINFO_RC_SUCCESS;

      IWbemClassObject *classObject = NULL;
      ULONG uRet;
      while (enumObject->Next(WBEM_INFINITE, 1, &classObject, &uRet) == S_OK)
      {
         VARIANT v;
         if (classObject->Get(L"Index", 0, &v, 0, 0) == S_OK)
         {
            value->add(VariantToInt(&v));
         }
         classObject->Release();
      }
      enumObject->Release();
      CloseWMIQuery(&ctx);
   }
   return rc;
}

/**
 * Handler for Hardware.NetworkAdapter.* parameters
 */
LONG H_NetworkAdapterProperty(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR indexText[64];
   if (!AgentGetParameterArg(cmd, 1, indexText, 64))
      return SYSINFO_RC_UNSUPPORTED;

   UINT32 index = _tcstoul(indexText, NULL, 0);

   WCHAR query[4096];
   _snwprintf(query, 4096, L"SELECT %s FROM Win32_NetworkAdapter %s AND Index=%u", arg, QUERY_CONDITION, index);

   WMI_QUERY_CONTEXT ctx;
   IEnumWbemClassObject *enumObject = DoWMIQuery(L"root\\cimv2", query, &ctx);
   if (enumObject == NULL)
      return SYSINFO_RC_ERROR;

   IWbemClassObject *classObject = NULL;
   ULONG uRet;
   LONG rc;
   if (enumObject->Next(WBEM_INFINITE, 1, &classObject, &uRet) == S_OK)
   {
      VARIANT v;
      if (classObject->Get(arg, 0, &v, 0, 0) == S_OK)
      {
         VariantToString(&v, value, MAX_RESULT_LENGTH);
         rc = SYSINFO_RC_SUCCESS;
      }
      else
      {
         rc = SYSINFO_RC_ERROR;
      }
      classObject->Release();
   }
   else
   {
      rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   }
   enumObject->Release();
   CloseWMIQuery(&ctx);
   return rc;
}
