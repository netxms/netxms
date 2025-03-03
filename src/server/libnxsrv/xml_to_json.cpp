/*
** NetXMS - Network Management System
** Server Library
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
** File: xml_to_json.cpp
**
**/

#include "libnxsrv.h"
#include <xml_to_json.h>


/**
 * Convert string to int if applicable
 */
static void ConvertValueXmlToJson(json_t *object, const char *name, const char *text)
{
   char *eptr;
   int32_t val = strtol(text, &eptr, 10);
   if (*eptr == 0 && strlen(text) > 0)
   {
      json_object_set_new(object, name, json_integer(val));
   }
   else
   {
      json_object_set_new(object, name, json_string(text));
   }
}

/**
 * Function to convert an XML node to a JSON object
 */
json_t LIBNXSRV_EXPORTABLE *XmlNodeToJson(const pugi::xml_node &node)
{
   json_t *jsonObject = json_object();

   for (pugi::xml_node child : node.children())
   {
      std::string childName = child.name();

      if (childName.empty() || child.first_child().empty())
      {
         continue;
      }

      //special processing of exceptional elements
      if (!strcmp(childName.c_str(), "TextBox") || !strcmp(childName.c_str(), "DCIList"))
      {
         pugi::xml_document xmlDoc;
         if (!xmlDoc.load_buffer(child.child_value(), strlen(child.child_value())))
         {
            json_object_set_new(jsonObject, child.name(), json_string(child.child_value()));
            continue; //ignore if failed to parse
         }

         pugi::xml_node xml = xmlDoc.first_child();
         if (!strcmp(childName.c_str(), "DCIList"))
         {
            if ((xml.child("config") != nullptr))
            {
               xml = xml.child("config");
            }
            if ((xml.child("dciImageConfiguration") != nullptr))
            {
               xml = xml.child("dciImageConfiguration");
            }
         }
         json_t *converted = XmlNodeToJson(xml);
         char *jsonText = json_dumps(converted, JSON_COMPACT);
         json_object_set_new(jsonObject, child.name(), json_string(jsonText));
         MemFree(jsonText);
         json_decref(converted);
      }
      else if ((child.attribute("length") != nullptr) ||
               child.attribute("lenght") != nullptr) // fix for spelling error in original XML ("lengHt")
      {
         if (strlen(child.child_value()) > 0)
         {
            String s(child.child_value(), "utf8");
            StringList values = s.split(_T(","), true);
            IntegerArray<int32_t> arr;
            for (int i = 0; i < values.size(); i++)
            {
               TCHAR *eptr;
               int32_t val = _tcstol(values.get(i), &eptr, 10);
               if (*eptr == 0)
               {
                  arr.add(val);
               }
            }
            json_object_set_new(jsonObject, child.name(), arr.toJson());
         }
         else
         {
            json_t *array = json_array();
            for (pugi::xml_node grandChild : child.children())
            {
               if (!strcmp(grandChild.name(), "int") || !strcmp(grandChild.name(), "long"))
               {
                  const char *text = grandChild.child_value();

                  char *eptr;
                  int32_t val = strtol(text, &eptr, 10);
                  if (*eptr == 0 && strlen(text) > 0)
                  {
                     json_array_append_new(array, json_integer(val));
                  }
               }
               else if (!strcmp(grandChild.name(), "string"))
               {
                  const char *text = grandChild.child_value();
                  if (strlen(text) > 0)
                  {
                     json_array_append_new(array, json_string(text));
                  }
               }
               else
                  json_array_append_new(array, XmlNodeToJson(grandChild));
            }
            json_object_set_new(jsonObject, childName.c_str(), array);
         }
      }
      else if (child.attribute("class") != nullptr)
      {
         json_t *array = json_array();
         for (pugi::xml_node grandChild : child.children())
         {
            if (!strcmp(grandChild.name(), "int") || !strcmp(grandChild.name(), "long"))
            {
               const char *text = grandChild.child_value();

               char *eptr;
               int32_t val = strtol(text, &eptr, 10);
               if (*eptr == 0 && strlen(text) > 0)
               {
                  json_array_append_new(array, json_integer(val));
               }
            }
            else if (!strcmp(grandChild.name(), "string"))
            {
               const char *text = grandChild.child_value();
               if (strlen(text) > 0)
               {
                  json_array_append_new(array, json_string(text));
               }
            }
            else
               json_array_append_new(array, XmlNodeToJson(grandChild));
         }
         json_object_set_new(jsonObject, childName.c_str(), array);
      }
      else if (child.first_child() == child.last_child())
      {
         ConvertValueXmlToJson(jsonObject, childName.c_str(), child.child_value());
      }
      else
      {
         json_t *object = XmlNodeToJson(child);
         for (pugi::xml_attribute attr : child.attributes())
         {
            if (strcmp(attr.name(), "class") == 0)
               continue;

            ConvertValueXmlToJson(object, attr.name(), attr.value());
         }
         json_object_set_new(jsonObject, childName.c_str(), object);
      }
   }

   for (pugi::xml_attribute attr : node.attributes())
   {
      ConvertValueXmlToJson(jsonObject, attr.name(), attr.value());
   }

   return jsonObject;
}
