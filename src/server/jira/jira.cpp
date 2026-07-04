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
** File: jira.cpp
**/

#include "jira.h"
#include <netxms-version.h>

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

/**
 * Constructor
 */
JiraLink::JiraLink() : HelpDeskLink(), m_recentComments(0, 16, Ownership::True)
{
   m_curl = nullptr;
   m_headers = nullptr;
   m_verifyPeer = false;
}

/**
 * Destructor
 */
JiraLink::~JiraLink()
{
   disconnect();
}

/**
 * Get link name
 *
 * @return link name
 */
const wchar_t *JiraLink::getName()
{
   return L"JIRA";
}

/**
 * Initialize link
 *
 * @return true if initialization was successful
 */
bool JiraLink::init()
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 1, L"cURL initialization failed");
      return false;
   }
   return true;
}

/**
 * Set credentials for connection
 */
void JiraLink::setCredentials()
{
   ConfigReadStrUTF8(L"Jira.Login", m_login, JIRA_MAX_LOGIN_LEN, "netxms");
   ConfigReadStrUTF8(L"Jira.Password", m_password, JIRA_MAX_PASSWORD_LEN, "");
   m_verifyPeer = ConfigReadBoolean(L"Jira.VerifyPeer", true);
   DecryptPasswordA(m_login, m_password, m_password, JIRA_MAX_PASSWORD_LEN);
   curl_easy_setopt(m_curl, CURLOPT_USERNAME, m_login);
   curl_easy_setopt(m_curl, CURLOPT_PASSWORD, m_password);
}

/**
 * Connect to Jira server
 */
uint32_t JiraLink::connect()
{
   disconnect();

   m_curl = curl_easy_init();
   if (m_curl == nullptr)
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Call to curl_easy_init() failed");
      return RCC_HDLINK_INTERNAL_ERROR;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, (long)1);
#endif
   curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);
   curl_easy_setopt(m_curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");  // enable cookies in memory
   curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(m_verifyPeer ? 1 : 0));

   setCredentials();

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   char url[MAX_PATH];
   ConfigReadStrUTF8(L"Jira.ServerURL", url, MAX_PATH, "https://jira.atlassian.com");
   strcat(url, "/rest/api/2/myself");
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   m_headers = curl_slist_append(m_headers, "Content-Type: application/json;codepage=utf8");
   m_headers = curl_slist_append(m_headers, "Accept: application/json");
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

   uint32_t rcc = RCC_HDLINK_COMM_FAILURE;
   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      responseData.write(static_cast<char>(0));
      nxlog_debug_tag(JIRA_DEBUG_TAG, 7, L"GET request completed, data: %hs", responseData.buffer());
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 200)
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Jira login successful");
         rcc = RCC_SUCCESS;
      }
      else
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Jira login failed, HTTP response code %03d", response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Call to curl_easy_perform() failed: %hs", m_errorBuffer);
   }

   return rcc;
}

/**
 * Disconnect (close session)
 */
void JiraLink::disconnect()
{
   if (m_curl == nullptr)
      return;

   curl_easy_cleanup(m_curl);
   m_curl = nullptr;

   curl_slist_free_all(m_headers);
   m_headers = nullptr;
}

/**
 * Check that connection with helpdesk system is working
 *
 * @return true if connection is working
 */
bool JiraLink::checkConnection()
{
   bool success = false;

   lock();

   if (m_curl != nullptr)
   {
      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

      setCredentials();

      curl_easy_setopt(m_curl, CURLOPT_POST, (long)0);

      char url[MAX_PATH];
      ConfigReadStrUTF8(L"Jira.ServerURL", url, MAX_PATH, "https://jira.atlassian.com");
      strcat(url, "/rest/api/2/myself");
      curl_easy_setopt(m_curl, CURLOPT_URL, url);
      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         responseData.write(static_cast<char>(0));
         nxlog_debug_tag(JIRA_DEBUG_TAG, 7, L"GET request completed, data: %hs", responseData.buffer());
         long response = 500;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
         if (response == 200)
         {
            success = true;
         }
         else
         {
            nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Jira connection check: HTTP response code is %03d", response);
         }
      }
      else
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Call to curl_easy_perform() failed (%hs)", m_errorBuffer);
      }
   }

   if (!success)
   {
      success = (connect() == RCC_SUCCESS);
   }

   unlock();
   return success;
}

/**
 * Open new issue in helpdesk system.
 *
 * @param description description for issue to be opened
 * @param hdref reference assigned to issue by helpdesk system
 * @return RCC ready to be sent to client
 */
