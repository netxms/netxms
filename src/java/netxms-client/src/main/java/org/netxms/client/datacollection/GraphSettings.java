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
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.netxms.base.NXCPMessage;
import org.netxms.client.AccessListElement;

/**
 * Settings for predefined graph
 */
public class GraphSettings
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
	
	private long id;
	private long ownerId;
	private String name;
	private String shortName;
	private List<AccessListElement> accessList;
	private String config;
	private Set<GraphSettingsChangeListener> changeListeners = new HashSet<GraphSettingsChangeListener>(0);
	
	/**
	 * Create default settings
	 */
	public GraphSettings()
	{
		id = 0;
		ownerId = 0;
		name = "noname";
		shortName = "noname";
		accessList = new ArrayList<AccessListElement>(0);
		config = "<chart></chart>";
	}
	
	/**
	 * Create settings
	 */
	public GraphSettings(long id, long ownerId, Collection<AccessListElement> accessList)
	{
		this.id = id;
		this.ownerId = ownerId;
		name = "noname";
		shortName = "noname";
		this.accessList = new ArrayList<AccessListElement>(accessList.size());
		this.accessList.addAll(accessList);
		config = "<chart></chart>";
	}
	
	/**
	 * Create graph settings object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	public GraphSettings(final NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		ownerId = msg.getVariableAsInt64(baseId + 1);
		name = msg.getVariableAsString(baseId + 2);
		
		String[] parts = name.split("->");
		shortName = (parts.length > 1) ? parts[parts.length - 1] : name;
		
		int count = msg.getVariableAsInteger(baseId + 4);  // ACL size
		long[] users = msg.getVariableAsUInt32Array(baseId + 5);
		long[] rights = msg.getVariableAsUInt32Array(baseId + 6);
		accessList = new ArrayList<AccessListElement>(count);
		for(int i = 0; i < count; i++)
		{
			accessList.add(new AccessListElement(users[i], (int)rights[i]));
		}
		
		config = msg.getVariableAsString(baseId + 3);
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
	 * @return the shortName
	 */
	public String getShortName()
	{
		return shortName;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
		String[] parts = name.split("->");
		shortName = (parts.length > 1) ? parts[parts.length - 1] : name;
	}

	/**
	 * Add change listener
	 * 
	 * @param listener change listener
	 */
	public void addChangeListener(GraphSettingsChangeListener listener)
	{
		changeListeners.add(listener);
	}
	
	/**
	 * Remove change listener
	 * 
	 * @param listener change listener to remove
	 */
	public void removeChangeListener(GraphSettingsChangeListener listener)
	{
		changeListeners.remove(listener);
	}
	
	/**
	 * Fire change notification
	 */
	public void fireChangeNotification()
	{
		for(GraphSettingsChangeListener l : changeListeners)
			l.onGraphSettingsChange(this);
	}

	/**
	 * @return the config
	 */
	public String getConfig()
	{
		return config;
	}

	/**
	 * @param config the config to set
	 */
	public void setConfig(String config)
	{
		this.config = config;
	}
}
