/*
** Skeleton NetXMS subagent
** Copyright (C) 2004-2009 Victor Kirhenshtein
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
** $module: skeleton.cpp
**
**/

//#include <windows.h>
#include <nms_common.h>
#include <nms_agent.h>

#ifdef _WIN32
#define SKELETON_EXPORTABLE __declspec(dllexport) __cdecl
#else
#define SKELETON_EXPORTABLE
#endif


//
// Hanlder functions
//

static LONG H_Version(const char *pszParam, const char *pArg, char *pValue)
{
	ret_uint(pValue, 0x01000000);
	return SYSINFO_RC_SUCCESS;
}

static LONG H_Echo(const char *pszParam, const char *pArg, char *pValue)
{
	char szArg[256];

	NxGetParameterArg(pszParam, 1, szArg, 255);
	ret_string(pValue, szArg);
	return SYSINFO_RC_SUCCESS;
}

static LONG H_Random(const char *pszParam, const char *pArg, char *pValue)
{
	srand(time(NULL));
	ret_int(pValue, rand() % 21 - 10);
	return SYSINFO_RC_SUCCESS;
}

static LONG H_Enum(const char *pszParam, const char *pArg, NETXMS_VALUES_LIST *pValue)
{
	int i;
	char szValue[256];

	for(i = 0; i < 10; i++)
	{
		sprintf(szValue, "Value %d", i);
		NxAddResultString(pValue, szValue);
	}
	return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent to initialize subagent
//

static BOOL SubAgentInit(Config *config)
{
	/* you can perform any initialization tasks here */
	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown(void)
{
	/* you can perform necessary shutdown tasks here */
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Skeleton.Version",				H_Version,				NULL,
		DCI_DT_STRING, "Skeleton version" },
	{ "Skeleton.Echo(*)",				H_Echo,					NULL,
		DCI_DT_STRING, "Echoes string back" },
	{ "Skeleton.Random",					H_Random,				NULL,
		DCI_DT_INT,    "Generates random number in range -10 .. 10" }
};
static NETXMS_SUBAGENT_ENUM m_enums[] =
{
	{ "Skeleton.Enum", H_Enum, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("SKELETON"), _T("1.0"),
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums,
	0,
	NULL
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(SKELETON)
{
	*ppInfo = &m_info;
	return TRUE;
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


//
// NetWare library entry point
//

#ifdef _NETWARE

int _init(void)
{
	return 0;
}

int _fini(void)
{
	return 0;
}

#endif
