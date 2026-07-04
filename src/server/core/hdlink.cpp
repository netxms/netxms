/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
** File: hdlink.cpp
**
**/

#include "nxcore.h"
#include <hdlink.h>

#define DEBUG_TAG L"hdlink"

/**
 * Active helpdesk link
 */
static HelpDeskLink *s_link = nullptr;

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
 * Get link name
 *
 * @return link name
 */
const wchar_t *HelpDeskLink::getName()
{
   return L"GENERIC";
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
uint32_t HelpDeskLink::openIssue(const wchar_t *description, wchar_t *hdref)
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
uint32_t HelpDeskLink::addComment(const wchar_t *hdref, const wchar_t *comment)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Get current state of given issue.
 *
 * @param hdref issue reference
 * @param open pointer to indicator to be set if issue is still open
 * @return RCC ready to be sent to client
 */
uint32_t HelpDeskLink::getIssueState(const wchar_t *hdref, bool *open)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Get URL to view issue in helpdesk system
 */
bool HelpDeskLink::getIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size)
{
   return false;
}

/**
 * Must be called by actual link implementation when issue
 * is resolved in helpdesk system.
 *
 * @param hdref helpdesk issue reference
 */
void HelpDeskLink::onResolveIssue(const wchar_t *hdref)
{
   ResolveAlarmByHDRef(hdref);
}

/**
 * Must be called by actual link implementation when issue
 * is closed in helpdesk system.
 *
 * @param hdref helpdesk issue reference
 */
void HelpDeskLink::onCloseIssue(const wchar_t *hdref)
{
   TerminateAlarmByHDRef(hdref);
}

/**
 * Must be called by actual link implementation when new comment is added to issue.
 *
 * @param hdref helpdesk issue reference
 * @param comment text of new comment
 */
void HelpDeskLink::onNewComment(const wchar_t *hdref, const wchar_t *comment)
{
   AddAlarmSystemComment(hdref, comment);
}

/**
 * Register active helpdesk link. Only one link can be active; first successful registration wins.
 */
bool NXCORE_EXPORTABLE RegisterHelpDeskLink(HelpDeskLink *link)
{
   if (s_link != nullptr)
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Registration of helpdesk link \"%s\" rejected (helpdesk link \"%s\" is already registered)", link->getName(), s_link->getName());
      return false;
   }
   s_link = link;
   InterlockedOr64(&g_flags, AF_HELPDESK_LINK_ACTIVE);
   nxlog_write_tag(NXLOG_INFO, DEBUG_TAG, L"Helpdesk link \"%s\" registered", link->getName());
   return true;
}

/**
 * Create helpdesk issue
 */
uint32_t CreateHelpdeskIssue(const wchar_t *description, wchar_t *hdref)
{
   return (s_link != nullptr) ? s_link->openIssue(description, hdref) : RCC_NO_HDLINK;
}

/**
 * Add comment to helpdesk issue
 */
uint32_t AddHelpdeskIssueComment(const wchar_t *hdref, const wchar_t *text)
{
   return (s_link != nullptr) ? s_link->addComment(hdref, text) : RCC_NO_HDLINK;
}

/**
 * Get URL to view helpdesk issue
 */
uint32_t GetHelpdeskIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size)
{
   if (s_link == nullptr)
      return RCC_NO_HDLINK;
   return s_link->getIssueUrl(hdref, url, size) ? RCC_SUCCESS : RCC_HDLINK_INTERNAL_ERROR;
}

/**
 * Get state of helpdesk issue
 */
uint32_t GetHelpdeskIssueState(const wchar_t *hdref, bool *open)
{
   return (s_link != nullptr) ? s_link->getIssueState(hdref, open) : RCC_NO_HDLINK;
}
