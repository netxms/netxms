/*
 ** NetXMS - Network Management System
 ** Subagent for PostgreSQL monitoring
 ** Copyright (C) 2015 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published
 ** by the Free Software Foundation; either version 3 of the License, or
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
 **/

/**
 * Driver handle
 */
DB_DRIVER g_driverHandle = NULL;

/*
 * Subagent initialization
 */
static BOOL SubAgentInit(Config *config)
{
   int i;

	// Init db driver
	g_driverHandle = DBLoadDriver(_T("pgsql.ddr"), NULL, TRUE, NULL, NULL);
	if (g_driverHandle == NULL)
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("%s: failed to load database driver"), MYNAMESTR);
		return FALSE;
	}

	return TRUE;
}

/**
 * Shutdown handler
 */
static void SubAgentShutdown()
{
	AgentWriteDebugLog(1, _T("PGSQL: stopped"));
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("PGSQL"), NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM), s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST), s_lists,
	sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE), s_tables,
	0,	NULL,    // actions
	0,	NULL     // push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(PGSQL)
{
	*ppInfo = &s_info;
	return TRUE;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
