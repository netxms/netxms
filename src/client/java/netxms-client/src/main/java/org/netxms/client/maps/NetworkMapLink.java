/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.client.maps;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.configs.LinkConfig;
import org.netxms.client.maps.configs.MapLinkDataSource;
import org.netxms.client.maps.configs.MapDataSource;
import com.google.gson.Gson;

/**
 * Represents link between two elements on map.
 */
public class NetworkMapLink
{
   //Link flags
   public static final int AUTO_GENERATED = 0x1;
   public static final int EXCLUDE_FROM_AUTO_UPDATE = 0x2; 

   // Link types
   public static final int NORMAL = 0;
   public static final int VPN = 1;
   public static final int MULTILINK = 2;
   public static final int AGENT_TUNEL = 3;
   public static final int AGENT_PROXY = 4;
   public static final int SSH_PROXY = 5;
   public static final int SNMP_PROXY = 6;
   public static final int ICMP_PROXY = 7;
   public static final int SENSOR_PROXY = 8;
   public static final int ZONE_PROXY = 9;

   // Routing types
   public static final int ROUTING_DEFAULT = 0;
   public static final int ROUTING_DIRECT = 1;
   public static final int ROUTING_MANHATTAN = 2;
   public static final int ROUTING_BENDPOINTS = 3;

   // Color sources
   public static final int COLOR_SOURCE_UNDEFINED = -1;
   public static final int COLOR_SOURCE_DEFAULT = 0;
   public static final int COLOR_SOURCE_OBJECT_STATUS = 1;
   public static final int COLOR_SOURCE_CUSTOM_COLOR = 2;
   public static final int COLOR_SOURCE_SCRIPT = 3;
   public static final int COLOR_SOURCE_LINK_UTILIZATION = 4;
   public static final int COLOR_SOURCE_INTERFACE_STATUS = 5;

   private long id;
   private String name;
   private int type;
   private long element1;
   private long element2;
   private long interfaceId1;
   private long interfaceId2;
   private String connectorName1;
   private String connectorName2;
   private int colorSource;
   private int color;
   private String colorProvider;
   private LinkConfig config = new LinkConfig();
   private int flags;
   private int duplicateCount = 0;
   private int position = 0;
   private long commonFirstElement = 0;
   private boolean directionInverted = false;

   /**
    * Create link object from scratch.
    *
    * @param id link object ID
    * @param name link name (will be displayed in the middle of the link)
    * @param type link type
    * @param element1 network map internal element id
    * @param interfaceId1 ID of interface object on element1 side
    * @param element2 network map internal element id
    * @param interfaceId2 ID of interface object on element2 side
    * @param connectorName1 connector name 1 (will be displayed close to element 1)
    * @param connectorName2 connector name 2 (will be displayed close to element 2)
    * @param dciList list of DCIs to be displayed on the link (can be null)
    * @param flags link flags
    */
   public NetworkMapLink(long id, String name, int type, long element1, long interfaceId1, long element2, long interfaceId2, String connectorName1, String connectorName2,
         MapLinkDataSource[] dciList, int flags)
   {
      this.id = id;
      this.name = name;
      this.type = type;
      this.element1 = element1;
      this.element2 = element2;
      this.interfaceId1 = interfaceId1;
      this.interfaceId2 = interfaceId2;
      this.connectorName1 = connectorName1;
      this.connectorName2 = connectorName2;
      this.flags = flags;
      this.colorSource = COLOR_SOURCE_DEFAULT;
      this.color = 0;
      this.colorProvider = null;
      config.setDciList(dciList);
   }

   /**
    * Create link object from scratch.
    *
    * @param id link object ID
    * @param name link name (will be displayed in the middle of the link)
    * @param type link type
    * @param element1 network map internal element id
    * @param element2 network map internal element id
    * @param connectorName1 connector name 1 (will be displayed close to element 1)  
    * @param connectorName2 connector name 2 (will be displayed close to element 2)
    * @param flags link flags
    */
   public NetworkMapLink(long id, String name, int type, long element1, long element2, String connectorName1, String connectorName2, int flags)
   {
      this(id, name, type, element1, 0, element2, 0, connectorName1, connectorName2, null, flags);
   }

