/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004-2009 Alex Kirhenshtein, Victor Kirhenshtein
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

#undef _XOPEN_SOURCE

#if __FreeBSD__ < 8
#define _SYS_LOCK_PROFILE_H_	/* prevent include of sys/lock_profile.h which can be C++ incompatible) */
#endif

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_util.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <sys/param.h>

#if __FreeBSD__ < 5
#include <sys/proc.h>
#endif

#include <sys/user.h>
#include <kvm.h>

#include "system.h"

#ifndef KERN_PROC_PROC
#define KERN_PROC_PROC KERN_PROC_ALL
#endif

#if __FreeBSD__ >= 5
#define PNAME p->ki_comm
#else
#define PNAME p->kp_proc.p_comm
#endif


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
				strncpy(&cmdLine[pos], argv[i], maxSize - pos);
				pos += strlen(argv[i]);
			}
		}
		else
		{
			// Use process name if command line is empty
			cmdLine[0] = '[';
			strncpy(&cmdLine[1], PNAME, maxSize - 2);
			strcat(cmdLine, "]");
		}
	}
	else
	{
		// Use process name if command line cannot be obtained
		cmdLine[0] = '[';
		strncpy(&cmdLine[1], PNAME, maxSize - 2);
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
		return ((*name != 0) ? RegexpMatchA(PNAME, name, FALSE) : TRUE) &&
		       ((*cmdLine != 0) ? RegexpMatchA(processCmdLine, cmdLine, TRUE) : TRUE);
	}
	else
	{
		return strcasecmp(PNAME, name) == 0;
	}
}


//
// Handler for Process.Count, Process.CountEx and System.ProcessCount parameters
//

LONG H_ProcessCount(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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
		AgentGetParameterArgA(param, 1, name, sizeof(name));
		if (*arg == 'E')	// Process.CountEx
		{
			AgentGetParameterArgA(param, 2, cmdLine, sizeof(cmdLine));
		}
	}

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != NULL)
	{
		kp = kvm_getprocs(kd, KERN_PROC_PROC, 0, &nCount);

		if (kp != NULL)
		{
			if (*arg != 'S')	// Not System.ProcessCount
			{
				nResult = 0;
				if (*arg == 'T')  // System.ThreadCount
				{
					for (i = 0; i < nCount; i++)
					{
						nResult += kp[i].ki_numthreads;
					}
				}
				else
				{
					for (i = 0; i < nCount; i++)
					{
						if (MatchProcess(kd, &kp[i], *arg == 'E', name, cmdLine))
						{
							nResult++;
						}
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

LONG H_ProcessInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
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
	AgentGetParameterArgA(param, 2, buffer, sizeof(buffer));
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

	AgentGetParameterArgA(param, 1, name, sizeof(name));
	AgentGetParameterArgA(param, 3, cmdLine, sizeof(cmdLine));

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != NULL)
	{
		kp = kvm_getprocs(kd, KERN_PROC_PROC, 0, &nCount);

		if (kp != NULL)
		{
			result = 0;
			nMatched = 0;
			for (i = 0; i < nCount; i++)
			{
				if (MatchProcess(kd, &kp[i], *cmdLine != 0, name, cmdLine))
				{
					nMatched++;
					switch(CAST_FROM_POINTER(arg, int))
					{
						case PROCINFO_CPUTIME:
							currValue = kp[i].ki_runtime / 1000;  // microsec -> millisec
							break;
						case PROCINFO_THREADS:
							currValue = kp[i].ki_numthreads;
							break;
						case PROCINFO_VMSIZE:
							currValue = kp[i].ki_size;
							break;
						case PROCINFO_WKSET:
							currValue = kp[i].ki_rssize * getpagesize();
							break;
					}

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

LONG H_ProcessList(const TCHAR *pszParam, const TCHAR *pArg, StringList *pValue, AbstractCommSession *session)
{
	int nRet = SYSINFO_RC_ERROR;
	int nCount = -1;
	int i;
	struct kinfo_proc *kp;
	kvm_t *kd;

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, NULL);
	if (kd != 0)
	{
		kp = kvm_getprocs(kd, KERN_PROC_PROC, 0, &nCount);

		if (kp != NULL)
		{
			for (i = 0; i < nCount; i++)
			{
				char szBuff[128];

				snprintf(szBuff, sizeof(szBuff), "%d %s",
#if __FreeBSD__ >= 5
						kp[i].ki_pid, kp[i].ki_comm
#else
						kp[i].kp_proc.p_pid, kp[i].kp_proc.p_comm
#endif
						);
#ifdef UNICODE
				pValue->addPreallocated(WideStringFromMBString(szBuff));
#else
				pValue->add(szBuff);
#endif
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
