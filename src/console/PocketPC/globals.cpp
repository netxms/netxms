/* 
** NetXMS - Network Management System
** PocketPC Console
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: globals.cpp
** This file contain all global vartiables
**
**/

#include "stdafx.h"
#include "nxpc.h"
#include <nxnt.h>


//
// Session handle
//

NXC_SESSION g_hSession = NULL;


//
// Connection parameters
//

TCHAR g_szServer[MAX_SERVER_NAME_LEN] = L"localhost";
TCHAR g_szLogin[MAX_LOGIN_NAME_LEN] = L"";
TCHAR g_szPassword[MAX_PASSWORD_LEN] = L"";
DWORD g_dwEncryptionMethod = CSCP_ENCRYPTION_NONE;


//
// Global configurable parameters
//

DWORD g_dwFlags = 0;
TCHAR g_szWorkDir[MAX_PATH] = L"";


//
// Global string constants
//

TCHAR *g_szStatusText[] = { L"NORMAL", L"WARNING", L"MINOR", L"MAJOR",
                            L"CRITICAL", L"UNKNOWN", L"UNMANAGED",
                            L"DISABLED", L"TESTING" };
TCHAR *g_szStatusTextSmall[] = { L"Normal", L"Warning", L"Minor", L"Major",
                                 L"Critical", L"Unknown", L"Unmanaged",
                                 L"Disabled", L"Testing" };
TCHAR *g_szObjectClass[] = { _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"), _T("Network"), 
                             _T("Container"), _T("Zone"), _T("ServiceRoot"), _T("Template"), 
                             _T("TemplateGroup"), _T("TemplateRoot"), _T("NetworkService") };
TCHAR *g_szActionType[] = { _T("Execute"), _T("Remote"), _T("E-Mail"), _T("SMS") };
TCHAR *g_szServiceType[] = { _T("User-defined"), _T("SSH"), _T("POP3"), _T("SMTP"),
                             _T("FTP"), _T("HTTP"), NULL };
TCHAR *g_szInterfaceTypes[] = 
{
   L"Unknown",
   L"Other",
   L"Regular 1822",
   L"HDH 1822",
   L"DDN X.25",
   L"RFC877 X.25",
   L"Ethernet CSMA/CD",
   L"ISO 802.3 CSMA/CD",
   L"ISO 802.4 Token Bus",
   L"ISO 802.5 Token Ring",
   L"ISO 802.6 MAN",
   L"StarLan",
   L"PROTEON 10 Mbps",
   L"PROTEON 80 Mbps",
   L"Hyper Channel",
   L"FDDI",
   L"LAPB",
   L"SDLC",
   L"DS1",
   L"E1",
   L"ISDN BRI",
   L"ISDN PRI",
   L"Proprietary Serial Pt-to-Pt",
   L"PPP",
   L"Software Loopback",
   L"EON (CLNP over IP)",
   L"Ethernet 3 Mbps",
   L"NSIP (XNS over IP)",
   L"SLIP",
   L"DS3",
   L"SMDS",
   L"Frame Relay"
};


//
// Status color table
//

COLORREF g_statusColorTable[9] =
{
   RGB(0, 127, 0),      // Normal
   RGB(255, 255, 0),    // Warning
   RGB(249, 131, 0),    // Minor
   RGB(248, 63, 1),     // Major
   RGB(200, 0, 0),      // Critical
   RGB(61, 12, 187),    // Unknown
   RGB(192, 192, 192),  // Unmanaged
   RGB(91, 0, 6),       // Disabled
   RGB(255, 135, 255)   // Testing
};


//
// Data collection item texts
//

TCHAR *g_pszItemOrigin[] = { _T("Internal"), _T("Agent"), _T("SNMP") };
TCHAR *g_pszItemOriginLong[] = { _T("Internal"), _T("NetXMS Agent"), _T("SNMP Agent") };
TCHAR *g_pszItemDataType[] = { _T("Integer"), _T("Unsigned Integer"), _T("Int64"), _T("Unsigned Int64"), _T("String"), _T("Float") };
TCHAR *g_pszItemStatus[] = { _T("Active"), _T("Disabled"), _T("Not supported") };


//
// Container categories list
//

NXC_CC_LIST *g_pCCList = NULL;


//
// Well-known node types
//

CODE_TO_TEXT g_ctNodeType[] =
{
   { NODE_TYPE_GENERIC, _T("Generic") },
   { NODE_TYPE_NORTEL_ACCELAR, _T("Nortel Networks Passport switch") },
   { 0, NULL }    // End of list
};
