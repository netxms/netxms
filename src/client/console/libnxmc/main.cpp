/* 
** NetXMS - Network Management System
** Portable management console - plugin API library
** Copyright (C) 2007 Victor Kirhenshtein
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

#include "libnxmc.h"


//
// Global variables
//

wxAuiNotebook *g_auiNotebook = NULL;
wxAuiManager *g_auiManager = NULL;
wxWindow *g_auiDefaultParent = NULL;
wxFrame *g_appMainFrame = NULL;




//
// Custom events
//

DEFINE_EVENT_TYPE(nxEVT_REQUEST_COMPLETED)
DEFINE_EVENT_TYPE(nxEVT_REFRESH_VIEW)
DEFINE_EVENT_TYPE(nxEVT_NXC_ALARM_CHANGE)
DEFINE_EVENT_TYPE(nxEVT_NXC_OBJECT_CHANGE)


//
// Static data
//

static bool s_isUiInitialized = false;
static nxmcArrayOfRegItems s_regItemList;
static NXC_SESSION s_session = NULL;


//
// Registration item class implementation
//

nxmcItemRegistration::nxmcItemRegistration(NXMC_PLUGIN_HANDLE plugin, const TCHAR *name, int id,
														 int type, const wxIcon &icon, void (*fpHandler)(int))
{
	m_plugin = plugin;
	m_name = _tcsdup(name);
	m_id = id;
	m_type = type;
	m_icon = icon;
	m_fpHandler = fpHandler;
}

nxmcItemRegistration::~nxmcItemRegistration()
{
	safe_free(m_name);
}

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(nxmcArrayOfRegItems);


//
// Add Control Panel item
//

bool LIBNXMC_EXPORTABLE NXMCAddControlPanelItem(NXMC_PLUGIN_HANDLE handle, const TCHAR *name, int id, const wxIcon &icon)
{
	if (s_isUiInitialized)
		return false;	// Currently this registration is not allowed after initialization

	if ((id < 0) || (id >= NXMC_PLUGIN_ID_LIMIT))
		return false;	// ID is out of allowed range

	s_regItemList.Add(new nxmcItemRegistration(handle, name, id, REGITEM_CONTROL_PANEL, icon, NULL));
	return true;
}


//
// Add View menu item
//

bool LIBNXMC_EXPORTABLE NXMCAddViewMenuItem(NXMC_PLUGIN_HANDLE handle, const TCHAR *name, int id)
{
	if (s_isUiInitialized)
		return false;	// Currently this registration is not allowed after initialization

	if ((id < 0) || (id >= NXMC_PLUGIN_ID_LIMIT))
		return false;	// ID is out of allowed range

	s_regItemList.Add(new nxmcItemRegistration(handle, name, id, REGITEM_VIEW_MENU, wxNullIcon, NULL));
	return true;
}


//
// Get list of current registrations
//

nxmcArrayOfRegItems LIBNXMC_EXPORTABLE &NXMCGetRegistrations()
{
	return s_regItemList;
}


//
// Change UI initialization flag
//

void LIBNXMC_EXPORTABLE NXMCInitializationComplete(void)
{
	s_isUiInitialized = true;
}


//
// Set AUI elements
//

void LIBNXMC_EXPORTABLE NXMCInitAUI(wxAuiManager *mgr, wxAuiNotebook *nb, wxWindow *defParent)
{
	g_auiManager = mgr;
	g_auiNotebook = nb;
	g_auiDefaultParent = defParent;
}


//
// Set app main frame window
//

void LIBNXMC_EXPORTABLE NXMCSetMainFrame(wxFrame *frame)
{
	g_appMainFrame = frame;
}


//
// Get default parent window
//

wxWindow LIBNXMC_EXPORTABLE *NXMCGetDefaultParent()
{
	return g_auiDefaultParent;
}


//
// Set current session handle
//

void LIBNXMC_EXPORTABLE NXMCSetSession(NXC_SESSION session)
{
	s_session = session;
}


//
// Get current session handle
//

NXC_SESSION LIBNXMC_EXPORTABLE NXMCGetSession()
{
	return s_session;
}


//
// Get status text
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCGetStatusText(int status)
{
	static const TCHAR *texts[] =
	{
		_T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"), _T("CRITICAL"),
		_T("UNKNOWN"), _T("UNMANAGED"), _T("DISABLED"), _T("TESTING")
	};
	
	return ((status >= 0) && (status < 9)) ? texts[status] : _T("INVALID");
}


//
// Get status text in small letters
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCGetStatusTextSmall(int status)
{
	static const TCHAR *texts[] =
	{
		_T("Normal"), _T("Warning"), _T("Minor"), _T("Major"), _T("Critical"),
		_T("Unknown"), _T("Unmanaged"), _T("Disabled"), _T("Testing")
	};
	
	return ((status >= 0) && (status < 9)) ? texts[status] : _T("Invalid");
}


//
// Get alarm state name
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCGetAlarmStateName(int state)
{
	static const TCHAR *texts[] =
	{
		_T("Outstanding"), _T("Acknowledged"), _T("Terminated")
	};
	
	return ((state >= 0) && (state < 3)) ? texts[state] : _T("Invalid");
}


//
// Get object class name
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCGetClassName(int objClass)
{
	static const TCHAR *names[] = { _T("Generic"), _T("Subnet"), _T("Node"), _T("Interface"), _T("Network"), 
                                   _T("Container"), _T("Zone"), _T("ServiceRoot"), _T("Template"), 
                                   _T("TemplateGroup"), _T("TemplateRoot"), _T("NetworkService"),
                                   _T("VPNConnector"), _T("Condition"), _T("Cluster") };
	
	return ((objClass >= 0) && (objClass <= OBJECT_CLUSTER)) ? names[objClass] : _T("Unknown");
}


//
// Get interface type name
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCGetIfTypeName(int type)
{
	static const TCHAR *types[] = 
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
	
	return ((type >= 0) && (type < sizeof(types) / sizeof(TCHAR *))) ? types[type] : _T("Unknown");
}


//
// Get node type
//

const TCHAR LIBNXMC_EXPORTABLE *NXMCGetNodeTypeName(int type)
{
	static CODE_TO_TEXT types[] =
	{
		{ NODE_TYPE_GENERIC, _T("Generic") },
		{ NODE_TYPE_NORTEL_ACCELAR, _T("Nortel Networks Passport switch") },
		{ NODE_TYPE_NETSCREEN, _T("NetScreen Firewall/VPN") },
		{ NODE_TYPE_NORTEL_BAYSTACK, _T("Nortel Ethernet switch (former BayStack)") },
		{ NODE_TYPE_NORTEL_OPTERA, _T("Nortel Optera Metro switch") },
		{ 0, NULL }    // End of list
	};
	
	return NXMCCodeToText(type, types, _T("Unknown"));
}


//
// Get status color
//

const wxColour LIBNXMC_EXPORTABLE &NXMCGetStatusColor(int status)
{
	static wxColour statusColorTable[9] =
	{
		wxColour(0, 127, 0),      // Normal
		wxColour(0, 255, 255),    // Warning
		wxColour(255, 255, 0),    // Minor
		wxColour(255, 128, 0),    // Major
		wxColour(255, 0, 0),      // Critical
		wxColour(61, 12, 187),    // Unknown
		wxColour(192, 192, 192),  // Unmanaged
		wxColour(92, 0, 0),       // Disabled
		wxColour(255, 128, 255)   // Testing
	};
	return ((status >= STATUS_NORMAL) && (status <= STATUS_TESTING)) ? statusColorTable[status] : *wxBLACK;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
