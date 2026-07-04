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
** File: hdlink.h
**
**/

#ifndef _hdlink_h_
#define _hdlink_h_

#include <nms_core.h>

/**
 * Base class for helpdesk links
 */
class NXCORE_EXPORTABLE HelpDeskLink
{
protected:
   void onResolveIssue(const wchar_t *hdref);
   void onCloseIssue(const wchar_t *hdref);
   void onNewComment(const wchar_t *hdref, const wchar_t *comment);

public:
   HelpDeskLink();
   virtual ~HelpDeskLink();

   virtual const wchar_t *getName();

   virtual bool checkConnection();
   virtual uint32_t openIssue(const wchar_t *description, wchar_t *hdref);
   virtual uint32_t addComment(const wchar_t *hdref, const wchar_t *comment);
   virtual uint32_t getIssueState(const wchar_t *hdref, bool *open);
   virtual bool getIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size);
};

/**
 * Register active helpdesk link (normally called from module's initialization entry point).
 * Only one link can be active; if a link is already registered, the call is ignored and an error is logged.
 * Caller retains ownership of the link object, which must remain valid for the lifetime of the server process.
 * Returns true if link is registered.
 */
bool NXCORE_EXPORTABLE RegisterHelpDeskLink(HelpDeskLink *link);

#endif   /* _hdlink_h_ */
