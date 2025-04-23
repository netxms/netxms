/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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

package org.netxms.client.maps;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.NXCPMessage;
import org.netxms.client.datacollection.DataCollectionItem;

/**
 * Class that stores data of map DCI that's last values should be requested from server
 */
public class MapDCIInstance
{
   private long dciID;
   private long nodeID;
   private int type;
   private String column;
   private String instance;
   private Map<Long, Integer> mapObjectIdList = new HashMap<Long, Integer>();

   /**
    * Constructor for MapDCIInstance for table DCI
    *
    * @param dciID id of required DCI
    * @param nodeID id of associated node. Is collected to fully fill DciValue instances
    * @param column column if DCI is a table DCI
    * @param instance instance if DCI is a table DCI
    * @param type type of DCI
    * @param mapObjectId network map object ID
    */
   public MapDCIInstance(long dciID, long nodeID, String column, String instance, int type, long mapObjectId, int initialCount)
   {
      this.setDciID(dciID);
      this.setNodeID(nodeID);
      this.setColumn(column);
      this.setInstance(instance);
      this.type = type;
      mapObjectIdList.put(mapObjectId, initialCount);
   }

   /**
    * Constructor for MapDCIInstance for simple DCI
    *
    * @param dciID DCI ID
    * @param nodeID ID of associated node (needed to complete DciValue instances)
    * @param type type of DCI
    * @param mapObjectId network map object ID
    */
   public MapDCIInstance(long dciID, long nodeID, int type, long mapObjectId, int initialCount)
   {
      this.setDciID(dciID);
      this.setNodeID(nodeID);
      this.setColumn("");
      this.setInstance("");
      this.type = type;
      mapObjectIdList.put(mapObjectId, initialCount);
   }

   /**
    * Add map object ID to the list of map objects that should be processed. If same ID was already added, increase reference count.
    *
    * @param mapId map object ID
    */
   public void addMap(long mapId, int initialCount)
   {
      Integer count = mapObjectIdList.get(mapId);
      if (count != null)
         mapObjectIdList.put(mapId, count + 1);
      else
         mapObjectIdList.put(mapId, 1);
   }

   /**
    * Remove map object ID from the list of map objects that should be processed. Object will be removed from the list only if it's
    * reference count reaches 0.
    * 
    * @param mapId map object ID
    * @return true if map object ID list is empty (last element was removed)
    */
   public boolean removeMap(long mapId)
   {
      Integer count = mapObjectIdList.get(mapId);
      if (count != null)
      {
         if (count > 1)
            mapObjectIdList.put(mapId, count - 1);
         else
            mapObjectIdList.remove(mapId);
      }
      return mapObjectIdList.isEmpty();
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
    * @return the instance
    */
   public String getInstance()
   {
      return instance;
   }

   /**
    * @param instance the instance to set
    */
   public void setInstance(String instance)
   {
      this.instance = instance;
   }

   /**
    * @return the dciID
    */
   public long getDciID()
   {
      return dciID;
   }

   /**
    * @param dciID the dciID to set
    */
   public void setDciID(long dciID)
   {
      this.dciID = dciID;
   }

   /**
    * @return the type
    */
   public int getType()
   {
      return type;
   }

   /**
    * @param type the type to set
    */
   public void setType(int type)
   {
      this.type = type;
   }

   /**
    * @return the nodeID
    */
   public long getNodeID()
   {
      return nodeID;
   }

   /**
    * @param nodeID the nodeID to set
    */
   public void setNodeID(long nodeID)
   {
      this.nodeID = nodeID;
   }

   /**
    * This method fills message with data if all required data present. In case of table DCI if column or/and instance are not
    * present, then this DCI value will not be requested.
    *
    * @param msg Message that should be populated with data
    * @param base the base of this data
    */
   public void fillMessage(NXCPMessage msg, long base)
   {
      if (type == DataCollectionItem.DCO_TYPE_TABLE && (column.isEmpty() || instance.isEmpty()))
      {
         return;
      }
      msg.setFieldInt32(base++, (int)nodeID);
      msg.setFieldInt32(base++, (int)dciID);
      msg.setFieldInt32(base++, mapObjectIdList.entrySet().iterator().next().getKey().intValue());
      if (type == DataCollectionItem.DCO_TYPE_TABLE)
      {
         msg.setField(base++, column);
         msg.setField(base++, instance);
      }
   }

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + (((column == null) || (type == DataCollectionItem.DCO_TYPE_TABLE)) ? 0 : column.hashCode());
      result = prime * result + (int)(dciID ^ (dciID >>> 32));
      result = prime * result + (((instance == null) || (type == DataCollectionItem.DCO_TYPE_TABLE)) ? 0 : instance.hashCode());
      result = prime * result + (int)(nodeID ^ (nodeID >>> 32));
      result = prime * result + type;
      return result;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object object)
   {
      if (object == this)
         return true;

      if ((object == null) || !(object instanceof MapDCIInstance))
         return false;

      MapDCIInstance i = (MapDCIInstance)object;
      if (i.type != type)
         return false;

      boolean result = (i.dciID == dciID);

      if (result && (i.type == DataCollectionItem.DCO_TYPE_TABLE))
      {
         result = i.column.equals(column) && i.instance.equals(instance);
      }

      return result;
   }
}
