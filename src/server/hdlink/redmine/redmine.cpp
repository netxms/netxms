/*
** NetXMS - Network Management System
** Helpdesk link module for RedMine
** Copyright (C) 2014 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: redmine.cpp
**/

#include "redmine.h"
#include <jansson.h>

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

/**
 * Module name
 */
static TCHAR s_moduleName[] = _T("REDMINE");

/**
 * Module version
 */
static TCHAR s_moduleVersion[] = NETXMS_VERSION_STRING;

/**
 * Get integer value from json element
 */
static INT64 JsonIntegerValue(json_t *v)
{
   switch (json_typeof(v))
   {
   case JSON_INTEGER:
      return (INT64)json_integer_value(v);
   case JSON_REAL:
      return (INT64)json_real_value(v);
   case JSON_TRUE:
      return 1;
   case JSON_STRING:
      return strtoll(json_string_value(v), NULL, 0);
   default:
      return 0;
   }
}

/**
 * Constructor
 */
RedmineLink::RedmineLink() : HelpDeskLink()
{
   m_mutex = MutexCreate();
   strcpy(m_serverUrl, "http://localhost:3000");
   strcpy(m_apiKey, "netxms");
   m_password[0] = 0;
   m_curl = NULL;
}

/**
 * Destructor
 */
RedmineLink::~RedmineLink()
{
   disconnect();
   MutexDestroy(m_mutex);
}

/**
 * Get module name
 *
 * @return module name
 */
const TCHAR *RedmineLink::getName()
{
   return s_moduleName;
}

/**
 * Initialize module
 *
 * @return true if initialization was successful
 */
bool RedmineLink::init()
{
   ConfigReadStrUTF8(_T("RedmineServerURL"), m_serverUrl, MAX_OBJECT_NAME, "http://localhost");
   ConfigReadStrUTF8(_T("RedmineApiKey"), m_apiKey, JIRA_MAX_LOGIN_LEN, "n/a");
   DbgPrintf(5, _T("Redmine: server URL set to %hs"), m_serverUrl);
   return true;
}

/**
 * Get module version
 *
 * @return module version
 */
const TCHAR *RedmineLink::getVersion()
{
   return s_moduleVersion;
}

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   RequestData *data = (RequestData *)userdata;
   if ((data->allocated - data->size) < (size * nmemb))
   {
      char *newData = (char *)realloc(data->data, data->allocated + CURL_MAX_HTTP_HEADER);
      if (newData == NULL)
      {
         return 0;
      }
      data->data = newData;
      data->allocated += CURL_MAX_HTTP_HEADER;
   }

   memcpy(data->data + data->size, ptr, size * nmemb);
   data->size += size * nmemb;

   return size * nmemb;
}

/**
 * Connect to RedMine server
 */
UINT32 RedmineLink::connect()
{
   disconnect();

   m_curl = curl_easy_init();
   if (m_curl == NULL)
   {
      DbgPrintf(4, _T("RedMine: call to curl_easy_init() failed"));
      return RCC_HDLINK_INTERNAL_ERROR;
   }

   curl_easy_setopt(m_curl, CURLOPT_VERBOSE, (long)1);

#if HAVE_DECL_CURLOPT_NOSIGNAL
   curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, (long)1);
#endif
   curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errorBuffer);
   curl_easy_setopt(m_curl, CURLOPT_HEADER, (long)0); // do not include header in data
   curl_easy_setopt(m_curl, CURLOPT_COOKIEFILE, "");  // enable cookies in memory
   curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, (long)0);

   curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);

   struct curl_slist *headers = NULL;
   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json; charset=UTF-8");
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

   RequestData *data = (RequestData *)malloc(sizeof(RequestData));
   memset(data, 0, sizeof(RequestData));
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);
   return RCC_SUCCESS;
}

/**
 * Disconnect (close session)
 */
