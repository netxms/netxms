/*
** NetXMS - Network Management System
** Helpdesk link module for Jira
** Copyright (C) 2014-2022 Raden Solutions
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
** File: webhook.cpp
**/

#include "jira.h"

#if WITH_MICROHTTPD

#include <microhttpd.h>

#if MHD_VERSION < 0x00097002
#define MHD_Result int
#endif

/**
 * Process webhook call
 */
static int ProcessWebhookCall(const char *data, JiraLink *link)
{
   json_error_t error;
   json_t *request = json_loads(data, 0, &error);
   if (request == nullptr)
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 5, _T("Cannot parse webhook data (%hs)"), error.text);
      return 400; // Bad Request
   }

   const char *event = json_object_get_string_utf8(request, "webhookEvent", "");
   if (!strcmp(event, "comment_created"))
   {
      json_t *comment = json_object_get(request, "comment");
      json_t *issue = json_object_get(request, "issue");
      if ((issue != nullptr) && (comment != nullptr))
      {
         TCHAR *text = json_object_get_string_t(comment, "body", nullptr);
         TCHAR *hdref = json_object_get_string_t(issue, "key", nullptr);
         if ((hdref != nullptr) && (text != nullptr) && (*text != 0))
         {
            nxlog_debug_tag(JIRA_DEBUG_TAG, 5, _T("New comment for issue %s: %s"), hdref, text);
            link->onWebhookCommentUpdate(hdref, text);
         }
         MemFree(text);
         MemFree(hdref);
      }
   }
   else if (!strcmp(event, "jira:issue_updated"))
   {
      json_t *issue = json_object_get(request, "issue");
      if (issue != nullptr)
      {
         TCHAR *hdref = json_object_get_string_t(issue, "key", nullptr);
         if (hdref != nullptr)
         {
            json_t *changes = json_object_get(json_object_get(request, "changelog"), "items");
            if (json_is_array(changes))
            {
               size_t i;
               json_t *change;
               json_array_foreach(changes, i, change)
               {
                  const char *fieldId = json_object_get_string_utf8(change, "fieldId", nullptr);
                  if ((fieldId != nullptr) && !strcmp(fieldId, "status"))
                  {
                     const char *status = json_object_get_string_utf8(change, "toString", nullptr);
                     nxlog_debug_tag(JIRA_DEBUG_TAG, 6, _T("Status change for issue %s (new status %hs)"), hdref, CHECK_NULL_A(status));
                     if (status != nullptr)
                     {
                        char resolvedStatus[1024];
                        ConfigReadStrUTF8(_T("Jira.ResolvedStatus"), resolvedStatus, 1024, "Done");
                        char *curr = resolvedStatus;
                        char *next;
                        do
                        {
                           next = strchr(curr, ',');
                           if (next != nullptr)
                              *next = 0;
                           TrimA(curr);
                           if (!stricmp(curr, status))
                           {
                              nxlog_debug_tag(JIRA_DEBUG_TAG, 5, _T("Mark issue %s as resolved"), hdref);
                              link->onWebhookIssueClose(hdref);
                              break;
                           }
                           curr = next + 1;
                        }
                        while(next != nullptr);
                     }
                  }
               }
            }
            MemFree(hdref);
         }
      }
   }

   json_decref(request);
   return 200;
}

/**
 * Dump HTTP request headers
 */
static MHD_Result DumpHeaders(void *context, MHD_ValueKind kind, const char *key, const char *value)
{
   nxlog_debug_tag(JIRA_DEBUG_TAG, 7, _T("   %hs: %hs"), key, value);
   return MHD_YES;
}

/**
 * Connection handler
 */
