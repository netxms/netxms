/* $Id$ */

/* 
** NetXMS subagent for GNU/Linux
** Copyright (C) 2004 - 2008 NetXMS Team
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

#include <paths.h>
#include <mntent.h>
#include <sys/vfs.h>

#include "iostat.h"


#define IF_PARAM1 char szParam1[512] = {0}; NxGetParameterArg(pszParam, 1, szParam1, sizeof(szParam1)); if (szParam1[0] != 0)

LONG H_TransferRate(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}

	return nRet;
}

LONG H_BlockReadRate(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}

	return nRet;
}

LONG H_BlockWriteRate(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}

	return nRet;
}

LONG H_BytesReadRate(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}

	return nRet;
}

LONG H_BytesWriteRate(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}

	return nRet;
}

LONG H_DiskQueue(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}
	else
	{
		// for all disks
	}

	return nRet;
}

LONG H_DiskTime(char *pszParam, char *pArg, char *pValue)
{
	int nRet = SYSINFO_RC_ERROR;

	IF_PARAM1
	{
	}

	return nRet;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $

*/
