/*
** NetXMS - Network Management System
** Helpdesk link module for Redmine
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
** File: redmine.cpp
**/

#include "redmine.h"
#include <nxlibcurl.h>
#include <netxms-version.h>

#define DEBUG_TAG L"hdlink.redmine"

/**
 * Constructor
 */
RedmineLink::RedmineLink() : HelpDeskLink()
{
   strcpy(m_serverUrl, "http://localhost:3000");
   strcpy(m_apiKey, "netxms");
   m_curl = nullptr;
   m_verifyPeer = false;
}

/**
 * Destructor
 */
RedmineLink::~RedmineLink()
{
   disconnect();
}

/**
 * Get link name
 *
 * @return link name
 */
const wchar_t *RedmineLink::getName()
{
   return L"REDMINE";
}

/**
 * Initialize link
 *
 * @return true if initialization was successful
 */
bool RedmineLink::init()
{
   if (!InitializeLibCURL())
   {
      nxlog_debug_tag(DEBUG_TAG, 1, L"Redmine: cURL initialization failed");
      return false;
   }
   ConfigReadStrUTF8(L"Redmine.ServerURL", m_serverUrl, MAX_OBJECT_NAME, "http://localhost");
   ConfigReadStrUTF8(L"Redmine.ApiKey", m_apiKey, REDMINE_MAX_API_KEY_LEN, "n/a");
   m_verifyPeer = ConfigReadBoolean(L"Redmine.VerifyPeer", true);
   nxlog_debug_tag(DEBUG_TAG, 5, L"Redmine: server URL set to %hs", m_serverUrl);
   return true;
}

/**
 * Connect to RedMine server
 */
uint32_t RedmineLink::connect()
{
   disconnect();

   m_curl = curl_easy_init();
   if (m_curl == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: call to curl_easy_init() failed");
      return RCC_HDLINK_INTERNAL_ERROR;
   }

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, (long)1);
#endif
   curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);
   curl_easy_setopt(m_curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");  // enable cookies in memory
   curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, static_cast<long>(m_verifyPeer ? 1 : 0));

   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);

   struct curl_slist *headers = nullptr;
   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);
   return RCC_SUCCESS;
}

/**
 * Disconnect (close session)
 */
void RedmineLink::disconnect()
{
   if (m_curl == nullptr)
      return;

   curl_easy_cleanup(m_curl);
   m_curl = nullptr;
}

/**
 * Check that connection with helpdesk system is working
 *
 * @return true if connection is working
 */
bool RedmineLink::checkConnection()
{
   bool success = false;

   lock();

   if (m_curl != nullptr)
   {
      ByteStream responseData(32768);
      responseData.setAllocationStep(32768);
      curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

      curl_easy_setopt(m_curl, CURLOPT_POST, (long)0);

      char url[MAX_PATH];
      strcpy(url, m_serverUrl);
      strcat(url, "/users/current.json?key=");
      strcat(url, m_apiKey);
      curl_easy_setopt(m_curl, CURLOPT_URL, url);
      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         responseData.write(static_cast<char>(0));
         nxlog_debug_tag(DEBUG_TAG, 7, L"RedMine: GET request completed, data: %hs", responseData.buffer());
         long response = 500;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
         if (response == 200)
         {
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: check connection HTTP response code %03d", response);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: call to curl_easy_perform() failed: %hs", m_errorBuffer);
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
uint32_t RedmineLink::openIssue(const wchar_t *description, wchar_t *hdref)
{
   if (!checkConnection())
      return RCC_HDLINK_COMM_FAILURE;

   uint32_t rcc = RCC_HDLINK_COMM_FAILURE;
   lock();
   nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: create helpdesk issue with description \"%s\"", description);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);

   // Build request
   int projectId = ConfigReadInt(L"Redmine.ProjectId", 1);
   int trackerId = ConfigReadInt(L"Redmine.TrackerId", 3);
   int statusId = ConfigReadInt(L"Redmine.StatusId", 1);
   int priorityId = ConfigReadInt(L"Redmine.PriorityId", 2);

   char *mbdescr = UTF8StringFromWideString(description);
   json_t *requestRoot = json_pack("{s:{s:i, s:i, s:i, s:i, s:s}}",
      "issue",
      "project_id", projectId,
      "tracker_id", trackerId,
      "status_id", statusId,
      "priority_id", priorityId,
      "subject", mbdescr);
   MemFree(mbdescr);

   char *request = json_dumps(requestRoot, 0);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(requestRoot);

   char url[MAX_PATH];
   strcpy(url, m_serverUrl);
   strcat(url, "/issues.json?key=");
   strcat(url, m_apiKey);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      responseData.write(static_cast<char>(0));
      nxlog_debug_tag(DEBUG_TAG, 7, L"RedMine: POST request completed, data: %hs", responseData.buffer());
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 201)
      {
         rcc = RCC_HDLINK_INTERNAL_ERROR;

         json_error_t error;
         json_t *responseRoot = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
         if (responseRoot != nullptr)
         {
            json_t *issue = json_object_get(responseRoot, "issue");
            if (issue != nullptr)
            {
               json_t *issueId = json_object_get(issue, "id");
               if (json_is_number(issueId))
               {
                  IntegerToString(static_cast<int64_t>(json_integer_value_ex(issueId, 0)), hdref);
                  rcc = RCC_SUCCESS;
                  nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: created new issue with reference \"%s\"", hdref);
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot create issue (cannot extract issue ID)");
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot create issue (invalid response)");
            }
            json_decref(responseRoot);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot create issue (error parsing server response)");
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot create issue (HTTP response code %03d)", response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: call to curl_easy_perform() failed: %hs", m_errorBuffer);
   }
   MemFree(request);

   unlock();
   return rcc;
}

