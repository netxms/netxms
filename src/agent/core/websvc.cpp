/*
** NetXMS multiplatform core agent
** Copyright (C) 2020-2023 Raden Solutions
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

#ifdef HAVE_LIBJQ
extern "C" {
#include <jq.h>
}
#else
typedef int jv;
#define jv_free(p)
#endif   /* HAVE_LIBJQ */

#define DEBUG_TAG _T("websvc")

#if HAVE_LIBCURL

#include <nxlibcurl.h>
#include <netxms-regex.h>
#include <pugixml.h>

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
 * HTTP request methods
 */
static const char *s_httpRequestMethods[] = { "GET", "POST", "PUT", "DELETE", "PATCH" };

/**
 * Function to replace new line to space
 */
char *RemoveNewLines(char* str)
{
    char *currentPos = strchr(str,'\n');
    while (currentPos != nullptr)
    {
        *currentPos = ' ';
        currentPos = strchr(currentPos,'\n');
    }
    return str;
}

/**
 * One cached service entry
 */
class ServiceEntry
{
private:
   time_t m_lastRequestTime;
   Mutex m_mutex;
   DocumentType m_type;
   uint32_t m_responseCode;
   union
   {
      pugi::xml_document *xml;
      jv jvData;
      TCHAR *text;
   } m_content;

   void deleteContent()
   {
      switch(m_type)
      {
         case DocumentType::JSON:
            jv_free(m_content.jvData);
            break;
         case DocumentType::TEXT:
            MemFree(m_content.text);
            break;
         case DocumentType::XML:
            delete m_content.xml;
            break;
         default:
            break;
      }
      m_type = DocumentType::NONE;
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

   uint32_t getParams(StringList *params, NXCPMessage *response);
   uint32_t getList(const TCHAR *path, NXCPMessage *response);
   const TCHAR *getText();
   uint32_t getResponseCode();
   bool isDataExpired(uint32_t retentionTime) { return (time(nullptr) - m_lastRequestTime) >= retentionTime; }
   uint32_t query(const TCHAR *url, uint16_t requestMethod, const char *requestData, const char *userName, const char *password,
         WebServiceAuthType authType, struct curl_slist *headers, bool verifyPeer, bool verifyHost, bool followLocation, bool forcePlainTextParser, uint32_t requestTimeout);

   void lock() { m_mutex.lock(); }
   void unlock() { m_mutex.unlock(); }
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
   m_responseCode = 0;
}

/**
 * Destructor
 */
ServiceEntry::~ServiceEntry()
{
   deleteContent();
}

/**
 * Get parameters from XML cached data
 */
void ServiceEntry::getParamsFromXML(StringList *params, NXCPMessage *response)
{
   char xpath[1024];
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      const TCHAR *metric = params->get(i);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromXML(): get parameter \"%s\""), metric);
      tchar_to_utf8(metric, -1, xpath, 1024);
      pugi::xpath_node node = m_content.xml->select_node(xpath);
      if (node)
      {
         response->setField(fieldId++, params->get(i));
         if (node.node())
            response->setFieldFromUtf8String(fieldId++, node.node().text().as_string());
         else
            response->setFieldFromUtf8String(fieldId++, node.attribute().value());
         resultCount++;
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParamsFromXML(): cannot execute select_node() for parameter \"%s\""), metric);
      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

#ifdef HAVE_LIBJQ

/**
 * JQ logging callback
 */
void JqMessageCallback(void *data, jv error)
{
   char *msg = MemCopyStringA(jv_string_value(error));
   nxlog_debug_tag(DEBUG_TAG, 6, _T("%s: %hs"), data, RemoveNewLines(msg));
   MemFree(msg);
   jv_free(error);
}

/**
 * Convert old request format to new format if required
 * replace '/' to '.'
 */
String ConvertRequestToJqFormat(const TCHAR *originalValue)
{
   StringBuffer result;
   if (originalValue[0] == _T('/'))
   {
      result.append(_T(".\""));
      const TCHAR *element = originalValue+1;
      const TCHAR *next = _tcschr(element, _T('/'));
      while (next != nullptr)
      {
         result.append(element, next - element);
         if (next[1] != _T('\0'))
         {
            result.append(_T("\".\""));
         }
         element = next + 1;
         next = _tcschr(element, _T('/'));
      }
      result.append(element);
      result.append(_T("\""));
   }
   else
   {
      result = StringBuffer(originalValue);
   }
   return result;
}

/**
 * Set NXCP message field from jv object value
 */
