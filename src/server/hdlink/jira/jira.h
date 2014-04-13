/* 
** NetXMS - Network Management System
** Helpdesk link module for Jira
** Copyright (C) 2014 Raden Solutions
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
** File: jira.h
**
**/

#ifndef _jira_h_
#define _jira_h_

#include <nms_core.h>
#include <hdlink.h>

/**
 * Module class
 */
class JiraLink : public HelpDeskLink
{
private:
   TCHAR m_serverUrl[MAX_PATH];

public:
   JiraLink();

	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

   virtual bool init();
   virtual bool checkConnection();
   virtual UINT32 openIssue(const TCHAR *description, TCHAR *hdref);
   virtual UINT32 addComment(const TCHAR *hdref, const TCHAR *comment);
};

#endif
