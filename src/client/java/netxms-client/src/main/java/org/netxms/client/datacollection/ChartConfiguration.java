/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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

import java.io.StringWriter;
import java.io.Writer;
import java.util.Date;
import java.util.HashSet;
import java.util.Set;
import org.netxms.client.TimePeriod;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.xml.XMLTools;
import org.netxms.client.xml.XmlDateConverter;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.convert.Convert;

/**
 * Base class for all chart widget configs
 */
@Root(name="chart", strict=false)
public class ChartConfiguration
{
	public static final int MAX_GRAPH_ITEM_COUNT = 16;

   public static final int POSITION_LEFT = 1;
   public static final int POSITION_RIGHT = 2;
   public static final int POSITION_TOP = 4;
   public static final int POSITION_BOTTOM = 8;

   public static final int GAUGE_COLOR_MODE_ZONE = 0;
   public static final int GAUGE_COLOR_MODE_CUSTOM = 1;
   public static final int GAUGE_COLOR_MODE_THRESHOLD = 2;

	@Element(required = false)
   protected String title = "";

   @Element(required = false)
   private String yAxisLabel = null;

	@Element(required = false)
   protected int legendPosition = POSITION_BOTTOM;
	
	@Element(required = false)
	protected boolean showLegend = true;

	@Element(required = false)
   protected boolean extendedLegend = true;

	@Element(required = false)
	protected boolean showTitle = false;

	@Element(required = false)
	protected boolean showGrid = true;

   @Element(required = false)
   protected boolean showLabels = true;

	@Element(required = false)
	protected boolean showHostNames = false;

	@Element(required = false)
	protected boolean autoRefresh = true;

	@Element(required = false)
	protected boolean logScale = false;

   @Element(required = false)
   protected boolean stacked = false;

   @Element(required = false)
   protected boolean transposed = false;

   @Element(required = false)
   protected boolean translucent = false;

   @Element(required = false)
   protected boolean doughnutRendering = true;

   @Element(required = false)
   protected boolean showTotal = false;

   @Element(required = false)
   protected boolean area = false;

   @Element(required = false)
   protected int lineWidth = 2;

   @Element(required = false)
   protected boolean autoScale = true;

   @Element(required = false)
   protected double minYScaleValue = 0;

   @Element(required = false)
   protected double maxYScaleValue = 100;

	@Element(required = false)
	protected int refreshRate = 30;

   @Element(required = false)
   protected TimePeriod timePeriod = new TimePeriod();

	@Element(required=false)
   protected Integer timeUnit = null; // For reading old format configuration, new format will use TimePeriod

	@Element(required=false)
   protected Integer timeRange = null; // For reading old format configuration, new format will use TimePeriod

	@Element(required=false)
   protected Integer timeFrameType = null; // For reading old format configuration, new format will use TimePeriod

	@Element(required=false)
	@Convert(XmlDateConverter.class)
   protected Date timeFrom = null; // For reading old format configuration, new format will use TimePeriod

	@Element(required=false)
	@Convert(XmlDateConverter.class)
   protected Date timeTo = null; // For reading old format configuration, new format will use TimePeriod

   @Element(required = false)
   protected boolean modifyYBase = false;

   @Element(required = false)
   protected boolean useMultipliers = true;

   @Element(required = false)
   protected double rotation = 0;

   @Element(required = false)
   protected boolean zoomEnabled = false;

   @Element(required = false)
   private boolean labelsInside = false;

   @Element(required = false)
   private double leftYellowZone = 0.0;

   @Element(required = false)
   private double leftRedZone = 0.0;

   @Element(required = false)
   private double rightYellowZone = 70.0;

   @Element(required = false)
   private double rightRedZone = 90.0;

   @Element(required = false)
   private boolean elementBordersVisible = false;

   @Element(required = false)
   private int gaugeColorMode = GAUGE_COLOR_MODE_ZONE;

   @Element(required = false)
   private String fontName = "";

   @Element(required = false)
   private int fontSize = 0;

   @Element(required = false)
   private int expectedTextWidth = 0;

   private Set<ChartConfigurationChangeListener> changeListeners = new HashSet<ChartConfigurationChangeListener>(0);

