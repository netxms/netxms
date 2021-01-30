/*
** NetXMS multiplatform core agent
** Copyright (C) 2020-2021 Raden Solutions
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

#define DEBUG_TAG _T("websvc")

#if HAVE_LIBCURL

#include <curl/curl.h>
#include <netxms-regex.h>

/**
 * Global cache expiration time
 */
extern uint32_t g_webSvcCacheExpirationTime;

/**
 * Document type
 */
enum class DocumentType
{
   NONE,
   JSON,
   TEXT,
   XML
};

/**
 * One cached service entry
 */
class ServiceEntry
{
private:
   time_t m_lastRequestTime;
   MUTEX m_lock;
   DocumentType m_type;
   union
   {
      Config *xml;
      json_t *json;
      TCHAR *text;
   } m_content;

   void deleteContent()
   {
      switch(m_type)
      {
         case DocumentType::JSON:
            json_decref(m_content.json);
            break;
         case DocumentType::TEXT:
            MemFree(m_content.text);
            break;
         case DocumentType::XML:
            delete m_content.xml;
            break;
      }
   }

   void getParamsFromXML(StringList *params, NXCPMessage *response);
   void getParamsFromJSON(StringList *params, NXCPMessage *response);
   void getParamsFromText(StringList *params, NXCPMessage *response);

   void getListFromXML(const TCHAR *path, StringList *result);
   void getListFromJSON(const TCHAR *path, StringList *result);
   uint32_t getListFromText(const TCHAR *pattern, StringList *result);

public:
   ServiceEntry();
   ~ServiceEntry();

   void getParams(StringList *params, NXCPMessage *response);
   uint32_t getList(const TCHAR *path, NXCPMessage *response);
   bool isDataExpired(uint32_t retentionTime) { return (time(nullptr) - m_lastRequestTime) >= retentionTime; }
   uint32_t updateData(const TCHAR *url, const char *userName, const char *password, WebServiceAuthType authType,
            struct curl_slist *headers, bool peerVerify, bool hostVerify, bool forcePlainTextParser, uint32_t requestTimeout);

   void lock() { MutexLock(m_lock); }
   void unlock() { MutexUnlock(m_lock); }
};

/**
 * Static data
 */
Mutex s_serviceCacheLock;
SharedStringObjectMap<ServiceEntry> s_serviceCache;

/**
 * Constructor
 */
ServiceEntry::ServiceEntry()
{
   m_lastRequestTime = 0;
   m_type = DocumentType::NONE;
   m_lock = MutexCreate();
}

/**
 * Destructor
 */
ServiceEntry::~ServiceEntry()
{
   deleteContent();
   MutexDestroy(m_lock);
}

/**
 * Get parameters from XML cached data
 */
void ServiceEntry::getParamsFromXML(StringList *params, NXCPMessage *response)
{
   if (nxlog_get_debug_level_tag(DEBUG_TAG) == 9)
      nxlog_debug_tag(DEBUG_TAG, 9, _T("XML: %s"), m_content.xml->createXml().cstr());

   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromXML(): get parameter \"%s\""), params->get(i));
      const TCHAR *result = m_content.xml->getValue(params->get(i));
      if (result != nullptr)
      {
         response->setField(fieldId++, params->get(i));
         response->setField(fieldId++, result);
         resultCount++;
      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Set NXCP message field from JSON value
 */
static bool SetFieldFromJson(NXCPMessage *msg, uint32_t fieldId, json_t *json)
{
   bool skip = false;
   TCHAR result[MAX_RESULT_LENGTH];
   switch(json_typeof(json))
   {
      case JSON_STRING:
         msg->setFieldFromUtf8String(fieldId, json_string_value(json));
         break;
      case JSON_INTEGER:
         ret_int64(result, static_cast<int64_t>(json_integer_value(json)));
         msg->setField(fieldId, result);
         break;
      case JSON_REAL:
         ret_double(result, json_real_value(json));
         msg->setField(fieldId, result);
         break;
      case JSON_TRUE:
         msg->setField(fieldId, _T("true"));
         break;
      case JSON_FALSE:
         msg->setField(fieldId, _T("false"));
         break;
      default:
         skip = true;
         break;
   }
   return !skip;
}

/**
 * Get parameters from JSON cached data
 */
void ServiceEntry::getParamsFromJSON(StringList *params, NXCPMessage *response)
{
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromJSON(): get parameter \"%s\""), params->get(i));
      json_t *object = m_content.json;
