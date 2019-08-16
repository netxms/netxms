/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2019 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include "libnxsrv.h"

/**
 * Global variables
 */
LIBNXSRV_EXPORTABLE_VAR(UINT64 g_flags) = AF_USE_SYSLOG | AF_CATCH_EXCEPTIONS | AF_WRITE_FULL_DUMP | AF_LOG_SQL_ERRORS | AF_ENABLE_LOCAL_CONSOLE | AF_CACHE_DB_ON_STARTUP;

/**
 * Agent error codes
 */
static struct
{
   int iCode;
   const TCHAR *pszText;
} m_agentErrors[] =
{
   { ERR_SUCCESS, _T("Success") },
   { ERR_UNKNOWN_COMMAND, _T("Unknown command") },
   { ERR_AUTH_REQUIRED, _T("Authentication required") },
   { ERR_ACCESS_DENIED, _T("Access denied") },
   { ERR_UNKNOWN_PARAMETER, _T("Unknown parameter") },
   { ERR_REQUEST_TIMEOUT, _T("Request timeout") },
   { ERR_AUTH_FAILED, _T("Authentication failed") },
   { ERR_ALREADY_AUTHENTICATED, _T("Already authenticated") },
   { ERR_AUTH_NOT_REQUIRED, _T("Authentication not required") },
   { ERR_INTERNAL_ERROR, _T("Internal error") },
   { ERR_NOT_IMPLEMENTED, _T("Not implemented") },
   { ERR_OUT_OF_RESOURCES, _T("Out of resources") },
   { ERR_NOT_CONNECTED, _T("Not connected") },
   { ERR_CONNECTION_BROKEN, _T("Connection broken") },
   { ERR_BAD_RESPONSE, _T("Bad response") },
   { ERR_IO_FAILURE, _T("I/O failure") },
   { ERR_RESOURCE_BUSY, _T("Resource busy") },
   { ERR_EXEC_FAILED, _T("External program execution failed") },
   { ERR_ENCRYPTION_REQUIRED, _T("Encryption required") },
   { ERR_NO_CIPHERS, _T("No acceptable ciphers") },
   { ERR_INVALID_PUBLIC_KEY, _T("Invalid public key") },
   { ERR_INVALID_SESSION_KEY, _T("Invalid session key") },
   { ERR_CONNECT_FAILED, _T("Connect failed") },
   { ERR_MALFORMED_COMMAND, _T("Malformed command") },
   { ERR_SOCKET_ERROR, _T("Socket error") },
   { ERR_BAD_ARGUMENTS, _T("Bad arguments") },
   { ERR_SUBAGENT_LOAD_FAILED, _T("Subagent load failed") },
   { ERR_FILE_OPEN_ERROR, _T("File open error") },
   { ERR_FILE_STAT_FAILED, _T("File stat filed") },
   { ERR_MEM_ALLOC_FAILED, _T("Memory allocation failed") },
   { ERR_FILE_DELETE_FAILED, _T("File delete failed") },
   { ERR_NO_SESSION_AGENT, _T("Session agent not available") },
   { ERR_SERVER_ID_UNSET, _T("Server ID is not set") },
   { ERR_NO_SUCH_INSTANCE, _T("No such instance") },
   { ERR_OUT_OF_STATE_REQUEST, _T("Request is out of state") },
   { ERR_ENCRYPTION_ERROR, _T("Encryption error") },
   { -1, NULL }
};

/**
 * Resolve agent's error code to text
 */
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(UINT32 err)
{
   int i;

   for(i = 0; m_agentErrors[i].pszText != NULL; i++)
      if (err == (UINT32)m_agentErrors[i].iCode)
         return m_agentErrors[i].pszText;
   return _T("Unknown error code");
}

/**
 * Convert agent error code to client RCC
 */
UINT32 LIBNXSRV_EXPORTABLE AgentErrorToRCC(UINT32 err)
{
   switch(err)
   {
      case ERR_SUCCESS:
         return RCC_SUCCESS;
      case ERR_ACCESS_DENIED:
         return RCC_ACCESS_DENIED;
      case ERR_IO_FAILURE:
         return RCC_IO_ERROR;
      case ERR_ALREADY_AUTHENTICATED:
      case ERR_AUTH_FAILED:
      case ERR_AUTH_NOT_REQUIRED:
         return RCC_COMM_FAILURE;
      case ERR_NO_SUCH_INSTANCE:
         return RCC_NO_SUCH_INSTANCE;
      case ERR_REQUEST_TIMEOUT:
         return RCC_TIMEOUT;
      case ERR_ENCRYPTION_ERROR:
         return RCC_ENCRYPTION_ERROR;
      case ERR_OUT_OF_STATE_REQUEST:
         return RCC_OUT_OF_STATE_REQUEST;
      case ERR_FILE_ALREADY_EXISTS:
         return RCC_FILE_ALREADY_EXISTS;
      case ERR_FOLDER_ALREADY_EXISTS:
         return RCC_FOLDER_ALREADY_EXISTS;
   }
   return RCC_AGENT_ERROR;
}

/**
 * Destroy routing table
 */
void LIBNXSRV_EXPORTABLE DestroyRoutingTable(ROUTING_TABLE *pRT)
{
   if (pRT != NULL)
   {
      MemFree(pRT->pRoutes);
      free(pRT);
   }
}

/**
 * Route comparision callback
 */
static int CompareRoutes(const void *p1, const void *p2)
{
   return -(COMPARE_NUMBERS(((ROUTE *)p1)->dwDestMask, ((ROUTE *)p2)->dwDestMask));
}

/**
 * Sort routing table (put most specific routes first)
 */
void LIBNXSRV_EXPORTABLE SortRoutingTable(ROUTING_TABLE *pRT)
{
   qsort(pRT->pRoutes, pRT->iNumEntries, sizeof(ROUTE), CompareRoutes);
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
