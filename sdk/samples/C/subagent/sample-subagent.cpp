/*
** Sample NetXMS subagent
** Copyright (c) 2011 RadenSolutions
**
** This software is provided 'as-is', without any express or implied
** warranty.  In no event will the authors be held liable for any damages
** arising from the use of this software.
**
** Permission is granted to anyone to use this software for any purpose,
** including commercial applications, and to alter it and redistribute it
** freely, subject to the following restrictions:
**
** 1. The origin of this software must not be misrepresented; you must not
**    claim that you wrote the original software. If you use this software
**    in a product, an acknowledgment in the product documentation would be
**    appreciated but is not required.
** 2. Altered source versions must be plainly marked as such, and must not be
**    misrepresented as being the original software.
** 3. This notice may not be removed or altered from any source distribution.
**
**/

#include <nms_common.h>
#include <nms_agent.h>


//
// Hanlder functions
//

static LONG H_Version(const char *parameter, const char *arguments, char *value)
{
	ret_uint(value, 0x01000000);
	return SYSINFO_RC_SUCCESS;
}

static LONG H_Echo(const char *parameter, const char *arguments, char *value)
{
	char tmp[256];
	AgentGetParameterArg(parameter, 1, tmp, 255);
	ret_string(value, tmp);
	return SYSINFO_RC_SUCCESS;
}

static LONG H_Random(const char *parameter, const char *arguments, char *value)
{
	srand((unsigned int)time(NULL));
	ret_int(value, rand() % 21 - 10);
	return SYSINFO_RC_SUCCESS;
}

static LONG H_Enum(const char *parameter, const char *arguments, StringList *value)
{
	int i;
	char tmp[256];

	for(i = 0; i < 10; i++)
	{
		sprintf(tmp, "Value %d", i);
		value->add(tmp);
	}
	return SYSINFO_RC_SUCCESS;
}

//
// Action handler functions
//
static LONG H_ActionSample(const TCHAR *action, StringList *argumentsList, const TCHAR *data)
{
	return ERR_SUCCESS;
}

//
// Called by master agent to initialize subagent
//

static BOOL SubAgentInit(Config *config)
{
	/* you can perform any initialization tasks here */
	AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("Sample subagent initialized"));
	return TRUE;
}


//
// Called by master agent at unload
//

static void SubAgentShutdown()
{
	/* you can perform necessary shutdown tasks here */
	AgentWriteLog(EVENTLOG_INFORMATION_TYPE, _T("Sample subagent unloaded"));
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "Sample.Version",				H_Version,				NULL,
		DCI_DT_STRING, "Sample subagent version" },
	{ "Sample.Echo(*)",				H_Echo,					NULL,
		DCI_DT_STRING, "Echoes string back" },
	{ "Sample.Random",					H_Random,				NULL,
		DCI_DT_INT,    "Generates random number in range -10 .. 10" }
};
static NETXMS_SUBAGENT_LIST m_enums[] =
{
	{ "Sample.List", H_Enum, NULL }
};
static NETXMS_SUBAGENT_ACTION m_actions[] =
{
	{ "Sample.Action",					H_ActionSample, NULL, "Sample action" },
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("SAMPLE"), _T("1.0.0"),
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_LIST),
	m_enums,
	0, NULL,	// tables
	sizeof(m_actions) / sizeof(NETXMS_SUBAGENT_ACTION),
	m_actions,
	0, NULL	// push parameters
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(SAMPLE)
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
	{
		DisableThreadLibraryCalls(hInstance);
	}
	return TRUE;
}

#endif
