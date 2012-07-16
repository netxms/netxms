/* 
** NetXMS - Network Management System
** Windows Console
** Copyright (C) 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: globals.cpp
** This file contain all global vartiables
**
**/

#include "stdafx.h"
#include "nxcon.h"


//
// Session handle
//

NXC_SESSION g_hSession = NULL;


//
// MIB tree
//

SNMP_MIBObject *g_pMIBRoot = NULL;


//
// Graphs
//

DWORD g_dwNumGraphs = 0;
NXC_GRAPH *g_pGraphList = NULL;
HANDLE g_mutexGraphListAccess = INVALID_HANDLE_VALUE;


//
// Connection parameters
//

TCHAR g_szServer[MAX_SERVER_NAME_LEN] = _T("localhost");
TCHAR g_szLogin[MAX_LOGIN_NAME_LEN] = _T("");
TCHAR g_szPassword[MAX_PASSWORD_LEN] = _T("");
int g_nAuthType = NETXMS_AUTH_TYPE_PASSWORD;
TCHAR g_szLastCertName[MAX_DB_STRING] = _T("");


//
// Global configurable parameters
//

DWORD g_dwOptions = OPT_DONT_CACHE_OBJECTS | UI_OPT_CONFIRM_OBJ_DELETE;
TCHAR g_szWorkDir[MAX_PATH] = _T("");
DWORD g_dwMaxLogRecords = 1000;
ALARM_SOUND_CFG g_soundCfg;


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
                            _T("DISABLED"), _T("TESTING"), NULL };
TCHAR *g_szStatusTextSmall[] = { _T("Normal"), _T("Warning"), _T("Minor"), _T("Major"),
                                 _T("Critical"), _T("Unknown"), _T("Unmanaged"),
                                 _T("Disabled"), _T("Testing"), NULL };
TCHAR *g_szAlarmState[] = { _T("Outstanding"), _T("Acknowledged"), _T("Resolved"), _T("Terminated") };
TCHAR *g_szObjectClass[] = { _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"), _T("Network"), 
                             _T("Container"), _T("Zone"), _T("ServiceRoot"), _T("Template"), 
                             _T("TemplateGroup"), _T("TemplateRoot"), _T("NetworkService"),
                             _T("VPNConnector"), _T("Condition"), _T("Cluster"), _T("PolicyGroup"), 
									  _T("PolicyRoot"), _T("AgentPolicy"), _T("AgentPolicyConfig"),
									  _T("NetworkMapRoot"), _T("NetworkMapGroup"), _T("NetworkMap"),
                             _T("DashboardRoot"), _T("Dashboard") };
TCHAR *g_szActionType[] = { _T("Execute"), _T("Remote"), _T("E-Mail"), _T("SMS"), _T("Forward"), _T("Script") };
TCHAR *g_szServiceType[] = { _T("User-defined"), _T("SSH"), _T("POP3"), _T("SMTP"),
                             _T("FTP"), _T("HTTP"), NULL };
TCHAR *g_szDeploymentStatus[] = { _T("Pending"), _T("Uploading package"),
                                  _T("Installing"), _T("Completed"), _T("Failed"),
                                  _T("Initializing") };
TCHAR *g_szToolType[] = { _T("Internal"), _T("Action"), _T("SNMP Table"),
                          _T("Agent Table"), _T("URL"), _T("Client Command"), _T("Server Command") };
