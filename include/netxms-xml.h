/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: netxms-xml.h
**
**/

#ifndef _netxms_xml_h
#define _netxms_xml_h

#include <pugixml.h>
#include <nms_util.h>

/**
 * Write XML to string
 */
class xml_string_writer : public pugi::xml_writer
{
public:
   StringBuffer result;

   virtual void write(const void* data, size_t size) override
   {
      result.appendUtf8String(static_cast<const char*>(data), size);
   }
};


static inline String XMLGetValueAsString(const pugi::xml_node& parent, const char* tag, const TCHAR* defaultValue = _T(""))
{
   pugi::xml_node node = parent.child(tag);
   return node ? String(node.child_value(), "utf-8") : String(defaultValue);
}

static inline uint32_t XMLGetValueAsUint(const pugi::xml_node& parent, const char* tag, uint32_t defaultValue = 0)
{
   pugi::xml_node node = parent.child(tag);
   return node ? node.text().as_uint(defaultValue) : defaultValue;
}

static inline int32_t XMLGetValueAsInt(const pugi::xml_node& parent, const char* tag, int32_t defaultValue = 0)
{
   pugi::xml_node node = parent.child(tag);
   return node ? node.text().as_int(defaultValue) : defaultValue;
}

static inline int64_t XMLGetValueAsLong(const pugi::xml_node& parent, const char* tag, int64_t defaultValue = 0)
{
   pugi::xml_node node = parent.child(tag);
   return node ? node.text().as_llong(defaultValue) : defaultValue;
}

static inline uuid XMLGetValueAsUUID(const pugi::xml_node& parent, const char* tag)
{
   pugi::xml_node node = parent.child(tag);
   return node ? uuid::parseA(node.child_value()) : uuid::NULL_UUID;
}

static inline bool XMLGetValueAsBool(const pugi::xml_node& parent, const char* tag, bool defaultValue = false)
{
   pugi::xml_node node = parent.child(tag);
   return node ? node.text().as_bool(defaultValue) : defaultValue;
}

static inline String XMLGetNodeValueAsString(const pugi::xml_node& node, const TCHAR* defaultValue = _T(""))
{
   return node ? String(node.child_value(), "utf-8") : String(defaultValue);
}


#endif	/* _netxms_xml_h */
