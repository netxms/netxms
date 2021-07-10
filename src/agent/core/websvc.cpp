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


#ifdef HAVE_LIBJQ

extern "C" {
#include <jq.h>
}

#else
typedef int jv;
#define jv_free(p)
#endif

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
   MUTEX m_lock;
   DocumentType m_type;
   union
   {
      Config *xml;
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

#ifdef HAVE_LIBJQ

/**
 * JQ logging callback
 */
void JqMessageCallback(void *data, jv error)
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Message called"));
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
      const TCHAR *wordFistLetter = originalValue+1;
      const TCHAR *currentPos = _tcschr(wordFistLetter, _T('/'));
      while (currentPos != nullptr)
      {
         result.append(wordFistLetter, currentPos - wordFistLetter);
         if (currentPos[1] != _T('\0'))
         {
            result.append(_T("\".\""));
         }
         wordFistLetter = currentPos + 1;
         currentPos = _tcschr(wordFistLetter, _T('/'));
      }
      result.append(wordFistLetter);
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
         if (jv_is_valid(result))
         {
            if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 8)
            {
               jv resultAsText = jv_dump_string(jv_copy(result), 0);
               nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromJSON(): request query: %hs"), programm);
               nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromJSON(): result kind: %d"), jv_get_kind(result));
               nxlog_debug_tag(DEBUG_TAG, 8, _T("ServiceEntry::getParamsFromJSON(): result: %hs"), jv_string_value(resultAsText));
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
      MemFree(programm);
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
      for (;jv_is_valid(result);result = jq_next(jqState))
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
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (data.size() > 0 && response_code == 200)
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
                  {
                     rcc = ERR_MALFORMED_RESPONSE;
                     nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): Failed to load XML"));
                  }
               }
               else if (!forcePlainTextParser && (*text == '{'))
               {
                  m_type = DocumentType::JSON;
#ifdef HAVE_LIBJQ
                  m_content.jvData = jv_parse(text);
                  if (!jv_is_valid(m_content.jvData))
                  {
                     rcc = ERR_MALFORMED_RESPONSE;
                     jv error = jv_invalid_get_msg(jv_copy(m_content.jvData));
                     char *msg = MemCopyStringA(jv_string_value(error));
                     nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): Failed to parse json. Error: \"%hs\""), RemoveNewLines(msg));
                     MemFree(msg);

                  }
#else
                  rcc = ERR_FUNCTION_NOT_SUPPORTED;
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): libjq missing. Please install libjq to parse json."));
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
               if (data.size() == 0)
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): request returned empty document"));
               if (response_code != 200)
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("ServiceEntry::updateData(): request returned %d return code"), response_code);
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

static const char *s_httpRequestTypes[] = {"GET", "POST", "PUT", "DELETE", "PATCH"};

/**
 * Web service cusom request command executer
 */
void WebServiceCustomRequest(NXCPMessage *request, AbstractCommSession *session)
{
   uint16_t requestTypeCode = request->getFieldAsInt16(VID_HTTP_REQUEST_TYPE);
   if (requestTypeCode > static_cast<uint16_t>(WebServiceHTTPRequestType::_MAX_TYPE))
   {
      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
      response.setField(VID_RCC, ERR_INVALID_HTTP_REQUEST_CODE);
      session->sendMessage(&response);
      delete request;
   }

   TCHAR *url = request->getFieldAsString(VID_URL);
   char *login = request->getFieldAsUtf8String(VID_LOGIN_NAME);
   char *password = request->getFieldAsUtf8String(VID_PASSWORD);
   char *data = request->getFieldAsUtf8String(VID_REQUEST_DATA);
   bool hostVerify = request->getFieldAsBoolean(VID_VERIFY_HOST);
   bool peerVerify = request->getFieldAsBoolean(VID_VERIFY_CERT);
   WebServiceAuthType authType = WebServiceAuthTypeFromInt(request->getFieldAsInt16(VID_AUTH_TYPE));
   uint32_t requestTimeout = request->getFieldAsUInt32(VID_TIMEOUT);
   const char *requestType = s_httpRequestTypes[requestTypeCode];

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

   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   uint32_t rcc = ERR_SUCCESS;
   const TCHAR *errorText = _T("Curl request failure");
   CURL *curl = curl_easy_init();
   nxlog_debug_tag(DEBUG_TAG, 4, _T("WebServiceCustomRequest(): Request \"%s\" url, request type \"%hs\""), url, requestType);
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
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, requestType);
      if (data != nullptr)
      {
         curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
      }
      else if (requestTypeCode == static_cast<uint16_t>(WebServiceHTTPRequestType::_POST))
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
         curl_easy_setopt(curl, CURLOPT_USERNAME, login);
         curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, password);
#else
       rcc = ERR_NOT_IMPLEMENTED;
       errorText = _T("OAuth 2.0 Bearer Access Token not implemented");
#endif
      }
      else
      {
         curl_easy_setopt(curl, CURLOPT_USERNAME, login);
         curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
         curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CurlAuthType(authType));
      }

     if (rcc == ERR_SUCCESS)
     {
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
              long response_code;
              curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
              response.setField(VID_WEB_SWC_RESPONSE_CODE, static_cast<uint32_t>(response_code));

              data.write('\0');
              size_t size;
              const char *text = reinterpret_cast<const char*>(data.buffer(&size));
              response.setFieldFromMBString(VID_WEB_SWC_RESPONSE, text);
              errorText = _T("");

              if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 8)
              {
#ifdef UNICODE
                 WCHAR *responseText = WideStringFromUTF8String(reinterpret_cast<const char*>(data.buffer()));
#else
                 char *responseText = MBStringFromUTF8String(reinterpret_cast<const char*>(data.buffer()));
#endif
                 for (TCHAR *s = responseText; *s != 0; s++)
                    if (*s < ' ')
                       *s = ' ';
                 nxlog_debug_tag(DEBUG_TAG, 6, _T("WebServiceCustomRequest(): response data: %s"), responseText);
                 MemFree(responseText);
              }
           }
           else
           {
              nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceCustomRequest(): error making curl request: %hs"), errbuf);
              rcc = ERR_MALFORMED_RESPONSE;
           }
        }
        else
        {
           nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceCustomRequest(): curl_easy_setopt failed for CURLOPT_URL"));
           rcc = ERR_UNKNOWN_PARAMETER;
        }
        MemFree(urlUtf8);
     }
      curl_easy_cleanup(curl);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("WebServiceCustomRequest(): Get data from service: curl_init failed"));
      rcc = ERR_INTERNAL_ERROR;
   }

   response.setField(VID_WEB_SWC_ERROR_TEXT, errorText);
   response.setField(VID_RCC, rcc);

   curl_slist_free_all(headers);
   MemFree(login);
   MemFree(password);
   MemFree(data);
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
