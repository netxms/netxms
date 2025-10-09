/*
** NetXMS multiplatform core agent
** Copyright (C) 2020-2025 Raden Solutions
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
** File: websvc.cpp
**
**/

#include "nxagentd.h"
#include <netxms-version.h>
#include <nxsde.h>

#define DEBUG_TAG _T("websvc")

#include <nxlibcurl.h>

/**
 * Global cache expiration time
 */
extern uint32_t g_webSvcCacheExpirationTime;

/**
 * HTTP request methods
 */
static const char *s_httpRequestMethods[] = { "GET", "POST", "PUT", "DELETE", "PATCH" };

/**
 * Executed web service request
 */
class WebServiceRequest : public StructuredDataExtractor
{
private:
   uint32_t m_responseCode;
   Mutex m_mutex;

public:
   WebServiceRequest();

   uint32_t getResponseCode() const { return m_responseCode; }
   uint32_t query(const TCHAR *url, uint16_t requestMethod, const char *requestData, const char *userName, const char *password,
         WebServiceAuthType authType, struct curl_slist *headers, bool verifyPeer, bool verifyHost, bool followLocation, bool forcePlainTextParser, uint32_t requestTimeout);

   uint32_t getMetricSet(StringList *params, NXCPMessage *response);
   uint32_t getList(const TCHAR *path, NXCPMessage *response);

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }
};

/**
 * Static data
 */
Mutex s_serviceRequestCacheLock;
SharedStringObjectMap<WebServiceRequest> s_serviceRequestCache;

/**
 * Constructor
 */
WebServiceRequest::WebServiceRequest() : StructuredDataExtractor(DEBUG_TAG)
{
   m_responseCode = 0;
}

/**
 * Get curl auth type from NetXMS internal code
 */
static long CurlAuthType(WebServiceAuthType authType)
{
   switch(authType)
   {
      case WebServiceAuthType::ANYSAFE:
         return CURLAUTH_ANYSAFE;
      case WebServiceAuthType::BASIC:
         return CURLAUTH_BASIC;
      case WebServiceAuthType::DIGEST:
         return CURLAUTH_DIGEST;
      case WebServiceAuthType::NTLM:
         return CURLAUTH_NTLM;
      default:
         return CURLAUTH_ANY;
   }
}

/**
 * Get parameters from cached data
 */
uint32_t WebServiceRequest::getMetricSet(StringList *params, NXCPMessage *response)
{
   uint32_t result = ERR_SUCCESS;
   switch(m_type)
   {
      case DocumentType::XML:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataParser::getParams(%s): get parameter from XML"), m_source);
         getMetricsFromXML(params, response);
         break;
      case DocumentType::JSON:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataParser::getParams(%s): get parameter from JSON"), m_source);
         getMetricsFromJSON(params, response);
#ifndef HAVE_LIBJQ
         result = ERR_FUNCTION_NOT_SUPPORTED;
#endif
         break;
      case DocumentType::TEXT:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataParser::getParams(%s): get parameter from Text"), m_source);
         getMetricsFromText(params, response);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 4, _T("StructuredDataParser::getParams(%s): no data available"), m_source);
         break;
   }
   return result;
}

/**
 * Get list from cached data
 */
uint32_t WebServiceRequest::getList(const TCHAR *path, NXCPMessage *response)
{
   StringList list;
   uint32_t rc = StructuredDataExtractor::getList(path, &list);
   switch (rc)
   {
      case SYSINFO_RC_UNSUPPORTED:
         rc = ERR_NOT_IMPLEMENTED;
         break;
      case SYSINFO_RC_SUCCESS:
      default:
         rc = ERR_SUCCESS;
         break;
   }

   list.fillMessage(response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
   return rc;
}

/**
 * Query web service
 */
uint32_t WebServiceRequest::query(const TCHAR *url, uint16_t requestMethod, const char *requestData, const char *userName, const char *password,
      WebServiceAuthType authType, struct curl_slist *headers, bool verifyPeer, bool verifyHost, bool followLocation, bool forcePlainTextParser, uint32_t requestTimeout)
{
   uint32_t rcc = ERR_SUCCESS;
   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
      char errbuf[CURL_ERROR_SIZE];
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
#if HAVE_DECL_CURLOPT_PROTOCOLS_STR
      curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#else
      curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#endif
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_HEADER, static_cast<long>(0));
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>((requestTimeout != 0) ? requestTimeout : 10));
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ByteStream::curlWriteFunction);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Agent/" NETXMS_VERSION_STRING_A);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifyPeer ? 1 : 0);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifyHost ? 2 : 0);
      EnableLibCURLUnexpectedEOFWorkaround(curl);

      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, s_httpRequestMethods[requestMethod]);
      if (requestData != nullptr)
      {
         curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(requestData));
         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData);
      }
      else if (requestMethod == static_cast<uint16_t>(HttpRequestMethod::_POST))
      {
         curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0);
         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
      }

      if (authType == WebServiceAuthType::NONE)
      {
         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_NONE);
      }
      else if (authType == WebServiceAuthType::BEARER)
      {
#if HAVE_DECL_CURLOPT_XOAUTH2_BEARER
         curl_easy_setopt(curl, CURLOPT_USERNAME, userName);
         curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, password);