   /**
    * Create link object from scratch.
    *
    * @param id link object ID
    * @param type link type
    * @param element1 network map internal element id
    * @param element2 network map internal element id
    */
   public NetworkMapLink(long id, int type, long element1, long element2)
   {
      this(id, "", type, element1, 0, element2, 0, "", "", null, 0);
   }

   /**
    * Create link object from NXCP message.
    *
    * @param msg NXCP message
    * @param baseId base field ID
    */
   public NetworkMapLink(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt64(baseId);
      name = msg.getFieldAsString(baseId + 1);
      type = msg.getFieldAsInt32(baseId + 2);
      element1 = msg.getFieldAsInt64(baseId + 3);
      element2 = msg.getFieldAsInt64(baseId + 4);
      connectorName1 = msg.getFieldAsString(baseId + 5);
      connectorName2 = msg.getFieldAsString(baseId + 6);
      flags = msg.getFieldAsInt32(baseId + 7);
      colorSource = msg.getFieldAsInt32(baseId + 8);
      color = msg.getFieldAsInt32(baseId + 9);
      colorProvider = msg.getFieldAsString(baseId + 10);
      interfaceId1 = msg.getFieldAsInt64(baseId + 12);
      interfaceId2 = msg.getFieldAsInt64(baseId + 13);

      String json = msg.getFieldAsString(baseId + 11);
      try
      {
         config = ((json != null) && !json.isEmpty()) ? new Gson().fromJson(json, LinkConfig.class) : new LinkConfig();
      }
      catch(Exception e)
      {
         config =  new LinkConfig();
      }
   }

   /**
    * Fill NXCP message with link data
    *
    * @param msg    NXCP message
    * @param baseId base variable ID
    */
   public void fillMessage(NXCPMessage msg, long baseId)
   {
      msg.setFieldInt32(baseId, (int)id);
      msg.setField(baseId + 1, name);
      msg.setFieldInt16(baseId + 2, type);
      msg.setField(baseId + 3, connectorName1);
      msg.setField(baseId + 4, connectorName2);
      msg.setFieldInt32(baseId + 5, (int)element1);
      msg.setFieldInt32(baseId + 6, (int)element2);
      msg.setFieldInt32(baseId + 7, flags);
      msg.setFieldInt16(baseId + 8, colorSource);
      msg.setFieldInt32(baseId + 9, color);
      msg.setField(baseId + 10, colorProvider);
      msg.setFieldJson(baseId + 11, config);
      msg.setFieldInt32(baseId + 12, (int)interfaceId1);
      msg.setFieldInt32(baseId + 13, (int)interfaceId2);
   }

   /**
    * @return the id
    */
   public long getId()
   {
      return id;
   }

   /**
    * @return the linkType
    */
   public int getType()
   {
      return type;
   }

   /**
    * @return first (left) element
    */
   public long getElement1()
   {
      return element1;
   }

   /**
    * @return second (right) element
    */
   public long getElement2()
   {
      return element2;
   }

   /**
    * @return first (left) connector name
    */
   public String getConnectorName1()
   {
      return connectorName1;
   }

