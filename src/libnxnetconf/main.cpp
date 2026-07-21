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
** File: main.cpp
**
**/

#include "libnxnetconf.h"

/**
 * Get canonical name of NETCONF datastore
 */
const char LIBNXNETCONF_EXPORTABLE *NETCONF_DatastoreName(NetconfDatastore datastore)
{
   switch(datastore)
   {
      case NetconfDatastore::RUNNING:
         return "running";
      case NetconfDatastore::CANDIDATE:
         return "candidate";
      case NetconfDatastore::STARTUP:
         return "startup";
   }
   return "running";
}

/**
 * Get local part of possibly prefixed XML element name
 */
const char LIBNXNETCONF_EXPORTABLE *NETCONF_LocalName(const pugi::xml_node& node)
{
   const char *name = node.name();
   const char *p = strchr(name, ':');
   return (p != nullptr) ? p + 1 : name;
}

/**
 * Find child element by local name, ignoring namespace prefix
 */
pugi::xml_node LIBNXNETCONF_EXPORTABLE NETCONF_FindChildByLocalName(const pugi::xml_node& parent, const char *name)
{
   for(pugi::xml_node child = parent.first_child(); child; child = child.next_sibling())
   {
      if ((child.type() == pugi::node_element) && !strcmp(NETCONF_LocalName(child), name))
         return child;
   }
   return pugi::xml_node();
}

/**
 * Evaluate XPath expression in given context and return result as string
 */
String LIBNXNETCONF_EXPORTABLE NETCONF_GetValueByXPath(const pugi::xml_node& context, const char *xpath)
{
   pugi::xpath_query query(xpath);
   if (!query.result())
   {
      nxlog_debug_tag(DEBUG_TAG_NETCONF, 6, _T("NETCONF_GetValueByXPath: invalid XPath expression \"%hs\" (%hs)"), xpath, query.result().error);
      return String();
   }

   size_t size = query.evaluate_string(nullptr, 0, context);
   if (size <= 1)
      return String();

   char *buffer = MemAllocArrayNoInit<char>(size);
   query.evaluate_string(buffer, size, context);
   String result(buffer, "utf-8");
   MemFree(buffer);
   return result;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
