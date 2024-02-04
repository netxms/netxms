/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.base.annotations.Internal;
import org.netxms.client.AccessListElement;
import org.netxms.client.ObjectMenuFilter;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectAction;
import org.netxms.client.xml.XMLTools;
import org.simpleframework.xml.ElementArray;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Settings for predefined graph
 */
public class GraphDefinition extends ChartConfiguration implements ObjectAction
{
   private static Logger logger = LoggerFactory.getLogger(GraphDefinition.class);

   // Access rights
   public static final int ACCESS_READ  = 0x01;
   public static final int ACCESS_WRITE = 0x02;

   // Flags
   public static final int GF_TEMPLATE = 0x0001;

   @Internal
   private GraphFolder parent; // Used by predefined graph tree

   private long id;
   private long ownerId;
   private int flags;
   private String name;
   private String shortName;
   private String displayName;
   private List<AccessListElement> accessList;
   private ObjectMenuFilter filter;

   @ElementArray(required = true)
   protected ChartDciConfig[] dciList = new ChartDciConfig[0];

   /**
    * Create empty graph definition
    */
   public GraphDefinition()
   {
      id = 0;
      ownerId = 0;
      flags = 0;
      name = "noname";
      shortName = "noname";
      displayName = "noname";
      accessList = new ArrayList<AccessListElement>(0);
      filter = new ObjectMenuFilter();
      parent = null;
   }

   /**
    * Create graph definition
    *
    * @param id definition ID
    * @param ownerId owning user ID
    * @param flags flags
    * @param accessList graph access list
    */
   public GraphDefinition(long id, long ownerId, int flags, Collection<AccessListElement> accessList)
   {
      this.id = id;
      this.ownerId = ownerId;
      name = "noname";
      shortName = "noname";
      displayName = "noname";
      this.flags = flags;
      this.accessList = new ArrayList<AccessListElement>(accessList.size());
      this.accessList.addAll(accessList);
      filter = new ObjectMenuFilter();
      parent = null;
   }

   /**
    * Create copy of provided graph definition (with template flag removed if set in source).
    *
    * @param src source definition
    * @param title new title (or null to keep original)
    */
   public GraphDefinition(GraphDefinition src, String title)
   {
      super(src);
      id = src.id;
      ownerId = src.ownerId;
      this.name = src.name;
      shortName = src.shortName;
      displayName = src.displayName;
      flags = src.flags & ~GF_TEMPLATE;
      this.accessList = new ArrayList<AccessListElement>(src.accessList.size());
      this.accessList.addAll(src.accessList);
      filter = src.filter;
      parent = null;
      if (title != null)
         this.title = title;

      dciList = new ChartDciConfig[src.dciList.length];
      for(int i = 0; i < dciList.length; i++)
         dciList[i] = new ChartDciConfig(src.dciList[i]);
   }

   /**
    * Create copy of provided graph definition
    *
    * @param src source object
    */
   public GraphDefinition(GraphDefinition src)
   {
      super(src);
      id = src.id;
      ownerId = src.ownerId;
      this.name = src.name;
      shortName = src.shortName;
      displayName = src.displayName;
      flags = src.flags & ~GF_TEMPLATE;
      this.accessList = new ArrayList<AccessListElement>(src.accessList.size());
      this.accessList.addAll(src.accessList);
      filter = src.filter;
      parent = null;

      dciList = new ChartDciConfig[src.dciList.length];
      for(int i = 0; i < dciList.length; i++)
         dciList[i] = new ChartDciConfig(src.dciList[i]);
   }

   /**
    * Create graph settings object from NXCP message
    *
    * @param msg    NXCP message
    * @param baseId base variable id
    * @return The graph settings object
    */
   public static GraphDefinition createGraphSettings(final NXCPMessage msg, long baseId)
   {
      GraphDefinition gs;
      try
      {
         gs = XMLTools.createFromXml(GraphDefinition.class, msg.getFieldAsString(baseId + 4));
      }
      catch(Exception e)
      {
         gs = new GraphDefinition();
         logger.debug("Cannot create GraphSettings object from XML", e);
      }
      gs.id = msg.getFieldAsInt64(baseId);
      gs.ownerId = msg.getFieldAsInt64(baseId + 1);
      gs.flags = (int)msg.getFieldAsInt64(baseId + 2);
      gs.name = msg.getFieldAsString(baseId + 3);

      String filterXml = msg.getFieldAsString(baseId + 5);
      if ((filterXml != null) && !filterXml.isEmpty())
      {
         try
         {
            gs.filter = XMLTools.createFromXml(ObjectMenuFilter.class, filterXml);
         }
         catch(Exception e)
         {
            logger.debug("Cannot create ObjectMenuFilter object from XML", e);
            gs.filter = new ObjectMenuFilter();
         }
      }
      else
      {
         gs.filter = new ObjectMenuFilter();
      }

      String[] parts = gs.name.split("->");
      gs.shortName = (parts.length > 1) ? parts[parts.length - 1] : gs.name;
      gs.displayName = gs.shortName.replace("&", "");

      int count = msg.getFieldAsInt32(baseId + 6);  // ACL size
      long[] users = msg.getFieldAsUInt32Array(baseId + 7);
      long[] rights = msg.getFieldAsUInt32Array(baseId + 8);
      gs.accessList = new ArrayList<AccessListElement>(count);
      for(int i = 0; i < count; i++)
      {
         gs.accessList.add(new AccessListElement((int)users[i], (int)rights[i]));
      }

      return gs;
   }

