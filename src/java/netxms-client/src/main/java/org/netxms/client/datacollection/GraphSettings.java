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

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import org.netxms.base.NXCPMessage;
import org.netxms.client.AccessListElement;

/**
 * Settings for predefined graph
 *
 */
public class GraphSettings
{
	public static final int MAX_GRAPH_ITEM_COUNT = 16;
	
	public static final int TIME_FRAME_FIXED = 0;
	public static final int TIME_FRAME_BACK_FROM_NOW = 1;
	
	public static final int TIME_UNIT_MINUTE = 0;
	public static final int TIME_UNIT_HOUR = 1;
	public static final int TIME_UNIT_DAY = 2;
	
	public static final int GF_AUTO_UPDATE     = 0x000001;
	public static final int GF_AUTO_SCALE      = 0x000100;
	public static final int GF_SHOW_GRID       = 0x000200;
	public static final int GF_SHOW_LEGEND     = 0x000400;
	public static final int GF_SHOW_RULER      = 0x000800;
	public static final int GF_SHOW_HOST_NAMES = 0x001000;
	public static final int GF_LOG_SCALE       = 0x002000;
	
	private long id;
	private long ownerId;
	private String name;
	private String shortName;
	private int flags;
	private int timeFrameType;
	private int timeUnit;
	private int timeFrame;	// in given time units
	private Date timeFrom;
	private Date timeTo;
	private int autoRefreshInterval;
	private int axisColor;
	private int backgroundColor;
	private int gridColor;
	private int selectionColor;
	private int textColor;
	private int rulerColor;
	private GraphItemStyle[] itemStyles = new GraphItemStyle[MAX_GRAPH_ITEM_COUNT];
	private List<AccessListElement> accessList;
	
	/**
	 * Create graph settings object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	public GraphSettings(final NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		ownerId = msg.getVariableAsInt64(baseId + 1);
		name = msg.getVariableAsString(baseId + 2);
		
		String[] parts = name.split("->");
		shortName = (parts.length > 1) ? parts[parts.length - 1] : name;
		
		int count = msg.getVariableAsInteger(baseId + 4);  // ACL size
		long[] users = msg.getVariableAsUInt32Array(baseId + 5);
		long[] rights = msg.getVariableAsUInt32Array(baseId + 6);
		accessList = new ArrayList<AccessListElement>(count);
		for(int i = 0; i < count; i++)
		{
			accessList.add(new AccessListElement(users[i], (int)rights[i]));
		}
		
		parseGraphSettings(msg.getVariableAsString(baseId + 3));
	}
	
	/**
	 * Parse graph settings received from server.
	 * 
	 * @param settings settings string
	 */
	private void parseGraphSettings(String settings)
	{
		int dciCount = 0;
		flags = 0;
		
		for(int i = 0; i < MAX_GRAPH_ITEM_COUNT; i++)
			itemStyles[i] = new GraphItemStyle();
		
		String[] elements = settings.split("\u007F");
		for(int i = 0; i < elements.length; i++)
		{
			int index = elements[i].indexOf(':');
			if (index == -1)
				continue;
			
			final String name = elements[i].substring(0, index);
			final String value = elements[i].substring(index + 1);
			
			if (name.equals("A"))
			{
				autoRefreshInterval = safeParseInt(value, 30);
			}
			else if (name.equals("F"))
			{
				flags |= safeParseInt(value, 0);
			}
			else if (name.equals("N"))
			{
				dciCount = safeParseInt(value, 0);
			}
			else if (name.equals("TFT"))
			{
				timeFrameType = safeParseInt(value, TIME_FRAME_BACK_FROM_NOW);
			}
			else if (name.equals("TU"))
			{
				timeUnit = safeParseInt(value, TIME_UNIT_HOUR);
			}
			else if (name.equals("NTU"))
			{
				timeFrame = safeParseInt(value, 1);
			}
			else if (name.equals("TF"))
			{
				timeFrom = new Date((long)safeParseInt(value, 0) * 1000L);
			}
			else if (name.equals("TF"))
			{
				timeTo = new Date((long)safeParseInt(value, 0) * 1000L);
			}
			else if (name.equals("S"))
			{
				if (safeParseInt(value, 0) != 0)
					flags |= GF_AUTO_SCALE;
			}
			else if (name.equals("G"))
			{
				if (safeParseInt(value, 0) != 0)
					flags |= GF_SHOW_GRID;
			}
			else if (name.equals("L"))
			{
				if (safeParseInt(value, 0) != 0)
					flags |= GF_SHOW_LEGEND;
			}
			else if (name.equals("R"))
			{
				if (safeParseInt(value, 0) != 0)
					flags |= GF_SHOW_RULER;
			}
			else if (name.equals("H"))
			{
				if (safeParseInt(value, 0) != 0)
					flags |= GF_SHOW_HOST_NAMES;
			}
			else if (name.equals("O"))
			{
				if (safeParseInt(value, 0) != 0)
					flags |= GF_LOG_SCALE;
			}
			else if (name.equals("CA"))
			{
				axisColor = safeParseInt(value, 0);
			}
			else if (name.equals("CB"))
			{
				backgroundColor = safeParseInt(value, 0);
			}
			else if (name.equals("CG"))
			{
				gridColor = safeParseInt(value, 0);
			}
			else if (name.equals("CR"))
			{
				rulerColor = safeParseInt(value, 0);
			}
			else if (name.equals("CS"))
			{
				selectionColor = safeParseInt(value, 0);
			}
			else if (name.equals("CT"))
			{
				textColor = safeParseInt(value, 0);
			}
			else if (name.charAt(0) == 'C')	// Item color
			{
				int item = safeParseInt(name.substring(1), -1);
				if ((item >= 0) && (item < MAX_GRAPH_ITEM_COUNT))
					itemStyles[item].setColor(safeParseInt(value, 0));
			}
			else if (name.charAt(0) == 'T')	// Item type
			{
				int item = safeParseInt(name.substring(1), -1);
				if ((item >= 0) && (item < MAX_GRAPH_ITEM_COUNT))
					itemStyles[item].setColor(safeParseInt(value, 0));
			}
		}
	}
	
	/**
	 * Parse int without throwing exception
	 * @param text text to parse
	 * @param defVal default value to be used in case of parse error
	 * @return parsed value
	 */
	static private int safeParseInt(String text, int defVal)
	{
		try
		{
			return Integer.parseInt(text);
		}
		catch(NumberFormatException e)
		{
		}
		return defVal;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the ownerId
	 */
	public long getOwnerId()
	{
		return ownerId;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the accessList
	 */
	public List<AccessListElement> getAccessList()
	{
		return accessList;
	}

	/**
	 * @return the shortName
	 */
	public String getShortName()
	{
		return shortName;
	}
}
