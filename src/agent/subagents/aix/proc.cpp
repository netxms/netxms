/* $Id: proc.cpp,v 1.1 2006-09-27 13:04:45 victor Exp $ */
/*
** NetXMS subagent for AIX
** Copyright (C) 2004, 2005, 2006 Victor Kirhenshtein
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
** File: proc.cpp
**
**/

#include "aix_subagent.h"
#include <procinfo.h>


//
// Handler for System.ProcessList enum
//

LONG H_ProcessList(char *pszParam, char *pArg, NETXMS_VALUES_LIST *pValue)
{
	LONG nRet = SYSINFO_RC_ERROR;

	return nRet;
}
