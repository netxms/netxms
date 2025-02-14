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
package org.netxms.client.maps.configs;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.netxms.client.maps.NetworkMapLink;

public class LinkConfig
{
   private MapLinkDataSource[] dciList;
   private List<Long> objectStatusList = new ArrayList<Long>();
   private int routing;
   private long[] bendPoints;
   private boolean useActiveThresholds;
   private boolean useInterfaceUtilization;
   private int labelPosition = 50;
   private int style = 0;   
   private int width = 0;

   /**
    * Default constructor
    */
   public LinkConfig()
   {           
      routing = NetworkMapLink.ROUTING_DEFAULT;
      bendPoints = null;
      dciList = null;
      useActiveThresholds = false;
      useInterfaceUtilization = false;
   }

   /**
    * Create link object from scratch.
    *
    * @param dciList DCI list
    * @param objectStatusList list of object identifiers for status calculation (can be null)
    * @param routing routing type
    * @param bendPoints list of bend points (can be null)
    * @param useActiveThresholds true to use active DCI thresholds for status calculation
    * @param isLocked true if link is locked
    */
   public LinkConfig(MapLinkDataSource[] dciList, List<Long> objectStatusList, int routing, long[] bendPoints, boolean useActiveThresholds, boolean useInterfaceUtilization, boolean isLocked)
   {
      this.dciList = dciList;
      this.objectStatusList = objectStatusList;
      this.routing = routing;
      this.bendPoints = bendPoints;
      this.useActiveThresholds = useActiveThresholds;
      this.useInterfaceUtilization = useInterfaceUtilization;
   }   

   /**
    * @return the objectStatusList
    */
   public List<Long> getObjectStatusList()
   {
      return (objectStatusList != null) ? objectStatusList : new ArrayList<Long>(0);
   }

   /**
    * @param objectStatusList the objectStatusList to set
    */
   public void setObjectStatusList(List<Long> objectStatusList)
   {
      this.objectStatusList = objectStatusList;
   }

   /**
    * @return the dciList
    */
   public MapLinkDataSource[] getDciList()
   {
      return dciList;
   }

   /**
    * @param dciList the dciList to set
    */
   public void setDciList(MapLinkDataSource[] dciList)
   {
      this.dciList = dciList;
   }

   /**
    * @return the routing
    */
   public int getRouting()
   {
      return routing;
   }

   /**
    * @param routing the routing to set
    */
   public void setRouting(int routing)
   {
      this.routing = routing;
   }

   /**
    * @return the bendPoints
    */
   public long[] getBendPoints()
   {
      return bendPoints;
   }

   /**
    * @param bendPoints the bendPoints to set
    */
   public void setBendPoints(long[] bendPoints)
   {
      this.bendPoints = bendPoints;
   }
   
   /**
    * @param useActiveThresholds set use active thresholds
    */
   public void setUseActiveThresholds(boolean useActiveThresholds)
   {
      this.useActiveThresholds = useActiveThresholds;
   }
   
   /**
    * @return are active thresholds used
    */
   public boolean isUseActiveThresholds()
   {
      return useActiveThresholds;
   }

   /**
    * @return the useInterfaceUtilization
    */
   public boolean isUseInterfaceUtilization()
   {
      return useInterfaceUtilization;
   }

   /**
    * @param useInterfaceUtilization the useInterfaceUtilization to set
    */
   public void setUseInterfaceUtilization(boolean useInterfaceUtilization)
   {
      this.useInterfaceUtilization = useInterfaceUtilization;
   }

   /**
    * @return the labelPosition
    */
   public int getLabelPosition()
   {
      return labelPosition;
   }

   /**
    * @param labelPosition the labelPosition to set
    */
   public void setLabelPosition(int labelPosition)
   {
      this.labelPosition = labelPosition;
   }

   /**
    * @return the style
    */
   public int getStyle()
   {
      return style;
   }

   /**
    * @param style the style to set
    */
   public void setStyle(int style)
   {
      this.style = style;
   }

   /**
    * @return the width
    */
   public int getWidth()
   {
      return width;
   }

   /**
    * @param width the width to set
    */
   public void setWidth(int width)
   {
      this.width = width;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "LinkConfig [dciList=" + Arrays.toString(dciList) + ", objectStatusList=" + objectStatusList.toString() +
            ", routing=" + routing + ", bendPoints=" + Arrays.toString(bendPoints) + ", labelPosition=" + labelPosition + 
            ", lineStyle=" + style +", lineWidth=" + width + "]";
   }
}
