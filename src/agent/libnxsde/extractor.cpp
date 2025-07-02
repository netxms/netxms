/*
** NetXMS structured data extractor
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
** File: extractor.cpp
**
**/

#include "libnxsde.h"
#include <netxms-regex.h>

#define DEBUG_TAG _T("data.extractor")

/**
 * Function to replace new line to space
 */
static char *RemoveNewLines(char* str)
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
 * Constructor
 */
StructuredDataExtractor::StructuredDataExtractor(const TCHAR *source)
{
   m_lastRequestTime = 0;
   m_type = DocumentType::NONE;
   m_responseData = nullptr;
   _tcslcpy(m_source, source, 16);
}

/**
 * Get parameters from XML cached data
 */
void StructuredDataExtractor::getMetricsFromXML(StringList *params, NXCPMessage *response)
{
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      const TCHAR *metric = params->get(i);
      char result[MAX_RESULT_LENGTH];
      if (getMetricFromXML(metric, result, MAX_RESULT_LENGTH) == SYSINFO_RC_SUCCESS)
      {
         response->setField(fieldId++, metric);
         response->setFieldFromUtf8String(fieldId++, result);
         resultCount++;

      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Get parameter form XML type of document
 */
uint32_t StructuredDataExtractor::getMetricFromXML(const TCHAR *query, char *buffer, size_t size)
{
   char xpath[1024];
   nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getMetricFromXML(%s): get parameter \"%s\""), m_source, query);
   tchar_to_utf8(query, -1, xpath, 1024);
   pugi::xpath_node node = m_content.xml->select_node(xpath);
   uint32_t rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   if (node)
   {
      if (node.node())
         strlcpy(buffer, node.node().text().as_string(), size);
      else
         strlcpy(buffer, node.attribute().value(), size);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetricFromXML(%s): cannot execute select_node() for parameter \"%s\""), m_source, query);
   }
   return rc;
}

#ifdef HAVE_LIBJQ

/**
 * JQ logging callback
 */
static void JqMessageCallback(void *data, jv error)
{
   const char *text = jv_string_value(error);
   size_t len = strlen(text);
   Buffer<char, 1024> msg(text, len + 1);
   nxlog_debug_tag(DEBUG_TAG, 6, _T("%s: %hs"), data, RemoveNewLines(msg));
   jv_free(error);
}

/**
 * Convert old request format to new format if required
 * replace '/' to '.'
 */
static String ConvertRequestToJqFormat(const TCHAR *originalValue)
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
 * Get parameters from JSON cached data
 */
void StructuredDataExtractor::getMetricsFromJSON(StringList *params, NXCPMessage *response)
{
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      String param = ConvertRequestToJqFormat(params->get(i));
      char result[MAX_RESULT_LENGTH];
      if (getMetricFromJSON(param, result, MAX_RESULT_LENGTH) == SYSINFO_RC_SUCCESS)
      {
         response->setField(fieldId++, params->get(i));
         response->setFieldFromUtf8String(fieldId++, result);
         resultCount++;

      }
   }
   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Get parameter form JSOn type of document
 */
uint32_t StructuredDataExtractor::getMetricFromJSON(const TCHAR *param, char *buffer, size_t size)
{
   jq_state *jqState = jq_init();
   jq_set_error_cb(jqState, JqMessageCallback, const_cast<TCHAR *>(_T("StructuredDataExtractor::getParamsFromJSON()")));
   uint32_t rc = SYSINFO_RC_NO_SUCH_INSTANCE;

    char *program = UTF8StringFromTString(param);
    if (jv_is_valid(m_content.jvData) && jq_compile(jqState, program))
    {
       jq_start(jqState, jv_copy(m_content.jvData), 0);
       jv result = jq_next(jqState);
       if (jv_is_valid(result))
       {
          if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
          {
             jv resultAsText = jv_dump_string(jv_copy(result), 0);
             nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetricFromJSON(%s): request query: %hs"), m_source, program);
             nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetricFromJSON(%s): result kind: %d"), m_source, jv_get_kind(result));
             nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetricFromJSON(%s): result: %hs"), m_source, jv_string_value(resultAsText));
             jv_free(resultAsText);
          }

          switch(jv_get_kind(result))
          {
             case JV_KIND_STRING:
                strlcpy(buffer, jv_string_value(result), size);
                rc = SYSINFO_RC_SUCCESS;
                break;
             case JV_KIND_NULL: // Valid request - nothing found
                break;
             default:
                jv jvValueAsString = jv_dump_string(jv_copy(result), 0);
                strlcpy(buffer, jv_string_value(jvValueAsString), size);
                jv_free(jvValueAsString);
                rc = SYSINFO_RC_SUCCESS;
                break;
          }
       }
       jv_free(result);
    }
    MemFree(program);
    jq_teardown(&jqState);
    return rc;
}

