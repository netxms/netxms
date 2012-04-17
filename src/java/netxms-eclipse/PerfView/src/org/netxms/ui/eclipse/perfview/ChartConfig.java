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
package org.netxms.ui.eclipse.perfview;

import java.io.StringWriter;
import java.io.Writer;
import java.util.Date;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.charts.api.ChartDciConfig;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Base class for all chart widget configs
 */
@Root(name="chart", strict=false)
public class ChartConfig
{
	@ElementArray(required = true)
	private ChartDciConfig[] dciList = new ChartDciConfig[0];
	
	@Element(required = false)
	private String title = "";
	
	@Element(required = false)
	private int legendPosition = GraphSettings.POSITION_BOTTOM;
	
	@Element(required = false)
	private boolean showLegend = true;
	
	@Element(required = false)
	private boolean showTitle = false;

	@Element(required = false)
	private boolean showGrid = true;

	@Element(required = false)
	private boolean showHostNames = false;

	@Element(required = false)
	private boolean autoRefresh = true;

	@Element(required = false)
	private boolean logScale = false;

	@Element(required = false)
	private int refreshRate = 30;

	@Element(required=false)
	private int timeUnits = GraphSettings.TIME_UNIT_HOUR;
	
	@Element(required=false)
	private int timeRange = 1;
	
	@Element(required=false)
	private int timeFrameType = GraphSettings.TIME_FRAME_BACK_FROM_NOW;
	
	@Element(required=false)
	private Date timeFrom;
	
	@Element(required=false)
	private Date timeTo;
	
	/**
	 * Create chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static ChartConfig createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(ChartConfig.class, xml);
	}

	/**
	 * Create XML from configuration.
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public String createXml() throws Exception
	{
		Serializer serializer = new Persister();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}

	/**
	 * Get time range covered by graph in milliseconds
	 * 
	 * @return
	 */
	public long getTimeRangeMillis()
	{
		switch(timeUnits)
		{
			case GraphSettings.TIME_UNIT_MINUTE:
				return (long)timeRange * 60L * 1000L;
			case GraphSettings.TIME_UNIT_HOUR:
				return (long)timeRange * 60L * 60L * 1000L;
			case GraphSettings.TIME_UNIT_DAY:
				return (long)timeRange * 24L * 60L * 60L * 1000L;
		}
		return 0;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/**
	 * @return the dciList
	 */
	public ChartDciConfig[] getDciList()
	{
		return dciList;
	}

	/**
	 * @param dciList the dciList to set
	 */
	public void setDciList(ChartDciConfig[] dciList)
	{
		this.dciList = dciList;
	}

	/**
	 * @return the legendPosition
	 */
	public int getLegendPosition()
	{
		return legendPosition;
	}

	/**
	 * @param legendPosition the legendPosition to set
	 */
	public void setLegendPosition(int legendPosition)
	{
		this.legendPosition = legendPosition;
	}

	/**
	 * @return the showLegend
	 */
	public boolean isShowLegend()
	{
		return showLegend;
	}

	/**
	 * @param showLegend the showLegend to set
	 */
	public void setShowLegend(boolean showLegend)
	{
		this.showLegend = showLegend;
	}

	/**
	 * @return the refreshRate
	 */
	public int getRefreshRate()
	{
		return refreshRate;
	}

	/**
	 * @param refreshRate the refreshRate to set
	 */
	public void setRefreshRate(int refreshRate)
	{
		this.refreshRate = refreshRate;
	}
	
	/**
	 * @return the showTitle
	 */
	public boolean isShowTitle()
	{
		return showTitle;
	}

	/**
	 * @param showTitle the showTitle to set
	 */
	public void setShowTitle(boolean showTitle)
	{
		this.showTitle = showTitle;
	}

	/**
	 * @return the timeUnits
	 */
	public int getTimeUnits()
	{
		return timeUnits;
	}

	/**
	 * @param timeUnits the timeUnits to set
	 */
	public void setTimeUnits(int timeUnits)
	{
		this.timeUnits = timeUnits;
	}

	/**
	 * @return the timeRange
	 */
	public int getTimeRange()
	{
		return timeRange;
	}

	/**
	 * @param timeRange the timeRange to set
	 */
	public void setTimeRange(int timeRange)
	{
		this.timeRange = timeRange;
	}

	/**
	 * @return the showHostNames
	 */
	public boolean isShowHostNames()
	{
		return showHostNames;
	}

	/**
	 * @param showHostNames the showHostNames to set
	 */
	public void setShowHostNames(boolean showHostNames)
	{
		this.showHostNames = showHostNames;
	}

	/**
	 * @return the autoRefresh
	 */
	public boolean isAutoRefresh()
	{
		return autoRefresh;
	}

	/**
	 * @param autoRefresh the autoRefresh to set
	 */
	public void setAutoRefresh(boolean autoRefresh)
	{
		this.autoRefresh = autoRefresh;
	}

	/**
	 * @return the logScale
	 */
	public boolean isLogScale()
	{
		return logScale;
	}

	/**
	 * @param logScale the logScale to set
	 */
	public void setLogScale(boolean logScale)
	{
		this.logScale = logScale;
	}

	/**
	 * @return the timeFrameType
	 */
	public int getTimeFrameType()
	{
		return timeFrameType;
	}

	/**
	 * @param timeFrameType the timeFrameType to set
	 */
	public void setTimeFrameType(int timeFrameType)
	{
		this.timeFrameType = timeFrameType;
	}

	/**
	 * @return the timeFrom
	 */
	public Date getTimeFrom()
	{
		return timeFrom;
	}

	/**
	 * @param timeFrom the timeFrom to set
	 */
	public void setTimeFrom(Date timeFrom)
	{
		this.timeFrom = timeFrom;
	}

	/**
	 * @return the timeTo
	 */
	public Date getTimeTo()
	{
		return timeTo;
	}

	/**
	 * @param timeTo the timeTo to set
	 */
	public void setTimeTo(Date timeTo)
	{
		this.timeTo = timeTo;
	}

	/**
	 * @return the showGrid
	 */
	public boolean isShowGrid()
	{
		return showGrid;
	}

	/**
	 * @param showGrid the showGrid to set
	 */
	public void setShowGrid(boolean showGrid)
	{
		this.showGrid = showGrid;
	}
}
