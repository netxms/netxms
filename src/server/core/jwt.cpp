/*
** NetXMS - Network Management System
** Server Core
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: jwt.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("jwt")

/**
 * URL-safe Base64 encode
 */
static char *base64url_encode(const BYTE *data, size_t length)
{
   char *encoded = nullptr;
   size_t encodedLen = base64_encode_alloc(reinterpret_cast<const char*>(data), length, &encoded);
   if (encoded == nullptr)
      return nullptr;

   // Convert to URL-safe format: replace + with -, / with _, remove padding =
   for (size_t i = 0; i < encodedLen; i++)
   {
      if (encoded[i] == '+')
         encoded[i] = '-';
      else if (encoded[i] == '/')
         encoded[i] = '_';
      else if (encoded[i] == '=')
      {
         encoded[i] = '\0';
         break;
      }
   }
   return encoded;
}

/**
 * URL-safe Base64 decode
 */
static BYTE *base64url_decode(const char *data, size_t *outLength)
{
   if (data == nullptr || outLength == nullptr)
      return nullptr;

   size_t inputLen = strlen(data);
   size_t paddedLen = inputLen;
   
   // Calculate padding needed
   size_t padding = (4 - (inputLen % 4)) % 4;
   paddedLen += padding;
   
   char *padded = static_cast<char*>(MemAlloc(paddedLen + 1));
   strcpy(padded, data);
   
   // Convert URL-safe to standard Base64
   for (size_t i = 0; i < inputLen; i++)
   {
      if (padded[i] == '-')
         padded[i] = '+';
      else if (padded[i] == '_')
         padded[i] = '/';
   }
   
   // Add padding
   for (size_t i = 0; i < padding; i++)
      padded[inputLen + i] = '=';
   padded[paddedLen] = '\0';
   
   char *decoded = nullptr;
   bool success = base64_decode_alloc(padded, paddedLen, &decoded, outLength);
   MemFree(padded);
   
   if (!success || decoded == nullptr)
   {
      *outLength = 0;
      return nullptr;
   }
   
   return reinterpret_cast<BYTE*>(decoded);
}

/**
 * JWT Token class constructor
 */
JwtToken::JwtToken()
{
   m_header = nullptr;
   m_payload = nullptr;
   m_signature = nullptr;
   m_valid = false;
}

/**
 * JWT Token class destructor
 */
JwtToken::~JwtToken()
{
   if (m_header != nullptr)
      json_decref(m_header);
   if (m_payload != nullptr)
      json_decref(m_payload);
   MemFree(m_signature);
}

/**
 * Create JWT token from claims
 */
JwtToken *JwtToken::create(json_t *payload, const char *secret, time_t expiration)
{
   if (payload == nullptr || secret == nullptr)
      return nullptr;
   
   // Validate secret strength - minimum 32 bytes (256 bits) for HMAC-SHA256
   size_t secretLen = strlen(secret);
   if (secretLen < 32)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("JWT secret too short (%d bytes, minimum 32 required for security)"), static_cast<int>(secretLen));
      return nullptr;
   }

   JwtToken *token = new JwtToken();
   
   // Create header
   token->m_header = json_object();
   json_object_set_new(token->m_header, "alg", json_string("HS256"));
   json_object_set_new(token->m_header, "typ", json_string("JWT"));
   
   // Set payload
   token->m_payload = payload;
   json_incref(token->m_payload);
   
   // Add expiration time if specified
   if (expiration > 0)
   {
      json_object_set_new(token->m_payload, "exp", json_integer(expiration));
   }
   
   // Add issued at time
   json_object_set_new(token->m_payload, "iat", json_integer(time(nullptr)));
   
   token->m_valid = true;
   return token;
}

/**
 * Parse JWT token from string
 */