/**
 * Get string from JSON array element
 */
static void GetStringFromJvArrayElement(jv jvResult, StringList *resultList, const TCHAR *source)
{
   switch(jv_get_kind(jvResult))
   {
      case JV_KIND_STRING:
         resultList->addUTF8String(jv_string_value(jvResult));
         break;
      case JV_KIND_ARRAY: //do nothing for array in array
         nxlog_debug_tag(DEBUG_TAG, 6, _T("GetStringFromJvArrayElement(%s): skip array as element of array"), source);
         break;
      case JV_KIND_NULL://Valid request - nothing found
         nxlog_debug_tag(DEBUG_TAG, 6, _T("GetStringFromJvArrayElement(%s): array element is empty"), source);
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
uint32_t StructuredDataExtractor::getListFromJSON(const TCHAR *path, StringList *resultList)
{
   uint32_t rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromJSON(%s): Get child object list for JSON path \"%s\""), m_source, path);
   jq_state *jqState = jq_init();
   jq_set_error_cb(jqState, JqMessageCallback, const_cast<TCHAR *>(_T("StructuredDataExtractor::getListFromJSON()")));
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
            nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromJSON(%s): request query: %hs"), m_source, programm);
            nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromJSON(%s): result kind: %d"), m_source, jv_get_kind(result));
            nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromJSON(%s): result: %hs"), m_source, jv_string_value(resultAsText));
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
                  GetStringFromJvArrayElement(elem, resultList, m_source);
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
               nxlog_debug_tag(DEBUG_TAG, 6, _T("StructuredDataExtractor::getListFromJSON(%s): for \"%hs\" request result is empty"), m_source, programm);
               break;
            default://Valid request - nothing found
               if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 7)
               {
                  jv jvValueAsString = jv_dump_string(jv_copy(result), 0);
                  nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getListFromJSON(%s): invalid object: \"%s\", type: %d"), m_source, jv_string_value(jvValueAsString), jv_get_kind(result));
               }
               break;
         }
         jv_free(result);
         rc = SYSINFO_RC_SUCCESS;
      }
      jv_free(result);
   }
   jq_teardown(&jqState);
   MemFree(programm);
   return rc;
}

#else

/**
 * Default implementation for build without libjq
 */
void StructuredDataExtractor::getMetricsFromJSON(StringList *params, NXCPMessage *response)
{
   //do nothing
}

/**
 * Default implementation for build without libjq
 */
uint32_t StructuredDataExtractor::getMetricFromJSON(const TCHAR *param, char *buffer, size_t size)
{
   return SYSINFO_RC_UNSUPPORTED;
}

/**
 * Get list from JSON cached data
 */
uint32_t StructuredDataExtractor::getListFromJSON(const TCHAR *path, StringList *result)
{
   return SYSINFO_RC_UNSUPPORTED;
}

#endif

/**
 * Get multiple metrics from text document
 */
void StructuredDataExtractor::getMetricsFromText(StringList *params, NXCPMessage *response)
{
   uint32_t fieldId = VID_PARAM_LIST_BASE;
   int resultCount = 0;
   for (int i = 0; i < params->size(); i++)
   {
      const TCHAR *pattern = params->get(i);
      TCHAR result[MAX_RESULT_LENGTH];
      if (getMetricFromText(pattern, result, MAX_RESULT_LENGTH) == SYSINFO_RC_SUCCESS)
      {
         response->setField(fieldId++, pattern);
         response->setField(fieldId++, result);
         resultCount++;
      }
   }

   response->setField(VID_NUM_PARAMETERS, resultCount);
}

/**
 * Get single metric from text document
 */