static bool SetFieldFromJVObject(NXCPMessage *msg, uint32_t fieldId, jv jvResult)
{
   bool skip = false;
   switch(jv_get_kind(jvResult))
   {
      case JV_KIND_STRING:
         msg->setFieldFromUtf8String(fieldId, jv_string_value(jvResult));
         break;
      case JV_KIND_NULL://Valid request - nothing found
         skip = true;
         break;
      default:
         jv jvValueAsString = jv_dump_string(jv_copy(jvResult), 0);
         msg->setFieldFromUtf8String(fieldId, jv_string_value(jvValueAsString));
         jv_free(jvValueAsString);
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
   jq_state *jqState = jq_init();
   jq_set_error_cb(jqState, JqMessageCallback, const_cast<TCHAR *>(_T("ServiceEntry::getParamsFromJSON()")));
   for (int i = 0; i < params->size(); i++)
   {
      String param = ConvertRequestToJqFormat(params->get(i));
#ifdef UNICODE
      char *program = UTF8StringFromWideString(param);
#else
      char *program = MemCopyStringA(param);
#endif
      if (jv_is_valid(m_content.jvData) && jq_compile(jqState, program))
      {
         jq_start(jqState, jv_copy(m_content.jvData), 0);
         jv result = jq_next(jqState);
         if (jv_is_valid(result))
         {
            if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
            {
               jv resultAsText = jv_dump_string(jv_copy(result), 0);
               nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParamsFromJSON(): request query: %hs"), program);
               nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParamsFromJSON(): result kind: %d"), jv_get_kind(result));
               nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParamsFromJSON(): result: %hs"), jv_string_value(resultAsText));
               jv_free(resultAsText);
            }

            response->setField(fieldId++, params->get(i));
            if (SetFieldFromJVObject(response, fieldId, result))
            {
               fieldId++;
               resultCount++;
            }
            else
            {
               fieldId--;
            }
         }
         jv_free(result);
      }
      MemFree(program);
   }
   jq_teardown(&jqState);
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

void GetStirngFromJvArrayElement(jv jvResult, StringList *resultList)
{
   switch(jv_get_kind(jvResult))
   {
      case JV_KIND_STRING:
         resultList->addUTF8String(jv_string_value(jvResult));
         break;
      case JV_KIND_ARRAY: //do nothing for array in array
         nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::getListFromJSON(): skip array as element of array"));
         break;
      case JV_KIND_NULL://Valid request - nothing found
         nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::getListFromJSON(): array element is empty"));
         break;
      case JV_KIND_OBJECT:
         jv_object_foreach(jvResult, key, val)
         {
            resultList->addUTF8String(jv_string_value(key));
            jv_free(key);
            jv_free(val);
         }
         break;
      default:
         jv jvValueAsString = jv_dump_string(jv_copy(jvResult), 0);
         resultList->addUTF8String(jv_string_value(jvValueAsString));
         jv_free(jvValueAsString);
         break;
   }

}

/**
 * Get list from JSON cached data
 */
void ServiceEntry::getListFromJSON(const TCHAR *path, StringList *resultList)
{
   nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromJSON(): Get child object list for JSON path \"%s\""), path);
   jq_state *jqState = jq_init();
   jq_set_error_cb(jqState, JqMessageCallback, const_cast<TCHAR *>(_T("ServiceEntry::getListFromJSON()")));
   String param = ConvertRequestToJqFormat(path);
   char *programm;
#ifdef UNICODE
   programm = MBStringFromWideString(param);
#else
   programm = MemCopyString(param);
#endif
   if (jv_is_valid(m_content.jvData) && jq_compile(jqState, programm))
   {
      jq_start(jqState, jv_copy(m_content.jvData), 0);
      jv result = jq_next(jqState);
      for (; jv_is_valid(result); result = jq_next(jqState))
      {
         if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 8)
         {
            jv resultAsText = jv_dump_string(jv_copy(result), 0);
            nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromJSON(): request query: %hs"), programm);
            nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromJSON(): result kind: %d"), jv_get_kind(result));
            nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromJSON(): result: %hs"), jv_string_value(resultAsText));
            jv_free(resultAsText);
         }

         switch(jv_get_kind(result))
         {
            case JV_KIND_STRING:
               resultList->addUTF8String(jv_string_value(jv_copy(result)));
               break;
            case JV_KIND_OBJECT:
               jv_object_foreach(result, key, val)
               {
                  resultList->addUTF8String(jv_string_value(key));
                  jv_free(key);
                  jv_free(val);
               }
               break;
            case JV_KIND_ARRAY:
               jv_array_foreach(result, i, elem)
               {
                  GetStirngFromJvArrayElement(elem, resultList);
                  jv_free(elem);
               }
               break;
            case JV_KIND_NUMBER:
            {
               jv jvValueAsString = jv_dump_string(jv_copy(result), 0);
               resultList->addUTF8String(jv_string_value(jvValueAsString));
               break;
            }
            case JV_KIND_NULL://Valid request - nothing found
               nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::getListFromJSON(): for \"%hs\" request result is empty"), programm);
               break;
            default://Valid request - nothing found
               if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
               {
                  jv jvValueAsString = jv_dump_string(jv_copy(result), 0);
                  nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getListFromJSON(): invalid object: \"%s\", type: %d"), jv_string_value(jvValueAsString), jv_get_kind(result));
               }
               break;
         }
         jv_free(result);
      }
      jv_free(result);
   }
   jq_teardown(&jqState);
   MemFree(programm);
}