   /**
    * @return second (right) connector name
    */
   public String getConnectorName2()
   {
      return connectorName2;
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * Get label for display. If name is not null and not empty, label will have form
    * name (connector1 - connector2)
    * otherwise it will have form
    * connector1 - connector2
    * If any of connector names is null or empty, it will be replaced with string "&lt;noname&gt;".
    *
    * @return display label or null for unnamed link
    */
   public String getLabel()
   {
      if (isUnnamed())
         return null;

      StringBuilder sb = new StringBuilder();
      if ((name != null) && !name.isEmpty())
      {
         sb.append(name);
         sb.append(" (");
      }

      sb.append(((connectorName1 != null) && !connectorName1.isEmpty()) ? connectorName1 : "<noname>");
      sb.append(" - ");
      sb.append(((connectorName2 != null) && !connectorName2.isEmpty()) ? connectorName2 : "<noname>");

      if ((name != null) && !name.isEmpty())
      {
         sb.append(')');
      }
      return sb.toString();
   }

   /**
    * Check if link has non-empty name
    *
    * @return true if link has non-empty name
    */
   public boolean hasName()
   {
      return (name != null) && !name.isEmpty();
   }

   /**
    * Check if link has non-empty name for connector 1
    *
    * @return true if link has non-empty name for connector 1
    */
   public boolean hasConnectorName1()
   {
      return (connectorName1 != null) && !connectorName1.isEmpty();
   }

   /**
    * Check if link has non-empty name for connector 2
    *
    * @return true if link has non-empty name for connector 2
    */
   public boolean hasConnectorName2()
   {
      return (connectorName2 != null) && !connectorName2.isEmpty();
   }

   /**
    * Check if this link is unnamed.
    *
    * @return true if all names (link and both connectors) are null or empty
    */
   public boolean isUnnamed()
   {
      return ((name == null) || name.isEmpty()) && ((connectorName1 == null) || connectorName1.isEmpty()) && (
            (connectorName2 == null) || connectorName2.isEmpty());
   }

   /**
    * @return the color
    */
   public int getColor()
   {
      return color;
   }

   /**
    * @param color the color to set
    */
   public void setColor(int color)
   {
      this.color = color;
   }

   /**
    * Get color source
    *
    * @return color source
    */
   public int getColorSource()
   {
      return colorSource;
   }

   /**
    * Set color source
    *
    * @param colorSource new color source
    */
   public void setColorSource(int colorSource)
   {
      this.colorSource = colorSource;
   }

   /**
    * @return the colorProvider
    */
   public String getColorProvider()
   {
      return (colorProvider != null) ? colorProvider : "";
   }

   /**
    * @param colorProvider the colorProvider to set
    */
   public void setColorProvider(String colorProvider)
   {
      this.colorProvider = colorProvider;
   }

   /**
    * Get list of objects used for status calculation
    *
    * @return list of objects used for status calculation
    */
   public List<Long> getStatusObjects()
   {
      return config.getObjectStatusList();
   }

   /**
    * Set list of objects used for status calculation
    *
    * @param statusObjects new list of objects
    */
   public void setStatusObjects(List<Long> statusObjects)
   {
      config.setObjectStatusList((statusObjects != null) ? statusObjects : new ArrayList<Long>(0));
   }

   /**
    * @return the routing
    */
   public int getRouting()
   {
      return config.getRouting();
   }

   /**
    * @param routing the routing to set
    */
   public void setRouting(int routing)
   {
      config.setRouting(routing);
   }

   /**
    * @return the bendPoints
    */
   public long[] getBendPoints()
   {
      return config.getBendPoints();
   }

   /**
    * @param bendPoints the bendPoints to set
    */
   public void setBendPoints(long[] bendPoints)
   {
      config.setBendPoints(bendPoints);
   }

   /**
    * @return the flags
    */
   public int getFlags()
   {
      return flags;
   }

   /**
    * Check if this link was generated automatically.
    *
    * @return true if this link was generated automatically
    */
   public boolean isAutoGenerated()
   {
      return (flags & AUTO_GENERATED) != 0;
   }

   /**
    * Check if this link is excluded from automatic update
    *
    * @return true if this link should be excluded from auto update
    */
   public boolean isExcludedFromAutomaticUpdate()
   {
      return (flags & EXCLUDE_FROM_AUTO_UPDATE) != 0;
   }

   /**
    * Set if this link is excluded from automatic DCI data update
    * 
    * @param exclude if it is excluded
    */
   public void setExcludedFromAutomaticUpdate(boolean exclude)
   {
      if (exclude)
         flags |= EXCLUDE_FROM_AUTO_UPDATE;
      else
         flags &= ~EXCLUDE_FROM_AUTO_UPDATE;
   }


   /**
    * @return returns if DCI list is not empty
    */
   public boolean hasDciData()
   {
      MapDataSource[] dciList = config.getDciList();
      if (dciList != null && dciList.length > 0)
         return true;
      return false;
   }

   /**
    * @return returns the DCI list if not empty or null
    */
   public MapLinkDataSource[] getDciList()
   {
      if (hasDciData())
      {
         return config.getDciList();
      }
      else
      {
         return null;
      }
   }

   /**
    * Get copy of DCIs configured on this link as list. Any changes to returned list will not affect link configuration.
    * 
    * @return copy of DCIs configured on this link as list
    */
   public List<MapLinkDataSource> getDciAsList()
   {
      List<MapLinkDataSource> dciList = new ArrayList<MapLinkDataSource>();
      if (hasDciData())
      {
         for(MapLinkDataSource dci : getDciList())
            dciList.add(new MapLinkDataSource(dci));
      }
      return dciList;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @param connectorName1 the connectorName1 to set
    */
   public void setConnectorName1(String connectorName1)
   {
      this.connectorName1 = connectorName1;
   }

   /**
    * @param connectorName2 the connectorName2 to set
    */
   public void setConnectorName2(String connectorName2)
   {
      this.connectorName2 = connectorName2;
   }

   /*
    * Set link duplicate count
    */
   public void setDuplicateCount(int duplicateCount)
   {
      this.duplicateCount = duplicateCount;
   }

   /**
    * @return link duplicate count
    */
   public int getDuplicateCount()
   {
      return duplicateCount;
   }

   /**
    * Increase link position and duplicate count
    */
   public void updatePosition()
   {
      position++;
      duplicateCount++;
   }

   /**
    * Reset link position and duplicate count
    */
   public void resetPosition()
   {
      position = 0;
      duplicateCount = 0;
   }

   /**
    * @return link position
    */
   public int getPosition()
   {
      return position;
   }

   /**
    * @return link config
    */
   public LinkConfig getConfig()
   {
      return config;
   }

   /**
    * @return the interfaceId1
    */
   public long getInterfaceId1()
   {
      return interfaceId1;
   }

   /**
    * @param interfaceId1 the interfaceId1 to set
    */
   public void setInterfaceId1(long interfaceId1)
   {
      this.interfaceId1 = interfaceId1;
   }

   /**
    * @return the interfaceId2
    */
   public long getInterfaceId2()
   {
      return interfaceId2;
   }

   /**
    * @param interfaceId2 the interfaceId2 to set
    */
   public void setInterfaceId2(long interfaceId2)
   {
      this.interfaceId2 = interfaceId2;
   }

   /**
    * Get common first element - used to check if links are inverted to each other
    * 
    * @return common first element
    */
   public long getCommonFirstElement()
   {
      return commonFirstElement == 0 ? element1 : commonFirstElement;
   }

   /**
    * Update common first element and set inverted flag 
    * @param commonFirstElement
    */
   public void setCommonFirstElement(long commonFirstElement)
   {
      this.commonFirstElement = commonFirstElement;
      directionInverted  = commonFirstElement != element1;
   }

   /**
    * If link direction is inverted to other links
    * 
    * @return
    */
   public boolean isDirectionInverted()
   {
      return directionInverted;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "NetworkMapLink [name=" + name + ", type=" + type + ", element1=" + element1 + ", element2=" + element2
            + ", connectorName1=" + connectorName1 + ", connectorName2=" + connectorName2 + ", colorSource=" + colorSource + ", color=" +
            color + ", statusObject=" + config.getObjectStatusList() + ", routing=" + config.getRouting() + ", flags=" + flags + 
            ", labelPosition=" + config.getLabelPosition() + "]";
   }
}