uint32_t StructuredDataExtractor::getMetricFromText(const TCHAR *pattern, TCHAR *buffer, size_t size)
{
   StringList dataLines = String::split(m_content.text, _tcslen(m_content.text), _T("\n"));
   nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getParamsFromText(%s): using pattern \"%s\""), m_source, pattern);

   const char *eptr;
   int eoffset;
   PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (compiledPattern == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("StructuredDataExtractor::getMetricFromText(%s): \"%s\" pattern compilation failure: %hs at offset %d"), m_source, pattern, eptr, eoffset);
      return SYSINFO_RC_UNSUPPORTED;
   }

   uint32_t rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   for (int j = 0; j < dataLines.size(); j++)
   {
      const TCHAR *dataLine = dataLines.get(j);
      nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getMetricFromText(%s): checking data line \"%s\""), m_source, dataLine);
      int fields[30];
      int result = _pcre_exec_t(compiledPattern, nullptr, reinterpret_cast<const PCRE_TCHAR*>(dataLine), static_cast<int>(_tcslen(dataLine)), 0, 0, fields, 30);
      if (result >= 0)
      {
         if ((result >=2 || result == 0) && fields[2] != -1)
         {
            size_t minSize = std::min(static_cast<size_t>(fields[3] - fields[2]), size);
            _tcsncpy(buffer, &dataLine[fields[2]], minSize);
            buffer[minSize] = 0;
            nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getMetricFromText(%s): data match: \"%s\""), m_source, buffer);
            rc = SYSINFO_RC_SUCCESS;
         }
         break;
      }
   }

   _pcre_free_t(compiledPattern);
   return rc;
}

/**
 * Get parameters from cached data
 */
uint32_t StructuredDataExtractor::getMetric(const TCHAR *query, TCHAR *buffer, size_t size)
{
   uint32_t rc = SYSINFO_RC_ERROR;
   switch(m_type)
   {
      case DocumentType::XML:
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetric(%s): get metric from XML"), m_source);
         Buffer<char, 256> tmp(size);
         rc = getMetricFromXML(query, tmp, size);
         utf8_to_tchar(tmp, -1, buffer, size);
         break;
      }
      case DocumentType::JSON:
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetric(%s): get metric from JSON"), m_source);
#ifndef HAVE_LIBJQ
         rc = SYSINFO_RC_UNSUPPORTED;
#else
         Buffer<char, 256> tmp(size);
         rc = getMetricFromJSON(query, tmp, size);
         utf8_to_tchar(tmp, -1, buffer, size);
#endif
         break;
      }
      case DocumentType::TEXT:
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getMetric(%s): get metric from text document"), m_source);
         rc = getMetricFromText(query, buffer, size);
         break;
      }
      default:
         nxlog_debug_tag(DEBUG_TAG, 4, _T("StructuredDataExtractor::getMetric(%s): no data available"), m_source);
         break;
   }
   return rc;
}

/**
 * Get list from XML cached data
 */
void StructuredDataExtractor::getListFromXML(const TCHAR *path, StringList *result)
{
   nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromXML(%s): Get child tag list for path \"%s\""), m_source, path);

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
uint32_t StructuredDataExtractor::getListFromText(const TCHAR *pattern, StringList *resultList)
{
   uint32_t retVal = SYSINFO_RC_SUCCESS;
   StringList dataLines = String::split(m_content.text, _tcslen(m_content.text), _T("\n"));
   nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromText(%s): get list of matched lines for pattern \"%s\""), m_source, pattern);

   const char *eptr;
   int eoffset;
   PCRE *compiledPattern = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(pattern), PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (compiledPattern != nullptr)
   {
      TCHAR *matchedString = nullptr;
      for (int j = 0; j < dataLines.size(); j++)
      {
         const TCHAR *dataLine = dataLines.get(j);
         nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromText(%s): checking data line \"%s\""), m_source, dataLine);
         int fields[30];
         int result = _pcre_exec_t(compiledPattern, nullptr, reinterpret_cast<const PCRE_TCHAR*>(dataLine), static_cast<int>(_tcslen(dataLine)), 0, 0, fields, 30);
         if (result >= 0)
         {
            if ((result >=2 || result == 0) && fields[2] != -1)
            {
               uint32_t captureSize = fields[3] - fields[2];

               matchedString = MemAllocString(captureSize + 1);
               memcpy(matchedString, &dataLine[fields[2]], (captureSize) * sizeof(TCHAR));
               matchedString[captureSize] = 0;
               nxlog_debug_tag(DEBUG_TAG, 8, _T("StructuredDataExtractor::getListFromText(%s): data match: \"%s\""), m_source, matchedString);
               resultList->addPreallocated(matchedString);
            }
         }
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("StructuredDataExtractor::getListFromText(%s): \"%s\" pattern compilation failure: %hs at offset %d"), m_source, pattern, eptr, eoffset);
      retVal = SYSINFO_RC_UNSUPPORTED;
   }

   _pcre_free_t(compiledPattern);
   return retVal;
}

