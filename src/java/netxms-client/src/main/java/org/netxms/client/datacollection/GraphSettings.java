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
	private String title;
	private GraphItemStyle[] itemStyles = new GraphItemStyle[MAX_GRAPH_ITEM_COUNT];
	private GraphItem[] items;
	private List<AccessListElement> accessList;
	
	/**
	 * Create default settings
	 */
	public GraphSettings()
	{
		id = 0;
		ownerId = 0;
		name = "noname";
		shortName = "noname";
		flags = GF_AUTO_UPDATE | GF_SHOW_GRID | GF_SHOW_LEGEND;
		timeFrameType = TIME_FRAME_BACK_FROM_NOW;
		timeUnit = TIME_UNIT_MINUTE;
		timeFrame = 60;
		timeFrom = new Date();
		timeTo = new Date();
		autoRefreshInterval = 30000;
		axisColor = 0x161616;
		backgroundColor = 0xF0F0F0;
		gridColor = 0xE8E8E8;
		selectionColor = 0x800000;
		textColor = 0x000000;
		rulerColor = 0x000000;
		title = "";
		items = new GraphItem[0];
		accessList = new ArrayList<AccessListElement>(0);
		
		// Default item styles
		for(int i = 0; i < itemStyles.length; i++)
			itemStyles[i] = new GraphItemStyle(0); 
	}
	
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
				autoRefreshInterval = safeParseInt(value, 30) * 1000;	// convert to milliseconds
			}
			else if (name.equals("F"))
			{
				flags |= safeParseInt(value, 0);
			}
			else if (name.equals("N"))
			{
				dciCount = safeParseInt(value, 0);
				items = new GraphItem[dciCount];
				for(int j = 0; j < dciCount; j++)
					items[j] = new GraphItem();
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
			else if (name.equals("T"))
			{
				title = value;
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
			else if (name.charAt(0) == 'W')	// Item line width
			{
				int item = safeParseInt(name.substring(1), -1);
				if ((item >= 0) && (item < MAX_GRAPH_ITEM_COUNT))
					itemStyles[item].setLineWidth(safeParseInt(value, 0));
			}
			else if ((name.charAt(0) == 'F') && (name.charAt(1) == 'L'))	// Item flags
			{
				int item = safeParseInt(name.substring(2), -1);
				if ((item >= 0) && (item < MAX_GRAPH_ITEM_COUNT))
					itemStyles[item].setFlags(safeParseInt(value, 0));
			}
			else if (name.charAt(0) == 'N')	// Node ID
			{
				int item = safeParseInt(name.substring(1), -1);
				if ((item >= 0) && (item < dciCount))
					items[item].setNodeId(safeParseLong(value, 0));
			}
			else if (name.charAt(0) == 'I')	// DCI information
			{
				if (name.charAt(1) == 'D')	// description
				{
					int item = safeParseInt(name.substring(2), -1);
					if ((item >= 0) && (item < dciCount))
						items[item].setDescription(value);
				}
				else if (name.charAt(1) == 'N')	// name
				{
					int item = safeParseInt(name.substring(2), -1);
					if ((item >= 0) && (item < dciCount))
						items[item].setName(value);
				}
				else if (name.charAt(1) == 'S')	// source
				{
					int item = safeParseInt(name.substring(2), -1);
					if ((item >= 0) && (item < dciCount))
						items[item].setSource(safeParseInt(value, 0));
				}
				else if (name.charAt(1) == 'T')	// data type
				{
					int item = safeParseInt(name.substring(2), -1);
					if ((item >= 0) && (item < dciCount))
						items[item].setDataType(safeParseInt(value, 0));
				}
				else	// assume DCI ID - Ixxx
				{
					int item = safeParseInt(name.substring(1), -1);
					if ((item >= 0) && (item < dciCount))
						items[item].setDciId(safeParseLong(value, 0));
				}
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
	 * Parse long without throwing exception
	 * @param text text to parse
	 * @param defVal default value to be used in case of parse error
	 * @return parsed value
	 */
	static private long safeParseLong(String text, long defVal)
	{
		try
		{
			return Long.parseLong(text);
		}
		catch(NumberFormatException e)
		{
		}
		return defVal;
	}
	
	/**
	 * Get logarithmic scale mode
	 * 
	 * @return true if logarithmic scale mode is on
	 */
	public boolean isLogScale()
	{
		return (flags & GF_LOG_SCALE) != 0;
	}
	
	/**
	 * Enable or disable log scale usage flag
	 * 
	 * @param enable true to enable log scale
	 */
	public void setLogScale(boolean enable)
	{
		if (enable)
			flags |= GF_LOG_SCALE;
		else
			flags &= ~GF_LOG_SCALE;
	}
	
	/**
	 * Get auto refresh mode
	 * 
	 * @return true if auto refresh is on
	 */
	public boolean isAutoRefresh()
	{
		return (flags & GF_AUTO_UPDATE) != 0;
	}
	
	/**
	 * Set or clear automatic refresh flag
	 * 
	 * @param enable true to enable automatic refresh
	 */
	public void setAutoRefresh(boolean enable)
	{
		if (enable)
			flags |= GF_AUTO_UPDATE;
		else
			flags &= ~GF_AUTO_UPDATE;
	}

	/**
	 * Get grid display mode
	 * 
	 * @return true if grid should be shown
	 */
	public boolean isGridVisible()
	{
		return (flags & GF_SHOW_GRID) != 0;
	}
	
	/**
	 * Show or hide grid
	 * 
	 * @param enable true to show grid
	 */
	public void setGridVisible(boolean enable)
	{
		if (enable)
			flags |= GF_SHOW_GRID;
		else
			flags &= ~GF_SHOW_GRID;
	}

	/**
	 * Get host name display mode
	 * 
	 * @return true if host names should be shown
	 */
	public boolean isHostNamesVisible()
	{
		return (flags & GF_SHOW_HOST_NAMES) != 0;
	}

	/**
	 * Show or hide host names in legend
	 * 
	 * @param enable true to show host names in legend
	 */
	public void setHostNamesVisible(boolean enable)
	{
		if (enable)
			flags |= GF_SHOW_HOST_NAMES;
		else
			flags &= ~GF_SHOW_HOST_NAMES;
	}

	/**
	 * Get legend show mode
	 * 
	 * @return true if legend should be shown
	 */
	public boolean isLegendVisible()
	{
		return (flags & GF_SHOW_LEGEND) != 0;
	}
	
	/**
	 * Show or hide legend
	 * 
	 * @param enable true to show legend
	 */
	public void setLegendVisible(boolean enable)
	{
		if (enable)
			flags |= GF_SHOW_LEGEND;
		else
			flags &= ~GF_SHOW_LEGEND;
	}

	/**
	 * Get auto scale mode
	 * 
	 * @return true if auto scale is on
	 */
	public boolean isAutoScale()
	{
		return (flags & GF_AUTO_SCALE) != 0;
	}
	
	/**
	 * Enable or disable automatic scaling
	 * 
	 * @param enable true to enable automatic scaling
	 */
	public void setAutoScale(boolean enable)
	{
		if (enable)
			flags |= GF_AUTO_SCALE;
		else
			flags &= ~GF_AUTO_SCALE;
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

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @return the timeFrameType
	 */
	public int getTimeFrameType()
	{
		return timeFrameType;
	}

	/**
	 * @return the timeUnit
	 */
	public int getTimeUnit()
	{
		return timeUnit;
	}

	/**
	 * @return the timeFrame
	 */
	public int getTimeFrame()
	{
		return timeFrame;
	}

	/**
	 * @return the timeFrom
	 */
	public Date getTimeFrom()
	{
		return timeFrom;
	}

	/**
	 * @return the timeTo
	 */
	public Date getTimeTo()
	{
		return timeTo;
	}

	/**
	 * @return the autoRefreshInterval
	 */
	public int getAutoRefreshInterval()
	{
		return autoRefreshInterval;
	}

	/**
	 * @return the axisColor
	 */
	public int getAxisColor()
	{
		return axisColor;
	}

	/**
	 * @return the backgroundColor
	 */
	public int getBackgroundColor()
	{
		return backgroundColor;
	}

	/**
	 * @return the gridColor
	 */
	public int getGridColor()
	{
		return gridColor;
	}

	/**
	 * @return the selectionColor
	 */
	public int getSelectionColor()
	{
		return selectionColor;
	}

	/**
	 * @return the textColor
	 */
	public int getTextColor()
	{
		return textColor;
	}

	/**
	 * @return the rulerColor
	 */
	public int getRulerColor()
	{
		return rulerColor;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @return the itemStyles
	 */
	public GraphItemStyle[] getItemStyles()
	{
		return itemStyles;
	}

	/**
	 * @return the items
	 */
	public GraphItem[] getItems()
	{
		return items;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @param shortName the shortName to set
	 */
	public void setShortName(String shortName)
	{
		this.shortName = shortName;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}

	/**
	 * @param timeFrameType the timeFrameType to set
	 */
	public void setTimeFrameType(int timeFrameType)
	{
		this.timeFrameType = timeFrameType;
	}

	/**
	 * @param timeUnit the timeUnit to set
	 */
	public void setTimeUnit(int timeUnit)
	{
		this.timeUnit = timeUnit;
	}

	/**
	 * @param timeFrame the timeFrame to set
	 */
	public void setTimeFrame(int timeFrame)
	{
		this.timeFrame = timeFrame;
	}

	/**
	 * @param timeFrom the timeFrom to set
	 */
	public void setTimeFrom(Date timeFrom)
	{
		this.timeFrom = timeFrom;
	}

	/**
	 * @param timeTo the timeTo to set
	 */
	public void setTimeTo(Date timeTo)
	{
		this.timeTo = timeTo;
	}

	/**
	 * @param autoRefreshInterval the autoRefreshInterval to set
	 */
	public void setAutoRefreshInterval(int autoRefreshInterval)
	{
		this.autoRefreshInterval = autoRefreshInterval;
	}

	/**
	 * @param axisColor the axisColor to set
	 */
	public void setAxisColor(int axisColor)
	{
		this.axisColor = axisColor;
	}

	/**
	 * @param backgroundColor the backgroundColor to set
	 */
	public void setBackgroundColor(int backgroundColor)
	{
		this.backgroundColor = backgroundColor;
	}

	/**
	 * @param gridColor the gridColor to set
	 */
	public void setGridColor(int gridColor)
	{
		this.gridColor = gridColor;
	}

	/**
	 * @param selectionColor the selectionColor to set
	 */
	public void setSelectionColor(int selectionColor)
	{
		this.selectionColor = selectionColor;
	}

	/**
	 * @param textColor the textColor to set
	 */
	public void setTextColor(int textColor)
	{
		this.textColor = textColor;
	}

	/**
	 * @param rulerColor the rulerColor to set
	 */
	public void setRulerColor(int rulerColor)
	{
		this.rulerColor = rulerColor;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/**
	 * @param itemStyles the itemStyles to set
	 */
	public void setItemStyles(GraphItemStyle[] itemStyles)
	{
		this.itemStyles = itemStyles;
	}

	/**
	 * @param items the items to set
	 */
	public void setItems(GraphItem[] items)
	{
		this.items = items;
	}
	
	/**
	 * Get time range covered by graph in milliseconds
	 * 
	 * @return
	 */
	public long getTimeRangeMillis()
	{
		switch(timeUnit)
		{
			case TIME_UNIT_MINUTE:
				return timeFrame * 60 * 1000;
			case TIME_UNIT_HOUR:
				return timeFrame * 60 * 60 * 1000;
			case TIME_UNIT_DAY:
				return timeFrame * 24 * 60 * 60 * 1000;
		}
		return 0;
	}
}
