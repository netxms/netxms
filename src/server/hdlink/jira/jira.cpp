/*
** NetXMS - Network Management System
** Helpdesk link module for Jira
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
** File: jira.cpp
**/

#include "jira.h"
#include <jansson.h>

// workaround for older cURL versions
#ifndef CURL_MAX_HTTP_HEADER
#define CURL_MAX_HTTP_HEADER CURL_MAX_WRITE_SIZE
#endif

/**
 * Module name
 */
static TCHAR s_moduleName[] = _T("JIRA");

/**
 * Module version
 */
static TCHAR s_moduleVersion[] = NETXMS_VERSION_STRING;

/**
 * Get integer value from json element
 */
static INT64 JsonIntegerValue(json_t *v)
{
   switch(json_typeof(v))
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
JiraLink::JiraLink() : HelpDeskLink()
{
   m_mutex = MutexCreate();
   strcpy(m_serverUrl, "http://localhost");
   strcpy(m_login, "netxms");
   m_password[0] = 0;
   m_curl = NULL;
}

/**
 * Destructor
 */
JiraLink::~JiraLink()
{
   disconnect();
   MutexDestroy(m_mutex);
}

/**
 * Get module name
 *
 * @return module name
 */
const TCHAR *JiraLink::getName()
{
	return s_moduleName;
}

/**
 * Get module version
 *
 * @return module version
 */
const TCHAR *JiraLink::getVersion()
{
	return s_moduleVersion;
}

/**
 * Initialize module
 *
 * @return true if initialization was successful
 */
bool JiraLink::init()
{
   ConfigReadStrUTF8(_T("JiraServerURL"), m_serverUrl, MAX_OBJECT_NAME, "http://localhost");
   ConfigReadStrUTF8(_T("JiraLogin"), m_login, JIRA_MAX_LOGIN_LEN, "netxms");
   ConfigReadStrUTF8(_T("JiraPassword"), m_password, JIRA_MAX_PASSWORD_LEN, "");
   DbgPrintf(5, _T("Jira: server URL set to %hs"), m_serverUrl);
   return true;
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
 * Connect to Jira server
 */
UINT32 JiraLink::connect()
{
   disconnect();

   m_curl = curl_easy_init();
   if (m_curl == NULL)
   {
      DbgPrintf(4, _T("Jira: call to curl_easy_init() failed"));
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

   RequestData *data = (RequestData *)malloc(sizeof(RequestData));
   memset(data, 0, sizeof(RequestData));
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);

   // Build request
   json_t *root = json_object();
   json_object_set_new(root, "username", json_string(m_login));
   json_object_set_new(root, "password", json_string(m_password));
   char *request = json_dumps(root, 0);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(root);

   char url[MAX_PATH];
   strcpy(url, m_serverUrl);
   strcat(url, "/rest/auth/1/session");
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   struct curl_slist *headers = NULL;
   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
   curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

   UINT32 rcc = RCC_HDLINK_COMM_FAILURE;
   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      data->data[data->size] = 0;
      DbgPrintf(7, _T("Jira: POST request completed, data: %hs"), data->data);
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 200)
      {
         DbgPrintf(4, _T("Jira: login successful"));
         rcc = RCC_SUCCESS;
      }
      else
      {
         DbgPrintf(4, _T("Jira: login failed, HTTP response code %03d"), response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      DbgPrintf(4, _T("Jira: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
   }
   free(request);
   free(data->data);
   free(data);

   return rcc;
}

/**
 * Disconnect (close session)
 */
void JiraLink::disconnect()
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
bool JiraLink::checkConnection()
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
      strcat(url, "/rest/auth/1/session");
      curl_easy_setopt(m_curl, CURLOPT_URL, url);
      if (curl_easy_perform(m_curl) == CURLE_OK)
      {
         data->data[data->size] = 0;
         DbgPrintf(7, _T("Jira: GET request completed, data: %hs"), data->data);
         long response = 500;
         curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
         if (response == 200)
         {
            success = true;
         }
         else
         {
            DbgPrintf(4, _T("Jira: check connection HTTP response code %03d"), response);
         }
      }
      else
      {
         DbgPrintf(4, _T("Jira: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
      }
      free(data->data);
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
UINT32 JiraLink::openIssue(const TCHAR *description, TCHAR *hdref)
{
   if (!checkConnection())
      return RCC_HDLINK_COMM_FAILURE;

   UINT32 rcc = RCC_HDLINK_COMM_FAILURE;
   lock();
   DbgPrintf(4, _T("Jira: create helpdesk issue with description \"%s\""), description);

   // Build request
   json_t *root = json_object();
   json_t *fields = json_object();
   json_t *project = json_object();

   char projectCode[JIRA_MAX_PROJECT_CODE_LEN];
   ConfigReadStrUTF8(_T("JiraProjectCode"), projectCode, JIRA_MAX_PROJECT_CODE_LEN, "NETXMS");

   TCHAR projectComponent[JIRA_MAX_COMPONENT_NAME_LEN];
   ConfigReadStr(_T("JiraProjectComponent"), projectComponent, JIRA_MAX_COMPONENT_NAME_LEN, _T(""));
   ObjectArray<ProjectComponent> *projectComponents = getProjectComponents(projectCode);
   if ((projectComponent[0] != 0) && (projectComponents != NULL))
   {
      for(int i = 0; i < projectComponents->size(); i++)
      {
         ProjectComponent *c = projectComponents->get(i);
         if (!_tcsicmp(c->m_name, projectComponent))
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

   RequestData *data = (RequestData *)malloc(sizeof(RequestData));
   memset(data, 0, sizeof(RequestData));
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);
   json_object_set_new(project, "key", json_string(projectCode));
   json_object_set_new(fields, "project", project);
   char *mbdescr;
#ifdef UNICODE
   mbdescr = UTF8StringFromWideString(description);
#else
   mbdescr = const_cast<char *>(description);
#endif
   char summary[256];
   strlcpy(summary, mbdescr, 256);
   json_object_set_new(fields, "summary", json_string(summary));
   json_object_set_new(fields, "description", json_string(mbdescr));
#ifdef UNICODE
   free(mbdescr);
#endif
   json_t *issuetype = json_object();

   char issueType[JIRA_MAX_ISSUE_TYPE_LEN];
   ConfigReadStrUTF8(_T("JiraIssueType"), issueType, JIRA_MAX_ISSUE_TYPE_LEN, "Task");
   json_object_set_new(issuetype, "name", json_string(issueType));
   json_object_set_new(fields, "issuetype", issuetype);

   json_object_set_new(root, "fields", fields);
   char *request = json_dumps(root, 0);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(root);

   char url[MAX_PATH];
   strcpy(url, m_serverUrl);
   strcat(url, "/rest/api/2/issue");
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      data->data[data->size] = 0;
      DbgPrintf(7, _T("Jira: POST request completed, data: %hs"), data->data);
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 201)
      {
         rcc = RCC_HDLINK_INTERNAL_ERROR;

         json_error_t error;
         json_t *root = json_loads(data->data, 0, &error);
         if (root != NULL)
         {
            json_t *key = json_object_get(root, "key");
            if (json_is_string(key))
            {
#ifdef UNICODE
               MultiByteToWideChar(CP_UTF8, 0, json_string_value(key), -1, hdref, MAX_HELPDESK_REF_LEN);
               hdref[MAX_HELPDESK_REF_LEN - 1] = 0;
#else
               nx_strncpy(hdref, json_string_value(key), MAX_HELPDESK_REF_LEN);
#endif
               rcc = RCC_SUCCESS;
               DbgPrintf(4, _T("Jira: created new issue with reference \"%s\""), hdref);
            }
            else
            {
               DbgPrintf(4, _T("Jira: cannot create issue (cannot extract issue key)"));
            }
            json_decref(root);
         }
         else
         {
            DbgPrintf(4, _T("Jira: cannot create issue (error parsing server response)"));
         }
      }
      else
      {
         DbgPrintf(4, _T("Jira: cannot create issue (HTTP response code %03d)"), response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      DbgPrintf(4, _T("Jira: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
   }
   free(request);
   free(data->data);
   free(data);
   delete projectComponents;

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
UINT32 JiraLink::addComment(const TCHAR *hdref, const TCHAR *comment)
{
   if (!checkConnection())
      return RCC_HDLINK_COMM_FAILURE;

   UINT32 rcc = RCC_HDLINK_COMM_FAILURE;

   lock();
   DbgPrintf(4, _T("Jira: add comment to issue \"%s\" (comment text \"%s\")"), hdref, comment);

   RequestData *data = (RequestData *)malloc(sizeof(RequestData));
   memset(data, 0, sizeof(RequestData));
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);
   curl_easy_setopt(m_curl, CURLOPT_POST, (long)1);

   // Build request
   json_t *root = json_object();
#ifdef UNICODE
   char *mbtext = UTF8StringFromWideString(comment);
   json_object_set_new(root, "body", json_string(mbtext));
   free(mbtext);
#else
   json_object_set_new(root, "body", json_string(comment));
#endif
   char *request = json_dumps(root, 0);
   curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request);
   json_decref(root);

   char url[MAX_PATH];
#ifdef UNICODE
   char *mbref = UTF8StringFromWideString(hdref);
   snprintf(url, MAX_PATH, "%s/rest/api/2/issue/%s/comment", m_serverUrl, mbref);
   free(mbref);
#else
   snprintf(url, MAX_PATH, "%s/rest/api/2/issue/%s/comment", m_serverUrl, hdref);
#endif
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      data->data[data->size] = 0;
      DbgPrintf(7, _T("Jira: POST request completed, data: %hs"), data->data);
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 201)
      {
         rcc = RCC_SUCCESS;
         DbgPrintf(4, _T("Jira: added comment to issue \"%s\""), hdref);
      }
      else
      {
         DbgPrintf(4, _T("Jira: cannot add comment to issue (HTTP response code %03d)"), response);
         rcc = (response == 403) ? RCC_HDLINK_ACCESS_DENIED : RCC_HDLINK_INTERNAL_ERROR;
      }
   }
   else
   {
      DbgPrintf(4, _T("Jira: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
   }
   free(request);
   free(data->data);
   free(data);

   unlock();
   return rcc;
}

/**
 * Get URL to view issue in helpdesk system
 */
bool JiraLink::getIssueUrl(const TCHAR *hdref, TCHAR *url, size_t size)
{
#ifdef UNICODE
   WCHAR serverUrl[MAX_PATH];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, m_serverUrl, -1, serverUrl, MAX_PATH);
   _sntprintf(url, size, _T("%s/browse/%s"), serverUrl, hdref);
#else
   _sntprintf(url, size, _T("%s/browse/%s"), m_serverUrl, hdref);
#endif
   return true;
}

/**
 * Get list of project's components
 */
ObjectArray<ProjectComponent> *JiraLink::getProjectComponents(const char *project)
{
   ObjectArray<ProjectComponent> *components = NULL;

   RequestData *data = (RequestData *)malloc(sizeof(RequestData));
   memset(data, 0, sizeof(RequestData));
   curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, data);

   curl_easy_setopt(m_curl, CURLOPT_POST, (long)0);

   char url[MAX_PATH];
   snprintf(url, MAX_PATH, "%s/rest/api/2/project/%s/components", m_serverUrl, project);
   curl_easy_setopt(m_curl, CURLOPT_URL, url);

   if (curl_easy_perform(m_curl) == CURLE_OK)
   {
      data->data[data->size] = 0;
      DbgPrintf(7, _T("Jira: POST request completed, data: %hs"), data->data);
      long response = 500;
      curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response);
      if (response == 200)
      {
         json_error_t err;
         json_t *root = json_loads(data->data, 0, &err);
         if (root != NULL)
         {
            if (json_is_array(root))
            {
               DbgPrintf(4, _T("Jira: got components list for project %hs"), project);
               int size = (int)json_array_size(root);
               components = new ObjectArray<ProjectComponent>(size, 8, true);
               for(int i = 0; i < size; i++)
               {
                  json_t *e = json_array_get(root, i);
                  if (e == NULL)
                     break;

                  json_t *id = json_object_get(e, "id");
                  json_t *name = json_object_get(e, "name");
                  if ((id != NULL) && (name != NULL))
                  {
                     components->add(new ProjectComponent(JsonIntegerValue(id), json_string_value(name)));
                  }
               }
            }
            else
            {
               DbgPrintf(4, _T("Jira: cannot get components for project %hs (JSON root element is not an array)"), project);
            }
            json_decref(root);
         }
         else
         {
            DbgPrintf(4, _T("Jira: cannot get components for project %hs (JSON parse error: %hs)"), project, err.text);
         }
      }
      else
      {
         DbgPrintf(4, _T("Jira: cannot get components for project %hs (HTTP response code %03d)"), project, response);
      }
   }
   else
   {
      DbgPrintf(4, _T("Jira: call to curl_easy_perform() failed: %hs"), m_errorBuffer);
   }
   free(data->data);
   free(data);

   return components;
}

/**
 * Module entry point
 */
DECLARE_HDLINK_ENTRY_POINT(s_moduleName, JiraLink);

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
