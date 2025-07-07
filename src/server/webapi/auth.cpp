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

#include "webapi.h"
#include <nxcore_2fa.h>
#include <nms_users.h>

/**
 * JWT secret key (configurable in production)
 */
const char *s_jwtSecret = nullptr;

/**
 * Initialize JWT configuration
 */
void InitializeJwtConfiguration()
{
   const TCHAR *configuredSecret = ConfigReadStr(_T("WebAPI.JWT.Secret"), nullptr);
   if (configuredSecret != nullptr)
   {
      s_jwtSecret = UTF8StringFromTString(configuredSecret);
   }
   else
   {
      // Use default secret - should be changed in production
      s_jwtSecret = MemCopyStringA("your-256-bit-secret-key-here-change-in-production-please-make-it-long-and-random");
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_WEBAPI, _T("JWT secret key not configured, using default (not secure for production)"));
   }
   
   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 1, _T("JWT authentication initialized"));
}

/**
 * Get JWT access token validity time
 */
int GetJwtAccessTokenValidityTime()
{
   return ConfigReadInt(_T("WebAPI.JWT.AccessTokenValidityTime"), JWT_ACCESS_TOKEN_VALIDITY_TIME);
}

/**
 * Get JWT refresh token validity time
 */
int GetJwtRefreshTokenValidityTime()
{
   return ConfigReadInt(_T("WebAPI.JWT.RefreshTokenValidityTime"), JWT_REFRESH_TOKEN_VALIDITY_TIME);
}

/**
 * Generate random UUID string for JTI
 */
static String generateJti()
{
   uuid jti = uuid::generate();
   return jti.toString();
}

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
 * Create JWT tokens for authentication
 */
static json_t *createJwtTokens(uint32_t userId, uint64_t systemAccessRights)
{
   time_t now = time(nullptr);
   
   // Create access token payload
   json_t *accessPayload = json_object();
   json_object_set_new(accessPayload, "userId", json_integer(userId));
   json_object_set_new(accessPayload, "systemAccessRights", json_integer(systemAccessRights));
   json_object_set_new(accessPayload, "type", json_string("access"));
   String accessJti = generateJti();
   json_object_set_new(accessPayload, "jti", json_string_t(accessJti.cstr()));
   
   // Create refresh token payload  
   json_t *refreshPayload = json_object();
   json_object_set_new(refreshPayload, "userId", json_integer(userId));
   json_object_set_new(refreshPayload, "type", json_string("refresh"));
   String refreshJti = generateJti();
   json_object_set_new(refreshPayload, "jti", json_string_t(refreshJti.cstr()));
   
   // Generate tokens
   int accessTokenValidityTime = GetJwtAccessTokenValidityTime();
   int refreshTokenValidityTime = GetJwtRefreshTokenValidityTime();
   JwtToken *accessToken = JwtToken::create(accessPayload, s_jwtSecret, now + accessTokenValidityTime);
   JwtToken *refreshToken = JwtToken::create(refreshPayload, s_jwtSecret, now + refreshTokenValidityTime);
   
   json_t *tokens = json_object();
   if (accessToken != nullptr && refreshToken != nullptr)
   {
      String accessTokenStr = accessToken->encode(s_jwtSecret);
      String refreshTokenStr = refreshToken->encode(s_jwtSecret);
      
      if (!accessTokenStr.isEmpty() && !refreshTokenStr.isEmpty())
      {
         json_object_set_new(tokens, "accessToken", json_string_t(accessTokenStr.cstr()));
         json_object_set_new(tokens, "refreshToken", json_string_t(refreshTokenStr.cstr()));
         json_object_set_new(tokens, "accessTokenExpires", json_integer(now + accessTokenValidityTime));
         json_object_set_new(tokens, "refreshTokenExpires", json_integer(now + refreshTokenValidityTime));
         json_object_set_new(tokens, "tokenType", json_string("Bearer"));
      }
   }
   
   delete accessToken;
   delete refreshToken;
   json_decref(accessPayload);
   json_decref(refreshPayload);
   
   return tokens;
}

/**
 * Complete login (legacy token support + new JWT tokens)
 */
