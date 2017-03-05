/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

#include "libnxclient.h"

/**
 * Debug callback
 */
static NXC_DEBUG_CALLBACK s_debugCallback = NULL;

/**
 * Print debug messages
 */
void DebugPrintf(const TCHAR *format, ...)
{
   if (s_debugCallback == NULL)
      return;

   va_list args;
   TCHAR buffer[4096];

   va_start(args, format);
   _vsntprintf(buffer, 4096, format, args);
   va_end(args);
   s_debugCallback(buffer);
}

/**
 * Initialization function
 */
bool LIBNXCLIENT_EXPORTABLE NXCInitialize()
{
   return InitCryptoLib(0xFFFF);
}

/**
 * Shutdown function
 */
void LIBNXCLIENT_EXPORTABLE NXCShutdown()
{
   MsgWaitQueue::shutdown();
}

/**
 * Get library version
 */
UINT32 LIBNXCLIENT_EXPORTABLE NXCGetVersion()
{
   return (NETXMS_VERSION_MAJOR << 24) | (NETXMS_VERSION_MINOR << 16) | NETXMS_VERSION_BUILD;
}

/**
 * Set callback for debug messages
 */
void LIBNXCLIENT_EXPORTABLE NXCSetDebugCallback(NXC_DEBUG_CALLBACK cb)
{
   s_debugCallback = cb;
}

/**
 * Get text for error
 */
const TCHAR LIBNXCLIENT_EXPORTABLE *NXCGetErrorText(UINT32 error)
{
   static const TCHAR *errorText[] =
   {
      _T("Request completed successfully"),
      _T("Component locked"),
      _T("Access denied"),
      _T("Invalid request"),
      _T("Request timed out"),
      _T("Request is out of state"),
      _T("Database failure"),
      _T("Invalid object ID"),
      _T("Object already exist"),
      _T("Communication failure"),
      _T("System failure"),
      _T("Invalid user ID"),
      _T("Invalid argument"),
      _T("Duplicate DCI"),
      _T("Invalid DCI ID"),
      _T("Out of memory"),
      _T("Input/Output error"),
      _T("Incompatible operation"),
      _T("Object creation failed"),
      _T("Loop in object relationship detected"),
      _T("Invalid object name"),
      _T("Invalid alarm ID"),
      _T("Invalid action ID"),
      _T("Operation in progress"),
      _T("Copy operation failed for one or more DCI(s)"),
      _T("Invalid or unknown event code"),
      _T("No interfaces suitable for sending magic packet"),
      _T("No MAC address on interface"),
      _T("Command not implemented"),
      _T("Invalid trap configuration record ID"),
      _T("Requested data collection item is not supported by agent"),
      _T("Client and server versions mismatch"),
      _T("Error parsing package information file"),
      _T("Package with specified properties already installed on server"),
      _T("Package file already exist on server"),
      _T("Server resource busy"),
      _T("Invalid package ID"),
      _T("Invalid IP address"),
      _T("Action is used in event processing policy"),
      _T("Variable not found"),
      _T("Server uses incompatible version of communication protocol"),
      _T("Address already in use"),
      _T("Unable to select cipher"),
      _T("Invalid public key"),
      _T("Invalid session key"),
      _T("Encryption is not supported by peer"),
      _T("Server internal error"),
      _T("Execution of external command failed"),
      _T("Invalid object tool ID"),
      _T("SNMP protocol error"),
      _T("Incorrect regular expression"),
      _T("Parameter is not supported by agent"),
      _T("File I/O operation failed"),
      _T("MIB file is corrupted"),
      _T("File transfer operation already in progress"),
      _T("Invalid job ID"),
      _T("Invalid script ID"),
      _T("Invalid script name"),
      _T("Unknown map name"),
      _T("Invalid map ID"),
      _T("Account disabled"),
      _T("No more grace logins"),
      _T("Server connection broken"),
      _T("Invalid agent configuration ID"),
      _T("Server has lost connection with backend database"),
      _T("Alarm is still open in helpdesk system"),
      _T("Alarm is not in \"outstanding\" state"),
      _T("DCI data source is not a push agent"),
      _T("Error parsing configuration import file"),
      _T("Configuration cannot be imported because of validation errors"),
		_T("Invalid graph ID"),
		_T("Local cryptographic provider failure"),
		_T("Unsupported authentication type"),
		_T("Bad certificate"),
		_T("Invalid certificate ID"),
		_T("SNMP failure"),
		_T("Node has no support for layer 2 topology discovery"),
		_T("Invalid situation ID"),
		_T("No such instance"),
		_T("Invalid event ID"),
		_T("Operation cannot be completed due to agent error"),
		_T("Unknown variable"),
		_T("Requested resource not available"),
		_T("Job cannot be cancelled"),
		_T("Invalid policy ID"),
		_T("Unknown log name"),
		_T("Invalid log handle"),
		_T("New password is too weak"),
		_T("Password was used before"),
		_T("Invalid session handle"),
		_T("Node already is a member of a cluster"),
		_T("Job cannot be hold"),
		_T("Job cannot be unhold"),
		_T("Zone ID is already in use"),
		_T("Invalid zone ID"),
		_T("Cannot delete non-empty zone object"),
		_T("No physical component data"),
		_T("Invalid alarm note ID"),
		_T("Encryption error"),
		_T("Invalid mapping table ID"),
      _T("No software package data"),
		_T("Invalid DCI summary table ID"),
      _T("User is logged in"),
      _T("Error parsing XML"),
      _T("SQL query cost is too high"),
      _T("License violation"),
      _T("Number of client licenses exceeded"),
      _T("Object already exist"),
      _T("No helpdesk link"),
      _T("Helpdesk link communication failure"),
      _T("Helpdesk link access denied"),
      _T("Helpdesk link internal error"),
      _T("LDAP connection error"),
      _T("Routing table unavailable"),
      _T("Switch forwarding database unavailable"),
      _T("Location history not available"),
      _T("Object is in use and cannot be deleted"),
      _T("Script compilation error"),
      _T("Script execution error"),
      _T("Unknown configuration variable"),
      _T("Authentication method not supported"),
      _T("Object with given name already exists"),
      _T("Category is used in event processing policy"),
      _T("Category name is empty"),
      _T("Unable to download file from agent"),
      _T("Invalid tunnel ID")
   };
	return (error <= RCC_INVALID_TUNNEL_ID) ? errorText[error] : _T("No text message for this error");
}

#if defined(_WIN32) && !defined(UNDER_CE)

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
