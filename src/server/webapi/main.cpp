/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
** File: main.cpp
**
**/

#include "webapi.h"
#include <netxms-version.h>

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("WEBAPI", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Handlers
 */
int H_AlarmAcknowledge(Context *context);
int H_AlarmDetails(Context *context);
int H_AlarmResolve(Context *context);
int H_AlarmTerminate(Context *context);
int H_Alarms(Context *context);
int H_DataCollectionCurrentValues(Context *context);
int H_DataCollectionHistory(Context *context);
int H_GrafanaDciList(Context *context);
int H_FindMacAddress(Context *context);
int H_GrafanaGetAlarms(Context *context);
int H_GrafanaGetSummaryTable(Context *context);
int H_GrafanaObjectList(Context *context);
int H_GrafanaGetObjectQuery(Context *context);
int H_GrafanaGetObjectsStatus(Context *context);
int H_ObjectDetails(Context *context);
int H_ObjectSubTree(Context *context);
int H_ObjectExecuteAgentCommand(Context *context);
int H_ObjectExecuteScript(Context *context);
int H_ObjectExpandText(Context *context);
int H_ObjectQuery(Context *context);
int H_ObjectSetMaintenance(Context *context);
int H_ObjectSetManaged(Context *context);
int H_Objects(Context *context);
int H_ObjectSearch(Context *context);
int H_ObjectTools(Context *context);
int H_GrafanaObjectQueryList(Context *context);
int H_QueryAdHocSummaryTable(Context *context);
int H_QuerySummaryTable(Context *context);
int H_ServerInfo(Context *context);
int H_Status(Context *context);
int H_SummaryTables(Context *context);
int H_GrafanaSummaryTablesList(Context *context);
int H_TakeScreenshot(Context *context);

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   RouteBuilder("v1/alarms")
      .GET(H_Alarms)
      .build();
   RouteBuilder("v1/alarms/:alarm-id")
      .GET(H_AlarmDetails)
      .build();
   RouteBuilder("v1/alarms/:alarm-id/acknowledge")
      .POST(H_AlarmAcknowledge)
      .build();
   RouteBuilder("v1/alarms/:alarm-id/resolve")
      .POST(H_AlarmResolve)
      .build();
   RouteBuilder("v1/alarms/:alarm-id/terminate")
      .POST(H_AlarmTerminate)
      .build();
   RouteBuilder("v1/dci-summary-tables")
      .GET(H_SummaryTables)
      .build();
   RouteBuilder("v1/dci-summary-tables/:table-id/query")
      .POST(H_QuerySummaryTable)
      .build();
   RouteBuilder("v1/dci-summary-tables/adhoc-query")
      .POST(H_QueryAdHocSummaryTable)
      .build();
   RouteBuilder("v1/find/mac-address")
      .GET(H_FindMacAddress)
      .build();
   RouteBuilder("v1/grafana/infinity/alarms")
      .POST(H_GrafanaGetAlarms)
      .build();
   RouteBuilder("v1/grafana/infinity/object-query")
      .POST(H_GrafanaGetObjectQuery)
      .build();
   RouteBuilder("v1/grafana/infinity/summary-table")
      .POST(H_GrafanaGetSummaryTable)
      .build();
   RouteBuilder("v1/grafana/objects/:object-id/dci-list")
      .GET(H_GrafanaDciList)
      .build();
   RouteBuilder("v1/grafana/objects-status")
      .POST(H_GrafanaGetObjectsStatus)
      .build();
   RouteBuilder("v1/grafana/object-list")
      .GET(H_GrafanaObjectList)
      .build();
   RouteBuilder("v1/grafana/summary-table-list")
      .GET(H_GrafanaSummaryTablesList)
      .build();
   RouteBuilder("v1/grafana/query-list")
      .GET(H_GrafanaObjectQueryList)
      .build();
   RouteBuilder("v1/objects")
      .GET(H_Objects)
      .build();
   RouteBuilder("v1/objects/:object-id")
      .GET(H_ObjectDetails)
      .build();
   RouteBuilder("v1/objects/:object-id/data-collection/current-values")
      .GET(H_DataCollectionCurrentValues)
      .build();
   RouteBuilder("v1/objects/:object-id/data-collection/:dci-id/history")
      .GET(H_DataCollectionHistory)
      .build();
   RouteBuilder("v1/objects/:object-id/execute-agent-command")
      .POST(H_ObjectExecuteAgentCommand)
      .build();
   RouteBuilder("v1/objects/:object-id/execute-script")
      .POST(H_ObjectExecuteScript)
      .build();
   RouteBuilder("v1/objects/:object-id/expand-text")
      .POST(H_ObjectExpandText)
      .build();
   RouteBuilder("v1/objects/:object-id/set-maintenance")
      .POST(H_ObjectSetMaintenance)
      .build();
   RouteBuilder("v1/objects/:object-id/set-managed")
      .POST(H_ObjectSetManaged)
      .build();
   RouteBuilder("v1/objects/:object-id/take-screenshot")
      .GET(H_TakeScreenshot)
      .build();
   RouteBuilder("v1/objects/:object-id/sub-tree")
      .GET(H_ObjectSubTree)
      .build();
   RouteBuilder("v1/objects/query")
      .POST(H_ObjectQuery)
      .build();
   RouteBuilder("v1/objects/search")
      .POST(H_ObjectSearch)
      .build();
   RouteBuilder("v1/object-tools")
      .GET(H_ObjectTools)
      .build();
   RouteBuilder("v1/server-info")
      .GET(H_ServerInfo)
      .build();
   RouteBuilder("v1/status")
      .GET(H_Status)
      .build();

   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"WEBAPI");
   module->pfInitialize = InitModule;
   return true;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif
