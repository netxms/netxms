/*
** NetXMS - Network Management System
** Helpdesk link module for Jira
** Copyright (C) 2014-2022 Raden Solutions
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

#define JIRA_DEBUG_TAG _T("hdlink.jira")

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

   Comment(const TCHAR *_issue, const TCHAR *_text) : issue(_issue), text(_text)
   {
      timestamp = time(nullptr);
   }
};

/**
 * Module class
 */
class JiraLink : public HelpDeskLink
{
private:
   Mutex m_mutex;
   CURL *m_curl;
   curl_slist *m_headers;
   char m_errorBuffer[CURL_ERROR_SIZE];
   void *m_webhookHandle;
   char m_webhookUrl[MAX_PATH];
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

   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual bool init();
   virtual bool checkConnection();
   virtual uint32_t openIssue(const TCHAR *description, TCHAR *hdref);
   virtual uint32_t addComment(const TCHAR *hdref, const TCHAR *comment);
   virtual bool getIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size);

   const char *getWebhookURL() const { return m_webhookUrl; }

   void onWebhookCommentUpdate(const TCHAR *hdref, const TCHAR *text);
   void onWebhookIssueClose(const TCHAR *hdref) { onResolveIssue(hdref); }
};

/**
 * Webhook management
 */
void *CreateWebhook(JiraLink *link, uint16_t port);
void DestroyWebhook(void *handle);

#endif