	/**
	 * Create chart settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
   public static ChartConfiguration createFromXml(final String xml) throws Exception
   {
      ChartConfiguration config = XMLTools.createFromXml(ChartConfiguration.class, xml);
      if (config.timeFrameType != null)
      {
         // Old format, create TimePeriod object
         config.timePeriod = new TimePeriod(TimeFrameType.getByValue(config.timeFrameType),
               config.timeRange != null ? config.timeRange : 1,
               config.timeUnit != null ? TimeUnit.getByValue(config.timeUnit) : TimeUnit.HOUR,
               config.timeFrom, config.timeTo);
         config.timeFrameType = null;
         config.timeRange = null;
         config.timeUnit = null;
         config.timeFrom = null;
         config.timeTo = null;
      }
      return config;
   }

   /**
    * Default constructor
    */
   public ChartConfiguration()
   {
   }

   /**
    * Copy constructor
    *
    * @param src source object
    */
   public ChartConfiguration(ChartConfiguration src)
   {
      title = src.title;
      yAxisLabel = src.yAxisLabel;
      legendPosition = src.legendPosition;
      showLegend = src.showLegend;
      extendedLegend = src.extendedLegend;
      showTitle = src.showTitle;
      showLabels = src.showLabels;
      showGrid = src.showGrid;
      showHostNames = src.showHostNames;
      autoRefresh = src.autoRefresh;
      logScale = src.logScale;
      stacked = src.stacked;
      transposed = src.transposed;
      translucent = src.translucent;
      doughnutRendering = src.doughnutRendering;
      showTotal = src.showTotal;
      area = src.area;
      lineWidth = src.lineWidth;
      autoScale = src.autoScale;
      minYScaleValue = src.minYScaleValue;
      maxYScaleValue = src.maxYScaleValue;
      refreshRate = src.refreshRate;
      timePeriod = (src.timePeriod != null) ? new TimePeriod(src.timePeriod) : null;
      modifyYBase = src.modifyYBase;
      useMultipliers = src.useMultipliers;
      rotation = src.rotation;
      zoomEnabled = src.zoomEnabled;
      labelsInside = src.labelsInside;
      leftYellowZone = src.leftYellowZone;
      leftRedZone = src.leftRedZone;
      rightYellowZone = src.rightYellowZone;
      rightRedZone = src.rightRedZone;
      elementBordersVisible = src.elementBordersVisible;
      gaugeColorMode = src.gaugeColorMode;
      fontName = src.fontName;
      fontSize = src.fontSize;
      expectedTextWidth = src.expectedTextWidth;
   }

	/**
	 * Create XML from configuration.
	 * 
	 * @return XML document
	 * @throws Exception if the schema for the object is not valid
	 */
	public String createXml() throws Exception
	{
		Serializer serializer = XMLTools.createSerializer();
		Writer writer = new StringWriter();
		serializer.write(this, writer);
		return writer.toString();
	}

