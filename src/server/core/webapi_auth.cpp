/*
** NetXMS - Network Management System
** Copyright (C) 2023-2025 Raden Solutions
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
** File: auth.cpp
**
**/

#include "nxcore.h"
#include <netxms-webapi.h>
#include <nxcore_2fa.h>
#include <nms_users.h>

/**
 * Pending login request
 */
struct LoginRequest
{
   uint32_t userId;
   uint32_t graceLogins;
   uint64_t systemAccessRights;
   bool changePassword;
   bool intruderLockout;
   bool closeOtherSessions;
   time_t timestamp;
   TwoFactorAuthenticationToken *token;
};

/**
 * Pending login requests
 */
static SynchronizedHashMap<uuid, LoginRequest> s_pendingLoginRequests(Ownership::False);

/**
 * Scheduled task for checking pending login requests
 */
void CheckPendingLoginRequests(const shared_ptr<ScheduledTaskParameters>& parameters)
{
   time_t expirationTime = time(nullptr) - 60;
   StructArray<uuid> expiredTokens;
   s_pendingLoginRequests.forEach(
      [expirationTime, &expiredTokens] (const uuid& id, LoginRequest *request) -> EnumerationCallbackResult
      {
         if (request->timestamp < expirationTime)
            expiredTokens.add(id);
         return _CONTINUE;
      });
   for(int i = 0; i < expiredTokens.size(); i++)
   {
      uuid id(*expiredTokens.get(i));
      LoginRequest *request = s_pendingLoginRequests.take(id);
      if (request != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 5, _T("Login request \"%s\" has expired"), id.toString().cstr());
         delete request->token;
         delete request;
      }
   }
}

/**
 * Complete login
 */
static inline void CompleteLogin(json_t *response, const LoginRequest& loginRequest)
{
   UserAuthenticationToken token = IssueAuthenticationToken(loginRequest.userId, AUTH_TOKEN_VALIDITY_TIME, AuthenticationTokenType::EPHEMERAL, _T("Web access token"))->token;
   char encodedToken[64];
   json_object_set_new(response, "token", json_string(token.toStringA(encodedToken)));
   json_object_set_new(response, "userId", json_integer(loginRequest.userId));
   json_object_set_new(response, "systemAccessRights", json_integer(loginRequest.systemAccessRights));
   json_object_set_new(response, "changePassword", json_boolean(loginRequest.changePassword));
   json_object_set_new(response, "graceLogins", json_integer(loginRequest.graceLogins));
}

/**
 * Process 2FA response
 */
static int Process2FAResponse(Context *context)
{
   json_t *request = context->getRequestDocument();

   uuid requestId = uuid::parseA(json_object_get_string_utf8(request, "requestId", ""));
   LoginRequest *loginRequest = s_pendingLoginRequests.take(requestId);
   if (loginRequest == nullptr)
   {
      TCHAR buffer[64];
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Invalid pending login request ID %s"), requestId.toString(buffer));
      return 403;
   }

   json_t *response = json_object();
   int responseCode;

   TCHAR *userResponse = json_object_get_string_t(request, "response", nullptr);
   if (userResponse != nullptr)
   {
      if (Validate2FAResponse(loginRequest->token, userResponse, loginRequest->userId, nullptr, nullptr))
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Two-factor authentication user response validation successful"));
         CompleteLogin(response, *loginRequest);
         responseCode = 201;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Two-factor authentication user response validation failed"));
         responseCode = 403;
      }
      MemFree(userResponse);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Two-factor authentication user response is missing"));
      responseCode = 400;
   }

   context->setResponseData(response);
   json_decref(response);

   delete loginRequest->token;
   delete loginRequest;

   return responseCode;
}

/**
 * Handler for /v1/login
 */
int H_Login(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Empty login request"));
      return 400;
   }

   if (json_object_get(request, "requestId") != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Processing as 2FA response"));
      return Process2FAResponse(context);
   }

   TCHAR *username = json_object_get_string_t(request, "username", nullptr);
   if (username == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("User name missing in login request"));
      return 400;
   }

   TCHAR *password = json_object_get_string_t(request, "password", nullptr);
   if (password == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Password missing in login request"));
      MemFree(username);
      return 400;
   }

#if WITH_PRIVATE_EXTENSIONS
   if (!IsComponentRegistered(_T("EMCL")) && !IsComponentRegistered(_T("WEBAPI")))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Password missing in login request"));
      MemFree(username);
      MemFree(password);
      context->setErrorResponse("Required license not installed");
      return 403;
   }
#endif

   json_t *response = json_object();

   int responseCode;
   LoginRequest loginRequest;
   memset(&loginRequest, 0, sizeof(loginRequest));
   uint32_t rcc = AuthenticateUser(username, password, 0, nullptr, nullptr, &loginRequest.userId, &loginRequest.systemAccessRights,
         &loginRequest.changePassword, &loginRequest.intruderLockout, &loginRequest.closeOtherSessions, false, &loginRequest.graceLogins);
   if (rcc == RCC_SUCCESS)
   {
      unique_ptr<StringList> methods = GetUserConfigured2FAMethods(loginRequest.userId);
      if (!methods->isEmpty())
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Two-factor authentication is required (%d methods available)"), methods->size());

         TCHAR *selectedMethod = json_object_get_string_t(request, "method", nullptr);
         if ((selectedMethod == nullptr) || !methods->contains(selectedMethod))
         {
            StringBuffer content(_T("2FA "));
            for(int i = 0; i < methods->size(); i++)
            {
               if (i > 0)
                  content.append(_T(','));
               content.append(methods->get(i));
            }
            context->setResponseHeader(_T("WWW-Authenticate"), content);

            json_object_set_new(response, "methods", methods->toJson());

            responseCode = 401;
         }
         else
         {
            loginRequest.token = Prepare2FAChallenge(selectedMethod, loginRequest.userId);
            if (loginRequest.token != nullptr)
            {
               uuid requestId = uuid::generate();

               StringBuffer content(_T("2FA "));
               content.append(selectedMethod);
               content.append(_T('='));
               content.append(requestId);
               content.append(_T('+'));
               content.append(loginRequest.token->getChallenge());
               context->setResponseHeader(_T("WWW-Authenticate"), content);

               json_object_set_new(response, "requestId", requestId.toJson());
               json_object_set_new(response, "method", json_string_t(selectedMethod));
               json_object_set_new(response, "challenge", json_string_t(loginRequest.token->getChallenge()));
               json_object_set_new(response, "qrLabel", json_string_t(loginRequest.token->getQRLabel()));

               loginRequest.timestamp = time(nullptr);
               s_pendingLoginRequests.set(requestId, new LoginRequest(loginRequest));

               responseCode = 401;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Cannot prepare two-factor authentication challenge for method \"%s\""), selectedMethod);
               responseCode = 500;
            }
         }

         MemFree(selectedMethod);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("Two-factor authentication is not required (not configured)"));
         CompleteLogin(response, loginRequest);
         responseCode = 201;
      }
   }
   else
   {
      json_object_set_new(response, "errorCode", json_integer(rcc));
      responseCode = 403;
   }

   context->setResponseData(response);
   json_decref(response);

   MemFree(username);
   MemFree(password);

   return responseCode;
}

/**
 * Handler for /v1/logout
 */
int H_Logout(Context *context)
{
   RevokeAuthenticationToken(context->getAuthenticationToken());
   return 204;
}
