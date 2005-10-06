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
#include <nxnt.h>

#define _GETOPT_H_ 1    /* Prevent including getopt.h from net-snmp */
#define HAVE_SOCKLEN_T  /* Prevent defining socklen_t in net-snmp */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/mib_api.h>
#include <net-snmp/config_api.h>


//
// Session handle
//

NXC_SESSION g_hSession = NULL;


//
// Connection parameters
//

TCHAR g_szServer[MAX_SERVER_NAME_LEN] = _T("localhost");
TCHAR g_szLogin[MAX_LOGIN_NAME_LEN] = _T("");
TCHAR g_szPassword[MAX_PASSWORD_LEN] = _T("");


//
// Global configurable parameters
//

DWORD g_dwOptions = 0;
TCHAR g_szWorkDir[MAX_PATH] = _T("");
DWORD g_dwMaxLogRecords = 2000;


//
// Server image list
//

NXC_IMAGE_LIST *g_pSrvImageList = NULL;


//
// Image lists with object images
//

CImageList *g_pObjectSmallImageList = NULL;
CImageList *g_pObjectNormalImageList = NULL;


//
// Global string constants
//

TCHAR *g_szStatusText[] = { _T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"),
                            _T("CRITICAL"), _T("UNKNOWN"), _T("UNMANAGED"),
                            _T("DISABLED"), _T("TESTING") };
TCHAR *g_szStatusTextSmall[] = { _T("Normal"), _T("Warning"), _T("Minor"), _T("Major"),
                                 _T("Critical"), _T("Unknown"), _T("Unmanaged"),
                                 _T("Disabled"), _T("Testing") };
TCHAR *g_szObjectClass[] = { _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"), _T("Network"), 
                             _T("Container"), _T("Zone"), _T("ServiceRoot"), _T("Template"), 
                             _T("TemplateGroup"), _T("TemplateRoot"), _T("NetworkService"),
                             _T("VPNConnector") };
TCHAR *g_szActionType[] = { _T("Execute"), _T("Remote"), _T("E-Mail"), _T("SMS") };
TCHAR *g_szServiceType[] = { _T("User-defined"), _T("SSH"), _T("POP3"), _T("SMTP"),
                             _T("FTP"), _T("HTTP"), NULL };
TCHAR *g_szDeploymentStatus[] = { _T("Pending"), _T("Uploading package"),
                                  _T("Installing"), _T("Completed"), _T("Failed"),
                                  _T("Initializing") };
TCHAR *g_szSyslogSeverity[] = { _T("Emergency"), _T("Alert"), _T("Critical"),
                                _T("Error"), _T("Warning"), _T("Notice"),
                                _T("Informational"), _T("Debug") };
TCHAR *g_szSyslogFacility[] =
{
   _T("Kernel"),
   _T("User"),
   _T("Mail"),
   _T("System"),
   _T("Auth"),
   _T("Syslog"),
   _T("Lpr"),
   _T("News"),
   _T("UUCP"),
   _T("Cron"),
   _T("Security"),
   _T("FTP daemon"),
   _T("NTP"),
   _T("Log audit"),
   _T("Log alert"),
   _T("Clock daemon"),
   _T("Local0"),
   _T("Local1"),
   _T("Local2"),
   _T("Local3"),
   _T("Local4"),
   _T("Local5"),
   _T("Local6"),
   _T("Local7")
};
TCHAR *g_szInterfaceTypes[] = 
{
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
   RGB(192, 192, 192),  // Unmanaged
   RGB(91, 0, 6),       // Disabled
   RGB(255, 135, 255)   // Testing
};


//
// Data collection item texts
//

TCHAR *g_pszItemOrigin[] = { _T("Internal"), _T("Agent"), _T("SNMP"), _T("CheckPoint") };
TCHAR *g_pszItemOriginLong[] = { _T("Internal"), _T("NetXMS Agent"), _T("SNMP Agent"),
                                 _T("CheckPoint SNMP Agent") };
TCHAR *g_pszItemDataType[] = { _T("Integer"), _T("Unsigned Integer"), _T("Int64"), 
                               _T("Unsigned Int64"), _T("String"), _T("Float") };
char *g_pszItemStatus[] = { "Active", "Disabled", "Not supported" };
char *g_pszThresholdOperation[] = { "<", "<=", "=", ">=", ">", "!=", "~", "!~" };
char *g_pszThresholdOperationLong[] = { "less", "less or equal", "equal", 
                                        "greater or equal", "greater", "not equal", 
                                        "like", "not like" };
char *g_pszThresholdFunction[] = { "last", "average", "deviation", "diff" };
char *g_pszThresholdFunctionLong[] = { "last polled value", "average value",
                                       "mean deviation", "diff with previous value" };


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


//
// Default object image list
//

DWORD g_dwDefImgListSize = 0;
DEF_IMG *g_pDefImgList = NULL;


//
// Action list
//

DWORD g_dwNumActions = 0;
NXC_ACTION *g_pActionList = NULL;
HANDLE g_mutexActionListAccess = INVALID_HANDLE_VALUE;


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


//
// Object tools
//

DWORD g_dwNumObjectTools = 0;
NXC_OBJECT_TOOL *g_pObjectToolList = NULL;
CMenu *g_pObjectToolsMenu = NULL;
