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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;

/**
 * Table row
 */
public class TableRow
{
   private List<TableCell> cells;
   private long objectId;
   
   /**
    * Create new row
    * 
    * @param rowCount
    */
   public TableRow(int rowCount)
   {
      objectId = 0;
      cells = new ArrayList<TableCell>(rowCount);
      for(int i = 0; i < rowCount; i++)
         cells.add(new TableCell(""));
   }
   
   /**
    * Copy constructor
    * 
    * @param src
    */
   public TableRow(TableRow src)
   {
      objectId = src.objectId;
      cells = new ArrayList<TableCell>(src.cells.size());
      for(int i = 0; i < src.cells.size(); i++)
         cells.add(new TableCell(src.get(i)));
   }
   
   /**
    * @param column
    * @return
    * @throws ArrayIndexOutOfBoundsException
    */
   public TableCell get(int column) throws ArrayIndexOutOfBoundsException
   {
      return cells.get(column);
   }
   
   /**
    * @return
    */
   public int size()
   {
      return cells.size();
   }
   
   /**
    * @param msg
    * @param baseId
    * @return
    */
   public long fillMessage(final NXCPMessage msg, long baseId, boolean extendedFormat)
   {
      long varId = baseId;
      if (extendedFormat)
      {
         msg.setVariableInt32(varId++, (int)objectId);
         varId += 9;
      }
      for(TableCell c : cells)
      {
         msg.setVariable(varId++, c.getValue());
         if (extendedFormat)
         {
            msg.setVariableInt16(varId++, c.getStatus());
            varId += 8;
         }
      }
      return varId;
   }

   /**
    * @return the objectId
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @param objectId the objectId to set
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   }
}
