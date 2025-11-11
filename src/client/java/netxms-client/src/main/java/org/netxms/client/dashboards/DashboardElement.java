/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
   // deprecated: public static final int CUSTOM = 9;
	public static final int GEO_MAP = 10;
	public static final int ALARM_VIEWER = 11;
	public static final int AVAILABLITY_CHART = 12;
   public static final int DIAL_CHART = 13;
   public static final int WEB_PAGE = 14;
	public static final int TABLE_BAR_CHART = 15;
	public static final int TABLE_PIE_CHART = 16;
	public static final int TABLE_TUBE_CHART = 17;
	public static final int SEPARATOR = 18;
	public static final int TABLE_VALUE = 19;
	public static final int STATUS_MAP = 20;
   public static final int DCI_SUMMARY_TABLE = 21;
   public static final int SYSLOG_MONITOR = 22;
   public static final int SNMP_TRAP_MONITOR = 23;
   public static final int EVENT_MONITOR = 24;
   public static final int SERVICE_COMPONENTS = 25;
   public static final int RACK_DIAGRAM = 26;
   public static final int OBJECT_TOOLS = 27;
   public static final int OBJECT_QUERY = 28;
   public static final int PORT_VIEW = 29;
   public static final int SCRIPTED_BAR_CHART = 30;
   public static final int SCRIPTED_PIE_CHART = 31;
   public static final int FILE_MONITOR = 32;

	public static final int FILL = 0;
	public static final int CENTER = 1;
	public static final int LEFT = 2;
	public static final int RIGHT = 3;
	public static final int TOP = 2;
	public static final int BOTTOM = 3;

	private int type;
	private String data;
	private String layout;
	private int index;

	/**
    * Create dashboard element which takes 1 cell with FILL layout in both directions.
    * 
    * @param type element's type
    * @param data element's data
    * @param index element's index within dashboard (used for scripted chart update)
    */
	public DashboardElement(int type, String data, int index)
	{
		this.type = type;
		this.data = data;
		this.index = index;
		layout = "{\"horizontalSpan\":1,\"verticalSpan\":1,\"horizontalAlignment\":0,\"verticalAlignment\":0}";
	}

	/**
    * Create dashboard element from NXCP message
    *
    * @param msg NXCP message
    * @param baseId base variable ID
    * @param index element's index within dashboard (used for scripted chart update)
    */
	public DashboardElement(NXCPMessage msg, long baseId, int index)
	{
		type = msg.getFieldAsInt32(baseId);
		data = msg.getFieldAsString(baseId + 1);
		layout = msg.getFieldAsString(baseId + 2);
		this.index = index;
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
		layout = src.layout;
      index = src.index;
	}

	/**
	 * Fill NXCP message with element's data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setFieldInt16(baseId, type);
		msg.setField(baseId + 1, data);
		msg.setField(baseId + 2, layout);
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
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * @return the layout
	 */
	public String getLayout()
	{
		return layout;
	}

	/**
	 * @param layout the layout to set
	 */
	public void setLayout(String layout)
	{
		this.layout = layout;
	}

   /**
    * @return current element index in dashboard
    */
   public int getIndex()
   {
      return index;
   }

   /**
    * @param index new element index in dashboard
    */
   public void setIndex(int index)
   {
      this.index = index;      
   }
}
