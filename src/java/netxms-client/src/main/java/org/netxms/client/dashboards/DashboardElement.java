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

import org.netxms.base.NXCPMessage;

/**
 * Dashboard's element
 */
public class DashboardElement
{
	public static final int LABEL = 0;
	public static final int LINE_CHART = 1;
	public static final int BAR_CHART = 2;
	public static final int PIE_CHART = 3;
	public static final int TUBE_CHART = 4;
	public static final int STATUS_CHART = 5;
	public static final int STATUS_INDICATOR = 6;
	public static final int DASHBOARD = 7;
	public static final int NETWORK_MAP = 8;
	public static final int CUSTOM = 9;
	
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
		horizontalSpan = 1;
		verticalSpan = 1;
		horizontalAlignment = FILL;
		verticalAlignment = FILL;
	}
	
	/**
	 * Create dashboard element from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public DashboardElement(NXCPMessage msg, long baseId)
	{
		type = msg.getVariableAsInteger(baseId);
		data = msg.getVariableAsString(baseId + 1);
		horizontalSpan = msg.getVariableAsInteger(baseId + 2);
		verticalSpan = msg.getVariableAsInteger(baseId + 3);
		horizontalAlignment = msg.getVariableAsInteger(baseId + 4);
		verticalAlignment = msg.getVariableAsInteger(baseId + 5);
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src original element
	 */
	public DashboardElement(DashboardElement src)
	{
		type = src.type;
		data = src.data;
		horizontalSpan = src.horizontalSpan;
		verticalSpan = src.verticalSpan;
		horizontalAlignment = src.horizontalAlignment;
		verticalAlignment = src.verticalAlignment;
	}

	/**
	 * Fill NXCP message with element's data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setVariableInt16(baseId, type);
		msg.setVariable(baseId + 1, data);
		msg.setVariableInt16(baseId + 2, horizontalSpan);
		msg.setVariableInt16(baseId + 3, verticalSpan);
		msg.setVariableInt16(baseId + 4, horizontalAlignment);
		msg.setVariableInt16(baseId + 5, verticalAlignment);
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
