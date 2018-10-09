/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import org.netxms.base.Logger;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.AccessListElement;
import org.netxms.client.ObjectMenuFilter;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objecttools.ObjectAction;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.convert.AnnotationStrategy;
import org.simpleframework.xml.core.Persister;

/**
 * Settings for predefined graph
 */
public class GraphSettings extends ChartConfig implements ObjectAction
{
	public static final int MAX_GRAPH_ITEM_COUNT = 16;
	
	public static final int TIME_FRAME_FIXED = 0;
	public static final int TIME_FRAME_BACK_FROM_NOW = 1;
	public static final int TIME_FRAME_CURRENT = 2;
	
	public static final int TIME_UNIT_MINUTE = 0;
	public static final int TIME_UNIT_HOUR = 1;
	public static final int TIME_UNIT_DAY = 2;
	
	public static final int GF_AUTO_UPDATE     = 0x000001;
	public static final int GF_AUTO_SCALE      = 0x000100;
	public static final int GF_SHOW_GRID       = 0x000200;
	public static final int GF_SHOW_LEGEND     = 0x000400;
	public static final int GF_SHOW_RULER      = 0x000800;
	public static final int GF_SHOW_HOST_NAMES = 0x001000;
	public static final int GF_LOG_SCALE       = 0x002000;
	public static final int GF_SHOW_TOOLTIPS   = 0x004000;
	public static final int GF_ENABLE_ZOOM     = 0x008000;
	
	public static final int POSITION_LEFT = 1;
	public static final int POSITION_RIGHT = 2;
	public static final int POSITION_TOP = 4;
	public static final int POSITION_BOTTOM = 8;
	
	public static final int ACCESS_READ  = 0x01;
	public static final int ACCESS_WRITE = 0x02;
	
	public static final int GRAPH_FLAG_TEMPLATE = 1;
	
	private long id;
	private long ownerId;
	private int flags;
	private String name;
	private String shortName;
	private String displayName;
	private List<AccessListElement> accessList;
	private ObjectMenuFilter filter;
	private GraphFolder parent;  // Used by predefined graph tree
	
	/**
	 * Create default settings
	 */
	public GraphSettings()
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
	 * Create settings
	 */
	public GraphSettings(long id, long ownerId, int flags, Collection<AccessListElement> accessList)
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
    * Create copy of provided settings
    * 
    * @param src source object
    * @param name new name
    */
   public GraphSettings(GraphSettings src, String name)
   {
      id = src.id;
      ownerId = src.ownerId;
      this.name = src.name;      
      shortName = src.shortName;
      displayName = src.displayName;
      flags = src.flags & ~GRAPH_FLAG_TEMPLATE;
      this.accessList = new ArrayList<AccessListElement>(src.accessList.size());
      this.accessList.addAll(src.accessList);
      filter = src.filter;
      setConfig(src);
      setTitle(name);
      parent = null;
   }

   /**
    * Create chart settings object from XML document
    * 
    * @param xml XML document
    * @return deserialized object
    * @throws Exception if the object cannot be fully deserialized
    */
   public static GraphSettings createFromXml(final String xml) throws Exception
   {
      Serializer serializer = new Persister(new AnnotationStrategy());
      return serializer.read(GraphSettings.class, xml);
   }
	
	/**
	 * Create graph settings object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 * @return The graph settings object
	 */
   public static GraphSettings createGraphSettings(final NXCPMessage msg, long baseId)
	{
	   GraphSettings gs;
      try
      {
         gs = GraphSettings.createFromXml(msg.getFieldAsString(baseId + 4));
      }
      catch(Exception e)
      {
         gs = new GraphSettings();
         Logger.debug("GraphSettings.CreateGraphSettings", "Cannot parse ChartConfig XML", e);
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
            gs.filter = ObjectMenuFilter.createFromXml(filterXml);
         }
         catch(Exception e)
         {
            Logger.debug("GraphSettings.CreateGraphSettings", "Cannot parse ObjectMenuFilter XML: ", e);
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
		   gs.accessList.add(new AccessListElement(users[i], (int)rights[i]));
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
	   msg.setFieldInt32(NXCPCodes.VID_GRAPH_ID, (int) id);
      msg.setField(NXCPCodes.VID_NAME, name);
      msg.setFieldInt32(NXCPCodes.VID_FLAGS, flags);
      msg.setField(NXCPCodes.VID_FILTER, filter.createXml());
      try
      {
         msg.setField(NXCPCodes.VID_GRAPH_CONFIG, createXml());
      }
      catch(Exception e)
      {
         Logger.debug("GraphSettings.CreateGraphSettings", "Cannot convert ChartConfig to XML: ", e);
         msg.setField(NXCPCodes.VID_GRAPH_CONFIG, "");
      }
      msg.setFieldInt32(NXCPCodes.VID_ACL_SIZE, accessList.size());
      long varId = NXCPCodes.VID_GRAPH_ACL_BASE;
      for(AccessListElement el : accessList)
      {
         msg.setFieldInt32(varId++, (int) el.getUserId());
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
	 * Get short name (last part of full path separated by ->)
	 * 
	 * @return short name
	 */
	public String getShortName()
	{
		return shortName;
	}

   /**
    * Get display name (short name with & shortcut marks removed)
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
    * Checks if this graph is template
    * 
    * @return isTemplate
    */
   public boolean isTemplate()
   {
      return (flags & GRAPH_FLAG_TEMPLATE) > 0;
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

   /* (non-Javadoc)
    * @see org.netxms.client.objecttools.ObjectAction#isApplicableForNode(org.netxms.client.objects.AbstractNode)
    */
   @Override
   public boolean isApplicableForNode(AbstractNode node)
   {      
      return filter.isApplicableForNode(node);
   }
   
   /* (non-Javadoc)
    * @see org.netxms.client.objecttools.ObjectAction#getMenuFilter()
    */
   @Override
   public ObjectMenuFilter getMenuFilter()
   {
      return filter;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objecttools.ObjectAction#setMenuFilter(org.netxms.client.ObjectMenuFilter)
    */
   @Override
   public void setMenuFilter(ObjectMenuFilter filter)
   {
      this.filter = filter;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objecttools.ObjectAction#getToolType()
    */
   @Override
   public int getToolType()
   {
      return 0;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "GraphSettings [id=" + id + ", ownerId=" + ownerId + ", flags=" + flags + ", name=" + name + ", shortName="
            + shortName + ", accessList=" + accessList + ", filter=" + filter + "]";
   }
}
