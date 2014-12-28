/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import java.util.List;

import org.netxms.client.datacollection.GraphItemStyle;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.datacollection.PerfTabDci;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Settings for performance tab graph
 */
@Root(name="config", strict=false)
public class PerfTabGraphSettings
{
	@Element(required=false)
	private boolean enabled = false;
	
   @Element(required = false)
   private boolean autoScale = true;
   
   @Element(required = false)
   private boolean logScaleEnabled = false;
   
   @Element(required = false)
   private int minYScaleValue = 1;
   
   @Element(required = false)
   private int maxYScaleValue = 100;
   
   @Element(required = false)
   private int timeRange = 1;
   
   @Element(required = false)
   private int timeUnits = 1;
	
	@Element(required=false)
	private int type = GraphItemStyle.LINE;
	
	@Element(required=false)
	private String color = "0x00C000"; //$NON-NLS-1$
	
	@Element(required=false)
	private String title = ""; //$NON-NLS-1$
	
	@Element(required=false)
	private String name = ""; //$NON-NLS-1$
	
	@Element(required=false)
	private boolean showThresholds = false;
	
	@Element(required=false)
	private long parentDciId = 0;

	@Element(required=false)
	private String parentDciName = null;
	
	@Element(required=false)
	private int order = 100;
	
	private PerfTabDci runtimeDciInfo = null;

	/**
	 * Create performance tab graph settings object from XML document
	 * 
	 * @param xml XML document
	 * @return deserialized object
	 * @throws Exception if the object cannot be fully deserialized
	 */
	public static PerfTabGraphSettings createFromXml(final String xml) throws Exception
	{
		Serializer serializer = new Persister();
		return serializer.read(PerfTabGraphSettings.class, xml);
	}
	
	/**
	 * Create XML document from object
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
	 * @return the enabled
	 */
	public boolean isEnabled()
	{
		return enabled;
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
	public String getColor()
	{
		return color;
	}

	/**
	 * @return the color
	 */
	public int getColorAsInt()
	{
		try
		{
			if (color.startsWith("0x")) //$NON-NLS-1$
				return Integer.parseInt(color.substring(2), 16);
			return Integer.parseInt(color, 10);
		}
		catch(NumberFormatException e)
		{
			return 0;	// black color in case of error
		}
	}

	/**
	 * Get configured title
	 * 
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}
	
	/**
	 * Get actual title to be shown in runtime
	 * 
	 * @return
	 */
	public String getRuntimeTitle()
	{
		String t = ((title == null) || title.isEmpty()) ? ((runtimeDciInfo != null) ? runtimeDciInfo.getDescription() : "") : title; //$NON-NLS-1$
		return t.replace("{instance}", (runtimeDciInfo != null) ? runtimeDciInfo.getInstance() : ""); //$NON-NLS-1$ //$NON-NLS-2$
	}

	/**
	 * @param enabled the enabled to set
	 */
	public void setEnabled(boolean enabled)
	{
		this.enabled = enabled;
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
	public void setColor(final String color)
	{
		this.color = color;
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(final int color)
	{
		this.color = Integer.toString(color);
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/**
	 * @return the showThresholds
	 */
	public boolean isShowThresholds()
	{
		return showThresholds;
	}

	/**
	 * @param showThresholds the showThresholds to set
	 */
	public void setShowThresholds(boolean showThresholds)
	{
		this.showThresholds = showThresholds;
	}

	/**
	 * @return the parentDciId
	 */
	public final long getParentDciId()
	{
		return parentDciId;
	}

	/**
	 * @param parentDciId the parentDciId to set
	 */
	public final void setParentDciId(long parentDciId)
	{
		this.parentDciId = parentDciId;
	}

	/**
	 * @return the parentDciName
	 */
	public final String getParentDciName()
	{
		return parentDciName;
	}

	/**
	 * @param parentDciName the parentDciName to set
	 */
	public final void setParentDciName(String parentDciName)
	{
		this.parentDciName = parentDciName;
	}

	/**
	 * @return the runtimeDciInfo
	 */
	public final PerfTabDci getRuntimeDciInfo()
	{
		return runtimeDciInfo;
	}

	/**
	 * @param runtimeDciInfo the runtimeDciInfo to set
	 */
	public final void setRuntimeDciInfo(PerfTabDci runtimeDciInfo)
	{
		this.runtimeDciInfo = runtimeDciInfo;
	}

	/**
	 * Get configured name for chart legend
	 * 
	 * @return the name
	 */
	public final String getName()
	{
		return name;
	}

	/**
	 * @param name the name to set
	 */
	public final void setName(String name)
	{
		this.name = name;
	}
	
	/**
	 * Get runtime name for chart legend
	 * 
	 * @return the name
	 */
	public String getRuntimeName()
	{
		String n = ((name == null) || name.isEmpty()) ? ((runtimeDciInfo != null) ? runtimeDciInfo.getDescription() : "") : name; //$NON-NLS-1$
		return n.replace("{instance}", (runtimeDciInfo != null) ? runtimeDciInfo.getInstance() : ""); //$NON-NLS-1$ //$NON-NLS-2$
	}

	/**
	 * Fix parent DCI ID (it can be template DCI ID - replace it with real DCI ID).
	 * 
	 * @param settings list of all DCIs for performance tab
	 */
	public final void fixParentDciId(List<PerfTabGraphSettings> settings)
	{
		if (parentDciId == 0)
			return;
		
		for(PerfTabGraphSettings s : settings)
		{
			if (parentDciId == s.getRuntimeDciInfo().getId())
				return;	// found valid parent ID
			if (((parentDciId == s.getRuntimeDciInfo().getTemplateDciId()) || (parentDciId == s.getRuntimeDciInfo().getRootTemplateDciId())) &&
			    s.getRuntimeDciInfo().getInstance().equals(runtimeDciInfo.getInstance()))
			{
				// found parent ID from template
				parentDciId = s.getRuntimeDciInfo().getId();
				return;
			}
		}
	}

	/**
	 * @return the order
	 */
	public final int getOrder()
	{
		return order;
	}

	/**
	 * @param order the order to set
	 */
	public final void setOrder(int order)
	{
		this.order = order;
	}
	
	public boolean isAutoScale()
   {
      return autoScale;
   }

   public void setAutoScale(boolean autoScale)
   {
      this.autoScale = autoScale;
   }

   public int getMinYScaleValue()
   {
      return minYScaleValue;
   }

   public void setMinYScaleValue(int minYScaleValue)
   {
      this.minYScaleValue = minYScaleValue;
   }

   public int getMaxYScaleValue()
   {
      return maxYScaleValue;
   }

   public void setMaxYScaleValue(int maxYScaleValue)
   {
      this.maxYScaleValue = maxYScaleValue;
   }

   /**
    * @return the timeInterval
    */
   public int getTimeRange()
   {
      return timeRange;
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
    * @param timeInterval the timeInterval to set
    */
   public void setTimeRange(int timeInterval)
   {
      this.timeRange = timeInterval;
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
    * @return the logScaleEnabled
    */
   public boolean isLogScaleEnabled()
   {
      return logScaleEnabled;
   }

   /**
    * @param logScaleEnabled the logScaleEnabled to set
    */
   public void setLogScaleEnabled(boolean logScaleEnabled)
   {
      this.logScaleEnabled = logScaleEnabled;
   }
}