#else
         curl_easy_cleanup(curl);
         return ERR_NOT_IMPLEMENTED;
#endif
      }
      else
      {
         curl_easy_setopt(curl, CURLOPT_USERNAME, userName);
         curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CurlAuthType(authType));
      }

      // Receiving buffer
      ByteStream data(32768);
      data.setAllocationStep(32768);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

#ifdef UNICODE
      char *urlUtf8 = UTF8StringFromWideString(url);
#else
      char *urlUtf8 = UTF8StringFromMBString(url);
#endif

      int redirectLimit = 10;

retry:
      if (curl_easy_setopt(curl, CURLOPT_URL, urlUtf8) == CURLE_OK)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("WebServiceRequest::query(%s): requesting URL %hs"), url, urlUtf8);
         if (requestData != nullptr)
            nxlog_debug_tag(DEBUG_TAG, 7, _T("WebServiceRequest::query(%s): request data: %hs"), url, requestData);

         CURLcode rc = curl_easy_perform(curl);
         if (rc == CURLE_OK)
         {
            deleteContent();
            long responseCode;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
            m_responseCode = static_cast<uint32_t>(responseCode);
            if (data.size() > 0 && (m_responseCode >= 200 && m_responseCode <= 299))
            {
               data.write('\0');

               size_t size;
               const char *text = reinterpret_cast<const char*>(data.buffer(&size));
               size--;  // Because of added zero byte
               while((size > 0) && isspace(*text))
               {
                  text++;
                  size--;
               }
               nxlog_debug_tag(DEBUG_TAG, 7, _T("WebServiceRequest::query(%s): response data: %hs"), url, text);
               updateContent(text, size, forcePlainTextParser, url);
            }
            else if ((m_responseCode >= 300) && (m_responseCode <= 399))
            {
               char *redirectURL = nullptr;
               curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirectURL);
               if (redirectURL != nullptr)
               {
                  if (followLocation)
                  {
                     if (redirectLimit-- > 0)
                     {
                        nxlog_debug_tag(DEBUG_TAG, 6, _T("WebServiceRequest::query(%s): follow redirect to %hs"), url, redirectURL);
                        MemFree(urlUtf8);
                        urlUtf8 = MemCopyStringA(redirectURL);
                        data.clear();
                        goto retry;
                     }
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("WebServiceRequest::query(%s): HTTP redirect to %hs, do not follow (redirect limit reached)"), url, redirectURL);
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("WebServiceRequest::query(%s): HTTP redirect to %hs, do not follow (forbidden)"), url, redirectURL);
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceRequest::query(%s): HTTP response %03d but redirect URL not set"), url, m_responseCode);
               }
               rcc = ERR_MALFORMED_RESPONSE;
            }
            else
            {
               if (m_responseCode < 200 || m_responseCode > 299)
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceRequest::query(%s): HTTP response %03d"), url, m_responseCode);
               else if (data.size() == 0)
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceRequest::query(%s): empty response"), url);
               m_type = DocumentType::NONE;
               rcc = ERR_MALFORMED_RESPONSE;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceRequest::query(%s): call to curl_easy_perform failed (%d: %hs)"), url, rc, errbuf);
            rcc = ERR_MALFORMED_RESPONSE;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceRequest::query(%s): curl_easy_setopt failed for CURLOPT_URL"), url);
         rcc = ERR_UNSUPPORTED_METRIC;
      }
      MemFree(urlUtf8);
      curl_easy_cleanup(curl);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceRequest::query(%s): curl_init failed"), url);
      rcc = ERR_INTERNAL_ERROR;
   }
   return rcc;
}

/**
 * Query web service
 */