static MHD_Result ConnectionHandler(void *context, MHD_Connection *connection, const char *url, const char *method, const char *version, const char *uploadData, size_t *uploadDataSize, void **connectionContext)
{
   if (*connectionContext == nullptr)
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 6, _T("Jira webhook request: %hs %hs"), method, url);
      if (nxlog_get_debug_level_tag(JIRA_DEBUG_TAG) >= 7)
         MHD_get_connection_values(connection, MHD_HEADER_KIND, DumpHeaders, nullptr);
   }

   int responseCode;
   if (!strcmp(url, static_cast<JiraLink*>(context)->getWebhookURL()))
   {
      if (!strcmp(method, "POST"))
      {
         if (*connectionContext == nullptr)  // First call
         {
            // Check content type
            const char *contentType = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
            if ((contentType != nullptr) && (!strcmp(contentType, "application/json") || !strncmp(contentType, "application/json;", 17)))
            {
               auto dataBuffer = new ByteStream(32768);
               dataBuffer->setAllocationStep(32768);
               *connectionContext = dataBuffer;
               return MHD_YES;
            }
            responseCode = 415;  // Unsupported Media Type
         }
         else
         {
            auto dataBuffer = static_cast<ByteStream*>(*connectionContext);
            if (*uploadDataSize != 0)
            {
               dataBuffer->write(uploadData, *uploadDataSize);
               *uploadDataSize = 0;
               return MHD_YES;
            }

            // process request
            dataBuffer->write('\0');
            nxlog_debug_tag(JIRA_DEBUG_TAG, 6, _T("Jira webhook request data: %hs"), dataBuffer->buffer());
            responseCode = ProcessWebhookCall(reinterpret_cast<const char*>(dataBuffer->buffer()), static_cast<JiraLink*>(context));
         }
      }
      else
      {
         responseCode = 405;  // Method Not Allowed
      }
   }
   else
   {
      responseCode = 404;  // Not Found
   }
   MHD_Response *response = MHD_create_response_from_buffer(0, nullptr, MHD_RESPMEM_PERSISTENT);
   MHD_Result rc = MHD_queue_response(connection, responseCode, response);
   MHD_destroy_response(response);
   nxlog_debug_tag(JIRA_DEBUG_TAG, 6, _T("Response code %d to webhook call"), responseCode);
   return rc;
}

/**
 * Request completion handler
 */
static void RequestCompleted(void *context, MHD_Connection *connection, void **connectionContext, MHD_RequestTerminationCode tcode)
{
   delete static_cast<ByteStream*>(*connectionContext);
   *connectionContext = nullptr;
}

/**
 * Custom logger
 */
static void Logger(void *context, const char *format, va_list args)
{
   char buffer[8192];
   vsnprintf(buffer, 8192, format, args);
   nxlog_debug_tag(JIRA_DEBUG_TAG, 6, _T("MicroHTTPD: %hs"), buffer);
}

/**
 * Create webhook
 */
void *CreateWebhook(JiraLink *link, uint16_t port)
{
   MHD_Daemon *daemon = MHD_start_daemon(
         MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_POLL | MHD_USE_ERROR_LOG,
         port, nullptr, nullptr, ConnectionHandler, link,
         MHD_OPTION_EXTERNAL_LOGGER, Logger, nullptr,
         MHD_OPTION_NOTIFY_COMPLETED, RequestCompleted, nullptr,
         MHD_OPTION_END);
   if (daemon != nullptr)
      nxlog_write_tag(NXLOG_INFO, JIRA_DEBUG_TAG, _T("Jira webhook initialized on port %u"), port);
   else
      nxlog_write_tag(NXLOG_WARNING, JIRA_DEBUG_TAG, _T("Jira webhook not initialized (MicroHTTPD initialization error)"));
   return daemon;
}

/**
 * Destroy webhook
 */
void DestroyWebhook(void *handle)
{
   MHD_stop_daemon(static_cast<MHD_Daemon*>(handle));
   nxlog_write_tag(NXLOG_INFO, JIRA_DEBUG_TAG, _T("Jira webhook closed"));
}

#else

/**
 * Create webhook
 */
void *CreateWebhook(JiraLink *link, uint16_t port)
{
   nxlog_write_tag(NXLOG_WARNING, JIRA_DEBUG_TAG, _T("Jira webhook not initialized (server was built without MicroHTTPD)"));
   return nullptr;
}

/**
 * Destroy webhook
 */
void DestroyWebhook(void *handle)
{
}

#endif
