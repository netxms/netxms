/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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

import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * DCI information for chart
 */
@Root(name="dci", strict=false)
public class ChartDciConfig
{
	public static final String UNSET_COLOR = "UNSET"; //$NON-NLS-1$
	
	public static final int ITEM = DataCollectionObject.DCO_TYPE_ITEM;
	public static final int TABLE = DataCollectionObject.DCO_TYPE_TABLE;
	
	// display types
	public static final int DEFAULT = 0;
   public static final int LINE = 1;
   public static final int AREA = 2;
	
	@Attribute
	public long nodeId;
	
	@Attribute
	public long dciId;

   @Element(required=false)
	public String dciName;

   @Element(required=false)
   public String dciDescription;
	
	@Element(required=false)
	public int type;

   @Element(required=false)
	public String color;

	@Element(required=false)
	public String name;
	
	@Element(required=false)
	public int lineWidth;
	
	@Element(required=false)
	public int displayType;
	
	@Element(required=false)
	public boolean area;
	
	@Element(required=false)
	public boolean showThresholds;

   @Element(required=false)
   public boolean invertValues;

   @Element(required=false)
   public boolean multiMatch;

	@Element(required=false)
	public String instance;
	
	@Element(required=false)
	public String column;
	
   @Element(required=false)
   public String displayFormat;
   
	/**
	 * Default constructor
	 */
	public ChartDciConfig()
	{
		nodeId = 0;
		dciId = 0;
		dciName = ""; //$NON-NLS-1$
		dciDescription = ""; //$NON-NLS-1$
		type = ITEM;
		color = UNSET_COLOR;
		name = ""; //$NON-NLS-1$
		lineWidth = 2;
		displayType = DEFAULT;
		area = false;
		showThresholds = false;
		invertValues = false;
		multiMatch = false;
		instance = ""; //$NON-NLS-1$
		column = ""; //$NON-NLS-1$
      displayFormat = "%s"; //$NON-NLS-1$
	}

	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ChartDciConfig(ChartDciConfig src)
	{
		this.nodeId = src.nodeId;
		this.dciId = src.dciId;
      this.dciName = src.dciName;
      this.dciDescription = src.dciDescription;
		this.type = src.type;
		this.color = src.color;
		this.name = src.name;
		this.lineWidth = src.lineWidth;
		this.displayType = src.displayType;
		this.area = src.area;
		this.showThresholds = src.showThresholds;
		this.invertValues = src.invertValues;
		this.multiMatch = src.multiMatch;
		this.instance = src.instance;
		this.column = src.column;
		this.displayFormat = src.displayFormat;
	}

	/**
	 * Create DCI info from DciValue object
	 * 
	 * @param dci The DciValue
	 */
	public ChartDciConfig(DciValue dci)
	{
		nodeId = dci.getNodeId();
		dciId = dci.getId();
      dciName = dci.getName();
      dciDescription = dci.getDescription();
		type = dci.getDcObjectType();
		name = dci.getDescription();
		color = UNSET_COLOR;
		lineWidth = 2;
		area = false;
		showThresholds = false;
		invertValues = false;
		multiMatch = false;
		instance = ""; //$NON-NLS-1$
		column = ""; //$NON-NLS-1$
      displayFormat = "%s"; //$NON-NLS-1$
	}

	/**
	 * @return the color
	 */
	public int getColorAsInt()
	{
		if (color.equals(UNSET_COLOR))
			return -1;
		if (color.startsWith("0x")) //$NON-NLS-1$
			return Integer.parseInt(color.substring(2), 16);
		return Integer.parseInt(color, 10);
	}

	/**
	 * @param value The colour value
	 */
	public void setColor(int value)
	{
		color = "0x" + Integer.toHexString(value); //$NON-NLS-1$
	}
	
	/**
	 * Get DCI name. Always returns non-empty string.
	 * @return The name
	 */
	public String getName()
	{
		return ((name != null) && !name.isEmpty()) ? name : ("[" + Long.toString(dciId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
	}
	
	/**
	 * Get display format
	 * 
	 * @return The display format
	 */
	public String getDisplayFormat()
	{
	   return ((displayFormat != null) && !displayFormat.isEmpty()) ? displayFormat : "%s"; //$NON-NLS-1$
	}
	
	/**
	 * Get display type
	 * 
	 * @return The display type
	 */
	public int getDisplayType()
	{
	   return ((displayType == DEFAULT) && area) ? AREA : displayType;
	}

   /**
    * @return the dciName
    */
   public String getDciName()
   {
      return dciName == null ? "" : dciName;
   }

   /**
    * @param dciName the dciName to set
    */
   public void setDciName(String dciName)
   {
      this.dciName = dciName;
   }

   /**
    * @return the dciDescription
    */
   public String getDciDescription()
   {
      return dciDescription == null ? "" : dciDescription;
   }

   /**
    * @param dciDescription the dciDescription to set
    */
   public void setDciDescription(String dciDescription)
   {
      this.dciDescription = dciDescription;
   }   


   /* (non-Javadoc)
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object arg0)
   {
      if(arg0 instanceof ChartDciConfig)
      {
         return dciId == ((ChartDciConfig)arg0).dciId;
      }
      
      return super.equals(arg0);
   }

   /* (non-Javadoc)
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      return Long.valueOf(dciId).hashCode();
   }
}