#ifdef UNICODE
      char *copy = UTF8StringFromWideString(params->get(i));
#else
      char *copy = UTF8StringFromMBString(params->get(i));
#endif
      char *item = copy;
      char *separator = nullptr;
      if (*item == '/')
         item++;
      do
      {
         separator = strchr(item, '/');
         if (separator != nullptr)
            *separator = 0;

         object = json_object_get(object, item);
         if (separator != nullptr)
            item = separator + 1;
      } while (separator != nullptr && *item != 0 && object != nullptr);
      MemFree(copy);
      if (object != nullptr)
      {
         response->setField(fieldId++, params->get(i));
         if (SetFieldFromJson(response, fieldId, object))
         {
            fieldId++;
            resultCount++;
         }
         else
         {
            fieldId--;
         }
      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Get parameters from Text cached data
 */
void ServiceEntry::getParamsFromText(StringList *params, NXCPMessage *response)
{
   StringList *dataLines = String::split(m_content.text, _tcslen(m_content.text), _T("\n"));
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      const TCHAR *pattern = params->get(i);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromText(): using pattern \"%s\""), pattern);

      const char *eptr;
      int eoffset;
      PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
      if (compiledPattern == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ServiceEntry::getListFromText(): \"%s\" pattern compilation failure: %hs at offset %d"), pattern, eptr, eoffset);
         continue;
      }

      TCHAR *matchedString = nullptr;
      for (int j = 0; j < dataLines->size(); j++)
      {
         const TCHAR *dataLine = dataLines->get(j);
         nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromText(): checking data line \"%s\""), dataLine);
         int fields[30];
         int result = _pcre_exec_t(compiledPattern, nullptr, reinterpret_cast<const PCRE_TCHAR*>(dataLine), static_cast<int>(_tcslen(dataLine)), 0, 0, fields, 30);
         if (result >= 0)
         {
            if ((result >=2 || result == 0) && fields[2] != -1)
            {
               matchedString = MemAllocString(fields[3] + 1 - fields[2]);
               memcpy(matchedString, &dataLine[fields[2]], (fields[3] - fields[2]) * sizeof(TCHAR));
               matchedString[fields[3] - fields[2]] = 0;
               nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromText(): data match: \"%s\""), matchedString);
            }
            break;
         }
      }

      _pcre_free_t(compiledPattern);

      if (matchedString != nullptr)
      {
         response->setField(fieldId++, pattern);
         response->setField(fieldId++, matchedString);
         resultCount++;
         MemFree(matchedString);
      }
   }

   response->setField(VID_NUM_PARAMETERS, resultCount);
   delete dataLines;
}

/**
 * Get parameters from cached data
 */
void ServiceEntry::getParams(StringList *params, NXCPMessage *response)
{
   switch(m_type)
   {
      case DocumentType::XML:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParams(): get parameter from XML"));
         getParamsFromXML(params, response);
         break;
      case DocumentType::JSON:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParams(): get parameter from JSON"));
         getParamsFromJSON(params, response);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParams(): get parameter from Text"));
         getParamsFromText(params, response);
         break;
   }
}

/**
 * Get list from XML cached data
 */
