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
#include <curl/curl.h>

#define JIRA_MAX_LOGIN_LEN          64
#define JIRA_MAX_PASSWORD_LEN       64
#define JIRA_MAX_PROJECT_CODE_LEN   32
#define JIRA_MAX_ISSUE_TYPE_LEN     32

/**
 * Request data for cURL call
 */
struct RequestData
{
   size_t size;
   size_t allocated;
   char *data;
};

/**
 * Module class
 */
class JiraLink : public HelpDeskLink
{
private:
   MUTEX m_mutex;
   char m_serverUrl[MAX_PATH];
   char m_login[JIRA_MAX_LOGIN_LEN];
   char m_password[JIRA_MAX_PASSWORD_LEN];
   char m_projectCode[JIRA_MAX_PROJECT_CODE_LEN];
   char m_issueType[JIRA_MAX_ISSUE_TYPE_LEN];
   CURL *m_curl;
   char m_errorBuffer[CURL_ERROR_SIZE];

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }
   UINT32 connect();
   void disconnect();

public:
   JiraLink();
   virtual ~JiraLink();

	virtual const TCHAR *getName();
	virtual const TCHAR *getVersion();

   virtual bool init();
   virtual bool checkConnection();
   virtual UINT32 openIssue(const TCHAR *description, TCHAR *hdref);
   virtual UINT32 addComment(const TCHAR *hdref, const TCHAR *comment);
};

#endif
