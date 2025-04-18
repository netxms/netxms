/*
** NetXMS - Network Management System
** Helpdesk link module for Redmine
** Copyright (C) 2017-2024 Raden Solutions
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
** File: redmine.h
**
**/

#ifndef __redmine__h__
#define __redmine__h__

#include <nms_core.h>
#include <hdlink.h>
#include <nxlibcurl.h>

#define JIRA_MAX_LOGIN_LEN 64
#define JIRA_MAX_PASSWORD_LEN 64
#define JIRA_MAX_PROJECT_CODE_LEN 32
#define JIRA_MAX_ISSUE_TYPE_LEN 32
#define JIRA_MAX_COMPONENT_NAME_LEN 128

/**
 * Jira project's component
 */
class ProjectComponent
{
 public:
   int64_t m_id;
   TCHAR *m_name;

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
 * Module class
 */
class RedmineLink : public HelpDeskLink
{
 private:
   Mutex m_mutex;
   char m_serverUrl[MAX_PATH];
   char m_apiKey[JIRA_MAX_LOGIN_LEN];
   char m_password[JIRA_MAX_PASSWORD_LEN];
   CURL *m_curl;
   char m_errorBuffer[CURL_ERROR_SIZE];
   bool m_verifyPeer;

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }

   uint32_t connect();
   void disconnect();

 public:
   RedmineLink();
   virtual ~RedmineLink();

   virtual const TCHAR *getName() override;
   virtual const TCHAR *getVersion() override;

   virtual bool init() override;
   virtual bool checkConnection() override;
   virtual uint32_t openIssue(const TCHAR *description, TCHAR *hdref) override;
   virtual uint32_t addComment(const TCHAR *hdref, const TCHAR *comment) override;
   virtual uint32_t getIssueState(const TCHAR *hdref, bool *open) override;
   virtual bool getIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size) override;
};

#endif
