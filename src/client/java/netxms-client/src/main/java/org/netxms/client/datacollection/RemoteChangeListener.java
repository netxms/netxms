/**
 * NetXMS - open source network management system
 * Copyright (C) 2018-2025 Raden Solutions
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
 * Callback to notify View about object change
 */
public interface RemoteChangeListener
{
   /**
    * Called when data collection object updated.
    * 
    * @param object data collection object
    */
   void onUpdate(DataCollectionObject object);
   
   /**
    * Called when data collection object is deleted
    * 
    * @param id data collection object ID
    */
   void onDelete(long id);
   
   /**
    * Called when state of data collection object was changed
    * 
    * @param id data collection object ID
    * @param status new status of data collection object
    */
   void onStatusChange(long id, DataCollectionObjectStatus status);
}
