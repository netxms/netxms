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
** File: webhook.cpp
**/

#include "jira.h"

/**
 * Link registered for webhook processing
 */
static JiraLink *s_link = nullptr;

/**
 * Process webhook call
 */
static int ProcessWebhookCall(json_t *request)
{
   const char *event = json_object_get_string_utf8(request, "webhookEvent", "");
   if (!strcmp(event, "comment_created"))
   {
      json_t *comment = json_object_get(request, "comment");
      json_t *issue = json_object_get(request, "issue");
      if ((issue != nullptr) && (comment != nullptr))
      {
         wchar_t *text = json_object_get_string_t(comment, "body", nullptr);
         wchar_t *hdref = json_object_get_string_t(issue, "key", nullptr);
         if ((hdref != nullptr) && (text != nullptr) && (*text != 0))
         {
            nxlog_debug_tag(JIRA_DEBUG_TAG, 5, L"New comment for issue %s: %s", hdref, text);
            s_link->onWebhookCommentUpdate(hdref, text);
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
         wchar_t *hdref = json_object_get_string_t(issue, "key", nullptr);
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
                     nxlog_debug_tag(JIRA_DEBUG_TAG, 6, L"Status change for issue %s (new status %hs)", hdref, CHECK_NULL_A(status));
                     if (status != nullptr)
                     {
                        char resolvedStatus[1024];
                        ConfigReadStrUTF8(L"Jira.ResolvedStatus", resolvedStatus, 1024, "Done");
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
                              nxlog_debug_tag(JIRA_DEBUG_TAG, 5, L"Mark issue %s as resolved", hdref);
                              s_link->onWebhookIssueClose(hdref);
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

   return 200;
}

/**
 * Webhook route handler
 */
static int H_JiraWebhook(Context *context)
{
   char secret[256];
   ConfigReadStrUTF8(L"Jira.Webhook.Secret", secret, sizeof(secret), "");
   if (secret[0] != 0)
   {
      const char *s = context->getQueryParameter("secret");
      if ((s == nullptr) || strcmp(s, secret))
      {
         nxlog_debug_tag(JIRA_DEBUG_TAG, 5, L"Jira webhook call from %s rejected (missing or invalid secret)", context->getClientAddress().toString().cstr());
         return 403;
      }
   }

   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(JIRA_DEBUG_TAG, 5, L"Cannot parse webhook data");
      return 400;
   }

   return ProcessWebhookCall(request);
}

/**
 * Register Jira webhook on common web API listener
 */
void RegisterJiraWebhook(JiraLink *link)
{
   s_link = link;
   RouteBuilder("jira-webhook")
      .POST(H_JiraWebhook)
      .noauth()
      .build();
   nxlog_write_tag(NXLOG_INFO, JIRA_DEBUG_TAG, L"Jira webhook registered on web API path /jira-webhook");
}
