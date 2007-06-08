/* $Id: logscan.cpp,v 1.6 2007-06-08 00:02:36 alk Exp $ */
/*
** NetXMS LogScan subagent
** Copyright (C) 2006 Alex Kirhenshtein
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
** File: logscan.cpp
**
**/

#include "logscan.h"

using namespace std;


//
// Typedefs
//
typedef map<string, long> tLocations;

//
// static vars
//

static tLocations m_locations;
static MUTEX m_locationsMutex;

//
// Forward declarations
//

LONG H_GetString(char *, char *, char *);


//
// initialization callback
//

static BOOL SubAgentInit(TCHAR *pszConfigFile)
{
	m_locationsMutex = MutexCreate();
	return TRUE;
}


//
// Cleanup callback
//

static void SubAgentShutdown(void)
{
	MutexDestroy(m_locationsMutex);
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ "LogScan.FindString(*)", H_GetString, NULL, DCI_DT_STRING, ""},
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	"LOGSCAN", NETXMS_VERSION_STRING,
	SubAgentInit, SubAgentShutdown, NULL,  // Handlers
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	0, //sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	NULL, //m_enums,
	0, NULL     // actions
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(LOGSCAN)
{
	*ppInfo = &m_info;
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// main code
//

LONG H_GetString(char *pszParam, char *pArg, char *pValue)
{
	int ret = SYSINFO_RC_ERROR;
	char fileName[1024];
	char subString[1024];

	NxGetParameterArg(pszParam, 1, fileName, sizeof(fileName));
	NxGetParameterArg(pszParam, 2, subString, sizeof(subString));

	if (fileName[0] != 0 && subString[0] != 0)
	{
		char tmp[10240];
		struct stat s;

		if (stat(fileName, &s) == 0)
		{
			long location = 0;
			MutexLock(m_locationsMutex, INFINITE);
			tLocations::iterator it = m_locations.find(string(fileName) + string(":::") + subString);
			if (it != m_locations.end())
			{
				// TODO: add more strict rotation checking
				if (it->second <= s.st_size) // last location is beyond end of file
				{
					location = it->second;
				}
			}
			MutexUnlock(m_locationsMutex);

			FILE *f = fopen(fileName, "r");
			if (f != NULL)
			{
				fseek(f, location, SEEK_SET);

				tmp[0] = 0;
				while (fgets(tmp,sizeof(tmp), f) != NULL)
				{
					for (int i = 0; i < strlen(tmp); i++)
					{
						if (tmp[i] == 0x0D || tmp[i] == 0x0A)
						{
							tmp[i] = 0;
							break;
						}
					}
					// TODO: change to strcasestr?
					if (strstr(tmp, subString) != NULL)
					{
						ret_string(pValue, tmp);
						ret = SYSINFO_RC_SUCCESS;
						break;
					}
					tmp[0] = 0;
				}

				if (ret != SYSINFO_RC_SUCCESS)
				{
					ret = SYSINFO_RC_SUCCESS;
					ret_string(pValue, "");
				}

				MutexLock(m_locationsMutex, INFINITE);
				//m_locations[string(fileName) + string(":::") + subString] = ftell(f);
				m_locations[string(fileName) + string(":::") + subString] = s.st_size;
				MutexUnlock(m_locationsMutex);

				fclose(f);
			}
		}
	}

	return ret;
}


///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.5  2007/04/24 18:22:11  victor
Subagent API changed

Revision 1.4  2006/03/09 15:18:49  alk
lineends are removed before sending responce

Revision 1.3  2006/03/09 12:25:26  victor
Windows port

Revision 1.2  2006/03/09 11:44:42  alk
*** empty log message ***

Revision 1.1  2006/03/09 11:14:55  alk
simple log scanner added


*/