void ServiceEntry::getListFromXML(const TCHAR *path, StringList *result)
{
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 9)
      nxlog_debug_tag(DEBUG_TAG, 9, _T("XML: %s"), m_content.xml->createXml().cstr());
   nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromXML(): Get child tag list for path \"%s\""), path);
   ConfigEntry *entry = m_content.xml->getEntry(path);
   ObjectArray<ConfigEntry> *elements = entry != nullptr ? entry->getSubEntries(_T("*")) : nullptr;
   if (elements != nullptr)
   {
      for(int i = 0; i < elements->size(); i++)
      {
         result->add(elements->get(i)->getName());
      }
      delete elements;
   }
}

/**
 * Get list from JSON cached data
 */
void ServiceEntry::getListFromJSON(const TCHAR *path, StringList *result)
{
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromJSON(): Get child object list for JSON path \"%s\""), path);

#ifdef UNICODE
   char *copy = UTF8StringFromWideString(path);
#else
   char *copy = UTF8StringFromMBString(path);
#endif
   char *item = copy;
   if (*item == '/')
      item++;

   char *separator = strchr(item, '/');
   if (separator != nullptr)
      *separator = 0;
   json_t *object = m_content.json;
   while (*item != 0)
   {
      object = json_object_get(object, item);
      if ((separator == nullptr) || (object == nullptr))
         break;

      item = separator + 1;
      separator = strchr(item, '/');
      if (separator != nullptr)
         *separator = 0;
   }
   MemFree(copy);

   if (object != nullptr)
   {
      void *it = json_object_iter(object);
      while(it != nullptr)
      {
         result->addUTF8String(json_object_iter_key(it));
         it = json_object_iter_next(object, it);
      }
   }
}

/**
 * Get list from Text cached data
 */
uint32_t ServiceEntry::getListFromText(const TCHAR *pattern, StringList *resultList)
{
   uint32_t retVal = ERR_SUCCESS;
   StringList *dataLines = String::split(m_content.text, _tcslen(m_content.text), _T("\n"));
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromText(): get list of matched lines for pattern \"%s\""), pattern);

   const char *eptr;
   int eoffset;
   PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (compiledPattern != nullptr)
   {
      TCHAR *matchedString = nullptr;
      for (int j = 0; j < dataLines->size(); j++)
      {
         const TCHAR *dataLine = dataLines->get(j);
         nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromText(): checking data line \"%s\""), dataLine);
         int fields[30];
         int result = _pcre_exec_t(compiledPattern, nullptr, reinterpret_cast<const PCRE_TCHAR*>(dataLine), static_cast<int>(_tcslen(dataLine)), 0, 0, fields, 30);
         if (result >= 0)
         {
            if ((result >=2 || result == 0) && fields[2] != -1)
            {
               matchedString = MemAllocString(fields[3] + 1 - fields[2]);
               memcpy(matchedString, &dataLine[fields[2]], (fields[3] - fields[2]) * sizeof(TCHAR));
               matchedString[fields[3] - fields[2]] = 0;
               nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromText(): data match: \"%s\""), matchedString);
               resultList->add(matchedString);
            }
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("ServiceEntry::getListFromText(): \"%s\" pattern compilation failure: %hs at offset %d"), pattern, eptr, eoffset);
      retVal = ERR_MALFORMED_COMMAND;
   }

   _pcre_free_t(compiledPattern);

   delete dataLines;
   return retVal;
}

/**
 * Get list from cached data
 * If path is empty: "/" will be used for XML and JSOn types and "(*)" will be used for text type
 */
