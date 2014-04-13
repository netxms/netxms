/* 
** NetXMS - Network Management System
** Helpdesk link module for Jira
** Copyright (C) 2014 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: jira.cpp
**/

#include "jira.h"

/**
 * Module name
 */
static TCHAR s_moduleName[] = _T("JIRA");

/**
 * Module version
 */
static TCHAR s_moduleVersion[] = NETXMS_VERSION_STRING;

/**
 * Constructor
 */
JiraLink::JiraLink() : HelpDeskLink()
{
   _tcscpy(m_serverUrl, _T("http://localhost"));
}

/**
 * Get module name
 *
 * @return module name
 */
const TCHAR *JiraLink::getName()
{
	return s_moduleName;
}

/**
 * Get module version
 *
 * @return module version
 */
const TCHAR *JiraLink::getVersion()
{
	return s_moduleVersion;
}

/**
 * Initialize module
 *
 * @return true if initialization was successful
 */
bool JiraLink::init()
{
   ConfigReadStr(_T("JiraServerURL"), m_serverUrl, MAX_OBJECT_NAME, _T("http://localhost"));
   DbgPrintf(5, _T("Jira: server URL set to %s"), m_serverUrl);
   return true;
}

/**
 * Check that connection with helpdesk system is working
 *
 * @return true if connection is working
 */
bool JiraLink::checkConnection()
{
   return false;
}

/**
 * Open new issue in helpdesk system.
 *
 * @param description description for issue to be opened
 * @param hdref reference assigned to issue by helpdesk system
 * @return RCC ready to be sent to client
 */
UINT32 JiraLink::openIssue(const TCHAR *description, TCHAR *hdref)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Add comment to existing issue
 *
 * @param hdref issue reference
 * @param comment comment text
 * @return RCC ready to be sent to client
 */
UINT32 JiraLink::addComment(const TCHAR *hdref, const TCHAR *comment)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Module entry point
 */
DECLARE_HDLINK_ENTRY_POINT(s_moduleName, JiraLink);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