JwtToken *JwtToken::parse(const char *tokenString, const char *secret)
{
   if (tokenString == nullptr || secret == nullptr)
      return nullptr;
   
   // Validate secret strength - minimum 32 bytes (256 bits) for HMAC-SHA256
   size_t secretLen = strlen(secret);
   if (secretLen < 32)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("JWT secret too short (%d bytes, minimum 32 required for security)"), static_cast<int>(secretLen));
      return nullptr;
   }

   // Split token into parts
   TCHAR *tokenStr = TStringFromUTF8String(tokenString);
   StringList parts;
   parts.splitAndAdd(tokenStr, _T("."));
   MemFree(tokenStr);
   if (parts.size() != 3)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Invalid JWT format: expected 3 parts, got %d"), parts.size());
      return nullptr;
   }

   JwtToken *token = new JwtToken();
   
   // Decode header
   size_t headerLen;
   char *headerPart = UTF8StringFromTString(parts.get(0));
   BYTE *headerData = base64url_decode(headerPart, &headerLen);
   MemFree(headerPart);
   if (headerData == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to decode JWT header"));
      delete token;
      return nullptr;
   }
   
   json_error_t error;
   token->m_header = json_loadb(reinterpret_cast<char*>(headerData), headerLen, 0, &error);
   MemFree(headerData);
   
   if (token->m_header == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to parse JWT header JSON: %hs"), error.text);
      delete token;
      return nullptr;
   }
   
   // Decode payload
   size_t payloadLen;
   char *payloadPart = UTF8StringFromTString(parts.get(1));
   BYTE *payloadData = base64url_decode(payloadPart, &payloadLen);
   MemFree(payloadPart);
   if (payloadData == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to decode JWT payload"));
      delete token;
      return nullptr;
   }
   
   token->m_payload = json_loadb(reinterpret_cast<char*>(payloadData), payloadLen, 0, &error);
   MemFree(payloadData);
   
   if (token->m_payload == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to parse JWT payload JSON: %hs"), error.text);
      delete token;
      return nullptr;
   }
   
   // Decode signature
   size_t signatureLen;
   char *signaturePart = UTF8StringFromTString(parts.get(2));
   token->m_signature = base64url_decode(signaturePart, &signatureLen);
   MemFree(signaturePart);
   if (token->m_signature == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to decode JWT signature"));
      delete token;
      return nullptr;
   }
   token->m_signatureLen = signatureLen;
   
   // Verify signature
   char *headerUtf8 = UTF8StringFromTString(parts.get(0));
   char *payloadUtf8 = UTF8StringFromTString(parts.get(1));
   bool signatureValid = token->verifySignature(secret, headerUtf8, payloadUtf8);
   MemFree(headerUtf8);
   MemFree(payloadUtf8);
   if (!signatureValid)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("JWT signature verification failed"));
      delete token;
      return nullptr;
   }
   
   // Check expiration
   json_t *exp = json_object_get(token->m_payload, "exp");
   if (exp != nullptr && json_is_integer(exp))
   {
      time_t expiration = static_cast<time_t>(json_integer_value(exp));
      if (time(nullptr) >= expiration)
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("JWT token has expired"));
         delete token;
         return nullptr;
      }
   }
   
   token->m_valid = true;
   return token;
}

/**
 * Verify JWT signature
 */
bool JwtToken::verifySignature(const char *secret, const char *header, const char *payload)
{
   if (secret == nullptr || header == nullptr || payload == nullptr || m_signature == nullptr)
      return false;

   // Create signing input: header.payload (as UTF-8 char*)
   size_t headerLen = strlen(header);
   size_t payloadLen = strlen(payload);
   size_t signingInputLen = headerLen + 1 + payloadLen; // header + "." + payload
   char *signingInput = static_cast<char*>(MemAlloc(signingInputLen + 1));
   strcpy(signingInput, header);
   strcat(signingInput, ".");
   strcat(signingInput, payload);
   
   // Calculate HMAC-SHA256
   BYTE calculatedSignature[SHA256_DIGEST_SIZE];
   SignMessage(signingInput, signingInputLen, 
               reinterpret_cast<const BYTE*>(secret), strlen(secret), 
               calculatedSignature);
   MemFree(signingInput);
   
   // Compare signatures (constant time to prevent timing attacks)
   if (m_signatureLen != SHA256_DIGEST_SIZE)
      return false;
   
   int result = 0;
   for (size_t i = 0; i < SHA256_DIGEST_SIZE; i++)
   {
      result |= (calculatedSignature[i] ^ m_signature[i]);
   }
   
   return result == 0;
}

/**
 * Encode JWT token to string
 */
