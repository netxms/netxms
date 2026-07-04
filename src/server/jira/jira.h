/*
** NetXMS - Network Management System
** Helpdesk link module for Jira
** Copyright (C) 2014-2026 Raden Solutions
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
#include <nxlibcurl.h>
#include <netxms-webapi.h>

#define JIRA_DEBUG_TAG L"hdlink.jira"

#define JIRA_MAX_LOGIN_LEN          128
#define JIRA_MAX_PASSWORD_LEN       128
#define JIRA_MAX_PROJECT_CODE_LEN   32
#define JIRA_MAX_ISSUE_TYPE_LEN     32
#define JIRA_MAX_COMPONENT_NAME_LEN 128

/**
 * Jira project's component
 */
class ProjectComponent
{
public:
   int64_t m_id;
   wchar_t *m_name;

   ProjectComponent(int64_t id, const char *name)
   {
      m_id = id;
      m_name = WideStringFromUTF8String(CHECK_NULL_EX_A(name));
   }

   ~ProjectComponent()
   {
      MemFree(m_name);
   }
};

/**
 * Comment sent to Jira
 */
struct Comment
{
   time_t timestamp;
   String issue;
   String text;

   Comment(const wchar_t *_issue, const wchar_t *_text) : issue(_issue), text(_text)
   {
      timestamp = time(nullptr);
   }
};

/**
 * Jira helpdesk link
 */
class JiraLink : public HelpDeskLink
{
private:
   Mutex m_mutex;
   CURL *m_curl;
   curl_slist *m_headers;
   char m_errorBuffer[CURL_ERROR_SIZE];
   char m_login[JIRA_MAX_LOGIN_LEN];
   char m_password[JIRA_MAX_PASSWORD_LEN];
   ObjectArray<Comment> m_recentComments;
   bool m_verifyPeer;

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   uint32_t connect();
   void disconnect();
   void setCredentials();

   void removeExpiredComments();

   unique_ptr<ObjectArray<ProjectComponent>> getProjectComponents(const char *project);

public:
   JiraLink();
   virtual ~JiraLink();

   virtual const wchar_t *getName() override;

   virtual bool checkConnection() override;
   virtual uint32_t openIssue(const wchar_t *description, wchar_t *hdref) override;
   virtual uint32_t addComment(const wchar_t *hdref, const wchar_t *comment) override;
   virtual bool getIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size) override;

   bool init();

   void onWebhookCommentUpdate(const wchar_t *hdref, const wchar_t *text);
   void onWebhookIssueClose(const wchar_t *hdref) { onResolveIssue(hdref); }
};

/**
 * Webhook management
 */
void RegisterJiraWebhook(JiraLink *link);

#endif
