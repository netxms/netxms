/* 
** NetXMS structured data extractor
** Copyright (C) 2003-2024 Reden Solutions
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
** File: nxsde.h
**
**/

#ifndef _nxsde_h_
#define _nxsde_h_



#ifdef LIBNXSDE_EXPORTS
#define LIBNXSDE_EXPORTABLE __EXPORT
#else
#define LIBNXSDE_EXPORTABLE __IMPORT
#endif

#ifdef HAVE_LIBJQ
extern "C" {
#include <jq.h>
}
#else
typedef int jv;
#define jv_free(p)
#endif   /* HAVE_LIBJQ */

#include <netxms-regex.h>
#include <pugixml.h>

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
 * Executed web service request
 */
class LIBNXSDE_EXPORTABLE StructuredDataParser
{
protected:
   time_t m_lastRequestTime;
   DocumentType m_type;
   char *m_responseData;
   const TCHAR *m_source;
   union
   {
      pugi::xml_document *xml;
      jv jvData;
      TCHAR *text;
   } m_content;

   void deleteContent()
   {
      MemFreeAndNull(m_responseData);
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
   uint32_t getParamsFromXML(const TCHAR *query, char *buffer, size_t size);
   uint32_t getParamsFromJSON(const TCHAR *query, char *buffer, size_t size);
   uint32_t getParamsFromText(const TCHAR *query, TCHAR *buffer, size_t size);

   void getListFromXML(const TCHAR *path, StringList *result);
   uint32_t  getListFromJSON(const TCHAR *path, StringList *result);
   uint32_t getListFromText(const TCHAR *pattern, StringList *result);

public:
   StructuredDataParser(const TCHAR *debugTag);
   virtual ~StructuredDataParser()
   {
      deleteContent();
   }

   uint32_t getMetric(const TCHAR *query, TCHAR *buffer, size_t size);
   uint32_t getList(const TCHAR *path, StringList *result);
   const char *getResponseData() const { return m_responseData; }
   bool isDataExpired(uint32_t retentionTime) { return (time(nullptr) - m_lastRequestTime) >= retentionTime; }
   uint32_t updateContent(const char *text, uint32_t size, bool forcePlainTextParser, const TCHAR *id);
};

#endif
