/*
** WMI NetXMS subagent
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: wmi.h
**
**/

#ifndef _wmi_h_
#define _wmi_h_

#define _WIN32_DCOM

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <wmiutils.h>
#include <wbemcli.h>
#include <wbemprov.h>

#ifdef _DEBUG
#define DEBUG_SUFFIX    "-debug"
#else
#define DEBUG_SUFFIX    ""
#endif


//
// WMI query context
//

typedef struct
{
	IWbemLocator *m_locator;
	IWbemServices *m_services;
} WMI_QUERY_CONTEXT;


//
// Functions
//

IEnumWbemClassObject *DoWMIQuery(WCHAR *ns, WCHAR *query, WMI_QUERY_CONTEXT *ctx);
void CloseWMIQuery(WMI_QUERY_CONTEXT *ctx);
TCHAR *VariantToString(VARIANT *pValue);
LONG VariantToInt(VARIANT *pValue);


#endif
