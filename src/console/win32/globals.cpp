/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2004 Victor Kirhenshtein
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
#include "nxcon.h"


//
// Connection parameters
//

char g_szServer[MAX_SERVER_NAME_LEN] = "localhost";
char g_szLogin[MAX_LOGIN_NAME_LEN] = "";
char g_szPassword[MAX_PASSWORD_LEN] = "";


//
// Global string constants
//

char *g_szStatusText[] = { "NORMAL", "MINOR", "WARNING", "MAJOR", "CRITICAL", "UNKNOWN", "UNMANAGED" };
char *g_szStatusTextSmall[] = { "Normal", "Minor", "Warning", "Major", "Critical", "Unknown", "Unmanaged" };
char *g_szObjectClass[] = { "Generic", "Subnet", "Node", "Interface", "Network", "Location", "Zone" };
char *g_szInterfaceTypes[] = {
   "Unknown",
   "Other",
   "Regular 1822",
   "HDH 1822",
   "DDN X.25",
   "RFC877 X.25",
   "Ethernet CSMA/CD",
   "ISO 802.3 CSMA/CD",
   "ISO 802.4 Token Bus",
   "ISO 802.5 Token Ring",
   "ISO 802.6 MAN",
   "StarLan",
   "PROTEON 10 Mbps",
   "PROTEON 80 Mbps",
   "Hyper Channel",
   "FDDI",
   "LAPB",
   "SDLC",
   "DS1",
   "E1",
   "ISDN BRI",
   "ISDN PRI",
   "Proprietary Serial Pt-to-Pt",
   "PPP",
   "Software Loopback",
   "EON (CLNP over IP)",
   "Ethernet 3 Mbps",
   "NSIP (XNS over IP)",
   "SLIP",
   "DS3",
   "SMDS",
   "Frame Relay"
};


//
// Status color table
//

COLORREF g_statusColorTable[7] =
{
   RGB(0, 127, 0),      // Normal
   RGB(255, 255, 0),    // Minor
   RGB(249, 131, 0),    // Warning
   RGB(248, 63, 1),     // Major
   RGB(200, 0, 0),      // Critical
   RGB(61, 12, 187),    // Unknown
   RGB(255, 255, 130)   // Unmanaged
};


//
// Data collection item texts
//

char *g_pszItemOrigin[] = { "Internal", "Agent", "SNMP" };
char *g_pszItemOriginLong[] = { "Internal", "NetXMS Agent", "SNMP Agent" };
char *g_pszItemDataType[] = { "Integer", "Int64", "String", "Float" };
char *g_pszItemStatus[] = { "Active", "Disabled", "Not supported" };
char *g_pszThresholdOperation[] = { "<", "<=", "=", ">=", ">", "!=", "~", "!~" };
char *g_pszThresholdFunction[] = { "LAST", "AVG", "DEV" };