uint32_t ServiceEntry::getList(const TCHAR *path, NXCPMessage *response)
{
   uint32_t result = ERR_SUCCESS;
   StringList list;
   const TCHAR *correctPath = (path[0] != 0) ? path : ((m_type == DocumentType::TEXT) ? _T("(.*)") : _T("/"));
   switch(m_type)
   {
      case DocumentType::XML:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getList(): get list from XML"));
         getListFromXML(correctPath, &list);
         break;
      case DocumentType::JSON:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getList(): get list from JSON"));
         getListFromJSON(correctPath, &list);
         break;
      case DocumentType::TEXT:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getList(): get list from TEXT"));
         result = getListFromText(correctPath, &list);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getList(): unsupported document type"));
         result = ERR_NOT_IMPLEMENTED;
         break;
   }
   list.fillMessage(response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
   return result;
}

/**
 * Callback for processing data received from cURL
 */
static size_t OnCurlDataReceived(char *ptr, size_t size, size_t nmemb, void *context)
{
   size_t bytes = size * nmemb;
   static_cast<ByteStream*>(context)->write(ptr, bytes);
   return bytes;
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
 * Update cached data
 */
uint32_t ServiceEntry::updateData(const TCHAR *url, const char *userName, const char *password, WebServiceAuthType authType,
         struct curl_slist *headers, bool peerVerify, bool hostVerify, bool forcePlainTextParser, uint32_t requestTimeout)
{
   uint32_t rcc = ERR_SUCCESS;
   CURL *curl = curl_easy_init();
   if (curl != nullptr)
   {
      char errbuf[CURL_ERROR_SIZE];
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

      curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(curl, CURLOPT_HEADER, static_cast<long>(0));
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>((requestTimeout != 0) ? requestTimeout : 10));
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &OnCurlDataReceived);
      curl_easy_setopt(curl, CURLOPT_USERAGENT, "NetXMS Agent/" NETXMS_VERSION_STRING_A);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, peerVerify ? 1 : 0);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, hostVerify ? 2 : 0);

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
      if (curl_easy_setopt(curl, CURLOPT_URL, urlUtf8) == CURLE_OK)
      {
         if (curl_easy_perform(curl) == CURLE_OK)
         {
            deleteContent();
            if (data.size() > 0)
            {
               data.write('\0');

               size_t size;
               const char *text = reinterpret_cast<const char*>(data.buffer(&size));
               while((size > 0) && isspace(*text))
               {
                  text++;
                  size--;
               }

               if (!forcePlainTextParser && (*text == '<'))
               {
                  m_type = DocumentType::XML;
                  m_content.xml = new Config();
                  if (!m_content.xml->loadXmlConfigFromMemory(text, static_cast<int>(size), nullptr, "*", false))
                     nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): Failed to load XML"));
               }
               else if (!forcePlainTextParser && (*text == '{'))
               {
                  m_type = DocumentType::JSON;
                  json_error_t error;
                  m_content.json = json_loads(text, 0, &error);
               }
               else
               {
                  m_type = DocumentType::TEXT;
#ifdef UNICODE
                  m_content.text = WideStringFromUTF8String(text);
#else
                  m_content.text = MBStringFromUTF8String(text);
#endif
               }
               m_lastRequestTime = time(nullptr);
               nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::updateData(): response data type=%d, length=%u"), static_cast<int>(m_type), static_cast<unsigned int>(data.size()));
               if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 8)
               {
#ifdef UNICODE
                  WCHAR *responseText = WideStringFromUTF8String(reinterpret_cast<const char*>(data.buffer()));
#else
                  char *responseText = MBStringFromUTF8String(reinterpret_cast<const char*>(data.buffer()));
#endif
                  for(TCHAR *s = responseText; *s != 0; s++)
                     if (*s < ' ')
                        *s = ' ';
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::updateData(): response data: %s"), responseText);
                  MemFree(responseText);
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): request returned empty document"));
               m_type = DocumentType::NONE;
               rcc = ERR_MALFORMED_RESPONSE;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): error making curl request: %hs"), errbuf);
            rcc = ERR_MALFORMED_RESPONSE;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): curl_easy_setopt failed for CURLOPT_URL"));
         rcc = ERR_UNKNOWN_PARAMETER;
      }
      MemFree(urlUtf8);
      curl_easy_cleanup(curl);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Get data from service: curl_init failed"));
      rcc = ERR_INTERNAL_ERROR;
   }
   return rcc;
}