void RedmineLink::disconnect()
{
   if (m_curl == NULL)
      return;

   curl_easy_cleanup(m_curl);
   m_curl = NULL;
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

   if (m_curl != NULL)
   {
      RequestData *data = (RequestData *)malloc(sizeof(RequestData));
      memset(data, 0, sizeof(RequestData));
      curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);

      curl_easy_setopt(m_curl, CURLOPT_POST, (long)0);

      char url[MAX_PATH];
      strcpy(url, m_serverUrl);
      strcat(url, "/users/current.json?key=");
      strcat(url, m_apiKey);
      curl_easy_setopt(m_curl, CURLOPT_URL, url);
      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         data->data[data->size] = 0;
         DbgPrintf(7, _T("RedMine: GET request completed, data: %hs"), data->data);
         long response = 500;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
         if (response == 200)
         {
            success = true;
         }
         else
         {
            DbgPrintf(4, _T("RedMine: check connection HTTP response code %03d"), response);
         }
      }
      else
      {
         DbgPrintf(4, _T("RedMine: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
      }
      free(data);
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
UINT32 RedmineLink::openIssue(const TCHAR *description, TCHAR *hdref)
{
   if (!checkConnection())
   {
      return RCC_HDLINK_COMM_FAILURE;
   }

   UINT32 rcc = RCC_HDLINK_COMM_FAILURE;
   lock();
   DbgPrintf(4, _T("RedMine: create helpdesk issue with description \"%s\""), description);

   RequestData *data = (RequestData *)malloc(sizeof(RequestData));
   memset(data, 0, sizeof(RequestData));
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);
   
   // Build request
   int projectId = ConfigReadInt(_T("RedmineProjectId"), 1);
   int trackerId = ConfigReadInt(_T("RedmineTrackerId"), 3);
   int statusId = ConfigReadInt(_T("RedmineStatusId"), 1);
   int priorityId = ConfigReadInt(_T("RedminePriorityId"), 2);

#ifdef UNICODE
   char *mbdescr = UTF8StringFromWideString(description);
   json_t *requestRoot = json_pack("{s:{s:i, s:i, s:i, s:i, s:s}}",
      "issue",
      "project_id", projectId,
      "tracker_id", trackerId,
      "status_id", statusId,
      "priority_id", priorityId,
      "subject", mbdescr);
   free(mbdescr);
#else
   json_t *requestRoot = json_pack("{s:{s:i, s:i, s:i, s:i, s:s}}",
      "issue",
      "project_id", projectId,
      "tracker_id", trackerId,
      "status_id", statusId,
      "priority_id", priorityId,
      "subject", description);
#endif

   char *request = json_dumps(requestRoot, 0);
   printf(">>> %s\n", request); fflush(stdout);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(requestRoot);

   char url[MAX_PATH];
   strcpy(url, m_serverUrl);
   strcat(url, "/issues.json?key=");
   strcat(url, m_apiKey);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      data->data[data->size] = 0;
      DbgPrintf(7, _T("RedMine: POST request completed, data: %hs"), data->data);
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 201)
      {
         rcc = RCC_HDLINK_INTERNAL_ERROR;

         json_error_t error;
         json_t *responseRoot = json_loads(data->data, 0, &error);
         if (responseRoot != NULL)
         {
            json_t *issue = json_object_get(responseRoot, "issue");
            if (issue != NULL) {
               json_t *issueId = json_object_get(issue, "id");
               if (json_is_number(issueId))
               {
                  INT64 id = JsonIntegerValue(issueId);
                  _sntprintf(hdref, MAX_HELPDESK_REF_LEN, _T("%ld"), id);

                  rcc = RCC_SUCCESS;
                  DbgPrintf(4, _T("RedMine: created new issue with reference \"%s\""), hdref);
               }
               else
               {
                  DbgPrintf(4, _T("RedMine: cannot create issue (cannot extract issue ID)"));
               }
            }
            else
            {
               DbgPrintf(4, _T("RedMine: cannot create issue (invalid response)"));
            }
            json_decref(responseRoot);
         }
         else
         {
            DbgPrintf(4, _T("RedMine: cannot create issue (error parsing server response)"));
         }
      }
      else
      {
         DbgPrintf(4, _T("RedMine: cannot create issue (HTTP response code %03d)"), response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      DbgPrintf(4, _T("RedMine: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
   }
   free(request);
   free(data);

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
UINT32 RedmineLink::addComment(const TCHAR *hdref, const TCHAR *comment)
{
   return RCC_HDLINK_COMM_FAILURE;
}

/**
 * Get URL to view issue in helpdesk system
 */
bool RedmineLink::getIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size)
{
#ifdef UNICODE
   WCHAR serverUrl[MAX_PATH];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_serverUrl, -1, serverUrl, MAX_PATH);
   _sntprintf(url, size, _T("%s/issues/%s"), serverUrl, hdref);
#else
   _sntprintf(url, size, _T("%s/issues/%s"), m_serverUrl, hdref);
#endif
   return true;
}

/**
 * Module entry point
 */
DECLARE_HDLINK_ENTRY_POINT(s_moduleName, RedmineLink);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
   {
      DisableThreadLibraryCalls(hInstance);
   }
   return TRUE;
}

#endif
