/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * Condition for table DCI threshold
 */
public class TableCondition
{
   private String column;
   private int operation;
   private String value;

   /**
    * Create new condition object.
    *
    * @param column column name
    * @param operation operation
    * @param value value for operation
    */
   public TableCondition(String column, int operation, String value)
   {
      this.column = column;
      this.operation = operation;
      this.value = value;
   }

   /**
    * Copy constructor
    *
    * @param src source object
    */
   public TableCondition(TableCondition src)
   {
      column = src.column;
      operation = src.operation;
      value = src.value;
   }

   /**
    * Create condition from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected TableCondition(NXCPMessage msg, long baseId)
   {
      column = msg.getFieldAsString(baseId);
      operation = msg.getFieldAsInt32(baseId + 1);
      value = msg.getFieldAsString(baseId + 2);
   }

   /**
    * @return the column
    */
   public String getColumn()
   {
      return column;
   }

   /**
    * @param column the column to set
    */
   public void setColumn(String column)
   {
      this.column = column;
   }

   /**
    * @return the operation
    */
   public int getOperation()
   {
      return operation;
   }

   /**
    * @param operation the operation to set
    */
   public void setOperation(int operation)
   {
      this.operation = operation;
   }

   /**
    * @return the value
    */
   public String getValue()
   {
      return value;
   }

   /**
    * @param value the value to set
    */
   public void setValue(String value)
   {
      this.value = value;
   }
}
