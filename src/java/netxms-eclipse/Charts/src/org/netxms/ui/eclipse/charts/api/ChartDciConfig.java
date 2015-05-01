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
package org.netxms.ui.eclipse.charts.api;

import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
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
	
	@Attribute
	public long nodeId;
	
	@Attribute
	public long dciId;
	
	@Element(required=false)
	public int type;

	@Element(required=false)
	public String color;

	@Element(required=false)
	public String name;
	
	@Element(required=false)
	public int lineWidth;
	
	@Element(required=false)
	public boolean area;
	
	@Element(required=false)
	public boolean showThresholds;

   @Element(required=false)
   public boolean invertValues;

	@Element(required=false)
	public String instance;
	
	@Element(required=false)
	public String column;
	
	/**
	 * Default constructor
	 */
	public ChartDciConfig()
	{
		nodeId = 0;
		dciId = 0;
		type = ITEM;
		color = UNSET_COLOR;
		name = ""; //$NON-NLS-1$
		lineWidth = 2;
		area = false;
		showThresholds = false;
		invertValues = false;
		instance = ""; //$NON-NLS-1$
		column = ""; //$NON-NLS-1$
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
		this.type = src.type;
		this.color = src.color;
		this.name = src.name;
		this.lineWidth = src.lineWidth;
		this.area = src.area;
		this.showThresholds = src.showThresholds;
		this.invertValues = src.invertValues;
		this.instance = src.instance;
		this.column = src.column;
	}

	/**
	 * Create DCI info from DciValue object
	 * 
	 * @param dci
	 */
	public ChartDciConfig(DciValue dci)
	{
		nodeId = dci.getNodeId();
		dciId = dci.getId();
		type = dci.getDcObjectType();
		name = dci.getDescription();
		color = UNSET_COLOR;
		lineWidth = 2;
		area = false;
		showThresholds = false;
		invertValues = false;
		instance = ""; //$NON-NLS-1$
		column = ""; //$NON-NLS-1$
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
	 * @param value
	 */
	public void setColor(int value)
	{
		color = "0x" + Integer.toHexString(value); //$NON-NLS-1$
	}
	
	/**
	 * Get DCI name. Always returns non-empty string.
	 * @return
	 */
	public String getName()
	{
		return ((name != null) && !name.isEmpty()) ? name : ("[" + Long.toString(dciId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
	}
}
