/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

/**
 * This class contains styling for individual graph items
 *
 */
public class GraphItemStyle
{
	public static final int LINE = 0;
	public static final int AREA = 1;
	
	public static final int SHOW_AVERAGE = 0x0001;
	public static final int SHOW_THRESHOLDS = 0x0002;
	public static final int SHOW_TREND = 0x0004;
	
	private int type;
	private int color;
	private int lineWidth;
	private int flags;
	
	/**
	 * Create style record with default values.
	 * 
	 */
	public GraphItemStyle()
	{
		type = LINE;
		color = 0;
		lineWidth = 0;
		flags = 0;
	}
	
	/**
	 * Create style record for line item with given color
	 * 
	 */
	public GraphItemStyle(int color)
	{
		type = LINE;
		this.color = color;
		lineWidth = 0;
		flags = 0;
	}
	
	/**
	 * constructor
	 * 
	 * @param type
	 * @param color
	 * @param lineWidth
	 * @param flags
	 */
	public GraphItemStyle(int type, int color, int lineWidth, int flags)
	{
		this.type = type;
		this.color = color;
		this.lineWidth = lineWidth;
		this.flags = flags;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the color
	 */
	public int getColor()
	{
		return color;
	}

	/**
	 * @return the lineWidth
	 */
	public int getLineWidth()
	{
		return lineWidth;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param type the type to set
	 */
	public void setType(int type)
	{
		this.type = type;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int color)
	{
		this.color = color;
	}

	/**
	 * @param lineWidth the lineWidth to set
	 */
	public void setLineWidth(int lineWidth)
	{
		this.lineWidth = lineWidth;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}
	
	/**
	 * Convenient method for checking SHOW_AVERAGE flag
	 * 
	 * @return true if SHOW_AVERAGE is set
	 */
	public boolean isShowAverage()
	{
		return (flags & SHOW_AVERAGE) != 0;
	}
	
	/**
	 * Convenient method for checking SHOW_THRESHOLDS flag
	 * 
	 * @return true if SHOW_THRESHOLDS is set
	 */
	public boolean isShowThresholds()
	{
		return (flags & SHOW_THRESHOLDS) != 0;
	}
	
	/**
	 * Convenient method for checking SHOW_TREND flag
	 * 
	 * @return true if SHOW_TREND is set
	 */
	public boolean isShowTrend()
	{
		return (flags & SHOW_TREND) != 0;
	}
}
