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
 * Export log parser rule to XML
 */
void ExportLogParserRule(const NXCPMessage& request, long countFieldId, long baseId, TextFileWriter *writer, wchar_t *logName, wchar_t *xmlTagName)
{
   writer->appendUtf8String("\t<");
   writer->append(xmlTagName);
   writer->appendUtf8String(">\n");

   StringBuffer endTag;
   endTag.appendFormattedString(_T("\t</%s>\n"), xmlTagName);

   int count = request.getFieldAsUInt32(countFieldId);
   if (count == 0)
   {
      writer->append(endTag);
      return;
   }
   char *value = ConfigReadCLOBUTF8(logName, nullptr);
   if (value == nullptr)
   {
      writer->append(endTag);
      return;
   }

   pugi::xml_document xml;
   if (!xml.load_buffer(value, strlen(value)))
   {
      writer->append(endTag);
      MemFree(value);
      return;
   }

   pugi::xml_node node = xml.select_node("/parser/rules").node();
   if (node != nullptr)
   {
      ObjectArray<char> macroList;
      uuid_t guid;
      writer->appendUtf8String("\t\t<rules>\n");
      for(int i = 0; i < count; i++)
      {
         request.getFieldAsBinary(baseId++, guid, UUID_LENGTH);
         char guidStr[64];
         _uuid_to_stringA(guid, guidStr);
         pugi::xml_node ruleNode = node.find_child_by_attribute("guid", guidStr);
         if (ruleNode != nullptr)
         {
            xml_string_writer ruleWriter;
            ruleNode.print(ruleWriter, "\t", pugi::format_default, pugi::encoding_utf8, 3);
            writer->append(ruleWriter.result);

            const char *match = ruleNode.select_node("match").node().text().get();
            FindMacros(match, &macroList);
         }
      }
      writer->appendUtf8String("\t\t</rules>\n");

      // Export macros
      pugi::xml_node macros = xml.select_node("/parser/macros").node();
      if ((macroList.size() > 0) && (macros != nullptr))
      {
         writer->appendUtf8String("\t\t<macros>\n");
         for(int i = 0; i < macroList.size(); i++)
         {
            char *macroName = macroList.get(i);
            pugi::xml_node macro = macros.find_child_by_attribute("name", macroName);
            if (macro != nullptr)
            {
               xml_string_writer macroWriter;
               macro.print(macroWriter, "\t", pugi::format_default, pugi::encoding_utf8, 3);
               writer->append(macroWriter.result);
            }
            MemFree(macroName);
         }
         writer->appendUtf8String("\t\t</macros>\n");
      }

      // Export rule order
      writer->appendUtf8String("\t\t<order>\n");
      for (pugi::xml_node child : node.children())
      {
         const char *guid = child.attribute("guid").as_string();
         writer->appendUtf8String("\t\t\t<rule>");
         writer->appendUtf8String(guid);
         writer->appendUtf8String("</rule>\n");

      }
      writer->appendUtf8String("\t\t</order>\n");
   }

   MemFree(value);

   writer->append(endTag);
   return;
}

/**
 * Export log parser rule to JSON
 */
void ExportLogParserRule(const NXCPMessage& request, long countFieldId, long baseId, json_t *array, const wchar_t *logName)
{
   int count = request.getFieldAsUInt32(countFieldId);
   if (count == 0)
      return;
   
   char *value = ConfigReadCLOBUTF8(logName, nullptr);
   if (value == nullptr)
      return;

   pugi::xml_document xml;
   if (!xml.load_buffer(value, strlen(value)))
   {
      MemFree(value);
      return;
   }

   pugi::xml_node node = xml.select_node("/parser/rules").node();
   if (node != nullptr)
   {
      ObjectArray<char> macroList;
      uuid_t guid;
      json_t *rulesArray = json_array();
      
      for(int i = 0; i < count; i++)
      {
         request.getFieldAsBinary(baseId++, guid, UUID_LENGTH);
         char guidStr[64];
         _uuid_to_stringA(guid, guidStr);
         pugi::xml_node ruleNode = node.find_child_by_attribute("guid", guidStr);
         if (ruleNode != nullptr)
         {
            json_t *ruleObj = json_object();
            json_object_set_new(ruleObj, "guid", json_string(guidStr));
            
            // Export rule attributes
            if (auto name = ruleNode.attribute("name"))
               json_object_set_new(ruleObj, "name", json_string(name.value()));
            if (auto description = ruleNode.attribute("description"))
               json_object_set_new(ruleObj, "description", json_string(description.value()));
               
            // Export rule content (match, event, etc.)
            for (auto child : ruleNode.children())
            {
               json_object_set_new(ruleObj, child.name(), json_string(child.text().get()));
            }
            
            json_array_append_new(rulesArray, ruleObj);

            const char *match = ruleNode.select_node("match").node().text().get();
            FindMacros(match, &macroList);
         }
      }
      
      json_t *parserObj = json_object();
      json_object_set_new(parserObj, "name", json_string_t(logName));
      json_object_set_new(parserObj, "rules", rulesArray);

      // Export macros
      pugi::xml_node macros = xml.select_node("/parser/macros").node();
      if ((macroList.size() > 0) && (macros != nullptr))
      {
         json_t *macrosArray = json_array();
         for(int i = 0; i < macroList.size(); i++)
         {
            char *macroName = macroList.get(i);
            pugi::xml_node macro = macros.find_child_by_attribute("name", macroName);
            if (macro != nullptr)
            {
               json_t *macroObj = json_object();
               json_object_set_new(macroObj, "name", json_string(macroName));
               if (auto description = macro.attribute("description"))
                  json_object_set_new(macroObj, "description", json_string(description.value()));
               json_object_set_new(macroObj, "value", json_string(macro.text().get()));
               json_array_append_new(macrosArray, macroObj);
            }
            MemFree(macroName);
         }
         json_object_set_new(parserObj, "macros", macrosArray);
      }

      // Export rule order
      json_t *orderArray = json_array();
      for (auto child : node.children())
      {
         const char *guid = child.attribute("guid").as_string();
         json_array_append_new(orderArray, json_string(guid));
      }
      json_object_set_new(parserObj, "order", orderArray);
      
      json_array_append_new(array, parserObj);
   }

   MemFree(value);
   return;
}
