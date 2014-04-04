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
package org.netxms.client.maps.configs;

import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * DCI information for map line
 */
@Root(name="dci")
public class SingleDciConfig
{
	public static final String UNSET_COLOR = "UNSET"; //$NON-NLS-1$
	
	public static final int ITEM = DataCollectionObject.DCO_TYPE_ITEM;
	public static final int TABLE = DataCollectionObject.DCO_TYPE_TABLE;
	
	@Attribute
	private long nodeId;
	
	@Attribute
	public long dciId;
	
	@Element(required=false)
	public int type;

	@Element(required=false)
	public String name;

	@Element(required=false)
	public String instance;
	
	@Element(required=false)
	public String column;
	
   @Element(required=false)
   public String formatString;   

	
	/**
	 * Default constructor
	 */
	public SingleDciConfig()
	{
		nodeId = 0;
		dciId = 0;
		type = ITEM;
		name = ""; //$NON-NLS-1$
		setInstance(""); //$NON-NLS-1$
		setColumn(""); //$NON-NLS-1$
	}

	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public SingleDciConfig(SingleDciConfig src)
	{
		this.nodeId = src.nodeId;
		this.dciId = src.dciId;
		this.type = src.type;
		this.name = src.name;
		this.formatString = src.formatString;
		this.setInstance(src.getInstance());
		this.setColumn(src.getColumn());
	}

	/**
	 * Create DCI info from DciValue object
	 * 
	 * @param dci
	 */
	public SingleDciConfig(DciValue dci)
	{
		nodeId = dci.getNodeId();
		dciId = dci.getId();
		type = dci.getDcObjectType();
		name = dci.getDescription();
		formatString = "";
		setInstance(""); //$NON-NLS-1$
		setColumn(""); //$NON-NLS-1$
	}
	
	/**
	 * Get DCI name. Always returns non-empty string.
	 * @return
	 */
	public String getName()
	{
		return ((name != null) && !name.isEmpty()) ? name : ("[" + Long.toString(dciId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
	}
	
   /**
    * @return the formatString
    */
   public String getFormatString()
   {
      return formatString == null ? "" : formatString;
   }

   /**
    * @param formatString the formatString to set
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
}
