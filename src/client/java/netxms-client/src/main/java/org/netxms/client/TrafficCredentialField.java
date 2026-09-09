/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Credential field descriptor declared by a traffic connector. Describes one key of the
 * observer's credentials JSON so clients can render a form instead of a raw JSON editor.
 * Password fields are never sent back to clients and are kept on the server when a
 * modification leaves them empty.
 */
public class TrafficCredentialField
{
   /**
    * Field type (values match TrafficCredentialFieldType on the server)
    */
   public enum Type
   {
      STRING, PASSWORD, INTEGER, BOOLEAN;

      /**
       * Get type from NXCP integer code.
       *
       * @param code integer code
       * @return type (STRING for unknown codes)
       */
      public static Type fromCode(int code)
      {
         Type[] values = values();
         return ((code >= 0) && (code < values.length)) ? values[code] : STRING;
      }
   }

   private String name;
   private String displayName;
   private String description;
   private Type type;
   private boolean required;
   private String defaultValue;

   /**
    * Create credential field descriptor from NXCP message. Record stride is 16 fields:
    * baseId = name, +1 = display name, +2 = description, +3 = type code, +4 = required,
    * +5 = default value (remaining fields reserved).
    *
    * @param msg NXCP message
    * @param baseId base field ID of this record
    */
   public TrafficCredentialField(final NXCPMessage msg, final long baseId)
   {
      name = msg.getFieldAsString(baseId);
      displayName = msg.getFieldAsString(baseId + 1);
      description = msg.getFieldAsString(baseId + 2);
      type = Type.fromCode(msg.getFieldAsInt32(baseId + 3));
      required = msg.getFieldAsBoolean(baseId + 4);
      defaultValue = msg.getFieldAsString(baseId + 5);
      if ((defaultValue != null) && defaultValue.isEmpty())
         defaultValue = null;
   }

   /**
    * @return key in credentials JSON
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return display name
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @return description (may be empty)
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return field type
    */
   public Type getType()
   {
      return type;
   }

   /**
    * @return true if a value is required
    */
   public boolean isRequired()
   {
      return required;
   }

   /**
    * @return textual default value or null if none
    */
   public String getDefaultValue()
   {
      return defaultValue;
   }
}
