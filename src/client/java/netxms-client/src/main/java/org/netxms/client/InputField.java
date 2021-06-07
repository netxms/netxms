/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.InputFieldType;

/**
 * Input field definition (used by object tools, object queries, etc.)
 */
public class InputField
{
   // Flags
   public static final int VALIDATE_PASSWORD = 0x0001;

   private String name;
   private InputFieldType type;
   private String displayName;
   private int orderNumber;
   private int flags;

   /**
    * Create text input field with default settings.
    * 
    * @param name field name
    */
   public InputField(String name)
   {
      this.name = name;
      this.type = InputFieldType.TEXT;
      this.displayName = name;
      this.orderNumber = 0;
      this.flags = 0;
   }

   /**
    * Create input field.
    * 
    * @param name field name
    * @param type field type
    * @param displayName field display name
    * @param flags field flags
    */
   public InputField(String name, InputFieldType type, String displayName, int flags)
   {
      this.name = name;
      this.type = type;
      this.displayName = displayName;
      this.orderNumber = 0;
      this.flags = flags;
   }

   /**
    * Copy constructor
    * 
    * @param src source object
    */
   public InputField(InputField src)
   {
      this.name = src.name;
      this.type = src.type;
      this.displayName = src.displayName;
      this.orderNumber = src.orderNumber;
      this.flags = src.flags;
   }

   /**
    * Create input field from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public InputField(NXCPMessage msg, long baseId)
   {
      name = msg.getFieldAsString(baseId);
      type = InputFieldType.getByValue(msg.getFieldAsInt32(baseId + 1));
      displayName = msg.getFieldAsString(baseId + 2);
      flags = msg.getFieldAsInt32(baseId + 3);
      orderNumber = msg.getFieldAsInt32(baseId + 4);
   }

   /**
    * Fill NXCP message with field data
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setField(baseId, name);
      msg.setFieldInt16(baseId + 1, type.getValue());
      msg.setField(baseId + 2, displayName);
      msg.setFieldInt32(baseId + 3, flags);
      msg.setFieldInt16(baseId + 4, orderNumber);
   }

   /**
    * Get field type.
    *
    * @return field type
    */
   public InputFieldType getType()
   {
      return type;
   }

   /**
    * Set field type.
    *
    * @param type new field type
    */
   public void setType(InputFieldType type)
   {
      this.type = type;
   }

   /**
    * @return the displayName
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @param displayName the displayName to set
    */
   public void setDisplayName(String displayName)
   {
      this.displayName = displayName;
   }

   /**
    * Check if password validation is needed (<code>VALIDATE_PASSWORD</code> flag is set).
    *
    * @return true if password validation is needed
    */
   public boolean isPasswordValidationNeeded()
   {
      return (flags & VALIDATE_PASSWORD) != 0;
   }

   /**
    * Get field flags.
    *
    * @return field flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * Set field flags.
    *
    * @param flags new field flags
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the sequence
    */
   public int getSequence()
   {
      return orderNumber;
   }

   /**
    * @param sequence the sequence to set
    */
   public void setSequence(int sequence)
   {
      this.orderNumber = sequence;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "InputField [name=" + name + ", type=" + type + ", displayName=" + displayName + ", orderNumber=" + orderNumber + ", flags=" + flags + "]";
   }
}
