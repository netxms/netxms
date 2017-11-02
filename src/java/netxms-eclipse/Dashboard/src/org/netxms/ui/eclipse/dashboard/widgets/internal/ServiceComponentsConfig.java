/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.eclipse.dashboard.widgets.internal;

import java.io.StringWriter;
import java.io.Writer;
import java.util.Map;
import java.util.Set;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.ui.eclipse.dashboard.dialogs.helpers.ObjectIdMatchingData;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;

/**
 * Configuration for service components widget
 */
public class ServiceComponentsConfig extends DashboardElementConfig
{
   @Element(required=true)
   private long objectId = 0;
   
   @Element(required=false)
   private String title = "";
   
   @Element(required=false)
   private int zoomLevel = 100;  // in percents

   @Element(required=false)
   private boolean objectDoubleClickEnabled = false;

   @Element(required=false)
   private MapObjectDisplayMode objectDisplayMode = MapObjectDisplayMode.ICON;
   
   @Element(required=false)
   private int flags = 0;
   
   @Element(required=false)
   private int linkRouting = 0;

   @Element(required=false)
   private int defaultLinkColor = -1;

   @Element(required=false)
   private MapLayoutAlgorithm mapLayout = MapLayoutAlgorithm.SPARSE_VTREE;
   
   /**
    * Create line chart settings object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static ServiceComponentsConfig createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister();
      return serializer.read(ServiceComponentsConfig.class, xml);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#createXml()
    */
   @Override
   public String createXml() throws Exception
   {
      Serializer serializer = new Persister();
      Writer writer = new StringWriter();
      serializer.write(this, writer);
      return writer.toString();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#getObjects()
    */
   @Override
   public Set<Long> getObjects()
   {
      Set<Long> objects = super.getObjects();
      objects.add(objectId);
      return objects;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig#remapObjects(java.util.Map)
    */
   @Override
   public void remapObjects(Map<Long, ObjectIdMatchingData> remapData)
   {
      super.remapObjects(remapData);
      ObjectIdMatchingData md = remapData.get(objectId);
      if (md != null)
         objectId = md.dstId;
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
    * @return the objectId
    */
   public long getObjectId()
   {
      return objectId;
   }

   /**
    * @param objectId the objectId to set
    */
   public void setObjectId(long objectId)
   {
      this.objectId = objectId;
   }
   
   /**
    * @return the zoomLevel
    */
   public int getZoomLevel()
   {
      return zoomLevel;
   }

   /**
    * @param zoomLevel the zoomLevel to set
    */
   public void setZoomLevel(int zoomLevel)
   {
      this.zoomLevel = zoomLevel;
   }

   /**
    * @return the objectDoubleClickEnabled
    */
   public boolean isObjectDoubleClickEnabled()
   {
      return objectDoubleClickEnabled;
   }

   /**
    * @param objectDoubleClickEnabled the objectDoubleClickEnabled to set
    */
   public void setObjectDoubleClickEnabled(boolean objectDoubleClickEnabled)
   {
      this.objectDoubleClickEnabled = objectDoubleClickEnabled;
   }
   
   /**
    * Get map object display mode
    * @return object display mode
    */
   public MapObjectDisplayMode getObjectDisplayMode()
   {
      return objectDisplayMode;
   }
   
   /**
    * Set object display mode
    * @param mode of object display
    */
   public void setObjectDisplayMode(MapObjectDisplayMode mode)
   {
      objectDisplayMode = mode;
   }
   
   /**
    * Get object flags
    * @return flags
    */
   public int getFlags()
   {
      return flags;
   }
   
   /**
    * Set object flags
    * @param flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }
   
   /**
    * Get default link routing
    * @return default link routing
    */
   public int getDefaultLinkRouting()
   {
      return linkRouting;
   }
   
   /**
    * Set default link routing
    * @param linkRouting to set
    */
   public void setDefaultLinkRouting(int linkRouting)
   {
      this.linkRouting = linkRouting;
   }
   
   /**
    * @return the defaultLinkColor
    */
   public int getDefaultLinkColor()
   {
      return defaultLinkColor;
   }
   
   /**
    * Set default link color
    * @param defaultLinkColor to set
    */
   public void setDefaultLinkColor(int defaultLinkColor)
   {
      this.defaultLinkColor = defaultLinkColor;
   }
   
   /**
    * Get map layout algorithm
    * @return layout algorithm
    */
   public MapLayoutAlgorithm getDefaultLayoutAlgorithm()
   {
      return mapLayout;
   }
   
   /**
    * Set map layoyut algorithm
    * @param algorithm to set
    */
   public void setDefaultLayoutAlgorithm(MapLayoutAlgorithm algorithm)
   {
      mapLayout = algorithm;
   }
}