uint32_t JiraLink::openIssue(const wchar_t *description, wchar_t *hdref)
{
   if (!checkConnection())
      return RCC_HDLINK_COMM_FAILURE;

   uint32_t rcc = RCC_HDLINK_COMM_FAILURE;
   lock();
   nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Creating new issue in Jira with description \"%s\"", description);

   // Build request
   json_t *root = json_object();
   json_t *fields = json_object();
   json_t *project = json_object();

   char projectCode[JIRA_MAX_PROJECT_CODE_LEN];
   ConfigReadStrUTF8(L"Jira.ProjectCode", projectCode, JIRA_MAX_PROJECT_CODE_LEN, "NETXMS");

   wchar_t projectComponent[JIRA_MAX_COMPONENT_NAME_LEN];
   ConfigReadStr(L"Jira.ProjectComponent", projectComponent, JIRA_MAX_COMPONENT_NAME_LEN, L"");
   if (projectComponent[0] != 0)
   {
      unique_ptr<ObjectArray<ProjectComponent>> projectComponents = getProjectComponents(projectCode);
      if (projectComponents != nullptr)
      {
         for (int i = 0; i < projectComponents->size(); i++)
         {
            ProjectComponent *c = projectComponents->get(i);
            if (!wcsicmp(c->m_name, projectComponent))
            {
               json_t *components = json_array();
               json_t *component = json_object();
               char buffer[32];
               snprintf(buffer, 32, INT64_FMTA, c->m_id);
               json_object_set_new(component, "id", json_string(buffer));
               json_array_append_new(components, component);
               json_object_set_new(fields, "components", components);
               break;
            }
         }
      }
   }

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);
   json_object_set_new(project, "key", json_string(projectCode));
   json_object_set_new(fields, "project", project);
   if (wcslen(description) < 256)
   {
      json_object_set_new(fields, "summary", json_string_w(description));
   }
   else
   {
      wchar_t summary[256];
      wcslcpy(summary, description, 256);
      json_object_set_new(fields, "summary", json_string_w(summary));
   }
   json_object_set_new(fields, "description", json_string_w(description));
   json_t *issuetype = json_object();

   char issueType[JIRA_MAX_ISSUE_TYPE_LEN];
   ConfigReadStrUTF8(L"Jira.IssueType", issueType, JIRA_MAX_ISSUE_TYPE_LEN, "Task");
   json_object_set_new(issuetype, "name", json_string(issueType));
   json_object_set_new(fields, "issuetype", issuetype);

   json_object_set_new(root, "fields", fields);
   char *request = json_dumps(root, 0);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(root);

   char url[MAX_PATH];
   ConfigReadStrUTF8(L"Jira.ServerURL", url, MAX_PATH, "https://jira.atlassian.com");
   strlcat(url, "/rest/api/2/issue", MAX_PATH);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   nxlog_debug_tag(JIRA_DEBUG_TAG, 7, L"Issue creation request: %hs", request);
   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      responseData.write(static_cast<char>(0));
      nxlog_debug_tag(JIRA_DEBUG_TAG, 7, L"POST request completed, data: %hs", responseData.buffer());
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 201)
      {
         rcc = RCC_HDLINK_INTERNAL_ERROR;

         json_error_t error;
         json_t *root = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
         if (root != nullptr)
         {
            json_t *key = json_object_get(root, "key");
            if (json_is_string(key))
            {
               utf8_to_wchar(json_string_value(key), -1, hdref, MAX_HELPDESK_REF_LEN);
               hdref[MAX_HELPDESK_REF_LEN - 1] = 0;
               rcc = RCC_SUCCESS;
               nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Created new issue in Jira with reference \"%s\"", hdref);
            }
            else
            {
               nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot create issue in Jira (cannot extract issue key)");
            }
            json_decref(root);
         }
         else
         {
            nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot create issue in Jira (error parsing server response)");
         }
      }
      else
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot create issue in Jira (HTTP response code %03d)", response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Call to curl_easy_perform() failed (%hs)", m_errorBuffer);
   }
   MemFree(request);

   unlock();
   return rcc;
}

/**
 * Add comment to existing issue
 *
 * @param hdref issue reference
 * @param comment comment text
 * @return RCC ready to be sent to client
 */