void QueryWebService(NXCPMessage* request, shared_ptr<AbstractCommSession> session)
{
   uint16_t requestMethodCode = request->getFieldAsInt16(VID_HTTP_REQUEST_METHOD);
   if (requestMethodCode > static_cast<uint16_t>(HttpRequestMethod::_MAX_TYPE))
   {
      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
      response.setField(VID_RCC, ERR_INVALID_HTTP_REQUEST_CODE);
      session->sendMessage(&response);
      delete request;
      return;
   }

   TCHAR *url = request->getFieldAsString(VID_URL);
   bool forcePlaintext = request->getFieldAsBoolean(VID_FORCE_PLAIN_TEXT_PARSER);
   StringBuffer cacheKey;
   if (forcePlaintext)
   {
      cacheKey = _T("forcePlaintext:");
   }
   cacheKey.append(url);

   s_serviceRequestCacheLock.lock();
   shared_ptr<WebServiceRequest> serviceRequest;
   if (requestMethodCode == (uint16_t)HttpRequestMethod::_GET)
   {
      serviceRequest = s_serviceRequestCache.getShared(cacheKey);
      if (serviceRequest == nullptr)
      {
         serviceRequest = make_shared<WebServiceRequest>();
         s_serviceRequestCache.set(cacheKey, serviceRequest);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("QueryWebService(): Create new cache entry for URL %s"), cacheKey.cstr());
      }
   }
   else
   {
      // Request methods other than GET are never cached, so we don't put it into s_serviceRequestCache.
      // Also such requests invalidate the cache for that URL per https://www.rfc-editor.org/rfc/rfc7234#section-4.4 .
      // Cache should be invalidated upon successful response, but unconditional early invalidation will still behave correctly.
      s_serviceRequestCache.remove(cacheKey);
      serviceRequest = make_shared<WebServiceRequest>();
   }
   s_serviceRequestCacheLock.unlock();

   serviceRequest->lock();
   uint32_t retentionTime = request->getFieldAsUInt32(VID_RETENTION_TIME);
   uint32_t result = ERR_SUCCESS;
   if (serviceRequest->isDataExpired(retentionTime))
   {
      char *login = request->getFieldAsUtf8String(VID_LOGIN_NAME);
      char *password = request->getFieldAsUtf8String(VID_PASSWORD);
      bool verifyHost = request->isFieldExist(VID_VERIFY_HOST) ? request->getFieldAsBoolean(VID_VERIFY_HOST) : true;
      WebServiceAuthType authType = WebServiceAuthTypeFromInt(request->getFieldAsInt16(VID_AUTH_TYPE));
      char *requestData = request->getFieldAsUtf8String(VID_REQUEST_DATA);

      struct curl_slist *headers = nullptr;
      uint32_t headerCount = request->getFieldAsUInt32(VID_NUM_HEADERS);
      uint32_t fieldId = VID_HEADERS_BASE;
      char header[2048];
      for(uint32_t i = 0; i < headerCount; i++)
      {
         request->getFieldAsUtf8String(fieldId++, header, sizeof(header) - 4);   // header name
         size_t len = strlen(header);
         header[len++] = ':';
         header[len++] = ' ';
         request->getFieldAsUtf8String(fieldId++, &header[len], sizeof(header) - len); // value
         headers = curl_slist_append(headers, header);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("QueryWebService(): request header: %hs"), header);
      }

      result = serviceRequest->query(url, requestMethodCode, requestData, login, password, authType, headers,
               request->getFieldAsBoolean(VID_VERIFY_CERT), verifyHost,
               request->getFieldAsBoolean(VID_FOLLOW_LOCATION),
               forcePlaintext,
               request->getFieldAsUInt32(VID_TIMEOUT));

      curl_slist_free_all(headers);
      MemFree(login);
      MemFree(password);
      MemFree(requestData);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("QueryWebService(): Cache for URL \"%s\" updated"), url);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("QueryWebService(): using cached result for \"%s\""), url);
   }
   MemFree(url);

   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   if (result == ERR_SUCCESS)
   {
      WebServiceRequestType requestType = static_cast<WebServiceRequestType>(request->getFieldAsUInt16(VID_REQUEST_TYPE));
      switch (requestType)
      {
         case WebServiceRequestType::PARAMETER:
         {
            StringList params(*request, VID_PARAM_LIST_BASE, VID_NUM_PARAMETERS);
            result = serviceRequest->getMetricSet(&params, &response);
            break;
         }
         case WebServiceRequestType::LIST:
         {
            TCHAR path[1024];
            request->getFieldAsString(VID_PARAM_LIST_BASE, path, 1024);
            result = serviceRequest->getList(path, &response);
            break;
         }
      }
   }
   serviceRequest->unlock();

   response.setField(VID_RCC, result);
   session->sendMessage(&response);
   delete request;
}

/**
 * Web service custom request command executor
 */
