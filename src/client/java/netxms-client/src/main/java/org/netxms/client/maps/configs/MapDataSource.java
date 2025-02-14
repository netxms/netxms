/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.client.maps.configs;

import org.netxms.base.NXCPMessage;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.interfaces.NodeItemPair;

/**
 * DCI information for map line
 */
public class MapDataSource implements NodeItemPair
{	
	public static final int ITEM = DataCollectionObject.DCO_TYPE_ITEM;
	public static final int TABLE = DataCollectionObject.DCO_TYPE_TABLE;

	protected long nodeId;
	protected long dciId;
	protected int type;
	protected String instance;
	protected String column;
	protected String formatString;

	/**
	 * Default constructor
	 */
	public MapDataSource()
	{
		nodeId = 0;
		dciId = 0;
		setType(ITEM);
      setInstance("");
      setColumn("");
      setFormatString("");
	}

	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public MapDataSource(MapDataSource src)
	{
		this.nodeId = src.nodeId;
		this.dciId = src.dciId;
		this.setType(src.getType());
		this.formatString = src.formatString;
		this.setInstance(src.getInstance());
		this.setColumn(src.getColumn());
	}	  

	/**
	 * Create DCI info from DciValue object
	 * 
	 * @param dci source DciValue object
	 */
	public MapDataSource(DciValue dci)
	{
		nodeId = dci.getNodeId();
		dciId = dci.getId();
		setType(dci.getDcObjectType());
		formatString = "";
      setInstance("");
      setColumn("");
	}

   /**
    * Create DCI info for node/DCI ID pair
    *
    * @param nodeId node ID
    * @param dciId DCI ID
    */
   public MapDataSource(long nodeId, long dciId)
   {
      this.nodeId = nodeId;
      this.dciId = dciId;
      setType(ITEM);
      setInstance("");
      setColumn("");
      setFormatString("");
   }

	/**
	 * Fill NXCP message with object data
	 *
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
	   if ((type == DataCollectionItem.DCO_TYPE_TABLE) && (column.isEmpty() || instance.isEmpty()))
	      return;

	   msg.setFieldInt32(baseId++, (int)nodeId);
	   msg.setFieldInt32(baseId++, (int)dciId);
	   if (type == DataCollectionItem.DCO_TYPE_TABLE)
	   {
	      msg.setField(baseId++, column);
	      msg.setField(baseId++, instance);
	   }
	}

   /**
    * Get format string.
    *
    * @return format string
    */
   public String getFormatString()
   {
      return formatString == null ? "" : formatString;
   }

   /**
    * Set format string.
    *
    * @param formatString new format string
    */
   public void setFormatString(String formatString)
   {
      this.formatString = formatString;
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
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @param nodeId the nodeId to set
    */
   public void setNodeId(long nodeId)
   {
      this.nodeId = nodeId;
   }

   /**
    * @return the dciId
    */
   public long getDciId()
   {
      return dciId;
   }

   /**
    * @param dciId the dciId to set
    */
   public void setDciId(long dciId)
   {
      this.dciId = dciId;
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
}
