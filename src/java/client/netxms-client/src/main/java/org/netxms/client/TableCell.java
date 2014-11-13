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

/**
 * Table cell
 */
public class TableCell
{
   private String value;
   private int status;
   
   /**
    * @param value
    */
   public TableCell(String value)
   {
      this.value = value;
      this.status = -1;
   }

   /**
    * @param value
    * @param status
    */
   public TableCell(String value, int status)
   {
      this.value = value;
      this.status = status;
   }
   
   /**
    * @param src
    */
   public TableCell(TableCell src)
   {
      value = src.value;
      status = src.status;
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

   /**
    * @return the status
    */
   public int getStatus()
   {
      return status;
   }

   /**
    * @param status the status to set
    */
   public void setStatus(int status)
   {
      this.status = status;
   }

   @Override
   public String toString() {
      return "TableCell{" +
              "value='" + value + '\'' +
              ", status=" + status +
              '}';
   }
}
