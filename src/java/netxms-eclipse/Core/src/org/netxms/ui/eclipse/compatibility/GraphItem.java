/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.compatibility;

import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.MeasurementUnit;

/**
 * This class represents single graph item (DCI)
 */
public class GraphItem
{
	private long nodeId;
	private long dciId;
	private int type;
	private String name;
	private String description;
   private String displayFormat;
	private String dataColumn;
	private String instance;
   private int lineChartType; // Line or area
   private int color;
   private int lineWidth;
   private boolean showThresholds;
   private boolean inverted;
   private MeasurementUnit measurementUnit;

	/**
	 * Create graph item object with default values
	 */
	public GraphItem()
	{
		nodeId = 0;
		dciId = 0;
		type = DataCollectionObject.DCO_TYPE_ITEM;
		name = "<noname>";
		description = "<noname>";
      displayFormat = "";
		dataColumn = "";
		instance = "";
      lineChartType = ChartDciConfig.DEFAULT;
      color = -1; // Use palette default
      lineWidth = 0;
      showThresholds = false;
      inverted = false;
      measurementUnit = null;
	}

	/**
    * Constructor for ITEM type
    * 
    * @param nodeId The node ID
    * @param dciId The dci ID
    * @param name The name
    * @param description The description
    * @param displayFormat The display format
    * @param lineChartType item line chart type (line or area)
    * @param color item color (-1 to use palette default)
    */
   public GraphItem(long nodeId, long dciId, String name, String description, String displayFormat, int lineChartType, int color)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
		this.type = DataCollectionObject.DCO_TYPE_ITEM;
		this.name = name;
		this.description = description;
      this.displayFormat = displayFormat;
		this.dataColumn = "";
		this.instance = "";
      this.lineChartType = lineChartType;
      this.color = color;
      this.lineWidth = 0;
      this.showThresholds = false;
      this.inverted = false;
      this.measurementUnit = null;
	}

   /**
    * Constructor for ad-hoc ITEM type (without related server-side DCI)
    * 
    * @param name The name
    * @param description The description
    * @param displayFormat The display format
    */
   public GraphItem(String name, String description, String displayFormat)
   {
      this(0, 0, name, description, displayFormat, ChartDciConfig.DEFAULT, -1);
   }

   /**
    * Constructor for ITEM type from existing DCI value
    * 
    * @param dciValue DCI value
    * @param displayFormat The display format
    */
   public GraphItem(DciValue dciValue, String displayFormat)
   {
      this(dciValue, displayFormat, ChartDciConfig.DEFAULT, -1);
   }

   /**
    * Constructor for ITEM type from existing DCI value and applied styling
    * 
    * @param dciValue DCI value
    * @param displayFormat The display format
    * @param lineChartType item line chart type (line or area)
    * @param color item color (-1 to use palette default)
    */
   public GraphItem(DciValue dciValue, String displayFormat, int lineChartType, int color)
   {
      this(dciValue.getNodeId(), dciValue.getId(), dciValue.getName(), dciValue.getDescription(), displayFormat, lineChartType, color);
      measurementUnit = dciValue.getMeasurementUnit();
   }

   /**
    * Constructor for ITEM type from chart DCI configuration
    * 
    * @param dciConfig DCI configuration
    */
   public GraphItem(ChartDciConfig dciConfig)
   {
      this(dciConfig.nodeId, dciConfig.dciId, dciConfig.getDciName(), dciConfig.getLabel(), dciConfig.getDisplayFormat(), dciConfig.getLineChartType(), dciConfig.getColorAsInt());
      this.instance = dciConfig.instance;
      this.inverted = dciConfig.invertValues;
      this.lineWidth = dciConfig.lineWidth;
      this.showThresholds = dciConfig.showThresholds;
      this.measurementUnit = dciConfig.measurementUnit;
   }

   /**
    * Constructor for TABLE type
    * 
    * @param nodeId The node ID
    * @param dciId The dci ID
    * @param name The name
    * @param description The description
    * @param instance The instance
    * @param dataColumn The data column
    * @param displayFormat The display format
	 */
   public GraphItem(long nodeId, long dciId, String name, String description, String instance, String dataColumn, String displayFormat)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
		this.type = DataCollectionObject.DCO_TYPE_TABLE;
		this.name = name;
		this.description = description;
      this.displayFormat = displayFormat;
		this.dataColumn = dataColumn;
		this.instance = instance;
      this.lineChartType = ChartDciConfig.DEFAULT;
      this.color = -1; // Use palette default
      this.lineWidth = 0;
      this.showThresholds = false;
      this.inverted = false;
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
    * @return the name
    */
	public String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
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
	 * @return the dataColumn
	 */
	public String getDataColumn()
	{
		return dataColumn;
	}

	/**
	 * @param dataColumn the dataColumn to set
	 */
	public void setDataColumn(String dataColumn)
	{
		this.dataColumn = dataColumn;
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
    * @return the displayFormat
    */
   public String getDisplayFormat()
   {
      return displayFormat;
   }

   /**
    * @param displayFormat the displayFormat to set
    */
   public void setDisplayFormat(String displayFormat)
   {
      this.displayFormat = displayFormat;
   }

   /**
    * @return the displayType
    */
   public int getLineChartType()
   {
      return lineChartType;
   }

   /**
    * @param lineChartType new line chart type
    */
   public void setLineChartType(int lineChartType)
   {
      this.lineChartType = lineChartType;
   }

   /**
    * Check if display type set to "area".
    *
    * @param defaultIsArea true if chart's default line chart type is AREA
    * @return true if display type set to "area".
    */
   public boolean isArea(boolean defaultIsArea)
   {
      return (lineChartType == ChartDciConfig.AREA) || ((lineChartType == ChartDciConfig.DEFAULT) && defaultIsArea);
   }

   /**
    * @return the color
    */
   public int getColor()
   {
      return color;
   }

   /**
    * @param color the color to set
    */
   public void setColor(int color)
   {
      this.color = color;
   }

   /**
    * @return the lineWidth
    */
   public int getLineWidth()
   {
      return lineWidth;
   }

   /**
    * @param lineWidth the lineWidth to set
    */
   public void setLineWidth(int lineWidth)
   {
      this.lineWidth = lineWidth;
   }

   /**
    * @return the showThresholds
    */
   public boolean isShowThresholds()
   {
      return showThresholds;
   }

   /**
    * @param showThresholds the showThresholds to set
    */
   public void setShowThresholds(boolean showThresholds)
   {
      this.showThresholds = showThresholds;
   }

   /**
    * @return the inverted
    */
   public boolean isInverted()
   {
      return inverted;
   }

   /**
    * @param inverted the inverted to set
    */
   public void setInverted(boolean inverted)
   {
      this.inverted = inverted;
   }

   /**
    * Get configured measurement unit.
    *
    * @return measurement unit or null
    */
   public MeasurementUnit getMeasurementUnit()
   {
      return measurementUnit;
   }

   /**
    * Set measurement unit to use.
    *
    * @param measurementUnit new measurement unit
    */
   public void setMeasurementUnit(MeasurementUnit measurementUnit)
   {
      this.measurementUnit = measurementUnit;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "GraphItem [nodeId=" + nodeId + ", dciId=" + dciId + ", type=" + type + ", name=" + name + ", description=" + description +
            ", displayFormat=" + displayFormat + ", dataColumn=" + dataColumn + ", instance=" + instance + ", lineChartType=" + lineChartType + ", color=" + color + ", lineWidth=" + lineWidth +
            ", showThresholds=" + showThresholds + ", inverted=" + inverted + ", binaryUnit=" + ", measurementUnit=" + measurementUnit + "]";
   }
}
