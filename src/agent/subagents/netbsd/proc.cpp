/* 
** NetXMS subagent for NetBSD
** Copyright (C) 2004-2009 Alex Kirhenshtein, Victor Kirhenshtein
** Copyright (C) 2008 Mark Ibell
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
**/

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>

#if !HAVE_STRUCT_KINFO_PROC2

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <kvm.h>
#include "system.h"


//
// Build process command line
//

static void BuildProcessCommandLine(kvm_t *kd, struct kinfo_proc *p, char *cmdLine, size_t maxSize)
{
	char **argv;
	int i, pos, len;

	*cmdLine = 0;
	argv = kvm_getargv(kd, p, 0);
	if (argv != NULL)
	{
		if (argv[0] != NULL)
		{
			for(i = 0, pos = 0; (argv[i] != NULL) && (pos < maxSize); i++)
			{
				if (i > 0)
					cmdLine[pos++] = ' ';
				nx_strncpy(&cmdLine[pos], argv[i], maxSize - pos);
				pos += strlen(argv[i]);
			}
		}
		else
		{
			// Use process name if command line is empty
			cmdLine[0] = '[';
			nx_strncpy(&cmdLine[1], p->kp_proc.p_comm, maxSize - 2);
			strcat(cmdLine, "]");
		}
	}
	else
	{
		// Use process name if command line cannot be obtained
		cmdLine[0] = '[';
		nx_strncpy(&cmdLine[1], p->kp_proc.p_comm, maxSize - 2);
		strcat(cmdLine, "]");
	}
}


//
// Check if given process matches filter
//

static BOOL MatchProcess(kvm_t *kd, struct kinfo_proc *p, BOOL extMatch, const char *name, const char *cmdLine)
{
	char processCmdLine[32768];

	if (extMatch)
	{
		BuildProcessCommandLine(kd, p, processCmdLine, sizeof(processCmdLine));
		return ((*name != 0) ? RegexpMatch(p->kp_proc.p_comm, name, FALSE) : TRUE) &&
		       ((*cmdLine != 0) ? RegexpMatch(processCmdLine, cmdLine, TRUE) : TRUE);
	}
	else
	{
		return strcasecmp(p->kp_proc.p_comm, name) == 0;
	}
}


//
// Handler for Process.Count, Process.CountEx and System.ProcessCount parameters
//

LONG H_ProcessCount(const char *param, const char *arg, char *value, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	char name[128] = "", cmdLine[128] = "";
	int nCount;
	int nResult = -1;
	int i;
	kvm_t *kd;
	struct kinfo_proc *kp;

	if ((*arg != 'S') && (*arg != 'T'))  // Not System.ProcessCount nor System.ThreadCount
	{
		AgentGetParameterArg(param, 1, name, sizeof(name));
		if (*arg == 'E')	// Process.CountEx
		{
			AgentGetParameterArg(param, 2, cmdLine, sizeof(cmdLine));
		}
	}

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != NULL)
	{
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);

		if (kp != NULL)
		{
			if (*arg != 'S')	// Not System.ProcessCount
			{
				nResult = 0;
				for (i = 0; i < nCount; i++)
				{
					if (MatchProcess(kd, &kp[i], *arg == 'E', name, cmdLine))
					{
						nResult++;
					}
				}
			}
			else
			{
				nResult = nCount;
			}
		}

		kvm_close(kd);
	}

	if (nResult >= 0)
	{
		ret_int(value, nResult);
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}


//
// Handler for Process.* parameters
//

LONG H_ProcessInfo(const char *param, const char *arg, char *value, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	char name[128] = "", cmdLine[128] = "", buffer[64] = "";
	int nCount, nMatched;
	INT64 currValue, result;
	int i, type;
	kvm_t *kd;
	struct kinfo_proc *kp;
	static const char *typeList[]={ "min", "max", "avg", "sum", NULL };

	 // Get parameter type arguments
	AgentGetParameterArg(param, 2, buffer, sizeof(buffer));
	if (buffer[0] == 0)     // Omited type
	{
		type = INFOTYPE_SUM;
	}
	else
	{
		for(type = 0; typeList[type] != NULL; type++)
			if (!stricmp(typeList[type], buffer))
				break;
		if (typeList[type] == NULL)
			return SYSINFO_RC_UNSUPPORTED;     // Unsupported type
	}

	AgentGetParameterArg(param, 1, name, sizeof(name));
	AgentGetParameterArg(param, 3, cmdLine, sizeof(cmdLine));

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != NULL)
	{
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);

		if (kp != NULL)
		{
			result = 0;
			nMatched = 0;
			for (i = 0; i < nCount; i++)
			{
				if (MatchProcess(kd, &kp[i], *cmdLine != 0, name, cmdLine))
				{
					nMatched++;
/* VK: this is just stub because I don't know how to retrieve any usable process information
  using only kinfo_proc
					switch(CAST_FROM_POINTER(arg, int))
					{
					}
*/

					switch(type)
					{
						case INFOTYPE_SUM:
						case INFOTYPE_AVG:
							result += currValue;
							break;
						case INFOTYPE_MIN:
							result = min(result, currValue);
							break;
						case INFOTYPE_MAX:
							result = max(result, currValue);
							break;
					}
				}
			}
			if ((type == INFOTYPE_AVG) && (nMatched > 0))
				result /= nMatched;
			ret_int64(value, result);
			nRet = SYSINFO_RC_SUCCESS;
		}

		kvm_close(kd);
	}

	return nRet;
}


//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(const char *pszParam, const char *pArg, StringList *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	int nCount = -1;
	int i;
	struct kinfo_proc *kp;
	kvm_t *kd;

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != 0)
	{
		kp = kvm_getprocs(kd, KERN_PROC_ALL, 0, &nCount);

		if (kp != NULL)
		{
			for (i = 0; i < nCount; i++)
			{
				char szBuff[128];

				snprintf(szBuff, sizeof(szBuff), "%d %s",
						kp[i].kp_proc.p_pid, kp[i].kp_proc.p_comm
						);
				pValue->add(szBuff);
			}
		}

		kvm_close(kd);
	}

	if (nCount >= 0)
	{
		nRet = SYSINFO_RC_SUCCESS;
	}

	return nRet;
}

#endif   /* !HAVE_STRUCT_KINFO_PROC2 */