String JwtToken::encode(const char *secret)
{
   if (!m_valid || secret == nullptr || m_header == nullptr || m_payload == nullptr)
      return String();

   // Encode header
   char *headerJson = json_dumps(m_header, JSON_COMPACT);
   if (headerJson == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to serialize JWT header"));
      return String();
   }
   
   char *encodedHeader = base64url_encode(reinterpret_cast<BYTE*>(headerJson), strlen(headerJson));
   free(headerJson);
   if (encodedHeader == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to encode JWT header"));
      return String();
   }
   
   // Encode payload
   char *payloadJson = json_dumps(m_payload, JSON_COMPACT);
   if (payloadJson == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to serialize JWT payload"));
      MemFree(encodedHeader);
      return String();
   }
   
   char *encodedPayload = base64url_encode(reinterpret_cast<BYTE*>(payloadJson), strlen(payloadJson));
   free(payloadJson);
   if (encodedPayload == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to encode JWT payload"));
      MemFree(encodedHeader);
      return String();
   }
   
   // Create signing input as UTF-8 char* (not TCHAR)
   size_t headerLen = strlen(encodedHeader);
   size_t payloadLen = strlen(encodedPayload);
   size_t signingInputLen = headerLen + 1 + payloadLen; // header + "." + payload
   char *signingInput = static_cast<char*>(MemAlloc(signingInputLen + 1));
   strcpy(signingInput, encodedHeader);
   strcat(signingInput, ".");
   strcat(signingInput, encodedPayload);
   
   // Sign
   BYTE signature[SHA256_DIGEST_SIZE];
   SignMessage(signingInput, signingInputLen,
               reinterpret_cast<const BYTE*>(secret), strlen(secret),
               signature);
   MemFree(signingInput);
   
   char *encodedSignature = base64url_encode(signature, SHA256_DIGEST_SIZE);
   if (encodedSignature == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Failed to encode JWT signature"));
      MemFree(encodedHeader);
      MemFree(encodedPayload);
      return String();
   }
   
   // Create final token
   String token = String(encodedHeader, "UTF-8") + _T(".") + String(encodedPayload, "UTF-8") + _T(".") + String(encodedSignature, "UTF-8");
   
   MemFree(encodedHeader);
   MemFree(encodedPayload);
   MemFree(encodedSignature);
   
   return token;
}

/**
 * Get claim value as string
 */
String JwtToken::getClaim(const char *name)
{
   if (!m_valid || name == nullptr || m_payload == nullptr)
      return String();

   json_t *value = json_object_get(m_payload, name);
   if (value == nullptr)
      return String();
   
   if (json_is_string(value))
   {
      TCHAR *str = TStringFromUTF8String(json_string_value(value));
      String result(str);
      MemFree(str);
      return result;
   }
   else if (json_is_integer(value))
   {
      return String::toString(static_cast<int64_t>(json_integer_value(value)));
   }
   else if (json_is_real(value))
   {
      return String::toString(json_real_value(value));
   }
   else if (json_is_boolean(value))
   {
      return String(json_boolean_value(value) ? _T("true") : _T("false"));
   }
   
   return String();
}

/**
 * Get claim value as integer
 */
int64_t JwtToken::getClaimAsInt(const char *name, int64_t defaultValue)
{
   if (!m_valid || name == nullptr || m_payload == nullptr)
      return defaultValue;

   json_t *value = json_object_get(m_payload, name);
   if (value == nullptr)
      return defaultValue;
   
   if (json_is_integer(value))
   {
      return json_integer_value(value);
   }
   else if (json_is_string(value))
   {
      return strtoll(json_string_value(value), nullptr, 10);
   }
   
   return defaultValue;
}

/**
 * Check if token is expired
 */
bool JwtToken::isExpired()
{
   if (!m_valid || m_payload == nullptr)
      return true;

   json_t *exp = json_object_get(m_payload, "exp");
   if (exp == nullptr || !json_is_integer(exp))
      return false; // No expiration set
   
   time_t expiration = static_cast<time_t>(json_integer_value(exp));
   return time(nullptr) >= expiration;
}

/**
 * Get expiration time
 */
time_t JwtToken::getExpiration()
{
   if (!m_valid || m_payload == nullptr)
      return 0;

   json_t *exp = json_object_get(m_payload, "exp");
   if (exp == nullptr || !json_is_integer(exp))
      return 0;
   
   return static_cast<time_t>(json_integer_value(exp));
}

/**
 * Get JTI (JWT ID) for blocklist tracking
 */
String JwtToken::getJti()
{
   return getClaim("jti");
}

/**
 * Set JTI (JWT ID) for blocklist tracking
 */
void JwtToken::setJti(const char *jti)
{
   if (!m_valid || jti == nullptr || m_payload == nullptr)
      return;

   json_object_set_new(m_payload, "jti", json_string(jti));
}

/**
 * JWT Token Blocklist Manager singleton instance
 */
static JwtTokenBlocklist *s_tokenBlocklist = nullptr;

