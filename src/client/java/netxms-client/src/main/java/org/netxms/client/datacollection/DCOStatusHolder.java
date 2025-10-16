/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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

import org.netxms.client.constants.DataCollectionObjectStatus;

/**
 * Class that stores DCO id and new status. Used in update notifications.
 */
public class DCOStatusHolder
{
   private long[] items;
   private DataCollectionObjectStatus status;

   /**
    * Status holder constructor
    *
    * @param items TODO
    * @param status TODO
    */
   public DCOStatusHolder(long[] items, DataCollectionObjectStatus status)
   {
      super();
      this.items = items;
      this.status = status;
   }

   /**
    * @return the dciId
    */
   public long[] getDciIdArray()
   {
      return items;
   }

   /**
    * TODO
    *
    * @param items TODO
    */
   public void setDciIdArray(long[] items)
   {
      this.items = items;
   }

   /**
    * @return the status
    */
   public DataCollectionObjectStatus getStatus()
   {
      return status;
   }

   /**
    * @param status the status to set
    */
   public void setStatus(DataCollectionObjectStatus status)
   {
      this.status = status;
   }
}