uint32_t JiraLink::addComment(const wchar_t *hdref, const wchar_t *comment)
{
   if (!checkConnection())
      return RCC_HDLINK_COMM_FAILURE;

   uint32_t rcc = RCC_HDLINK_COMM_FAILURE;

   lock();
   nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Adding comment to Jira issue \"%s\" (comment text \"%s\")", hdref, comment);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);
   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);

   // Build request
   json_t *root = json_object();
   json_object_set_new(root, "body", json_string_w(comment));
   char *request = json_dumps(root, 0);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(root);

   char url[MAX_PATH];
   ConfigReadStrUTF8(L"Jira.ServerURL", url, MAX_PATH, "https://jira.atlassian.com");
   strlcat(url, "/rest/api/2/issue/", MAX_PATH);
   size_t l = strlen(url);
   wchar_to_utf8(hdref, -1, &url[l], MAX_PATH - l);
   strlcat(url, "/comment", MAX_PATH);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      responseData.write(static_cast<char>(0));
      nxlog_debug_tag(JIRA_DEBUG_TAG, 7, L"POST request completed, data: %hs", responseData.buffer());
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 201)
      {
         rcc = RCC_SUCCESS;
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Added comment to Jira issue \"%s\"", hdref);
         removeExpiredComments();
         m_recentComments.add(new Comment(hdref, comment));
      }
      else
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot add comment to Jira issue \"%s\" (HTTP response code %03d)", hdref, response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Call to curl_easy_perform() failed: %hs", m_errorBuffer);
   }
   MemFree(request);

   unlock();
   return rcc;
}

/**
 * Get URL to view issue in helpdesk system
 */
bool JiraLink::getIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size)
{
   ConfigReadStr(L"Jira.ServerURL", url, size, L"https://jira.atlassian.com");
   wcslcat(url, L"/browse/", size);
   wcslcat(url, hdref, size);
   return true;
}

/**
 * Get list of project's components
 */
unique_ptr<ObjectArray<ProjectComponent>> JiraLink::getProjectComponents(const char *project)
{
   unique_ptr<ObjectArray<ProjectComponent>> components;

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)0);

   char url[MAX_PATH];
   ConfigReadStrUTF8(L"Jira.ServerURL", url, MAX_PATH, "https://jira.atlassian.com");
   strlcat(url, "/rest/api/2/project/", MAX_PATH);
   strlcat(url, project, MAX_PATH);
   strlcat(url, "/components", MAX_PATH);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      responseData.write(static_cast<char>(0));
      nxlog_debug_tag(JIRA_DEBUG_TAG, 7, L"POST request completed, data: %hs", responseData.buffer());
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 200)
      {
         json_error_t err;
         json_t *root = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &err);
         if (root != nullptr)
         {
            if (json_is_array(root))
            {
               nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Got components list for Jira project %hs", project);
               int size = (int)json_array_size(root);
               components = make_unique<ObjectArray<ProjectComponent>>(size, 8, Ownership::True);
               for(int i = 0; i < size; i++)
               {
                  json_t *e = json_array_get(root, i);
                  if (e == nullptr)
                     break;

                  json_t *id = json_object_get(e, "id");
                  json_t *name = json_object_get(e, "name");
                  if ((id != nullptr) && (name != nullptr))
                  {
                     components->add(new ProjectComponent(json_integer_value_ex(id, 0), json_string_value(name)));
                  }
               }
            }
            else
            {
               nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot get components for Jira project %hs (JSON root element is not an array)", project);
            }
            json_decref(root);
         }
         else
         {
            nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot get components for Jira project %hs (JSON parse error: %hs)", project, err.text);
         }
      }
      else
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Cannot get components for Jira project %hs (HTTP response code %03d)", project, response);
      }
   }
   else
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 4, L"Call to curl_easy_perform() failed (%hs)", m_errorBuffer);
   }

   return components;
}

/**
 * Process comment update from Jira webhook
 */
void JiraLink::onWebhookCommentUpdate(const wchar_t *hdref, const wchar_t *text)
{
   bool found = false;
   lock();
   removeExpiredComments();
   for(int i = 0; i < m_recentComments.size(); i++)
   {
      Comment *c = m_recentComments.get(i);
      if (!wcscmp(c->issue, hdref) && !wcscmp(c->text, text))
      {
         found = true;
         break;
      }
   }
   unlock();
   if (!found)
      onNewComment(hdref, text);
}

/**
 * Remove expired comments from list
 */
void JiraLink::removeExpiredComments()
{
   time_t threshold = time(nullptr) - 3600;
   for(int i = 0; i < m_recentComments.size(); i++)
   {
      if (m_recentComments.get(i)->timestamp < threshold)
      {
         m_recentComments.remove(i);
         i--;
      }
   }
}

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("JIRA", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   JiraLink *link = new JiraLink();
   if (!link->init())
   {
      nxlog_write_tag(NXLOG_ERROR, JIRA_DEBUG_TAG, L"Jira helpdesk link initialization failed");
      delete link;
      return false;
   }
   if (RegisterHelpDeskLink(link))
   {
      if (ConfigReadBoolean(L"Jira.Webhook.Enable", true))
         RegisterJiraWebhook(link);
      else
         nxlog_debug_tag(JIRA_DEBUG_TAG, 1, L"Jira webhook is disabled");
   }
   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"JIRA");
   module->pfInitialize = InitModule;
   return true;
}

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
