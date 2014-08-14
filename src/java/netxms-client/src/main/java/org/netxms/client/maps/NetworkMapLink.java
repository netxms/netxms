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
package org.netxms.client.maps;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.Logger;
import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.configs.LinkConfig;
import org.netxms.client.maps.configs.SingleDciConfig;

/**
 * Represents link between two elements on map
 */
public class NetworkMapLink
{   
	// Link types
	public static final int NORMAL = 0;
	public static final int VPN = 1;
	public static final int MULTILINK = 2;
	
	// Routing types
	public static final int ROUTING_DEFAULT = 0;
	public static final int ROUTING_DIRECT = 1;
	public static final int ROUTING_MANHATTAN = 2;
	public static final int ROUTING_BENDPOINTS = 3;
	
	private String name;
	private int type;
	private long element1;
	private long element2;
	private String connectorName1;
	private String connectorName2;
	private LinkConfig config = new LinkConfig();
	private int flags;

	/**
	 * 
	 * @param name
	 * @param type
	 * @param element1
	 * @param element2
	 * @param connectorName1
	 * @param connectorName2
	 * @param dciList
	 * @param flags
	 */
	public NetworkMapLink(String name, int type, long element1, long element2, String connectorName1, String connectorName2, SingleDciConfig[] dciList, int flags)
	{		
	   config.setDciList(dciList);
	   initData(name, type, element1, element2, connectorName1, connectorName2, flags);
	}

   /**
    * 
    * @param name
    * @param type
    * @param element1
    * @param element2
    * @param connectorName1
    * @param connectorName2
    * @param flags
    */
   public NetworkMapLink(String name, int type, long element1, long element2, String connectorName1, String connectorName2, int flags)
   {
      initData(name, type, element1, element2, connectorName1, connectorName2, flags);
   }
   
   /**
    * 
    * @param name
    * @param type
    * @param element1
    * @param element2
    * @param connectorName1
    * @param connectorName2
    * @param flags
    */
   public void initData(String name, int type, long element1, long element2, String connectorName1, String connectorName2, int flags)
   {
      this.name = name;
      this.type = type;
      this.element1 = element1;
      this.element2 = element2;
      this.connectorName1 = connectorName1;
      this.connectorName2 = connectorName2;
      this.flags = flags;
   }

	/**
	 * 
	 * @param type
	 * @param element1
	 * @param element2
	 */
	public NetworkMapLink(int type, long element1, long element2)
	{
		this.name = "";
		this.type = type;
		this.element1 = element1;
		this.element2 = element2;
		this.connectorName1 = "";
		this.connectorName2 = "";
      this.flags = 0;
	}
	
	/**
	 * Create link object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public NetworkMapLink(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId + 1);
		type = msg.getVariableAsInteger(baseId);
		element1 = msg.getVariableAsInt64(baseId + 4);
		element2 = msg.getVariableAsInt64(baseId + 5);
		connectorName1 = msg.getVariableAsString(baseId + 2);
		connectorName2 = msg.getVariableAsString(baseId + 3);
		flags = msg.getVariableAsInteger(baseId + 7);
		
      String xml = msg.getVariableAsString(baseId + 6);
		try
      {
		   config = ((xml != null) && !xml.isEmpty()) ? LinkConfig.createFromXml(xml) : new LinkConfig();
      }
      catch(Exception e)
      {
         Logger.warning("NetworkMapLink", "Cannot create data from XML (" + xml + ")", e);
         config = new LinkConfig();
      }		
	}
	
	/**
	 * Fill NXCP message with link data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{     
	   String xml = "";
      try
      {
         xml = config.createXml();
      }
      catch(Exception e)
      {
         Logger.warning("NetworkMapLink", "Cannot create XML from config (" + config.toString() + ")", e);
      }
	   
		msg.setVariableInt16(baseId, type);
		msg.setVariable(baseId + 1, name);
		msg.setVariable(baseId + 2, connectorName1);
		msg.setVariable(baseId + 3, connectorName2);
		msg.setVariableInt32(baseId + 4, (int)element1);
		msg.setVariableInt32(baseId + 5, (int)element2);
		msg.setVariable(baseId + 6, xml);
		msg.setVariableInt32(baseId + 7, flags);
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
	 * If any of connector names is null or empty, it will be replaced with string "<noname>".
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
		return ((name == null) || name.isEmpty()) &&
		       ((connectorName1 == null) || connectorName1.isEmpty()) &&
		       ((connectorName2 == null) || connectorName2.isEmpty());
	}

	/**
	 * @return the color
	 */
	public int getColor()
	{
		return config.getColor();
	}

	/**
	 * @param color the color to set
	 */
	public void setColor(int color)
	{
	   config.setColor(color);
	}

	/**
	 * @return the statusObject
	 */
	public List<Long> getStatusObject()
	{
	   Long [] statusObject = config.getObjectStatusList();
	   List<Long> result = new ArrayList<Long>();
	   if(statusObject == null)
	      return result;
	   for(int i = 0; i < statusObject.length; i++)
	      result.add(statusObject[i]);
		return result;
	}

	/**
	 * @param statusObject the statusObject to set
	 */
	public void setStatusObject(List<Long> statusObject)
	{
	   if(statusObject != null)
	      config.setObjectStatusList(statusObject.toArray(new Long[statusObject.size()]));
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
    * @param flags the flags to set
    */
   public void setFlags(int flags)
   {
      this.flags = flags;
   }
   
   /**
    * @return returns if DCI list is not empty
    */
   public boolean hasDciData()
   {
      SingleDciConfig[] dciList = config.getDciList();
      if(dciList != null && dciList.length > 0)
         return true;
      return false;
   }
   
   /**
    * @return returns the DCI list if not empty or null
    */
   public SingleDciConfig[] getDciList()
   {
      if(hasDciData())
      {
         return config.getDciList();
      }
      else 
      {
         return null;
      }
   }
   
   /**
    * Returns DCI array as a list
    */
   public List<SingleDciConfig> getDciAsList()
   {
      List<SingleDciConfig> dciList = new ArrayList<SingleDciConfig>();
      if(hasDciData())
      {
         for(SingleDciConfig dci : getDciList())
            dciList.add(new SingleDciConfig(dci));
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

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "NetworkMapLink [name=" + name + ", type=" + type + ", element1=" + element1 + ", element2=" + element2
            + ", connectorName1=" + connectorName1 + ", connectorName2=" + connectorName2 + ", color=" + config.getColor() + ", statusObject="
            + config.getObjectStatusList() + ", routing=" + config.getRouting() + ", flags=" + flags + "]";
   }
}
