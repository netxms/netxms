/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.client.log;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Log record details
 */
public class LogRecordDetails
{
   private long id;
   private List<LogColumn> columns;
   private List<String> values;

   /**
    * Create new object from server response
    */
   protected LogRecordDetails(long id, NXCPMessage msg)
   {
      this.id = id;

      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
      columns = new ArrayList<LogColumn>(count);
      values = new ArrayList<String>(count);

      long fieldIdMetadata = NXCPCodes.VID_COLUMN_INFO_BASE;
      long fieldIdData = NXCPCodes.VID_TABLE_DATA_BASE;
      for(int i = 0; i < count; i++, fieldIdMetadata += 10)
      {
         columns.add(new LogColumn(msg, fieldIdMetadata));
         values.add(msg.getFieldAsString(fieldIdData++));
      }
   }

   /**
    * Get ID of this record.
    * 
    * @return ID of this record
    */
   public long getId()
   {
      return id;
   }

   /**
    * Get number of "details" columns in this record.
    * 
    * @return number of "details" columns in this record
    */
   public int getColumnCount()
   {
      return columns.size();
   }

   /**
    * Get definitions for "details" columns.
    * 
    * @return definitions for "details" columns
    */
   public List<LogColumn> getColumnDefinitions()
   {
      return columns;
   }

   /**
    * Get index of column with given name.
    * 
    * @param name column name
    * @return column index or -1
    */
   public int getColumnIndex(String name)
   {
      for(int i = 0; i < columns.size(); i++)
      {
         if (columns.get(i).getName().equalsIgnoreCase(name))
            return i;
      }
      return -1;
   }

   /**
    * Get values in "details" columns.
    * 
    * @return values in "details" columns
    */
   public List<String> getValues()
   {
      return values;
   }

   /**
    * Get value for column with given index.
    * 
    * @param index column index
    * @return value for column with given index or null
    */
   public String getValue(int index)
   {
      return values.get(index);
   }

   /**
    * Get value for column with given name.
    * 
    * @param columnName column name
    * @return value for column with given name or null
    */
   public String getValue(String columnName)
   {
      return values.get(getColumnIndex(columnName));
   }
}
