/*
** NetXMS - Network Management System
** NETCONF protocol support library
** Copyright (C) 2026 Raden Solutions
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
** File: response.cpp
**
**/

#include "libnxnetconf.h"

/**
 * Get child element text as string, ignoring namespace prefixes
 */
static String GetChildValue(const pugi::xml_node& parent, const char *name)
{
   pugi::xml_node child = NETCONF_FindChildByLocalName(parent, name);
   return child ? String(child.child_value(), "utf-8") : String();
}

/**
 * Create error information from rpc-error element
 */
NETCONF_Error::NETCONF_Error(const pugi::xml_node& node) :
      m_type(GetChildValue(node, "error-type")),
      m_tag(GetChildValue(node, "error-tag")),
      m_severity(GetChildValue(node, "error-severity")),
      m_appTag(GetChildValue(node, "error-app-tag")),
      m_path(GetChildValue(node, "error-path")),
      m_message(GetChildValue(node, "error-message"))
{
}

/**
 * Response constructor
 */
NETCONF_Response::NETCONF_Response() : m_errors(0, 8, Ownership::True)
{
   m_messageId = 0;
   m_valid = false;
}

/**
 * Parse rpc-reply message
 */
bool NETCONF_Response::parse(const char *data, size_t size)
{
   m_valid = false;
   m_messageId = 0;
   m_errors.clear();

   pugi::xml_parse_result result = m_document.load_buffer(data, size);
   if (!result)
   {
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_Response::parse: XML parsing error (%hs)"), result.description());
      return false;
   }

   m_reply = NETCONF_FindChildByLocalName(m_document, "rpc-reply");
   if (!m_reply)
   {
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_Response::parse: rpc-reply element not found"));
      return false;
   }

   m_messageId = m_reply.attribute("message-id").as_uint(0);

   for(pugi::xml_node child = m_reply.first_child(); child; child = child.next_sibling())
   {
      if ((child.type() == pugi::node_element) && !strcmp(NETCONF_LocalName(child), "rpc-error"))
         m_errors.add(new NETCONF_Error(child));
   }

   m_valid = true;
   return true;
}

/**
 * Check if reply contains ok element
 */
bool NETCONF_Response::isOk() const
{
   return m_valid && NETCONF_FindChildByLocalName(m_reply, "ok");
}

/**
 * Check if reply indicates success (valid reply without errors; warnings are not
 * considered errors)
 */
bool NETCONF_Response::isSuccess() const
{
   if (!m_valid)
      return false;
   for(int i = 0; i < m_errors.size(); i++)
      if (!m_errors.get(i)->isWarning())
         return false;
   return true;
}

/**
 * Get all error messages as single semicolon-separated string
 */
String NETCONF_Response::getErrorText() const
{
   StringBuffer text;
   for(int i = 0; i < m_errors.size(); i++)
   {
      NETCONF_Error *error = m_errors.get(i);
      if (error->getMessage().isEmpty() && error->getTag().isEmpty())
         continue;
      if (!text.isEmpty())
         text.append(_T("; "));
      text.append(error->getMessage().isEmpty() ? error->getTag() : error->getMessage());
   }
   return text;
}

/**
 * Get data element of the reply
 */
pugi::xml_node NETCONF_Response::getData() const
{
   return m_valid ? NETCONF_FindChildByLocalName(m_reply, "data") : pugi::xml_node();
}

/**
 * Evaluate XPath expression against reply document and return result as string
 */
String NETCONF_Response::getValueByXPath(const char *xpath) const
{
   return m_valid ? NETCONF_GetValueByXPath(m_document, xpath) : String();
}
