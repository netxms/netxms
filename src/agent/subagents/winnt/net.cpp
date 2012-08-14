/*
** Windows NT/2000/XP/2003 NetXMS subagent
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: net.cpp
**
**/

#include "winnt_subagent.h"
#include <lm.h>


//
// Check availability of remote share
// Usage: Net.RemoteShareStatus(share,domain,login,password)
//    or: Net.RemoteShareStatusText(share,domain,login,password)
//

LONG H_RemoteShareStatus(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
	TCHAR share[MAX_PATH], domain[64], login[64], password[256];
#ifdef UNICODE
#define wshare share
#else
	WCHAR wshare[MAX_PATH], wdomain[64], wlogin[64], wpassword[64];
#endif
	USE_INFO_2 ui;
	NET_API_STATUS status;

	memset(share, 0, MAX_PATH * sizeof(TCHAR));
	memset(domain, 0, 64 * sizeof(TCHAR));
	memset(login, 0, 64 * sizeof(TCHAR));
	memset(password, 0, 256 * sizeof(TCHAR));

	AgentGetParameterArg(cmd, 1, share, MAX_PATH);
	AgentGetParameterArg(cmd, 2, domain, 64);
	AgentGetParameterArg(cmd, 3, login, 64);
	AgentGetParameterArg(cmd, 4, password, 256);

	if ((*share == 0) || (*domain == 0) || (*login == 0) || (*password == 0))
		return SYSINFO_RC_UNSUPPORTED;

	memset(&ui, 0, sizeof(USE_INFO_2));
	ui.ui2_asg_type = USE_WILDCARD;
	//ui.ui2_local = NULL;
#ifdef UNICODE
	ui.ui2_remote = share;
	ui.ui2_domainname = domain;
	ui.ui2_username = login;
	ui.ui2_password = password;
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, share, -1, wshare, MAX_PATH);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, domain, -1, wdomain, 64);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, login, -1, wlogin, 64);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, password, -1, wpassword, 256);
	ui.ui2_remote = wshare;
	ui.ui2_domainname = wdomain;
	ui.ui2_username = wlogin;
	ui.ui2_password = wpassword;
#endif
	if ((status = NetUseAdd(NULL, 2, (LPBYTE)&ui, NULL)) == NERR_Success)
	{
		NetUseDel(NULL, wshare, USE_FORCE);
	}

	if (*arg == 'C')
	{
		ret_int(value, status);	// Code
	}
	else
	{
		if (status == NERR_Success)
			ret_string(value, _T("OK"));
		else
			GetSystemErrorText(status, value, MAX_RESULT_LENGTH);
	}
	return SYSINFO_RC_SUCCESS;
}