/**
 * JWT Token Blocklist constructor
 */
JwtTokenBlocklist::JwtTokenBlocklist() : m_mutex(MutexType::FAST)
{
   m_lastCleanup = time(nullptr);
   m_cleanupThread = ThreadCreateEx(cleanupThread, static_cast<void*>(this));
}

/**
 * JWT Token Blocklist destructor
 */
JwtTokenBlocklist::~JwtTokenBlocklist()
{
   ThreadJoin(m_cleanupThread);
}

/**
 * Get blocklist singleton instance
 */
JwtTokenBlocklist *JwtTokenBlocklist::getInstance()
{
   if (s_tokenBlocklist == nullptr)
   {
      s_tokenBlocklist = new JwtTokenBlocklist();
   }
   return s_tokenBlocklist;
}

/**
 * Add token to blocklist
 */
void JwtTokenBlocklist::addToken(const char *jti, time_t expiration)
{
   if (jti == nullptr)
      return;

   m_mutex.lock();
   
   // Store JTI with expiration time (format: "jti:expiration")
   TCHAR jtiBuffer[256];
   utf8_to_tchar(jti, -1, jtiBuffer, 256);
   StringBuffer entry;
   entry.append(jtiBuffer);
   entry.append(_T(":"));
   entry.append(static_cast<uint64_t>(expiration));
   
   m_blockedTokens.add(entry);
   
   m_mutex.unlock();
   
   nxlog_debug_tag(DEBUG_TAG, 7, _T("Added token %hs to blocklist (expires at %u)"), jti, static_cast<uint32_t>(expiration));
}

/**
 * Check if token is blocked
 */
bool JwtTokenBlocklist::isBlocked(const char *jti)
{
   if (jti == nullptr)
      return false;

   m_mutex.lock();
   
   bool blocked = false;
   TCHAR jtiBuffer[256];
   utf8_to_tchar(jti, -1, jtiBuffer, 256);
   auto it = m_blockedTokens.begin();
   while(it.hasNext())
   {
      const TCHAR *entry = static_cast<const TCHAR*>(it.next());
      const TCHAR *sep = _tcschr(entry, ':');
      if (sep != nullptr)
      {
         String entryJti = String(entry, static_cast<int>(sep - entry));
         if (entryJti.equals(jtiBuffer))
         {
            blocked = true;
            break;
         }
      }
   }
   
   m_mutex.unlock();
   
   if (blocked)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Token %hs is blocked"), jti);
   }
   
   return blocked;
}

/**
 * Remove expired tokens from blocklist
 */
void JwtTokenBlocklist::removeExpiredTokens()
{
   time_t now = time(nullptr);
   
   m_mutex.lock();
   
   StringSet newBlocklist;
   auto it = m_blockedTokens.begin();
   while(it.hasNext())
   {
      const TCHAR *entry = static_cast<const TCHAR*>(it.next());
      const TCHAR *sep = _tcschr(entry, ':');
      if (sep != nullptr)
      {
         time_t expiration = static_cast<time_t>(_tcstoull(sep + 1, nullptr, 10));
         if (now < expiration)
         {
            newBlocklist.add(entry);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("Removed expired blocked token: %s"), entry);
         }
      }
   }
   
   m_blockedTokens = newBlocklist;
   m_lastCleanup = now;
   
   m_mutex.unlock();
   
   nxlog_debug_tag(DEBUG_TAG, 6, _T("Cleanup completed: %d tokens remaining in blocklist"), m_blockedTokens.size());
}

/**
 * Cleanup thread - removes expired tokens every 5 minutes
 */
void JwtTokenBlocklist::cleanupThread(void *arg)
{
   JwtTokenBlocklist *blocklist = static_cast<JwtTokenBlocklist*>(arg);
   
   nxlog_debug_tag(DEBUG_TAG, 1, _T("JWT token blocklist cleanup thread started"));
   
   while (!SleepAndCheckForShutdown(300000)) // 5 minutes
   {
      blocklist->cleanup();
   }
   
   nxlog_debug_tag(DEBUG_TAG, 1, _T("JWT token blocklist cleanup thread stopped"));
}

/**
 * Internal cleanup method
 */
void JwtTokenBlocklist::cleanup()
{
   time_t now = time(nullptr);
   
   // Only cleanup if it's been more than 5 minutes since last cleanup
   if ((now - m_lastCleanup) >= 300)
   {
      removeExpiredTokens();
   }
}