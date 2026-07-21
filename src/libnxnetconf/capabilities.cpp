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
** File: capabilities.cpp
**
**/

#include "libnxnetconf.h"

/**
 * Add capability URI to the set (duplicates are ignored)
 */
void NETCONF_CapabilitySet::add(const TCHAR *uri)
{
   if (!m_capabilities.contains(uri))
      m_capabilities.add(uri);
}

/**
 * Add capability URI in UTF-8 encoding to the set (duplicates are ignored)
 */
void NETCONF_CapabilitySet::addFromUTF8String(const char *uri)
{
#ifdef UNICODE
   TCHAR *s = WideStringFromUTF8String(uri);
#else
   TCHAR *s = MBStringFromUTF8String(uri);
#endif
   add(s);
   MemFree(s);
}

/**
 * Check if capability with given base URI is present, ignoring any parameters
 * after '?' in announced capability URIs
 */
bool NETCONF_CapabilitySet::containsBase(const TCHAR *baseUri) const
{
   size_t baseLen = _tcslen(baseUri);
   for(int i = 0; i < m_capabilities.size(); i++)
   {
      const TCHAR *uri = m_capabilities.get(i);
      if (!_tcsncmp(uri, baseUri, baseLen) && ((uri[baseLen] == 0) || (uri[baseLen] == _T('?'))))
         return true;
   }
   return false;
}

/**
 * Extract value of given parameter from capability URI within the set. Returns empty
 * string if capability is not present or does not carry the requested parameter.
 */
String NETCONF_CapabilitySet::getParameter(const TCHAR *baseUri, const TCHAR *name) const
{
   size_t baseLen = _tcslen(baseUri);
   size_t nameLen = _tcslen(name);
   for(int i = 0; i < m_capabilities.size(); i++)
   {
      const TCHAR *uri = m_capabilities.get(i);
      if (_tcsncmp(uri, baseUri, baseLen) || (uri[baseLen] != _T('?')))
         continue;

      for(const TCHAR *p = uri + baseLen + 1; *p != 0;)
      {
         const TCHAR *end = _tcschr(p, _T('&'));
         size_t len = (end != nullptr) ? end - p : _tcslen(p);
         if ((len > nameLen + 1) && !_tcsncmp(p, name, nameLen) && (p[nameLen] == _T('=')))
            return String(p + nameLen + 1, len - nameLen - 1);
         p = (end != nullptr) ? end + 1 : p + len;
      }
      break;
   }
   return String();
}

/**
 * Find YANG module capability by module name (matching "module" parameter of
 * capability URI). Returns full capability URI or empty string if not found.
 */
String NETCONF_CapabilitySet::findModule(const TCHAR *moduleName) const
{
   size_t nameLen = _tcslen(moduleName);
   for(int i = 0; i < m_capabilities.size(); i++)
   {
      const TCHAR *uri = m_capabilities.get(i);
      const TCHAR *params = _tcschr(uri, _T('?'));
      if (params == nullptr)
         continue;

      for(const TCHAR *p = params + 1; *p != 0;)
      {
         const TCHAR *end = _tcschr(p, _T('&'));
         size_t len = (end != nullptr) ? end - p : _tcslen(p);
         if ((len == nameLen + 7) && !_tcsncmp(p, _T("module="), 7) && !_tcsncmp(p + 7, moduleName, nameLen))
            return String(uri);
         p = (end != nullptr) ? end + 1 : p + len;
      }
   }
   return String();
}

/**
 * Get highest base protocol version announced in the set
 */
NetconfVersion NETCONF_CapabilitySet::getProtocolVersion() const
{
   return containsBase(NETCONF_CAPABILITY_BASE_1_1) ? NetconfVersion::NETCONF_1_1 : NetconfVersion::NETCONF_1_0;
}

/**
 * Parse NETCONF hello message, replacing current content of the set with capabilities
 * announced by the peer. Optionally extracts session ID assigned by the server.
 */
bool NETCONF_CapabilitySet::parseHelloMessage(const char *data, size_t size, uint32_t *sessionId)
{
   pugi::xml_document document;
   pugi::xml_parse_result result = document.load_buffer(data, size);
   if (!result)
   {
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_CapabilitySet::parseHelloMessage: XML parsing error (%hs)"), result.description());
      return false;
   }

   pugi::xml_node hello = NETCONF_FindChildByLocalName(document, "hello");
   if (!hello)
   {
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_CapabilitySet::parseHelloMessage: hello element not found"));
      return false;
   }

   pugi::xml_node capabilities = NETCONF_FindChildByLocalName(hello, "capabilities");
   if (!capabilities)
   {
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_CapabilitySet::parseHelloMessage: capabilities element not found"));
      return false;
   }

   m_capabilities.clear();
   for(pugi::xml_node capability = capabilities.first_child(); capability; capability = capability.next_sibling())
   {
      if ((capability.type() == pugi::node_element) && !strcmp(NETCONF_LocalName(capability), "capability"))
      {
         const char *uri = capability.child_value();
         if (*uri != 0)
            addFromUTF8String(uri);
      }
   }

   if (sessionId != nullptr)
   {
      pugi::xml_node sessionIdNode = NETCONF_FindChildByLocalName(hello, "session-id");
      *sessionId = sessionIdNode ? sessionIdNode.text().as_uint(0) : 0;
   }

   return true;
}

/**
 * Build NETCONF hello message announcing base protocol versions 1.0 and 1.1
 * plus optional additional capabilities
 */
void LIBNXNETCONF_EXPORTABLE NETCONF_BuildHelloMessage(pugi::xml_document& doc, const NETCONF_CapabilitySet *additionalCapabilities)
{
   doc.reset();
   pugi::xml_node hello = doc.append_child("hello");
   hello.append_attribute("xmlns") = NETCONF_XML_NAMESPACE;
   pugi::xml_node capabilities = hello.append_child("capabilities");
   capabilities.append_child("capability").text() = NETCONF_CAPABILITY_BASE_1_0_UTF8;
   capabilities.append_child("capability").text() = NETCONF_CAPABILITY_BASE_1_1_UTF8;
   if (additionalCapabilities != nullptr)
   {
      for(int i = 0; i < additionalCapabilities->size(); i++)
      {
         const TCHAR *uri = additionalCapabilities->get(i);
         char *utf8uri = UTF8StringFromTString(uri);
         if (strcmp(utf8uri, NETCONF_CAPABILITY_BASE_1_0_UTF8) && strcmp(utf8uri, NETCONF_CAPABILITY_BASE_1_1_UTF8))
            capabilities.append_child("capability").text() = utf8uri;
         MemFree(utf8uri);
      }
   }
}
