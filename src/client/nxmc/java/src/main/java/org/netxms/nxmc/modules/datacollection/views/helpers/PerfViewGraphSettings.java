/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views.helpers;

import org.netxms.client.TimePeriod;
import org.netxms.client.constants.TimeFrameType;
import org.netxms.client.constants.TimeUnit;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.nxmc.Registry;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Settings for performance view graph
 */
/**
 * 
 */
@Root(name = "config", strict = false)
public class PerfViewGraphSettings
{
   @Element(required = false)
   private boolean enabled = false;

   @Element(required = false)
   private boolean autoScale = true;

   @Element(required = false)
   private boolean logScaleEnabled = false;

   @Element(required = false)
   private boolean stacked = false;

   @Element(required = false)
   private boolean showLegendAlways = false;

   @Element(required = false)
   private boolean extendedLegend = true;

   @Element(required = false)
   private boolean useMultipliers = true;

   @Element(required = false)
   private double minYScaleValue = 0;

   @Element(required = false)
   private double maxYScaleValue = 100;

   @Element(required = false)
   private int timeRange = 1;

   @Element(required = false)
   private int timeUnits = 1;

   @Element(required = false)
   private int type = 1;

   @Element(required = false)
   private String color = "0x00C000";

   @Element(required = false)
   private boolean automaticColor = true;

   @Element(required = false)
   private String title = "";

   @Element(required = false)
   private String name = "";

   @Element(required = false)
   private boolean showThresholds = false;

   @Element(required = false)
   private String groupName = null;

   @Element(required = false)
   private int order = 100;

   @Element(required = false)
   private long parentDciId = 0;

   @Element(required = false)
   private boolean modifyYBase = false;

   @Element(required = false)
   private String yAxisLabel = null;

   @Element(required = false)
   private boolean invertedValues = false;

   @Element(required = false)
   private boolean translucent = true;

   private PerfTabDci runtimeDciInfo = null;

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
         return 0; // black color in case of error
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
      return t.replace("{instance}", (runtimeDciInfo != null) ? runtimeDciInfo.getInstance() : "") //$NON-NLS-1$ //$NON-NLS-2$
            .replace("{instance-name}", (runtimeDciInfo != null) ? runtimeDciInfo.getInstanceName() : "") //$NON-NLS-1$ //$NON-NLS-2$
            .replace("{node-name}", (runtimeDciInfo != null) ? Registry.getSession().getObjectNameWithAlias(runtimeDciInfo.getNodeId()) : ""); //$NON-NLS-1$ //$NON-NLS-2$
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
    * @return the groupName
    */
   public String getGroupName()
   {
      if (groupName != null)
         return runtimeDciInfo != null ? groupName.replace("{instance}", runtimeDciInfo.getInstance()).replace("{instance-name}", runtimeDciInfo.getInstanceName()) : groupName;
      return parentDciId != 0 ? "##" + Long.toString(parentDciId) : "";
   }

