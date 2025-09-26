/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package com.netxms.mcp.tools;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Server tool
 */
public abstract class ServerTool
{
   public abstract String getName();

   public abstract String getDescription();

   public abstract String execute(Map<String, Object> args) throws Exception;

   public String getSchema()
   {
      return "{\"type\": \"object\", \"properties\": {}, \"required\": [] }";
   }

   /**
    * Builder class for creating MCP tool input schemas
    */
   protected static class SchemaBuilder
   {
      private final Map<String, ArgumentDefinition> properties;
      private final List<String> required;

      public SchemaBuilder()
      {
         this.properties = new HashMap<>();
         this.required = new ArrayList<>();
      }

      /**
       * Add an input argument to the schema
       * 
       * @param name the argument name
       * @param dataType the data type (string, number, integer, boolean, array, object)
       * @param description the argument description
       * @param mandatory whether the argument is required
       * @return this builder instance for method chaining
       */
      public SchemaBuilder addArgument(String name, String dataType, String description, boolean mandatory)
      {
         properties.put(name, new ArgumentDefinition(dataType, description));
         if (mandatory)
         {
            required.add(name);
         }
         return this;
      }

      /**
       * Add a string argument
       */
      public SchemaBuilder addStringArgument(String name, String description, boolean mandatory)
      {
         return addArgument(name, "string", description, mandatory);
      }

      /**
       * Add a number argument
       */
      public SchemaBuilder addNumberArgument(String name, String description, boolean mandatory)
      {
         return addArgument(name, "number", description, mandatory);
      }

      /**
       * Add an integer argument
       */
      public SchemaBuilder addIntegerArgument(String name, String description, boolean mandatory)
      {
         return addArgument(name, "integer", description, mandatory);
      }

      /**
       * Add a boolean argument
       */
      public SchemaBuilder addBooleanArgument(String name, String description, boolean mandatory)
      {
         return addArgument(name, "boolean", description, mandatory);
      }

      /**
       * Build the JSON schema string
       * 
       * @return JSON schema as string
       */
      public String build()
      {
         StringBuilder sb = new StringBuilder();
         sb.append("{\"type\": \"object\", \"properties\": {");

         boolean first = true;
         for (Map.Entry<String, ArgumentDefinition> entry : properties.entrySet())
         {
            if (!first)
            {
               sb.append(", ");
            }
            first = false;

            sb.append("\"").append(entry.getKey()).append("\": {");
            sb.append("\"type\": \"").append(entry.getValue().type).append("\"");
            if (entry.getValue().description != null && !entry.getValue().description.isEmpty())
            {
               sb.append(", \"description\": \"").append(escapeJsonString(entry.getValue().description)).append("\"");
            }
            sb.append("}");
         }

         sb.append("}, \"required\": [");
         first = true;
         for (String requiredField : required)
         {
            if (!first)
            {
               sb.append(", ");
            }
            first = false;
            sb.append("\"").append(requiredField).append("\"");
         }
         sb.append("]}");

         return sb.toString();
      }

      private String escapeJsonString(String str)
      {
         return str.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t");
      }

      /**
       * Internal class to hold argument definition
       */
      private static class ArgumentDefinition
      {
         final String type;
         final String description;

         ArgumentDefinition(String type, String description)
         {
            this.type = type;
            this.description = description;
         }
      }
   }

   /**
    * Create a new schema builder instance
    * 
    * @return new SchemaBuilder instance
    */
   protected SchemaBuilder createSchemaBuilder()
   {
      return new SchemaBuilder();
   }
}