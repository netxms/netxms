/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2009 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: policy.cpp
**
**/

#include "nxagentd.h"

#ifdef _WIN32
#define write _write
#define close _close
#endif


//
// Deploy configuration file
//

static DWORD DeployConfig(DWORD session, CSCPMessage *msg)
{
	TCHAR path[MAX_PATH], name[MAX_PATH], *fname, tail;
	int i, fh;
	DWORD rcc;

	msg->GetVariableStr(VID_CONFIG_FILE_NAME, name, MAX_PATH);
	DebugPrintf(session, _T("DeployConfig(): original file name is %s"), name);
	for(i = (int)_tcslen(name) - 1; i >= 0; i--)
		if ((name[i] == '/') || (name[i] == '\\'))
		{
			break;
		}
	fname = &name[i + 1];
	tail = g_szConfigIncludeDir[_tcslen(g_szConfigIncludeDir) - 1];
	_sntprintf(path, MAX_PATH, _T("%s%s%s"), g_szConfigIncludeDir, ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""), fname);

	fh = _topen(path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, _S_IREAD | _S_IWRITE);
	if (fh != -1)
	{
		DWORD size = msg->GetVariableBinary(VID_CONFIG_FILE_DATA, 0, NULL);
		BYTE *data = (BYTE *)malloc(size);
		if (data != NULL)
		{
			msg->GetVariableBinary(VID_CONFIG_FILE_DATA, data, size);
			if (write(fh, data, size) == size)
			{
		      DebugPrintf(session, _T("Configuration file %s saved successfully"), path);
				rcc = ERR_SUCCESS;
			}
			else
			{
				rcc = ERR_IO_FAILURE;
			}
			free(data);
		}
		else
		{
			rcc = ERR_MEM_ALLOC_FAILED;
		}
		close(fh);
	}
	else
	{
      DebugPrintf(session, _T("Error opening file %s for writing: %s"), path, _tcserror(errno));
		rcc = ERR_FILE_OPEN_ERROR;
	}

	return rcc;
}


//
// Deploy policy on agent
//

DWORD DeployPolicy(DWORD session, CSCPMessage *request)
{
	DWORD type, rcc;

	type = request->GetVariableShort(VID_POLICY_TYPE);
	switch(type)
	{
		case AGENT_POLICY_CONFIG:
			rcc = DeployConfig(session, request);
			break;
		default:
			rcc = ERR_BAD_ARGUMENTS;
			break;
	}
	DebugPrintf(session, _T("Policy deployment: TYPE=%d RCC=%d"), type, rcc);
	return rcc;
}


//
// Remove configuration file
//

static DWORD RemoveConfig(DWORD session, CSCPMessage *msg)
{
	TCHAR path[MAX_PATH], name[MAX_PATH], *fname, tail;
	int i;
	DWORD rcc;

	msg->GetVariableStr(VID_CONFIG_FILE_NAME, name, MAX_PATH);
	DebugPrintf(session, _T("RemoveConfig(): original file name is %s"), name);
	for(i = (int)_tcslen(name) - 1; i >= 0; i--)
		if ((name[i] == '/') || (name[i] == '\\'))
		{
			break;
		}
	fname = &name[i + 1];
	tail = g_szConfigIncludeDir[_tcslen(g_szConfigIncludeDir) - 1];
	_sntprintf(path, MAX_PATH, _T("%s%s%s"), g_szConfigIncludeDir, ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""), fname);
	if (_tremove(path) != 0)
	{
		rcc = (errno == ENOENT) ? ERR_SUCCESS : ERR_IO_FAILURE;
	}
	else
	{
		rcc = ERR_SUCCESS;
	}
	return rcc;
}


//
// Uninstall policy from agent
//

DWORD UninstallPolicy(DWORD session, CSCPMessage *request)
{
	DWORD type, rcc;

	type = request->GetVariableShort(VID_POLICY_TYPE);
	switch(type)
	{
		case AGENT_POLICY_CONFIG:
			rcc = RemoveConfig(session, request);
			break;
		default:
			rcc = ERR_BAD_ARGUMENTS;
			break;
	}
	DebugPrintf(session, _T("Policy uninstall: TYPE=%d RCC=%d"), type, rcc);
	return rcc;
}
