/* $Id$ */

/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2006 Victor Kirhenshtein
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
#include "drbd.h"


//
// Open DRBD device
//

static int OpenDRBDDevice(int nDev)
{
	char szDevName[MAX_PATH];
	int nFd;
	struct stat drbdStat;

	snprintf(szDevName, MAX_PATH, "/dev/drbd%d", nDev);
	nFd = open(szDevName, O_RDONLY);
	if (nFd != -1)
	{
		if (fstat(nFd, &drbdStat) == 0)
		{
			if (!S_ISBLK(drbdStat.st_mode))
			{
				close(nFd);
				nFd = -1;
			}
		}
		else
		{
			close(nFd);
			nFd = -1;
		}
	}
	return nFd;
}


//
// Get description of cstate code
//

static const char *CStateText(int nCode)
{
	static char *pszCStateText[] =
	{
		"Unconfigured",
		"StandAlone",
		"Unconnected",
		"Timeout",
		"BrokenPipe",
		"NetworkFailure",
		"WFConnection",
		"WFReportParams",
		"Connected",
		"SkippedSyncS",
		"SkippedSyncT",
		"WFBitMapS",
		"WFBitMapT",
		"SyncSource",
		"SyncTarget",
		"PausedSyncS",
		"PausedSyncT"
	};
	return ((nCode >= 0) && (nCode <= PausedSyncT)) ? pszCStateText[nCode] : "Unknown";
}


//
// Get description of state code
//

static const char *StateText(int nCode)
{
	static char *pszStateText[] =
	{
		"Unknown",
		"Primary",
		"Secondary"
	};
	return ((nCode >= 0) && (nCode <= Secondary)) ? pszStateText[nCode] : "Unknown";
}


//
// Get list of configured DRBD devices
//

LONG H_DRBDDeviceList(TCHAR *pszCmd, TCHAR *pArg, NETXMS_VALUES_LIST *pValue)
{
	int nDev, nFd;
	struct ioctl_get_config drbdConfig;
	char szBuffer[1024];

	for(nDev = 0; nDev < 16; nDev++)
	{
		nFd = OpenDRBDDevice(nDev);
		if (nFd != -1)
		{
			if (ioctl(nFd, DRBD_IOCTL_GET_CONFIG, &drbdConfig) == 0)
			{
				snprintf(szBuffer, 1024, "/dev/drbd%d %d %d/%d %s %s/%s %s",
						nDev, drbdConfig.cstate, drbdConfig.state,
						drbdConfig.peer_state, CStateText(drbdConfig.cstate),
						StateText(drbdConfig.state), StateText(drbdConfig.peer_state),
						drbdConfig.lower_device_name);
				NxAddResultString(pValue, szBuffer);
			}
			close(nFd);
		}
	}
	return SYSINFO_RC_SUCCESS;
}


//
// Get information for specific DRBD device
//

LONG H_DRBDDeviceInfo(TCHAR *pszCmd, TCHAR *pArg, TCHAR *pValue)
{
	int nDev, nFd;
	struct ioctl_get_config drbdConfig;
	TCHAR szDev[256], *eptr;
	LONG nRet = SYSINFO_RC_ERROR;

	if (!NxGetParameterArg(pszCmd, 1, szDev, 256))
		return SYSINFO_RC_UNSUPPORTED;

	nDev = _tcstol(szDev, &eptr, 0);
	if ((nDev < 0) || (nDev > 255) || (*eptr != 0))
		return SYSINFO_RC_UNSUPPORTED;

	nFd = OpenDRBDDevice(nDev);
	if (nFd != -1)
	{
		if (ioctl(nFd, DRBD_IOCTL_GET_CONFIG, &drbdConfig) == 0)
		{
			nRet = SYSINFO_RC_SUCCESS;
			switch(*pArg)
			{
				case 'c':	// Connection state
					ret_int(pValue, drbdConfig.cstate);
					break;
				case 'C':	// Connection state as text
					ret_string(pValue, (char *)CStateText(drbdConfig.cstate));
					break;
				case 's':	// State
					ret_int(pValue, drbdConfig.state);
					break;
				case 'S':	// State as text
					ret_string(pValue, (char *)StateText(drbdConfig.state));
					break;
				case 'p':	// Peer state
					ret_int(pValue, drbdConfig.peer_state);
					break;
				case 'P':	// Peer state as text
					ret_string(pValue, (char *)StateText(drbdConfig.peer_state));
					break;
				case 'L':	// Lower device
					ret_string(pValue, drbdConfig.lower_device_name);
					break;
				default:
					nRet = SYSINFO_RC_UNSUPPORTED;
					break;
			}
		}
		close(nFd);
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
