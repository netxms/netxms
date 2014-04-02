/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

package org.netxms.client.maps;

import java.util.HashSet;
import java.util.Set;
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
   private Set<String> mapList = new HashSet<String>();

   /**
    * Constructor for MapDCIInstance for table DCI
    * 
    * @param dciID id of required DCI
    * @param nodeID id of associated node. Is collected to fully fill DciValue instances
    * @param column column if DCI is a table DCI
    * @param instance instance if DCI is a table DCI
    * @param type type of DCI
    */
   public MapDCIInstance(long dciID, long nodeID, String column, String instance, int type, String mapId)
   {
      this.setDciID(dciID);
      this.setNodeID(nodeID);
      this.setColumn(column);
      this.setInstance(instance);
      this.type = type;
      mapList.add(mapId);
   }

   /**
    * Constructor for MapDCIInstance for simple DCI
    * 
    * @param dciID id of required DCI
    * @param nodeID id of associated node. Is collected to fully fill DciValue instances
    * @param type type of DCI
    */
   public MapDCIInstance(long dciID, long nodeID, int type, String mapId)
   {
      this.setDciID(dciID);
      this.setNodeID(nodeID);
      this.setColumn("");
      this.setInstance("");
      this.type = type;
      mapList.add(mapId);
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
         System.out.println("For empty Table type: " + column + instance);
         return;
      }
      msg.setVariableInt32(base++, (int)nodeID);
      msg.setVariableInt32(base++, (int)dciID);
      if(type == DataCollectionItem.DCO_TYPE_TABLE)
      {
         msg.setVariable(base++, column);
         msg.setVariable(base++, instance);         
      }
   }  
   
   
   /**
    * Compares to instances of this object
    * 
    * @param dciID if of 
    * @return 
    */
   public boolean equals(MapDCIInstance instance)
   {
      if(instance.getType() == getType())
         return false;
      
      boolean result = (instance.getDciID() == getDciID());
      
      if(result && (instance.getType() == DataCollectionItem.DCO_TYPE_TABLE))
      {
         result = (instance.getColumn().compareTo(getColumn()) == 0);
         result &= (instance.getInstance().compareTo(getInstance()) == 0);
      }
      
      return result;
   }
   
   public void addMap(String id)
   {
      mapList.add(id);
   }
   
   public boolean removeMap(String id)
   {
      mapList.remove(id);
      if(mapList.size() == 0)
         return true;
      return false;
   }
}
