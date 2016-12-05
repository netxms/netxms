/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.android.main.activities.helpers;

import org.netxms.client.datacollection.DciValue;
import org.simpleframework.xml.Attribute;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * DCI information for chart
 */
@Root(name = "dci", strict = false)
public class ChartDciConfig
{
	public static final String UNSET_COLOR = "UNSET";

	@Attribute
	public long nodeId;

	@Attribute
	public long dciId;

	@Element(required = false)
	public String color;

	@Element(required = false)
	public String name;

	@Element(required = false)
	public int lineWidth;

	/**
	 * Default constructor
	 */
	public ChartDciConfig()
	{
		nodeId = 0;
		dciId = 0;
		color = UNSET_COLOR;
		name = "";
		lineWidth = 2;
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
		this.color = src.color;
		this.name = src.name;
		this.lineWidth = src.lineWidth;
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
		name = dci.getDescription();
		color = UNSET_COLOR;
		lineWidth = 2;
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
	 * @param value
	 */
	public void setColor(int value)
	{
		color = "0x" + Integer.toHexString(value);
	}

	/**
	 * Get DCI name. Always returns non-empty string.
	 * @return
	 */
	public String getName()
	{
		return ((name != null) && (name.length() != 0)) ? name : ("[" + Long.toString(dciId) + "]");
	}
}