#else

/**
 * Default implementation for build without libjq
 */
void ServiceEntry::getParamsFromJSON(StringList *params, NXCPMessage *response)
{
   //do nothing
}

/**
 * Get list from JSON cached data
 */
void ServiceEntry::getListFromJSON(const TCHAR *path, StringList *result)
{
   //do nothing
}

#endif

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
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ServiceEntry::getParamsFromText(): \"%s\" pattern compilation failure: %hs at offset %d"), pattern, eptr, eoffset);
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
uint32_t ServiceEntry::getParams(StringList *params, NXCPMessage *response)
{
   uint32_t result = ERR_SUCCESS;
   switch(m_type)
   {
      case DocumentType::XML:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParams(): get parameter from XML"));
         getParamsFromXML(params, response);
         break;
      case DocumentType::JSON:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParams(): get parameter from JSON"));
         getParamsFromJSON(params, response);
#ifndef HAVE_LIBJQ
         result = ERR_FUNCTION_NOT_SUPPORTED;
#endif
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::getParams(): get parameter from Text"));
         getParamsFromText(params, response);
         break;
   }
   return result;
}

/**
 * Get list from XML cached data
 */
void ServiceEntry::getListFromXML(const TCHAR *path, StringList *result)
{
   nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getListFromXML(): Get child tag list for path \"%s\""), path);

   char xpath[1024];
   tchar_to_utf8(path, -1, xpath, 1024);
   pugi::xpath_node_set nodes = m_content.xml->select_nodes(xpath);
   for (pugi::xpath_node node : nodes)
   {
      if (node.node())
         result->addUTF8String(node.node().text().as_string());
      else
         result->addUTF8String(node.attribute().value());
   }
}

/**
 * Get list from Text cached data
 */
uint32_t ServiceEntry::getListFromText(const TCHAR *pattern, StringList *resultList)
{
   uint32_t retVal = ERR_SUCCESS;
   StringList *dataLines = String::split(m_content.text, _tcslen(m_content.text), _T("\n"));
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
#ifndef HAVE_LIBJQ
         result = ERR_FUNCTION_NOT_SUPPORTED;
#endif
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
 * Get Text cached data
 */
const TCHAR *ServiceEntry::getText()
{
   return m_content.text;
}

/**
 * Get HTTP response code
 */
uint32_t ServiceEntry::getResponseCode()
{
   return m_responseCode;
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
 * Query web service
 */
uint32_t ServiceEntry::query(const TCHAR *url, uint16_t requestMethod, const char *requestData, const char *userName, const char *password,
      WebServiceAuthType authType, struct curl_slist *headers, bool verifyPeer, bool verifyHost, bool followLocation, bool forcePlainTextParser, uint32_t requestTimeout)
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
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnCurlDataReceived);
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
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::query(%s): requesting URL %hs"), url, urlUtf8);
         if (requestData != nullptr)
            nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::query(%s): request data: %hs"), url, requestData);

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
               nxlog_debug_tag(DEBUG_TAG, 7, _T("ServiceEntry::query(%s): response data: %hs"), url, text);

               if (!forcePlainTextParser && (*text == '<'))
               {
                  m_type = DocumentType::XML;
                  m_content.xml = new pugi::xml_document();
                  if (!m_content.xml->load_buffer(text, size))
                  {
                     rcc = ERR_MALFORMED_RESPONSE;
                     nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): Failed to load XML"), url);
                  }
               }
               else if (!forcePlainTextParser && ((*text == '{') || (*text == '[')))
               {
                  m_type = DocumentType::JSON;
#ifdef HAVE_LIBJQ
                  m_content.jvData = jv_parse(text);
                  if (!jv_is_valid(m_content.jvData))
                  {
                     rcc = ERR_MALFORMED_RESPONSE;
                     jv error = jv_invalid_get_msg(jv_copy(m_content.jvData));
                     char *msg = MemCopyStringA(jv_string_value(error));
                     nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): JSON document parsing error (%hs)"), url, RemoveNewLines(msg));
                     MemFree(msg);

                  }
