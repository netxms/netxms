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
package org.netxms.client.datacollection;

import org.netxms.client.objects.interfaces.NodeItemPair;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * DCI information for chart
 */
@Root(name="dci", strict=false)
public class ChartDciConfig implements NodeItemPair
{
   public static final String UNSET_COLOR = "UNSET";

	public static final int ITEM = DataCollectionObject.DCO_TYPE_ITEM;
	public static final int TABLE = DataCollectionObject.DCO_TYPE_TABLE;

   // line chart types
   public static final int DEFAULT = -1;
   public static final int LINE = 0;
   public static final int AREA = 1;

   // Unset integer field indicator
   private static final int UNSET = -99;

   @Attribute
   public long nodeId;

   @Attribute
   public long dciId;

   @Element(required = false)
   public String dciName;

   @Element(required = false)
   public String dciDescription;

   @Element(required = false)
   public int type;

   @Element(required = false)
   public String color;

   @Element(required = false)
   public String name;

   @Element(required = false)
   public int lineWidth;

   @Element(required = false)
   public int lineChartType;

   @Element(required = false)
   protected int displayType; // For compatibility, new configurations use lineChartType

   @Element(required = false)
   public boolean showThresholds;

   @Element(required = false)
   public boolean invertValues;

   @Element(required = false)
   public boolean useRawValues;

   @Element(required = false)
   public boolean multiMatch;

   @Element(required = false)
   public String instance;

   @Element(required = false)
   public String column;

   @Element(required = false)
   public String displayFormat;

	/**
	 * Default constructor
	 */
	public ChartDciConfig()
	{
		nodeId = 0;
		dciId = 0;
      dciName = "";
      dciDescription = "";
		type = ITEM;
		color = UNSET_COLOR;
      name = "";
		lineWidth = 2;
      lineChartType = DEFAULT;
      displayType = UNSET;
		showThresholds = false;
		invertValues = false;
		multiMatch = false;
      instance = "";
      column = "";
      displayFormat = "";
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
      this.lineChartType = src.lineChartType;
      this.displayType = src.displayType;
		this.showThresholds = src.showThresholds;
		this.invertValues = src.invertValues;
		this.useRawValues = src.useRawValues;
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
      lineChartType = DEFAULT;
      displayType = UNSET;
      showThresholds = false;
      invertValues = false;
      multiMatch = false;
      instance = "";
      column = "";
      displayFormat = "";
   }

	/**
	 * Create DCI info from DciValue object
	 * 
	 * @param dci The DciValue
	 */
	public ChartDciConfig(ChartDciConfig src, DciValue dci)
	{
		nodeId = dci.getNodeId();
		dciId = dci.getId();
      dciName = dci.getName();
      dciDescription = dci.getDescription();
		type = dci.getDcObjectType();
		name = dci.getDescription();
      this.color = src.color;
      this.lineWidth = src.lineWidth;
      this.lineChartType = src.lineChartType;
      this.displayType = src.displayType;
      this.showThresholds = src.showThresholds;
      this.invertValues = src.invertValues;
      this.useRawValues = src.useRawValues;
      this.multiMatch = src.multiMatch;
      this.instance = src.instance;
      this.column = src.column;
      this.displayFormat = src.displayFormat;
	}

   /**
    * Create DCI info from DataCollectionObject object
    * 
    * @param dci DCI to use as source
    */
   public ChartDciConfig(DataCollectionObject dci)
   {
      nodeId = dci.getNodeId();
      dciId = dci.getId();
      dciName = dci.getName();
      dciDescription = dci.getDescription();
      type = (dci instanceof DataCollectionItem) ? ITEM : TABLE;
      name = dci.getDescription();
      color = UNSET_COLOR;
      lineWidth = 2;
      lineChartType = DEFAULT;
      displayType = UNSET;
      showThresholds = false;
      invertValues = false;
      useRawValues = false;
      multiMatch = false;
      instance = "";
      column = "";
      displayFormat = "";
   }

	/**
	 * @return the color
	 */
	public int getColorAsInt()
	{
		if (color.equals(UNSET_COLOR))
			return -1;
      if (color.startsWith("0x"))
			return Integer.parseInt(color.substring(2), 16);
		return Integer.parseInt(color, 10);
	}

	/**
	 * @param value The colour value
	 */
	public void setColor(int value)
	{
      color = "0x" + Integer.toHexString(value);
	}

	/**
    * Get DCI label. Always returns non-empty string.
    * 
    * @return DCI label
    */
	public String getLabel()
	{
	   if ((name != null) && !name.isEmpty())
	      return name;
	   if ((dciDescription != null) && !dciDescription.isEmpty())
	      return dciDescription;
      if ((dciName != null) && !dciName.isEmpty())
         return dciName;
      return "[" + Long.toString(dciId) + "]";
	}

   /**
    * Get display format
    * 
    * @return The display format
    */
   public String getDisplayFormat()
   {
      return displayFormat;
   }

	/**
    * Get line chart type
    * 
    * @return The display type
    */
   public int getLineChartType()
	{
      return (displayType != UNSET) ? displayType - 1 : lineChartType; // old configuration format
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

   /**
    * @see java.lang.Object#hashCode()
    */
   @Override
   public int hashCode()
   {
      final int prime = 31;
      int result = 1;
      result = prime * result + (int)(dciId ^ (dciId >>> 32));
      if (dciId == 0)
      {
         result = prime * result + ((dciDescription == null) ? 0 : dciDescription.hashCode());
         result = prime * result + ((dciName == null) ? 0 : dciName.hashCode());
      }
      return result;
   }

   /**
    * @see java.lang.Object#equals(java.lang.Object)
    */
   @Override
   public boolean equals(Object obj)
   {
      if (this == obj)
         return true;
      if (obj == null)
         return false;
      if (getClass() != obj.getClass())
         return false;
      ChartDciConfig other = (ChartDciConfig)obj;
      if (dciId != other.dciId)
         return false;
      if (dciId != 0)
         return true;
      if (dciDescription == null)
      {
         if (other.dciDescription != null)
            return false;
      }
      else if (!dciDescription.equals(other.dciDescription))
         return false;
      if (dciName == null)
      {
         if (other.dciName != null)
            return false;
      }
      else if (!dciName.equals(other.dciName))
         return false;
      return true;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "ChartDciConfig [nodeId=" + nodeId + ", dciId=" + dciId + ", dciName=" + dciName + ", dciDescription=" + dciDescription + ", type=" + type + ", color=" + color + ", name=" + name +
            ", lineWidth=" + lineWidth + ", lineChartType=" + lineChartType + ", displayType=" + displayType + ", showThresholds=" + showThresholds + ", invertValues=" +
            invertValues + ", useRawValues=" + useRawValues + ", multiMatch=" + multiMatch + ", instance=" + instance + ", column=" + column + ", displayFormat=" + displayFormat + "]";
   }

   @Override
   public long getNodeId()
   {
      return nodeId;
   }

   @Override
   public long getDciId()
   {
      return dciId;
   }
}
