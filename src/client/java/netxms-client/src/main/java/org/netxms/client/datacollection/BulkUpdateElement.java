/**
 * NetXMS - open source network management system
 * Copyright (C) 2020 Raden Solutions
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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * Class that contains information about edited element
 */
public class BulkUpdateElement
{
   protected Object value;
   private long fieldId;
   
   public BulkUpdateElement(long fieldId)
   {
      this.fieldId = fieldId;
      value = null;
   }
   
   /**
    * @param value2 the value to set
    */
   public void setValue(Object value)
   {
      this.value = value;
   }

   /**
    * Returns if filed is modified
    * 
    * @return true if field is modified
    */
   public boolean isModified()
   {
      return value != null && !(value instanceof String && ((String)value).isEmpty()) && !(value instanceof Integer && ((Integer)value) == -1);
   }

   /**
    * Set updated field to message
    * 
    * @param msg NXCPMessage to update information on server
    */
   public void setField(NXCPMessage msg)
   {
      if (!isModified())
         return;

      if (value instanceof String)
      {
         msg.setField(fieldId, (String)value);         
      }
      else if (value instanceof Integer)
      {
         msg.setFieldInt32(fieldId, (Integer)value);         
      }
   }
   
   
}