#else
                  rcc = ERR_FUNCTION_NOT_SUPPORTED;
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): JSON document parsing error (agent was built without libjq support)"), url);
#endif
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
               nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::query(%s): response data type=%d, length=%u"), url, static_cast<int>(m_type), static_cast<unsigned int>(data.size()));
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
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::query(%s): response data: %s"), url, responseText);
                  MemFree(responseText);
               }
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
                        nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::query(%s): follow redirect to %hs"), url, redirectURL);
                        MemFree(urlUtf8);
                        urlUtf8 = MemCopyStringA(redirectURL);
                        data.clear();
                        goto retry;
                     }
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::query(%s): HTTP redirect to %hs, do not follow (redirect limit reached)"), url, redirectURL);
                  }
                  else
                  {
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("ServiceEntry::query(%s): HTTP redirect to %hs, do not follow (forbidden)"), url, redirectURL);
                  }
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): HTTP response %03d but redirect URL not set"), url, m_responseCode);
               }
               rcc = ERR_MALFORMED_RESPONSE;
            }
            else
            {
               if (m_responseCode < 200 || m_responseCode > 299)
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): HTTP response %03d"), url, m_responseCode);
               else if (data.size() == 0)
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): empty response"), url);
               m_type = DocumentType::NONE;
               rcc = ERR_MALFORMED_RESPONSE;
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): call to curl_easy_perform failed (%d: %hs)"), url, rc, errbuf);
            rcc = ERR_MALFORMED_RESPONSE;
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): curl_easy_setopt failed for CURLOPT_URL"), url);
         rcc = ERR_UNSUPPORTED_METRIC;
      }
      MemFree(urlUtf8);
      curl_easy_cleanup(curl);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::query(%s): curl_init failed"), url);
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

      result = cachedEntry->query(url, requestMethodCode, requestData, login, password, authType, headers,
               request->getFieldAsBoolean(VID_VERIFY_CERT), verifyHost,
               request->getFieldAsBoolean(VID_FOLLOW_LOCATION),
               request->getFieldAsBoolean(VID_FORCE_PLAIN_TEXT_PARSER),
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
            result = cachedEntry->getParams(&params, &response);
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

   s_serviceCacheLock.lock();
   shared_ptr<ServiceEntry> cachedEntry = s_serviceCache.getShared(url);
   if (cachedEntry == nullptr)
   {
      cachedEntry = make_shared<ServiceEntry>();
      s_serviceCache.set(url, cachedEntry);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceCustomRequest(): Create new cache entry for URL %s"), url);
   }
   s_serviceCacheLock.unlock();

   cachedEntry->lock();
   uint32_t retentionTime = request->getFieldAsUInt32(VID_RETENTION_TIME);
   uint32_t result = ERR_SUCCESS;
   if (cachedEntry->isDataExpired(retentionTime))
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

      result = cachedEntry->query(url, requestMethodCode, requestData, login, password, authType, headers,
               request->getFieldAsBoolean(VID_VERIFY_CERT),
               request->getFieldAsBoolean(VID_VERIFY_HOST),
               request->getFieldAsBoolean(VID_FOLLOW_LOCATION),
               request->getFieldAsBoolean(VID_FORCE_PLAIN_TEXT_PARSER),
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
   response.setField(VID_WEBSVC_RESPONSE, cachedEntry->getText());
   response.setField(VID_WEBSVC_RESPONSE_CODE, cachedEntry->getResponseCode());
   cachedEntry->unlock();

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

   s_serviceCacheLock.lock();
   auto it = s_serviceCache.begin();
   while(it.hasNext())
   {
      auto entry = it.next();
      shared_ptr<ServiceEntry> svc = *entry->value;
      if (svc->isDataExpired(g_webSvcCacheExpirationTime))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceHousekeeper(): Cache entry for URL \"%s\" removed because of inactivity"), entry->key);
         it.remove();
      }
   }
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
void QueryWebService(NXCPMessage* request, shared_ptr<AbstractCommSession> session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("QueryWebService(): agent was compiled without libcurl"));
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   response.setField(VID_RCC, ERR_NOT_IMPLEMENTED);
   session->sendMessage(&response);
   delete request;
}

/**
 * Web service cusom request command executer
 */
void WebServiceCustomRequest(NXCPMessage* request, shared_ptr<AbstractCommSession> session)
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("WebServiceCustomRequest(): agent was compiled without libcurl"));
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
