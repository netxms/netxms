/*
** NetXMS subagent for SunOS/Solaris
** Copyright (C) 2004, 2005 Victor Kirhenshtein
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
** $module: system.cpp
**
**/

#include "sunos_subagent.h"
#include <sys/systeminfo.h>


//
// Handler for System.Uname parameter
//

LONG H_Uname(char *pszParam, char *pArg, char *pValue)
{
	char szSysStr[7][64];
	int i;
	LONG nRet = SYSINFO_RC_SUCCESS;
	static int nSysCode[7] = 
	{
		SI_SYSNAME,
		SI_HOSTNAME,
		SI_RELEASE,
		SI_VERSION,
		SI_MACHINE,
		SI_ARCHITECTURE,
		SI_PLATFORM
	};

   for(i = 0; i < 7; i++)
		if (sysinfo(nSysCode[i], szSysStr[i], 64) == -1)
		{
			nRet = SYSINFO_RC_ERROR;
			break;
		}

	if (nRet == SYSINFO_RC_SUCCESS)
	{
		snprintf(pValue, MAX_RESULT_LENGTH, "%s %s %s %s %s %s %s",
					szSysStr[0], szSysStr[1], szSysStr[2], szSysStr[3],
					szSysStr[4], szSysStr[5], szSysStr[6]);
	}

   return nRet;
}


//
// Handler for System.Uptime parameter
//

LONG H_Uptime(char *pszParam, char *pArg, char *pValue)
{
	kstat_ctl_t *kc;
	kstat_t *kp;
	kstat_named_t *kn;
	DWORD hz, secs;
	LONG nRet = SYSINFO_RC_ERROR;

	hz = sysconf(_SC_CLK_TCK);

	// Open kstat
	kc = kstat_open();
	if (kc != NULL)
	{
		// read uptime counter
		kp = kstat_lookup(kc, "unix", 0, "system_misc");
		if (kp != NULL)
		{
			if(kstat_read(kc, kp, 0) != -1)
			{
				kn = (kstat_named_t *)kstat_data_lookup(kp, "clk_intr");
				secs = kn->value.ul / hz;
				ret_uint(pValue, secs);
				nRet = SYSINFO_RC_SUCCESS;
			}
		}
		kstat_close(kc);
	}

	return nRet;
}

//
// Handler for System.Hostname parameter
//

LONG H_Hostname(char *pszParam, char *pArg, char *pValue)
{
	return (sysinfo(SI_HOSTNAME, pValue, MAX_RESULT_LENGTH) == -1) ?
											SYSINFO_RC_ERROR : SYSINFO_RC_SUCCESS;
}
