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
import java.util.List;
import org.netxms.base.NXCPMessage;
import org.netxms.client.AccessListElement;

/**
 * Settings for predefined graph
 *
 */
public class GraphSettings
{
	private long id;
	private long ownerId;
	private String name;
	private String shortName;
	private List<AccessListElement> accessList;
	
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
		
		parseGraphSettings(msg.getVariableAsString(baseId + 3));
	}
	
	/**
	 * Parse graph settings received from server.
	 * 
	 * @param settings settings string
	 */
	private void parseGraphSettings(String settings)
	{
		String[] elements = settings.split("\u007F");
		for(int i = 0; i < elements.length; i++)
		{
			int index = elements[i].indexOf(':');
			if (index == -1)
				continue;
			
			
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
	 * @return the shortName
	 */
	public String getShortName()
	{
		return shortName;
	}
}