/**
 * Query web service
 */
void QueryWebService(NXCPMessage *request, AbstractCommSession *session)
{
   TCHAR *url = request->getFieldAsString(VID_URL);

   s_serviceCacheLock.lock();
   shared_ptr<ServiceEntry> cachedEntry = s_serviceCache.getShared(url);
   if (cachedEntry == nullptr)
   {
      cachedEntry = make_shared<ServiceEntry>();
      s_serviceCache.set(url, cachedEntry);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("QueryWebService(): Create new cache entry for URL %s"), url);
   }
   s_serviceCacheLock.unlock();

   cachedEntry->lock();
   uint32_t retentionTime = request->getFieldAsUInt32(VID_RETENTION_TIME);
   uint32_t result = ERR_SUCCESS;
   if (cachedEntry->isDataExpired(retentionTime))
   {
      char *login = request->getFieldAsUtf8String(VID_LOGIN_NAME);
      char *password = request->getFieldAsUtf8String(VID_PASSWORD);
      bool verifyHost = request->isFieldExist(VID_VERIFY_HOST) ? request->getFieldAsBoolean(VID_VERIFY_HOST) : true;
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
      }

      result = cachedEntry->updateData(url, login, password, authType, headers,
               request->getFieldAsBoolean(VID_VERIFY_CERT), verifyHost,
               request->getFieldAsBoolean(VID_FORCE_PLAIN_TEXT_PARSER),
               request->getFieldAsUInt32(VID_TIMEOUT));

      curl_slist_free_all(headers);
      MemFree(login);
      MemFree(password);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("QueryWebService(): Cache for URL \"%s\" updated"), url);
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
            StringList params(request, VID_PARAM_LIST_BASE, VID_NUM_PARAMETERS);
            cachedEntry->getParams(&params, &response);
            break;
         }
         case WebServiceRequestType::LIST:
         {
            TCHAR path[1024];
            request->getFieldAsString(VID_PARAM_LIST_BASE, path, 1024);
            result = cachedEntry->getList(path, &response);
            break;
         }
      }
   }
   cachedEntry->unlock();

   response.setField(VID_RCC, result);
   session->sendMessage(&response);
   delete request;
}

/**
 * Web service housekeeper
 */
static void WebServiceHousekeeper()
{
   nxlog_debug_tag(DEBUG_TAG, 6, _T("WebServiceHousekeeper(): running cache entry check"));

   s_serviceCacheLock.lock();
   auto it = s_serviceCache.iterator();
   while(it->hasNext())
   {
      auto entry = it->next();
      shared_ptr<ServiceEntry> svc = *entry->second;
      if (svc->isDataExpired(g_webSvcCacheExpirationTime))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceHousekeeper(): Cache entry for URL \"%s\" removed because of inactivity"), entry->first);
         it->remove();
      }
   }
   delete it;
   s_serviceCacheLock.unlock();

   ThreadPoolScheduleRelative(g_webSvcThreadPool, (MAX(g_webSvcCacheExpirationTime, 60) / 2) * 1000, WebServiceHousekeeper);
}

/**
 * Start web service housekeeper
 */
void StartWebServiceHousekeeper()
{
   ThreadPoolScheduleRelative(g_webSvcThreadPool, (MAX(g_webSvcCacheExpirationTime, 60) / 2) * 1000, WebServiceHousekeeper);
}

#else /* HAVE_LIBCURL */

/**
 * Get parameters from web service
 */
void QueryWebService(NXCPMessage *request, AbstractCommSession *session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("QueryWebService(): agent was compiled without libcurl"));
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   response.setField(VID_RCC, ERR_NOT_IMPLEMENTED);
   session->sendMessage(&response);
   delete request;
}

/**
 * Start web service housekeeper
 */
void StartWebServiceHousekeeper()
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("StartWebServiceHousekeeper(): agent was compiled without libcurl"));
}

#endif