   /**
    * @param groupName the groupName to set
    */
   public void setGroupName(String groupName)
   {
      this.groupName = groupName;
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
      return n.replace("{instance}", (runtimeDciInfo != null) ? runtimeDciInfo.getInstance() : "") //$NON-NLS-1$ //$NON-NLS-2$
            .replace("{instance-name}", (runtimeDciInfo != null) ? runtimeDciInfo.getInstanceName() : "") //$NON-NLS-1$ //$NON-NLS-2$
            .replace("{node-name}", (runtimeDciInfo != null) ? Registry.getSession().getObjectNameWithAlias(runtimeDciInfo.getNodeId()) : ""); //$NON-NLS-1$ //$NON-NLS-2$
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

   /**
    * @return
    */
   public boolean isAutoScale()
   {
      return autoScale;
   }

   /**
    * @param autoScale
    */
   public void setAutoScale(boolean autoScale)
   {
      this.autoScale = autoScale;
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
    * @return
    */
   public double getMinYScaleValue()
   {
      return minYScaleValue;
   }

   /**
    * @param minYScaleValue
    */
   public void setMinYScaleValue(double minYScaleValue)
   {
      this.minYScaleValue = minYScaleValue;
   }

   /**
    * @return
    */
   public double getMaxYScaleValue()
   {
      return maxYScaleValue;
   }

   /**
    * @param maxYScaleValue
    */
   public void setMaxYScaleValue(double maxYScaleValue)
   {
      this.maxYScaleValue = maxYScaleValue;
   }

   /**
    * Get graph time interval expressed in time units.
    *
    * @return graph time interval expressed in time units
    */
   public int getTimeRange()
   {
      return timeRange;
   }

   /**
    * Get time range covered by graph in milliseconds.
    * 
    * @return time range covered by graph in milliseconds
    */
   public long getTimeRangeMillis()
   {
      switch(TimeUnit.getByValue(timeUnits))
      {
         case MINUTE:
            return (long)timeRange * 60L * 1000L;
         case HOUR:
            return (long)timeRange * 60L * 60L * 1000L;
         case DAY:
            return (long)timeRange * 24L * 60L * 60L * 1000L;
      }
      return 0;
   }

   /**
    * Get time range covered by graph as TimePeriod object.
    *
    * @return time range covered by graph as TimePeriod object
    */
   public TimePeriod getTimePeriod()
   {
      return new TimePeriod(TimeFrameType.BACK_FROM_NOW, timeRange, TimeUnit.getByValue(timeUnits), null, null);
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

   /**
    * @return the showLegendAlways
    */
   public boolean isShowLegendAlways()
   {
      return showLegendAlways;
   }

   /**
    * @param showLegendAlways the showLegendAlways to set
    */
   public void setShowLegendAlways(boolean showLegendAlways)
   {
      this.showLegendAlways = showLegendAlways;
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
    * Set modify Y base
    * 
    * @param modifyYBase if true, use min DCI value as Y base
    */
   public void setModifyYBase(boolean modifyYBase)
   {
      this.modifyYBase = modifyYBase;
   }

   /**
    * Modify Y base
    * 
    * @return true if use min DCI value as Y base
    */
   public boolean modifyYBase()
   {
      return modifyYBase;
   }

   /**
    * @return the yAxisLabel
    */
   public String getYAxisLabel()
   {
      return (yAxisLabel != null) ? yAxisLabel : "";
   }

   /**
    * @param yAxisLabel the yAxisLabel to set
    */
   public void setYAxisLabel(String yAxisLabel)
   {
      this.yAxisLabel = yAxisLabel;
   }

   /**
    * @return the invertedValues
    */
   public boolean isInvertedValues()
   {
      return invertedValues;
   }

   /**
    * @param invertedValues the invertedValues to set
    */
   public void setInvertedValues(boolean invertedValues)
   {
      this.invertedValues = invertedValues;
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
    * @return the automaticColor
    */
   public boolean isAutomaticColor()
   {
      return automaticColor;
   }

   /**
    * @param automaticColor the automaticColor to set
    */
   public void setAutomaticColor(boolean automaticColor)
   {
      this.automaticColor = automaticColor;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "PerfTabGraphSettings [enabled=" + enabled + ", autoScale=" + autoScale + ", logScaleEnabled=" + logScaleEnabled + ", stacked=" + stacked + ", showLegendAlways=" + showLegendAlways +
            ", extendedLegend=" + extendedLegend + ", useMultipliers=" + useMultipliers + ", minYScaleValue=" + minYScaleValue + ", maxYScaleValue=" + maxYScaleValue + ", timeRange=" + timeRange +
            ", timeUnits=" + timeUnits + ", type=" + type + ", color=" + color + ", title=" + title + ", name=" + name + ", showThresholds=" + showThresholds + ", groupName=" + groupName +
            ", order=" + order + ", parentDciId=" + parentDciId + ", modifyYBase=" + modifyYBase + ", invertedValues=" + invertedValues + ", translucent=" + translucent + ", runtimeDciInfo=" +
            runtimeDciInfo + "]";
   }
}
