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
package org.netxms.client.dashboards;

/**
 * Dashboard's element
 */
public class DashboardElement
{
	public static final int LABEL = 0;
	public static final int LINE_CHART = 1;
	public static final int BAR_CHART = 2;
	public static final int PIE_CHART = 3;
	public static final int STATUS_INDICATOR = 4;
	
	public static final int FILL = 0;
	public static final int CENTER = 1;
	public static final int LEFT = 2;
	public static final int RIGHT = 3;
	public static final int TOP = 2;
	public static final int BOTTOM = 3;
	
	private int type;
	private String data;
	private int horizontalSpan;
	private int verticalSpan;
	private int horizontalAlignment;
	private int verticalAlignment;
	
	/**
	 * Create dashboard element which takes 1 cell with FILL layout in both directions
	 * 
	 * @param type element's type
	 * @param data element's data
	 */
	public DashboardElement(int type, String data)
	{
		this.type = type;
		this.data = data;
	}

	/**
	 * @return the data
	 */
	public String getData()
	{
		return data;
	}

	/**
	 * @param data the data to set
	 */
	public void setData(String data)
	{
		this.data = data;
	}

	/**
	 * @return the horizontalSpan
	 */
	public int getHorizontalSpan()
	{
		return horizontalSpan;
	}

	/**
	 * @param horizontalSpan the horizontalSpan to set
	 */
	public void setHorizontalSpan(int horizontalSpan)
	{
		this.horizontalSpan = horizontalSpan;
	}

	/**
	 * @return the verticalSpan
	 */
	public int getVerticalSpan()
	{
		return verticalSpan;
	}

	/**
	 * @param verticalSpan the verticalSpan to set
	 */
	public void setVerticalSpan(int verticalSpan)
	{
		this.verticalSpan = verticalSpan;
	}

	/**
	 * @return the horizontalAlignment
	 */
	public int getHorizontalAlignment()
	{
		return horizontalAlignment;
	}

	/**
	 * @param horizontalAlignment the horizontalAlignment to set
	 */
	public void setHorizontalAlignment(int horizontalAlignment)
	{
		this.horizontalAlignment = horizontalAlignment;
	}

	/**
	 * @return the verticalAlignment
	 */
	public int getVerticalAlignment()
	{
		return verticalAlignment;
	}

	/**
	 * @param verticalAlignment the verticalAlignment to set
	 */
	public void setVerticalAlignment(int verticalAlignment)
	{
		this.verticalAlignment = verticalAlignment;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
}
