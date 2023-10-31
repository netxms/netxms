/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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

import java.io.StringWriter;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.netxms.client.maps.NetworkMapLink;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.ElementArray;
import org.simpleframework.xml.ElementList;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

@Root(name = "config", strict = false)
public class LinkConfig
{
   @ElementArray(required = false)
   private SingleDciConfig[] dciList;

   @ElementList(required = false)
   private List<Long> objectStatusList = new ArrayList<Long>();

   @Element(required = false)
   private int routing;

   @Element(required = false)
   private long[] bendPoints;

   @Element(required = false)
   private boolean useActiveThresholds;

   @Element(required = false)
   private int labelPosition = 50;

   /**
    * Default constructor
    */
   public LinkConfig()
   {           
      routing = NetworkMapLink.ROUTING_DEFAULT;
      bendPoints = null;
      dciList = null;
      useActiveThresholds = false;
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
   public LinkConfig(SingleDciConfig[] dciList, List<Long> objectStatusList, int routing, long[] bendPoints, boolean useActiveThresholds, boolean isLocked)
   {
      this.dciList = dciList;
      this.objectStatusList = objectStatusList;
      this.routing = routing;
      this.bendPoints = bendPoints;
      this.useActiveThresholds = useActiveThresholds;
   }   
   
   /**
    * Create link object from XML document.
    *
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static LinkConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(LinkConfig.class, xml);
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
   public SingleDciConfig[] getDciList()
   {
      return dciList;
   }

   /**
    * @param dciList the dciList to set
    */
   public void setDciList(SingleDciConfig[] dciList)
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
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "LinkConfig [dciList=" + Arrays.toString(dciList) + ", objectStatusList=" + objectStatusList.toString() +
            ", routing=" + routing + ", bendPoints=" + Arrays.toString(bendPoints) + ", labelPosition=" + labelPosition + "]";
   }
}