   /**
    * Fill NXCP message
    *
    * @param msg NXCP message
    */
   public void fillMessage(NXCPMessage msg)
   {
      msg.setFieldInt32(NXCPCodes.VID_GRAPH_ID, (int)id);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      msg.setField(NXCPCodes.VID_FILTER, filter.createXml());
      try
      {
         msg.setField(NXCPCodes.VID_GRAPH_CONFIG, createXml());
      }
      catch(Exception e)
      {
         logger.debug("Cannot convert GraphSettings object to XML", e);
         msg.setField(NXCPCodes.VID_GRAPH_CONFIG, "");
      }
      msg.setFieldInt32(NXCPCodes.VID_ACL_SIZE, accessList.size());
      long varId = NXCPCodes.VID_GRAPH_ACL_BASE;
      for(AccessListElement el : accessList)
      {
         msg.setFieldInt32(varId++, (int)el.getUserId());
         msg.setFieldInt32(varId++, el.getAccessRights());
      }
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
    * @param accessList the accessList to set
    */
   public void setAccessList(Collection<AccessListElement> accessList)
   {
      this.accessList = new ArrayList<AccessListElement>(accessList);
   }

   /**
    * Get short name (last part of full path separated by "-&gt;")
    *
    * @return short name
    */
   public String getShortName()
   {
      return shortName;
   }

   /**
    * Get display name (short name with &amp; shortcut marks removed)
    *
    * @return display name
    */
   public String getDisplayName()
   {
      return displayName;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
      String[] parts = name.split("->");
      shortName = (parts.length > 1) ? parts[parts.length - 1] : name;
      displayName = shortName.replace("&", "");
   }

   /**
    * Set predefined graph flags.
    *
    * @param flags new predefined graph flags
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }

   /**
    * Get predefined graph flags.
    *
    * @return predefined graph flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * Checks if this graph definition is template
    *
    * @return isTemplate
    */
   public boolean isTemplate()
   {
      return (flags & GF_TEMPLATE) != 0;
   }

   /**
    * Get parent folder
    *
    * @return parent folder
    */
   public GraphFolder getParent()
   {
      return parent;
   }

   /**
    * Set parent folder
    *
    * @param parent new parent folder
    */
   public void setParent(GraphFolder parent)
   {
      this.parent = parent;
   }

   /**
    * @see org.netxms.client.objecttools.ObjectAction#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return filter.isApplicableForObject(object);
   }

   /**
    * @see org.netxms.client.objecttools.ObjectAction#getMenuFilter()
    */
   @Override
   public ObjectMenuFilter getMenuFilter()
   {
      return filter;
   }

   /**
    * @see org.netxms.client.objecttools.ObjectAction#setMenuFilter(org.netxms.client.ObjectMenuFilter)
    */
   @Override
   public void setMenuFilter(ObjectMenuFilter filter)
   {
      this.filter = filter;
   }

   /**
    * @see org.netxms.client.objecttools.ObjectAction#getToolType()
    */
   @Override
   public int getToolType()
   {
      return 0;
   }

   /**
    * @param id the id to set
    */
   public void setId(long id)
   {
      this.id = id;
   }

   /**
    * @param ownerId the ownerId to set
    */
   public void setOwnerId(long ownerId)
   {
      this.ownerId = ownerId;
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
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "GraphSettings [parent=" + ((parent != null) ? parent.getName() : "null") + ", id=" + id + ", ownerId=" + ownerId
            + ", flags=" + flags + ", name=" + name + ", shortName=" + shortName + ", displayName=" + displayName + ", accessList="
            + accessList + ", filter=" + filter + ", dciList=" + Arrays.toString(dciList) + ", title=" + title + ", legendPosition="
            + legendPosition + ", showLegend=" + showLegend + ", extendedLegend=" + extendedLegend + ", showTitle=" + showTitle
            + ", showGrid=" + showGrid + ", showHostNames=" + showHostNames + ", autoRefresh=" + autoRefresh + ", logScale="
            + logScale + ", stacked=" + stacked + ", translucent=" + translucent + ", area=" + area + ", lineWidth=" + lineWidth
            + ", autoScale=" + autoScale + ", minYScaleValue=" + minYScaleValue + ", maxYScaleValue=" + maxYScaleValue
            + ", refreshRate=" + refreshRate + ", timeUnits=" + timeUnit + ", timeRange=" + timeRange + ", timeFrameType="
            + timeFrameType + ", timeFrom=" + timeFrom + ", timeTo=" + timeTo + "]";
   }
}
