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
// Global configurable parameters
//

char g_szWorkDir[MAX_PATH] = "";


//
// Global string constants
//

char *g_szStatusText[] = { "NORMAL", "WARNING", "MINOR", "MAJOR", "CRITICAL", "UNKNOWN", "UNMANAGED", "DISABLED", "TESTING" };
char *g_szStatusTextSmall[] = { "Normal", "Warning", "Minor", "Major", "Critical", "Unknown", "Unmanaged", "Disabled", "Testing" };
char *g_szObjectClass[] = { "Generic", "Subnet", "Node", "Interface", "Network", "Container", "Zone", "ServiceRoot" };
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

COLORREF g_statusColorTable[9] =
{
   RGB(0, 127, 0),      // Normal
   RGB(255, 255, 0),    // Warning
   RGB(249, 131, 0),    // Minor
   RGB(248, 63, 1),     // Major
   RGB(200, 0, 0),      // Critical
   RGB(61, 12, 187),    // Unknown
   RGB(255, 255, 130),  // Unmanaged
   RGB(91, 0, 6),       // Disabled
   RGB(255, 135, 255)   // Testing
};


//
// Data collection item texts
//

char *g_pszItemOrigin[] = { "Internal", "Agent", "SNMP" };
char *g_pszItemOriginLong[] = { "Internal", "NetXMS Agent", "SNMP Agent" };
char *g_pszItemDataType[] = { "Integer", "Int64", "String", "Float" };
char *g_pszItemStatus[] = { "Active", "Disabled", "Not supported" };
char *g_pszThresholdOperation[] = { "<", "<=", "=", ">=", ">", "!=", "~", "!~" };
char *g_pszThresholdOperationLong[] = { "less", "less or equal", "equal", "greater or equal", "greater", "not equal", "like", "not like" };
char *g_pszThresholdFunction[] = { "last", "average", "deviation" };
char *g_pszThresholdFunctionLong[] = { "last polled value", "average value", "mean deviation" };


//
// SNMP codes
//

CODE_TO_TEXT g_ctSnmpMibStatus[] =
{
   { MIB_STATUS_MANDATORY, "Mandatory" },
   { MIB_STATUS_OPTIONAL, "Optional" },
   { MIB_STATUS_OBSOLETE, "Obsolete" },
   { MIB_STATUS_DEPRECATED, "Deprecated" },
   { MIB_STATUS_CURRENT, "Current" },
   { 0, NULL }    // End of list
};
CODE_TO_TEXT g_ctSnmpMibAccess[] =
{
   { MIB_ACCESS_READONLY, "Read" },
   { MIB_ACCESS_READWRITE, "Read/Write" },
   { MIB_ACCESS_WRITEONLY, "Write" },
   { MIB_ACCESS_NOACCESS, "None" },
   { MIB_ACCESS_NOTIFY, "Notify" },
   { MIB_ACCESS_CREATE, "Create" },
   { 0, NULL }    // End of list
};
CODE_TO_TEXT g_ctSnmpMibType[] =
{
   { TYPE_OTHER, "Other" },
   { TYPE_OBJID, "Object ID" }, 
   { TYPE_OCTETSTR, "Octet String" },
   { TYPE_INTEGER, "Integer" },
   { TYPE_NETADDR, "Net Address" },
   { TYPE_IPADDR, "IP Address" },
   { TYPE_COUNTER, "Counter" },
   { TYPE_GAUGE, "Gauge" },
   { TYPE_TIMETICKS, "Timeticks" },
   { TYPE_OPAQUE, "Opaque" },
   { TYPE_NULL, "Null" },
   { TYPE_COUNTER64, "Counter 64bit" },
   { TYPE_BITSTRING, "Bit String" },
   { TYPE_NSAPADDRESS, "NSAP Address" },
   { TYPE_UINTEGER, "Unsigned Integer" },
   { TYPE_UNSIGNED32, "Unsigned Integer 32bit" },
   { TYPE_INTEGER32, "Integer 32bit" },
   { TYPE_TRAPTYPE, "TRAPTYPE" },
   { TYPE_NOTIFTYPE, "NOTIFTYPE" },
   { TYPE_OBJGROUP, "Object Group" },
   { TYPE_NOTIFGROUP, "NOTIFGROUP" },
   { TYPE_MODID, "Module ID" },
   { TYPE_AGENTCAP, "AGENTCAP" },
   { TYPE_MODCOMP, "MODCOMP" },
   { 0, NULL }    // End of list
};