	/**
	 * Get time range covered by graph in milliseconds
	 * 
	 * @return The time range
	 */
	public long getTimeRangeMillis()
	{
      switch(timePeriod.getTimeUnit())
		{
         case MINUTE:
            return (long)timePeriod.getTimeRange() * 60L * 1000L;
         case HOUR:
            return (long)timePeriod.getTimeRange() * 60L * 60L * 1000L;
         case DAY:
            return (long)timePeriod.getTimeRange() * 24L * 60L * 60L * 1000L;
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
    * Get Y axis label.
    *
    * @return Y axis label
    */
   public String getYAxisLabel()
   {
      return (yAxisLabel != null) ? yAxisLabel : "";
   }

   /**
    * Set Y axis label (empty string or null to hide).
    * 
    * @param yAxisLabel new Y axis label
    */
   public void setYAxisLabel(String yAxisLabel)
   {
      this.yAxisLabel = yAxisLabel;
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
   public boolean isLegendVisible()
	{
		return showLegend;
	}

	/**
    * @param visible true to set legend visible
    */
   public void setLegendVisible(boolean visible)
	{
      this.showLegend = visible;
	}

   /**
    * @return true if labels are visible
    */
   public boolean areLabelsVisible()
   {
      return showLabels;
   }

   /**
    * @param visible true to set labels visible
    */
   public void setLabelsVisible(boolean visible)
   {
      this.showLabels = visible;
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
    * @return true if title is visible
    */
	public boolean isTitleVisible()
	{
		return showTitle;
	}

	/**
    * @param visible true to set title visible
    */
   public void setTitleVisible(boolean visible)
	{
      this.showTitle = visible;
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
    * Check if grid is visible.
    *
    * @return true if grid is visible
    */
   public boolean isGridVisible()
   {
      return showGrid;
   }

   /**
    * Set grid visibility flag.
    *
    * @param visible true if grid should be visible
    */
   public void setGridVisible(boolean visible)
   {
      this.showGrid = visible;
   }

   /**
    * @return the stacked
    */
   public boolean isStacked()
   {
      return stacked;
   }

   /**
    * @param stacked the stacked to set
    */
   public void setStacked(boolean stacked)
   {
      this.stacked = stacked;
   }

   /**
    * @return the transposed
    */
   public boolean isTransposed()
   {
      return transposed;
   }

   /**
    * @param transposed the transposed to set
    */
   public void setTransposed(boolean transposed)
   {
      this.transposed = transposed;
   }

   /**
    * @return the translucent
    */
   public boolean isTranslucent()
   {
      return translucent;
   }

   /**
    * @param translucent the translucent to set
    */
   public void setTranslucent(boolean translucent)
   {
      this.translucent = translucent;
   }

   /**
    * @return the doughnutRendering
    */
   public boolean isDoughnutRendering()
   {
      return doughnutRendering;
   }

   /**
    * @param doughnutRendering the doughnutRendering to set
    */
   public void setDoughnutRendering(boolean doughnutRendering)
   {
      this.doughnutRendering = doughnutRendering;
   }

   /**
    * @return the showTotal
    */
   public boolean isShowTotal()
   {
      return showTotal;
   }

   /**
    * @param showTotal the showTotal to set
    */
   public void setShowTotal(boolean showTotal)
   {
      this.showTotal = showTotal;
   }

   /**
    * @return the extendedLegend
    */
   public boolean isExtendedLegend()
   {
      return extendedLegend;
   }

   /**
    * @param extendedLegend the extendedLegend to set
    */
   public void setExtendedLegend(boolean extendedLegend)
   {
      this.extendedLegend = extendedLegend;
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
    * @return true if auto scale is set
    */
   public boolean isAutoScale()
   {
      return autoScale;
   }

   /**
    * @param autoScale Set auto scale
    */
   public void setAutoScale(boolean autoScale)
   {
      this.autoScale = autoScale;
   }

   /**
    * @return the area
    */
   public boolean isArea()
   {
      return area;
   }

   /**
    * @param area the area to set
    */
   public void setArea(boolean area)
   {
      this.area = area;
   }

   /**
    * @return Minimal Y scale
    */
   public double getMinYScaleValue()
   {
      return minYScaleValue;
   }

   /**
    * @param minYScaleValue The scale value to set
    */
   public void setMinYScaleValue(double minYScaleValue)
   {
      this.minYScaleValue = minYScaleValue;
   }

   /**
    * @return Max Y scale
    */
   public double getMaxYScaleValue()
   {
      return maxYScaleValue;
   }

   /**
    * @param maxYScaleValue max Y scale
    */
   public void setMaxYScaleValue(double maxYScaleValue)
   {
      this.maxYScaleValue = maxYScaleValue;
   }

   /**
    * @return The time period
    */
   public TimePeriod getTimePeriod()
   {
      return timePeriod;
   }

   /**
    * Set time period for chart.
    *
    * @param timePeriod new time period
    */
   public void setTimePeriod(TimePeriod timePeriod)
   {
      this.timePeriod = timePeriod;
   }

   /**
    * Get start time for time period.
    *
    * @return start time for time period
    */
   public Date getTimeFrom()
   {
      return timePeriod.getTimeFrom();
   }

   /**
    * Set start time for time period.
    *
    * @param timeFrom new start time for time period
    */
   public void setTimeFrom(Date timeFrom)
   {
      timePeriod.setTimeFrom(timeFrom);
   }

   /**
    * Get end time for time period.
    *
    * @return end time for time period
    */
   public Date getTimeTo()
   {
      return timePeriod.getTimeTo();
   }

   /**
    * Set end time for time period.
    *
    * @param timeTo new end time for time period
    */
   public void setTimeTo(Date timeTo)
   {
      timePeriod.setTimeTo(timeTo);
   }

   /**
    * Add change listener
    * 
    * @param listener change listener
    */
   public void addChangeListener(ChartConfigurationChangeListener listener)
   {
      changeListeners.add(listener);
   }
   
   /**
    * Remove change listener
    * 
    * @param listener change listener to remove
    */
   public void removeChangeListener(ChartConfigurationChangeListener listener)
   {
      changeListeners.remove(listener);
   }
   
   /**
    * Fire change notification
    */
   public void fireChangeNotification()
   {
      for(ChartConfigurationChangeListener l : changeListeners)
         l.onChartConfigurationChange(this);
   }
  
   /**
    * Set modify Y base
    * 
    * @param modifyYBase if true, use min DCI value as Y base
    */
   public void setModifyYBase(boolean modifyYBase)
   {
      this.modifyYBase = modifyYBase;
   }

   /**
    * Check if "modify Y base" flag is set
    * 
    * @return true if "modify Y base" flag is set
    */
   public boolean isModifyYBase()
   {
      return modifyYBase;
   }

   /**
    * @return the useMultipliers
    */
   public boolean isUseMultipliers()
   {
      return useMultipliers;
   }

   /**
    * @param useMultipliers the useMultipliers to set
    */
   public void setUseMultipliers(boolean useMultipliers)
   {
      this.useMultipliers = useMultipliers;
   }

   /**
    * Check if zoom is enabled on chart.
    *
    * @return true if zoom is enabled
    */
   public boolean isZoomEnabled()
   {
      return zoomEnabled;
   }

   /**
    * Enable/disable zoom on chart.
    *
    * @param enabled true to enable zoom
    */
   public void setZoomEnabled(boolean enabled)
   {
      this.zoomEnabled = enabled;
   }

   /**
    * Set rotation angle for chart types where it applicable (like pie chart)
    * 
    * @param angle rotation angle
    */
   public void setRotation(double angle)
   {
      rotation = angle;
   }

   /**
    * Get currently set rotation angle
    * 
    * @return currently set rotation angle
    */
   public double getRotation()
   {
      return rotation;
   }

   /**
    * Check if labels should be placed inside chart.
    *
    * @return true if labels should be placed inside chart
    */
   public boolean areLabelsInside()
   {
      return labelsInside;
   }

   /**
    * Set if labels should be placed inside chart.
    *
    * @param labelsInside true to put labels inside chart
    */
   public void setLabelsInside(boolean labelsInside)
   {
      this.labelsInside = labelsInside;
   }

   /**
    * @return the leftYellowZone
    */
   public double getLeftYellowZone()
   {
      return leftYellowZone;
   }

   /**
    * @param leftYellowZone the leftYellowZone to set
    */
   public void setLeftYellowZone(double leftYellowZone)
   {
      this.leftYellowZone = leftYellowZone;
   }

   /**
    * @return the leftRedZone
    */
   public double getLeftRedZone()
   {
      return leftRedZone;
   }

   /**
    * @param leftRedZone the leftRedZone to set
    */
   public void setLeftRedZone(double leftRedZone)
   {
      this.leftRedZone = leftRedZone;
   }

   /**
    * @return the rightYellowZone
    */
   public double getRightYellowZone()
   {
      return rightYellowZone;
   }

   /**
    * @param rightYellowZone the rightYellowZone to set
    */
   public void setRightYellowZone(double rightYellowZone)
   {
      this.rightYellowZone = rightYellowZone;
   }

   /**
    * @return the rightRedZone
    */
   public double getRightRedZone()
   {
      return rightRedZone;
   }

   /**
    * @param rightRedZone the rightRedZone to set
    */
   public void setRightRedZone(double rightRedZone)
   {
      this.rightRedZone = rightRedZone;
   }

   /**
    * @return the elementBordersVisible
    */
   public boolean isElementBordersVisible()
   {
      return elementBordersVisible;
   }

   /**
    * @param elementBordersVisible the elementBordersVisible to set
    */
   public void setElementBordersVisible(boolean elementBordersVisible)
   {
      this.elementBordersVisible = elementBordersVisible;
   }

   /**
    * @return the gaugeColorMode
    */
   public int getGaugeColorMode()
   {
      return gaugeColorMode;
   }

   /**
    * @param gaugeColorMode the gaugeColorMode to set
    */
   public void setGaugeColorMode(int gaugeColorMode)
   {
      this.gaugeColorMode = gaugeColorMode;
   }

   /**
    * @return the fontName
    */
   public String getFontName()
   {
      return ((fontName != null) && !fontName.isEmpty()) ? fontName : "Verdana";
   }

   /**
    * @param fontName the fontName to set
    */
   public void setFontName(String fontName)
   {
      this.fontName = fontName;
   }

   /**
    * @return the fontSize
    */
   public int getFontSize()
   {
      return fontSize;
   }

   /**
    * @param fontSize the fontSize to set
    */
   public void setFontSize(int fontSize)
   {
      this.fontSize = fontSize;
   }

   /**
    * @return the expectedTextWidth
    */
   public int getExpectedTextWidth()
   {
      return expectedTextWidth;
   }

   /**
    * @param expectedTextWidth the expectedTextWidth to set
    */
   public void setExpectedTextWidth(int expectedTextWidth)
   {
      this.expectedTextWidth = expectedTextWidth;
   }
}
