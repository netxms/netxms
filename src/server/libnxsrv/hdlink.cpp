/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: hdlink.cpp
**/

#include "libnxsrv.h"
#include <hdlink.h>

/**
 * Server entry points for alarm resolve and close
 */
static UINT32 (*__resolveAlarmByHdRef)(const TCHAR *hdref);
static UINT32 (*__closeAlarmByHdRef)(const TCHAR *hdref);

/**
 * Initialize server entry points
 */
void LIBNXSRV_EXPORTABLE SetHDLinkEntryPoints(UINT32 (*__resolve)(const TCHAR *), UINT32 (*__close)(const TCHAR *))
{
   __resolveAlarmByHdRef = __resolve;
   __closeAlarmByHdRef = __close;
}

/**
 * Constructor
 */
HelpDeskLink::HelpDeskLink()
{
}

/**
 * Destructor
 */
HelpDeskLink::~HelpDeskLink()
{
}

/**
 * Get module name
 *
 * @return module name
 */
const TCHAR *HelpDeskLink::getName()
{
	return _T("GENERIC");
}

/**
 * Get module version
 *
 * @return module version
 */
const TCHAR *HelpDeskLink::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Initialize module
 *
 * @return true if initialization was successful
 */
bool HelpDeskLink::init()
{
   return true;
}

/**
 * Check that connection with helpdesk system is working
 *
 * @return true if connection is working
 */
bool HelpDeskLink::checkConnection()
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
UINT32 HelpDeskLink::openIssue(const TCHAR *description, TCHAR *hdref)
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
UINT32 HelpDeskLink::addComment(const TCHAR *hdref, const TCHAR *comment)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Must be called by actual link implementation when issue 
 * is resolved in helpdesk system.
 *
 * @param hdref helpdesk issue reference
 */
void HelpDeskLink::onResolveIssue(const TCHAR *hdref)
{
   __resolveAlarmByHdRef(hdref);
}

/**
 * Must be called by actual link implementation when issue 
 * is closed in helpdesk system.
 *
 * @param hdref helpdesk issue reference
 */
void HelpDeskLink::onCloseIssue(const TCHAR *hdref)
{
   __closeAlarmByHdRef(hdref);
}

/**
 * Get URL to view issue in helpdesk system
 */
bool HelpDeskLink::getIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size)
{
   return false;
}