TCHAR *g_szToolColFmt[] = { _T("String"), _T("Integer"), _T("Float"),
                            _T("IP Address"), _T("MAC Address"), _T("IfIndex") };
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
   _T("Unknown"),
   _T("Other"),
   _T("Regular 1822"),
   _T("HDH 1822"),
   _T("DDN X.25"),
   _T("RFC877 X.25"),
   _T("Ethernet CSMA/CD"),
   _T("ISO 802.3 CSMA/CD"),
   _T("ISO 802.4 Token Bus"),
   _T("ISO 802.5 Token Ring"),
   _T("ISO 802.6 MAN"),
   _T("StarLan"),
   _T("PROTEON 10 Mbps"),
   _T("PROTEON 80 Mbps"),
   _T("Hyper Channel"),
   _T("FDDI"),
   _T("LAPB"),
   _T("SDLC"),
   _T("DS1"),
   _T("E1"),
   _T("ISDN BRI"),
   _T("ISDN PRI"),
   _T("Proprietary Serial Pt-to-Pt"),
   _T("PPP"),
   _T("Software Loopback"),
   _T("EON (CLNP over IP)"),
   _T("Ethernet 3 Mbps"),
   _T("NSIP (XNS over IP)"),
   _T("SLIP"),
   _T("DS3"),
   _T("SMDS"),
   _T("Frame Relay"),
   _T("RS-232"),
   _T("PARA"),
   _T("ArcNet"),
   _T("ArcNet Plus"),
   _T("ATM"),
   _T("MIO X.25"),
   _T("SONET"),
   _T("X.25 PLE"),
   _T("ISO 88022 LLC"),
   _T("LocalTalk"),
   _T("SMDS DXI"),
   _T("Frame Relay Service"),
   _T("V.35"),
   _T("HSSI"),
   _T("HIPPI"),
   _T("Modem"),
   _T("AAL5"),
   _T("SONET PATH"),
   _T("SONET VT"),
   _T("SMDS ICIP"),
   _T("Proprietary Virtual"),
   _T("Proprietary Multiplexor"),
   _T("IEEE 802.12"),
   _T("FibreChannel")
};
TCHAR *g_szAuthMethod[] = { _T("NetXMS Password"), _T("RADIUS"), _T("Certificate"), 
                            _T("Certificate or Password"), _T("Certificate or RADIUS"), NULL };
TCHAR *g_szCertType[] = { _T("Trusted CA"), _T("User"), NULL };
TCHAR *g_szCertMappingMethod[] = { _T("Subject"), _T("Public Key"), NULL };
TCHAR *g_szGraphType[] = { _T("Line"), _T("Area"), _T("Stacked"), NULL };


//
// Status color table
//

COLORREF g_statusColorTable[9] =
{
   RGB(0, 127, 0),      // Normal
   RGB(0, 255, 255),    // Warning
   RGB(255, 255, 0),    // Minor
   RGB(255, 128, 0),    // Major
   RGB(200, 0, 0),      // Critical
   RGB(61, 12, 187),    // Unknown
   RGB(192, 192, 192),  // Unmanaged
   RGB(92, 0, 0),       // Disabled
   RGB(255, 128, 255)   // Testing
};


//
// Data collection item texts
//

TCHAR *g_pszItemOrigin[] = { _T("Internal"), _T("Agent"), _T("SNMP"), _T("CheckPoint"), _T("Push") };
TCHAR *g_pszItemOriginLong[] = { _T("Internal"), _T("NetXMS Agent"), _T("SNMP Agent"),
                                 _T("CheckPoint SNMP Agent"), _T("Push Agent") };
TCHAR *g_pszItemDataType[] = { _T("Integer"), _T("Unsigned Integer"), _T("Int64"), 
                               _T("Unsigned Int64"), _T("String"), _T("Float") };
TCHAR *g_pszItemStatus[] = { _T("Active"), _T("Disabled"), _T("Not supported") };
TCHAR *g_pszThresholdOperation[] = { _T("<"), _T("<="), _T("="), _T(">="), _T(">"), _T("!="), _T("~"), _T("!~") };
TCHAR *g_pszThresholdOperationLong[] = { _T("less"), _T("less or equal"), _T("equal"), 
                                         _T("greater or equal"), _T("greater"), _T("not equal"), 
                                         _T("like"), _T("not like") };
TCHAR *g_pszThresholdFunction[] = { _T("last"), _T("average"), _T("deviation"),
                                    _T("diff"), _T("error") };
TCHAR *g_pszThresholdFunctionLong[] = { _T("last polled value"), _T("average value"),
                                       _T("mean deviation"),
                                       _T("diff with previous value"),
                                       _T("data collection error") };


//
// SNMP codes
//