/**
 * Get list from cached data
 * If path is empty: "/" will be used for XML and JSOn types and "(*)" will be used for text type
 */
uint32_t StructuredDataExtractor::getList(const TCHAR *path, StringList *result)
{
   uint32_t rc = SYSINFO_RC_SUCCESS;
   const TCHAR *correctPath = (path[0] != 0) ? path : ((m_type == DocumentType::TEXT) ? _T("(.*)") : _T("/"));
   switch(m_type)
   {
      case DocumentType::XML:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getList(%s): get list from XML"), m_source);
         getListFromXML(correctPath, result);
         break;
      case DocumentType::JSON:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getList(%s): get list from JSON"), m_source);
         rc = getListFromJSON(correctPath, result);
         break;
      case DocumentType::TEXT:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getList(%s): get list from TEXT"), m_source);
         rc = getListFromText(correctPath, result);
         break;
      default:
         nxlog_debug_tag(DEBUG_TAG, 7, _T("StructuredDataExtractor::getList(%s): unsupported document type"), m_source);
         rc = SYSINFO_RC_NO_SUCH_INSTANCE;
         break;
   }
   return rc;
}

/**
 * Update cached content
 */
uint32_t StructuredDataExtractor::updateContent(const char *text, size_t size, bool forcePlainTextParser, const TCHAR *id)
{
   deleteContent();
   uint32_t rcc = ERR_SUCCESS;
   //Check for UTF-8 BOM
   if (size >= 3 && (static_cast<unsigned char>(text[0]) == 0xEF) && (static_cast<unsigned char>(text[1]) == 0xBB) && (static_cast<unsigned char>(text[2]) == 0xBF))
   {
      text += 3;
      size -= 3;
      nxlog_debug_tag(DEBUG_TAG, 6, _T("StructuredDataExtractor::updateContent(%s, %s): skipped BOM sequence"), m_source, id);
   }
   m_responseData = MemCopyBlock(text, size + 1); // +1 for null terminator
   if (!forcePlainTextParser && (*text == '<'))
   {
      m_type = DocumentType::XML;
      m_content.xml = new pugi::xml_document();
      if (!m_content.xml->load_buffer(text, size))
      {
         rcc = ERR_MALFORMED_RESPONSE;
         nxlog_debug_tag(DEBUG_TAG, 1, _T("StructuredDataExtractor::updateContent(%s, %s): Failed to load XML"), m_source, id);
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
         nxlog_debug_tag(DEBUG_TAG, 1, _T("StructuredDataExtractor::updateContent(%s, %s): JSON document parsing error (%hs)"), m_source, id, RemoveNewLines(msg));
         MemFree(msg);

      }
#else
      rcc = ERR_FUNCTION_NOT_SUPPORTED;
      nxlog_debug_tag(DEBUG_TAG, 1, _T("StructuredDataExtractor::updateContent(%s, %s): JSON document parsing error (agent was built without libjq support)"), m_source, id);
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
   nxlog_debug_tag(DEBUG_TAG, 6, _T("StructuredDataExtractor::updateContent(%s, %s): response data type=%d, length=%u"), m_source, id, static_cast<int>(m_type), static_cast<unsigned int>(size));
   if (nxlog_get_debug_level_tag(DEBUG_TAG) >= 8)
   {
#ifdef UNICODE
      WCHAR *responseText = WideStringFromUTF8String(text);
#else
      char *responseText = MBStringFromUTF8String(text);
#endif
      for(TCHAR *s = responseText; *s != 0; s++)
         if (*s < ' ')
            *s = ' ';
      nxlog_debug_tag(DEBUG_TAG, 6, _T("StructuredDataExtractor::updateContent(%s, %s): response data: %s"), m_source, id, responseText);
      MemFree(responseText);
   }
   return rcc;
}
