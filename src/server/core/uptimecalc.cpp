/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 NetXMS Team
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
** File: uptimecalc.cpp
**
**/

#include "nxcore.h"

//
//  Uptime calculator thread
//

THREAD_RESULT THREAD_CALL UptimeCalculator(void *arg)
{
	const int calcInterval = 60; 

	DbgPrintf(1, _T("UptimeCalculator thread started"));

	while (TRUE)
	{
		g_pBusinessServiceRoot->updateUptimeStats(time(NULL), TRUE);
		if (SleepAndCheckForShutdown(calcInterval))
			break;
	}

	DbgPrintf(1, _T("UptimeCalculator thread terminated"));

	return THREAD_OK;
}