void WebServiceCustomRequest(NXCPMessage* request, shared_ptr<AbstractCommSession> session)
{
   uint16_t requestMethodCode = request->getFieldAsInt16(VID_HTTP_REQUEST_METHOD);
   if (requestMethodCode > static_cast<uint16_t>(HttpRequestMethod::_MAX_TYPE))
   {
      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
      response.setField(VID_RCC, ERR_INVALID_HTTP_REQUEST_CODE);
      session->sendMessage(&response);
      delete request;
      return;
   }

   TCHAR *url = request->getFieldAsString(VID_URL);
   bool forcePlaintext = request->getFieldAsBoolean(VID_FORCE_PLAIN_TEXT_PARSER);
   StringBuffer cacheKey;
   if (forcePlaintext)
   {
      cacheKey = _T("forcePlaintext:");
   }
   cacheKey.append(url);

   s_serviceRequestCacheLock.lock();
   shared_ptr<WebServiceRequest> serviceRequest;
   if (requestMethodCode == (uint16_t)HttpRequestMethod::_GET)
   {
      serviceRequest = s_serviceRequestCache.getShared(cacheKey);
      if (serviceRequest == nullptr)
      {
         serviceRequest = make_shared<WebServiceRequest>();
         s_serviceRequestCache.set(cacheKey, serviceRequest);
         nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceCustomRequest(): Create new cache entry for URL %s"), cacheKey.cstr());
      }
   }
   else
   {
      // Request methods other than GET are never cached, so we don't put it into s_serviceRequestCache.
      // Also such requests invalidate the cache for that URL per https://www.rfc-editor.org/rfc/rfc7234#section-4.4 .
      // Cache should be invalidated upon successful response, but unconditional early invalidation will still behave correctly.
      s_serviceRequestCache.remove(cacheKey);
      serviceRequest = make_shared<WebServiceRequest>();
   }
   s_serviceRequestCacheLock.unlock();

   serviceRequest->lock();
   uint32_t retentionTime = request->getFieldAsUInt32(VID_RETENTION_TIME);
   uint32_t result = ERR_SUCCESS;
   if (serviceRequest->isDataExpired(retentionTime))
   {
      char *login = request->getFieldAsUtf8String(VID_LOGIN_NAME);
      char *password = request->getFieldAsUtf8String(VID_PASSWORD);
      char *requestData = request->getFieldAsUtf8String(VID_REQUEST_DATA);
      WebServiceAuthType authType = WebServiceAuthTypeFromInt(request->getFieldAsInt16(VID_AUTH_TYPE));

      struct curl_slist *headers = nullptr;
      uint32_t headerCount = request->getFieldAsUInt32(VID_NUM_HEADERS);
      uint32_t fieldId = VID_HEADERS_BASE;
      char header[2048];
      for(uint32_t i = 0; i < headerCount; i++)
      {
         request->getFieldAsUtf8String(fieldId++, header, sizeof(header) - 4);   // header name
         size_t len = strlen(header);
         header[len++] = ':';
         header[len++] = ' ';
         request->getFieldAsUtf8String(fieldId++, &header[len], sizeof(header) - len); // value
         headers = curl_slist_append(headers, header);
         nxlog_debug_tag(DEBUG_TAG, 7, _T("WebServiceCustomRequest(): request header: %hs"), header);
      }

      result = serviceRequest->query(url, requestMethodCode, requestData, login, password, authType, headers,
               request->getFieldAsBoolean(VID_VERIFY_CERT),
               request->getFieldAsBoolean(VID_VERIFY_HOST),
               request->getFieldAsBoolean(VID_FOLLOW_LOCATION),
               forcePlaintext,
               request->getFieldAsUInt32(VID_TIMEOUT));

      curl_slist_free_all(headers);
      MemFree(login);
      MemFree(password);
      MemFree(requestData);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceCustomRequest(): Cache for URL \"%s\" updated (was expired, retention time %u)"), url, retentionTime);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceCustomRequest(): Cache for URL \"%s\" was used (retention time %u)"), url, retentionTime);
   }

   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   response.setFieldFromUtf8String(VID_WEBSVC_RESPONSE, serviceRequest->getResponseData());
   response.setField(VID_WEBSVC_RESPONSE_CODE, serviceRequest->getResponseCode());
   serviceRequest->unlock();

   response.setField(VID_RCC, result);

   MemFree(url);
   session->sendMessage(&response);

   delete request;
}

/**
 * Web service housekeeper
 */
static void WebServiceHousekeeper()
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("WebServiceHousekeeper(): running cache entry check"));

   s_serviceRequestCacheLock.lock();
   auto it = s_serviceRequestCache.begin();
   while(it.hasNext())
   {
      auto entry = it.next();
      shared_ptr<WebServiceRequest> svc = *entry->value;
      if (svc->isDataExpired(g_webSvcCacheExpirationTime))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceHousekeeper(): Cache entry for URL \"%s\" removed because of inactivity"), entry->key);
         it.remove();
      }
   }
   s_serviceRequestCacheLock.unlock();

   ThreadPoolScheduleRelative(g_webSvcThreadPool, (MAX(g_webSvcCacheExpirationTime, 60) / 2) * 1000, WebServiceHousekeeper);
}

/**
 * Start web service housekeeper
 */
void StartWebServiceHousekeeper()
{
   ThreadPoolScheduleRelative(g_webSvcThreadPool, (MAX(g_webSvcCacheExpirationTime, 60) / 2) * 1000, WebServiceHousekeeper);
}