/**
 * Get current state of given issue.
 *
 * @param hdref issue reference
 * @param open pointer to indicator to be set if issue is still open
 * @return RCC ready to be sent to client
 */
uint32_t RedmineLink::getIssueState(const wchar_t *hdref, bool *open)
{
   if (!checkConnection())
      return RCC_HDLINK_COMM_FAILURE;

   lock();
   nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: reading state for issue \"%s\"", hdref);

   ByteStream responseData(32768);
   responseData.setAllocationStep(32768);
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseData);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)0);

   char url[MAX_PATH];
   strcpy(url, m_serverUrl);
   strlcat(url, "/issues.json?key=", MAX_PATH);
   strlcat(url, m_apiKey, MAX_PATH);
   strlcat(url, "&issue_id=", MAX_PATH);
   size_t l = strlen(url);
   wchar_to_utf8(hdref, -1, &url[l], MAX_PATH - l - 1);
   url[MAX_PATH - 1] = 0;
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   uint32_t rcc = RCC_HDLINK_COMM_FAILURE;
   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      responseData.write(static_cast<char>(0));
      nxlog_debug_tag(DEBUG_TAG, 7, L"RedMine: GET request completed, data: %hs", responseData.buffer());
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 200)
      {
         rcc = RCC_HDLINK_INTERNAL_ERROR;

         json_error_t error;
         json_t *responseRoot = json_loads(reinterpret_cast<const char*>(responseData.buffer()), 0, &error);
         if (responseRoot != nullptr)
         {
            json_t *issues = json_object_get(responseRoot, "issues");
            if (json_is_array(issues))
            {
               if (json_array_size(issues) > 0)
               {
                  json_t *issue = json_array_get(issues, 0);
                  if (json_is_object(issue))
                  {
                     json_t *status = json_object_get_by_path_a(issue, "status/is_closed");
                     if (json_is_boolean(status))
                     {
                        *open = !json_boolean_value(status);
                        rcc = RCC_SUCCESS;
                        nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: issue state is %s", *open ? L"OPEN" : L"CLOSED");
                     }
                     else
                     {
                        nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot get issue state (status/is_closed unavailable)");
                     }
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot get issue state (invalid response)");
                  }
               }
               else
               {
                  *open = false;
                  rcc = RCC_SUCCESS;
                  nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: issue is not listed, assuming closed");
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot get issue state (invalid response)");
            }
            json_decref(responseRoot);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot get issue state (error parsing server response)");
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: cannot get issue state (HTTP response code %03d)", response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, L"RedMine: call to curl_easy_perform() failed: %hs", m_errorBuffer);
   }

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
uint32_t RedmineLink::addComment(const wchar_t *hdref, const wchar_t *comment)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Get URL to view issue in helpdesk system
 */
bool RedmineLink::getIssueUrl(const wchar_t *hdref, wchar_t *url, size_t size)
{
   wchar_t serverUrl[MAX_PATH];
   utf8_to_wchar(m_serverUrl, -1, serverUrl, MAX_PATH);
   nx_swprintf(url, size, L"%s/issues/%s", serverUrl, hdref);
   return true;
}

/**
 * Module metadata
 */
DEFINE_MODULE_METADATA("REDMINE", "Raden Solutions", NETXMS_VERSION_STRING_A, NETXMS_BUILD_TAG_A)

/**
 * Initialize module
 */
static bool InitModule(Config *config)
{
   RedmineLink *link = new RedmineLink();
   if (!link->init())
   {
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Redmine helpdesk link initialization failed");
      delete link;
      return false;
   }
   RegisterHelpDeskLink(link);
   return true;
}

/**
 * Module entry point
 */
extern "C" bool __EXPORT NXM_Register(NXMODULE *module, Config *config)
{
   module->dwSize = sizeof(NXMODULE);
   wcscpy(module->name, L"REDMINE");
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
