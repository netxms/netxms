/*
** NetXMS - Network Management System
** Helpdesk link module for Redmine
** Copyright (C) 2017-2026 Raden Solutions
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

#define REDMINE_MAX_API_KEY_LEN 64

/**
 * Redmine helpdesk link
 */
class RedmineLink : public HelpDeskLink
{
 private:
   Mutex m_mutex;
   char m_serverUrl[MAX_PATH];
   char m_apiKey[REDMINE_MAX_API_KEY_LEN];
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

   virtual const wchar_t *getName() override;

   virtual bool checkConnection() override;
   virtual uint32_t openIssue(const wchar_t *description, wchar_t *hdref) override;
   virtual uint32_t addComment(const wchar_t *hdref, const wchar_t *comment) override;
   virtual uint32_t getIssueState(const wchar_t *hdref, bool *open) override;
   virtual bool getIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size) override;

   bool init();
};

#endif
