/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2025 Raden Solutions
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
** File: tools.cpp
**
**/

#include "libnxsrv.h"

/**
 * Global variables
 */
LIBNXSRV_EXPORTABLE_VAR(VolatileCounter64 g_flags) = AF_USE_SYSLOG | AF_CATCH_EXCEPTIONS | AF_WRITE_FULL_DUMP | AF_CACHE_DB_ON_STARTUP;
LIBNXSRV_EXPORTABLE_VAR(int g_dbSyntax) = DB_SYNTAX_UNKNOWN;

/**
 * Agent error codes
 */
static struct
{
   uint32_t code;
   const TCHAR *text;
} s_agentErrors[] =
{
   { ERR_SUCCESS, _T("Success") },
   { ERR_UNKNOWN_COMMAND, _T("Unknown command") },
   { ERR_AUTH_REQUIRED, _T("Authentication required") },
   { ERR_ACCESS_DENIED, _T("Access denied") },
   { ERR_UNKNOWN_METRIC, _T("Unknown metric") },
   { ERR_UNSUPPORTED_METRIC, _T("Unsupported metric") },
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
   { ERR_FILE_STAT_FAILED, _T("Cannot read file metadata") },
   { ERR_MEM_ALLOC_FAILED, _T("Memory allocation failed") },
   { ERR_FILE_DELETE_FAILED, _T("File delete failed") },
   { ERR_NO_SESSION_AGENT, _T("Session agent not available") },
   { ERR_SERVER_ID_UNSET, _T("Server ID is not set") },
   { ERR_NO_SUCH_INSTANCE, _T("No such instance") },
   { ERR_OUT_OF_STATE_REQUEST, _T("Request is out of state") },
   { ERR_ENCRYPTION_ERROR, _T("Encryption error") },
   { ERR_MALFORMED_RESPONSE, _T("Malformed response") },
   { ERR_INVALID_OBJECT, _T("Invalid object") },
   { ERR_FILE_ALREADY_EXISTS, _T("File already exist") },
   { ERR_FOLDER_ALREADY_EXISTS, _T("Folder already exist") },
   { ERR_INVALID_SSH_KEY_ID, _T("Invalid SSH key") },
   { ERR_AGENT_DB_FAILURE, _T("Agent database failure") },
   { ERR_INVALID_HTTP_REQUEST_CODE, _T("Invalid HTTP request code") },
   { ERR_FILE_HASH_MISMATCH, _T("File hash mismatch") },
   { ERR_FUNCTION_NOT_SUPPORTED, _T("Function not supported") },
   { ERR_FILE_APPEND_POSSIBLE, _T("New data can be appended to existing file") },
   { ERR_REMOTE_CONNECT_FAILED, _T("Connect to remote system failed") },
   { ERR_SYSCALL_FAILED, _T("System API call failed") },
   { ERR_TCP_PROXY_DISABLED, _T("TCP proxy function is disabled") },
   { 0xFFFFFFFF, nullptr }
};

/**
 * Resolve agent's error code to text
 */
const TCHAR LIBNXSRV_EXPORTABLE *AgentErrorCodeToText(uint32_t err)
{
   for(int i = 0; s_agentErrors[i].text != nullptr; i++)
      if (err == s_agentErrors[i].code)
         return s_agentErrors[i].text;
   return _T("Unknown error code");
}

/**
 * Convert agent error code to client RCC
 */
uint32_t LIBNXSRV_EXPORTABLE AgentErrorToRCC(uint32_t err)
{
   switch(err)
   {
      case ERR_SUCCESS:
         return RCC_SUCCESS;
      case ERR_ACCESS_DENIED:
         return RCC_AGENT_ACCESS_DENIED;
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
      case ERR_UNKNOWN_METRIC:
         return RCC_UNKNOWN_METRIC;
      case ERR_FILE_APPEND_POSSIBLE:
         return RCC_FILE_APPEND_POSSIBLE;
      case ERR_REMOTE_CONNECT_FAILED:
         return RCC_REMOTE_CONNECT_FAILED;
      case ERR_TCP_PROXY_DISABLED:
         return RCC_TCP_PROXY_DISABLED;
   }
   return RCC_AGENT_ERROR;
}
