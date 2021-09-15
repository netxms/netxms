/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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

import java.util.ArrayList;
import java.util.List;

/**
 * Table row
 */
public class TableRow
{
   private List<TableCell> cells;
   private long objectId;
   private int baseRow;
   
   /**
    * Create new row
    * 
    * @param rowCount The amount of new rows to create
    */
   public TableRow(int rowCount)
   {
      objectId = 0;
      baseRow = -1;
      cells = new ArrayList<TableCell>(rowCount);
      for(int i = 0; i < rowCount; i++)
         cells.add(new TableCell(""));
   }
   
   /**
    * Copy constructor
    * 
    * @param src The TableRow source object
    */
   public TableRow(TableRow src)
   {
      objectId = src.objectId;
      baseRow = src.baseRow;
      cells = new ArrayList<TableCell>(src.cells.size());
      for(int i = 0; i < src.cells.size(); i++)
         cells.add(new TableCell(src.get(i)));
   }
   
   /**
    * Get table cell object for given column.
    *
    * @param column Column index
    * @return table Cell at given index
    * @throws IndexOutOfBoundsException If the index is out of bounds
    */
   public TableCell get(int column) throws IndexOutOfBoundsException
   {
      return cells.get(column);
   }

   /**
    * Get value for given column as string. If column index is out of range returns null.
    *
    * @param column column index
    * @return column value or null
    */
   public String getValue(int column)
   {
      try
      {
         return cells.get(column).getValue();
      }
      catch(IndexOutOfBoundsException e)
      {
         return null;
      }
   }

   /**
    * Get value for given column as long integer. If column index is out of range or cannot be interpreted as long integer returns
    * 0.
    *
    * @param column column index
    * @return column value interpreted as long integer or 0
    */
   public long getValueAsLong(int column)
   {
      try
      {
         return cells.get(column).getValueAsLong();
      }
      catch(IndexOutOfBoundsException e)
      {
         return 0;
      }
   }

   /**
    * Get value for given column as integer. If column index is out of range or cannot be interpreted as integer returns 0.
    *
    * @param column column index
    * @return column value interpreted as integer or 0
    */
   public int getValueAsInteger(int column)
   {
      try
      {
         return cells.get(column).getValueAsInteger();
      }
      catch(IndexOutOfBoundsException e)
      {
         return 0;
      }
   }

   /**
    * Get value for given column as floating point number. If column index is out of range or cannot be interpreted as floating
    * point number returns 0.
    *
    * @param column column index
    * @return column value interpreted as floating point number or 0
    */
   public double getValueAsDouble(int column)
   {
      try
      {
         return cells.get(column).getValueAsDouble();
      }
      catch(IndexOutOfBoundsException e)
      {
         return 0;
      }
   }

   /**
    * @return The amount of cells
    */
   public int size()
   {
      return cells.size();
   }

   /**
    * @param msg The NXCPMessage
    * @param baseId The base ID
    * @param extendedFormat True if extended format
    * @return The cell ID
    */
   public long fillMessage(final NXCPMessage msg, long baseId, boolean extendedFormat)
   {
      long varId = baseId;
      if (extendedFormat)
      {
         msg.setFieldInt32(varId++, (int)objectId);
         msg.setFieldInt32(varId++, baseRow);
         varId += 8;
      }
      for(TableCell c : cells)
      {
         msg.setField(varId++, c.getValue());
         if (extendedFormat)
         {
            msg.setFieldInt16(varId++, c.getStatus());
            msg.setFieldInt32(varId++, (int)c.getObjectId());
            varId += 7;
         }
      }
      return varId;
   }

   /**
    * Get associated object ID.
    * 
    * @return associated object ID
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * Set associated object ID.
    * 
    * @param objectId associated object ID to set
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   }

   /**
    * @return the baseRow
    */
   public int getBaseRow()
   {
      return baseRow;
   }

   /**
    * @param baseRow the baseRow to set
    */
   public void setBaseRow(int baseRow)
   {
      this.baseRow = baseRow;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString() {
      return "TableRow{" +
              "cells=" + cells +
              ", objectId=" + objectId +
              ", baseRow=" + baseRow +
              '}';
   }
}