static inline void CompleteLogin(json_t *response, const LoginRequest& loginRequest)
{
   // Legacy token for backward compatibility
   UserAuthenticationToken token = IssueAuthenticationToken(loginRequest.userId, AUTH_TOKEN_VALIDITY_TIME, AuthenticationTokenType::EPHEMERAL, _T("Web access token"))->token;
   char encodedToken[64];
   json_object_set_new(response, "token", json_string(token.toStringA(encodedToken)));
   
   // User information
   json_object_set_new(response, "userId", json_integer(loginRequest.userId));
   json_object_set_new(response, "systemAccessRights", json_integer(loginRequest.systemAccessRights));
   json_object_set_new(response, "changePassword", json_boolean(loginRequest.changePassword));
   json_object_set_new(response, "graceLogins", json_integer(loginRequest.graceLogins));
   
   // New JWT tokens
   json_t *jwtTokens = createJwtTokens(loginRequest.userId, loginRequest.systemAccessRights);
   if (json_object_size(jwtTokens) > 0)
   {
      json_object_set(response, "jwt", jwtTokens);
   }
   json_decref(jwtTokens);
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
 * Handler for /login
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
 * Handler for /logout
 */
int H_Logout(Context *context)
{
   // Check if JWT tokens are being used
   json_t *request = context->getRequestDocument();
   if (request != nullptr)
   {
      const char *accessToken = json_object_get_string_utf8(request, "accessToken", nullptr);
      const char *refreshToken = json_object_get_string_utf8(request, "refreshToken", nullptr);
      
      if (accessToken != nullptr)
      {
         JwtToken *token = JwtToken::parse(accessToken, s_jwtSecret);
         if (token != nullptr)
         {
            String jti = token->getJti();
            if (!jti.isEmpty())
            {
               char *jtiUtf8 = UTF8StringFromTString(jti.cstr());
               JwtTokenBlocklist::getInstance()->addToken(jtiUtf8, token->getExpiration());
               MemFree(jtiUtf8);
            }
            delete token;
         }
      }
      
      if (refreshToken != nullptr)
      {
         JwtToken *token = JwtToken::parse(refreshToken, s_jwtSecret);
         if (token != nullptr)
         {
            String jti = token->getJti();
            if (!jti.isEmpty())
            {
               char *jtiUtf8 = UTF8StringFromTString(jti.cstr());
               JwtTokenBlocklist::getInstance()->addToken(jtiUtf8, token->getExpiration());
               MemFree(jtiUtf8);
            }
            delete token;
         }
      }
   }
   
   // Legacy token revocation
   RevokeAuthenticationToken(context->getAuthenticationToken());
   return 204;
}

/**
 * Handler for /refresh - JWT token refresh endpoint
 */
int H_RefreshToken(Context *context)
{
   json_t *request = context->getRequestDocument();
   if (request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Empty refresh token request"));
      return 400;
   }

   const char *refreshTokenStr = json_object_get_string_utf8(request, "refreshToken", nullptr);
   if (refreshTokenStr == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Refresh token missing in request"));
      return 400;
   }

   // Parse and validate refresh token
   JwtToken *refreshToken = JwtToken::parse(refreshTokenStr, s_jwtSecret);
   if (refreshToken == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Invalid refresh token"));
      return 401;
   }

   // Check if token is blocked
   String jti = refreshToken->getJti();
   char *jtiUtf8 = UTF8StringFromTString(jti.cstr());
   bool isBlocked = !jti.isEmpty() && JwtTokenBlocklist::getInstance()->isBlocked(jtiUtf8);
   MemFree(jtiUtf8);
   if (isBlocked)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Refresh token is blocked"));
      delete refreshToken;
      return 401;
   }

   // Verify it's a refresh token
   String tokenType = refreshToken->getClaim("type");
   if (!tokenType.equals(_T("refresh")))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Token is not a refresh token"));
      delete refreshToken;
      return 401;
   }

   // Get user information
   uint32_t userId = static_cast<uint32_t>(refreshToken->getClaimAsInt("userId"));
   delete refreshToken;

   if (userId == 0)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Invalid user ID in refresh token"));
      return 401;
   }

   // Get current user information (to check if user is still valid)
   uint64_t systemAccessRights;
   uint32_t rcc;
   if (!ValidateUserId(userId, nullptr, &systemAccessRights, &rcc))
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("User ID %u is no longer valid"), userId);
      return 401;
   }

   // Create new access token
   time_t now = time(nullptr);
   json_t *accessPayload = json_object();
   json_object_set_new(accessPayload, "userId", json_integer(userId));
   json_object_set_new(accessPayload, "systemAccessRights", json_integer(systemAccessRights));
   json_object_set_new(accessPayload, "type", json_string("access"));
   String accessJti = generateJti();
   json_object_set_new(accessPayload, "jti", json_string_t(accessJti.cstr()));

   int accessTokenValidityTime = GetJwtAccessTokenValidityTime();
   JwtToken *newAccessToken = JwtToken::create(accessPayload, s_jwtSecret, now + accessTokenValidityTime);
   json_decref(accessPayload);

   if (newAccessToken == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Failed to create new access token"));
      return 500;
   }

   String newAccessTokenStr = newAccessToken->encode(s_jwtSecret);
   delete newAccessToken;

   if (newAccessTokenStr.isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG_WEBAPI, 6, _T("Failed to encode new access token"));
      return 500;
   }

   json_t *response = json_object();
   json_object_set_new(response, "accessToken", json_string_t(newAccessTokenStr.cstr()));
   json_object_set_new(response, "accessTokenExpires", json_integer(now + accessTokenValidityTime));
   json_object_set_new(response, "tokenType", json_string("Bearer"));

   context->setResponseData(response);
   json_decref(response);

   nxlog_debug_tag(DEBUG_TAG_WEBAPI, 4, _T("New access token issued for user %u"), userId);
   return 200;
}
