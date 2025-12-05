/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: export.cpp
**
**/

#include "nxcore.h"
#include <netxms-xml.h>

/**
 * Expand macros in regexp
 */
void FindMacros(const char *regexp, ObjectArray<char> *macroList)
{
   const char *curr, *prev;
   char name[256];

   for(curr = prev = regexp; *curr != 0; curr++)
   {
      if (*curr == '@')
      {
         // Check for escape
         if ((curr != regexp) && (*(curr - 1) == '\\'))
         {
            // escaped @ - do nothing
         }
         else
         {
            // { should follow @
            if (*(curr + 1) == '{')
            {
               int i;

               curr += 2;
               for(i = 0; (*curr != 0) && (*curr != '}'); i++)
                  name[i] = *curr++;
               name[i] = 0;
               macroList->add(MemCopyStringA(name));
            }
         }
         prev = curr + 1;
      }
   }
}

/**
 * Export log parser rules to JSON object (matching XML structure)
 */
static void ExportLogParserRulesJSON(const NXCPMessage& request, long countFieldId, long baseId, json_t *object, const WCHAR *configKey)
{
   int count = request.getFieldAsUInt32(countFieldId);
   
   if (count == 0)
   {
      // Create empty rules and order objects/arrays
      json_object_set_new(object, "rules", json_object());
      json_object_set_new(object, "order", json_array());
      return;
   }
   
   char *value = ConfigReadCLOBUTF8(configKey, nullptr);
   if (value == nullptr)
   {
      json_object_set_new(object, "rules", json_object());
      json_object_set_new(object, "order", json_array());
      return;
   }

   pugi::xml_document xml;
   if (!xml.load_buffer(value, strlen(value)))
   {
      json_object_set_new(object, "rules", json_object());
      json_object_set_new(object, "order", json_array());
      MemFree(value);
      return;
   }

   pugi::xml_node node = xml.select_node("/parser/rules").node();
   if (node != nullptr)
   {
      ObjectArray<char> macroList;
      uuid_t guid;
      json_t *rulesObject = json_object();
      
      for(int i = 0; i < count; i++)
      {
         request.getFieldAsBinary(baseId++, guid, UUID_LENGTH);
         char guidStr[64];
         _uuid_to_stringA(guid, guidStr);
         pugi::xml_node ruleNode = node.find_child_by_attribute("guid", guidStr);
         if (ruleNode != nullptr)
         {
            // Export rule as XML string to maintain exact structure
            xml_string_writer ruleWriter;
            ruleNode.print(ruleWriter, "", pugi::format_default | pugi::format_raw, pugi::encoding_utf8, 0);
            char *xmlUtf8 = UTF8StringFromWideString(ruleWriter.result.cstr());
            json_object_set_new(rulesObject, guidStr, json_string(xmlUtf8));
            MemFree(xmlUtf8);

            const char *match = ruleNode.select_node("match").node().text().get();
            FindMacros(match, &macroList);
         }
      }
      
      json_object_set_new(object, "rules", rulesObject);

      // Export macros as name-value object
      pugi::xml_node macros = xml.select_node("/parser/macros").node();
      if ((macroList.size() > 0) && (macros != nullptr))
      {
         json_t *macrosObject = json_object();
         for(int i = 0; i < macroList.size(); i++)
         {
            char *macroName = macroList.get(i);
            pugi::xml_node macro = macros.find_child_by_attribute("name", macroName);
            if (macro != nullptr)
            {
               const char *macroValue = macro.text().get();
               json_object_set_new(macrosObject, macroName, json_string(macroValue));
            }
            MemFree(macroName);
         }
         json_object_set_new(object, "macros", macrosObject);
      }

      // Export rule order
      json_t *orderArray = json_array();
      for (auto child : node.children())
      {
         const char *guid = child.attribute("guid").as_string();
         json_array_append_new(orderArray, json_string(guid));
      }
      json_object_set_new(object, "order", orderArray);
   }

   MemFree(value);
}

/**
 * Export syslog parser rules to JSON object (matching XML structure)
 */
void ExportSyslogParserRulesJSON(const NXCPMessage& request, long countFieldId, long baseId, json_t *object)
{
   ExportLogParserRulesJSON(request, countFieldId, baseId, object, L"SyslogParser");
}

/**
 * Export winlog parser rules to JSON object (matching XML structure)
 */
void ExportWinlogParserRulesJSON(const NXCPMessage& request, long countFieldId, long baseId, json_t *object)
{
   ExportLogParserRulesJSON(request, countFieldId, baseId, object, L"WindowsEventParser");
}