CODE_TO_TEXT g_ctSnmpMibStatus[] =
{
   { MIB_STATUS_MANDATORY, _T("Mandatory") },
   { MIB_STATUS_OPTIONAL, _T("Optional") },
   { MIB_STATUS_OBSOLETE, _T("Obsolete") },
   { MIB_STATUS_DEPRECATED, _T("Deprecated") },
   { MIB_STATUS_CURRENT, _T("Current") },
   { 0, NULL }    // End of list
};
CODE_TO_TEXT g_ctSnmpMibAccess[] =
{
   { MIB_ACCESS_READONLY, _T("Read") },
   { MIB_ACCESS_READWRITE, _T("Read/Write") },
   { MIB_ACCESS_WRITEONLY, _T("Write") },
   { MIB_ACCESS_NOACCESS, _T("None") },
   { MIB_ACCESS_NOTIFY, _T("Notify") },
   { MIB_ACCESS_CREATE, _T("Create") },
   { 0, NULL }    // End of list
};
CODE_TO_TEXT g_ctSnmpMibType[] =
{
   { MIB_TYPE_OTHER, _T("Other") },
   { MIB_TYPE_OBJID, _T("Object ID") }, 
   { MIB_TYPE_OCTETSTR, _T("Octet String") },
   { MIB_TYPE_INTEGER, _T("Integer") },
   { MIB_TYPE_NETADDR, _T("Net Address") },
   { MIB_TYPE_IPADDR, _T("IP Address") },
   { MIB_TYPE_COUNTER, _T("Counter") },
   { MIB_TYPE_COUNTER32, _T("Counter") },
   { MIB_TYPE_GAUGE, _T("Gauge") },
   { MIB_TYPE_GAUGE32, _T("Gauge") },
   { MIB_TYPE_TIMETICKS, _T("Timeticks") },
   { MIB_TYPE_OPAQUE, _T("Opaque") },
   { MIB_TYPE_NULL, _T("Null") },
   { MIB_TYPE_COUNTER64, _T("Counter 64bit") },
   { MIB_TYPE_BITSTRING, _T("Bit String") },
   { MIB_TYPE_NSAPADDRESS, _T("NSAP Address") },
   { MIB_TYPE_UINTEGER, _T("Unsigned Integer") },
   { MIB_TYPE_UNSIGNED32, _T("Unsigned Integer 32bit") },
   { MIB_TYPE_INTEGER32, _T("Integer 32bit") },
   { MIB_TYPE_TRAPTYPE, _T("Trap") },
   { MIB_TYPE_NOTIFTYPE, _T("Notification") },
   { MIB_TYPE_OBJGROUP, _T("Object Group") },
   { MIB_TYPE_NOTIFGROUP, _T("NOTIFGROUP") },
   { MIB_TYPE_MODID, _T("Module ID") },
   { MIB_TYPE_AGENTCAP, _T("AGENTCAP") },
   { MIB_TYPE_MODCOMP, _T("MODCOMP") },
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
// Object tools
//

DWORD g_dwNumObjectTools = 0;
NXC_OBJECT_TOOL *g_pObjectToolList = NULL;


//
// Configuration file keywords
//

char g_szConfigKeywords[] = "Action ActionShellExec ControlServers EnableActions EnabledCiphers "
                            "EnableProxy EnableSNMPProxy EnableSubagentAutoload "
									 "ExecTimeout ExternalParameter ExternalParameterShellExec "
									 "FileStore ListenAddress ListenPort LogFile LogHistorySize"
									 "LogUnresolvedSymbols MasterServers MaxLogSize MaxSessions "
                            "PlatformSuffix RequireAuthentication "
                            "RequireEncryption Servers SessionIdleTimeout "
                            "SharedSecret StartupDelay SubAgent WaitForProcess";


//
// Script keywords
//

char g_szScriptKeywords[] = "break continue do else exit for if ilike imatch "
                            "int32 int64 like match NULL print println real return "
                            "string sub typeof uint32 uint64 use while";


//
// Log parser configuration keywords
//

char g_szLogParserKeywords[] = "parser file rules rule match event params";
