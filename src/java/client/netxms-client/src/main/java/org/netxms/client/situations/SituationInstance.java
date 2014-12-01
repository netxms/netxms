/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.situations;

import java.util.HashMap;
import java.util.Map;
import org.netxms.base.NXCPMessage;

/**
 * Situation instance object
 *
 */
public class SituationInstance
{
	private Situation parent;
	private String name;
	private Map<String, String> attributes;
	
	/**
	 * Create situation instance from message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable id
	 */
	protected SituationInstance(Situation parent, NXCPMessage msg, long baseId)
	{
		this.parent = parent;
		name = msg.getFieldAsString(baseId);
		
		int count = msg.getFieldAsInt32(baseId + 1);
		attributes = new HashMap<String, String>(count);
		long varId = baseId + 2;
		for(int i = 0; i < count; i++)
		{
			final String name = msg.getFieldAsString(varId++);
			final String value = msg.getFieldAsString(varId++);
			attributes.put(name, value);
		}
	}
	
	/**
	 * Get number of attributes
	 * 
	 * @return
	 */
	public int getAttributeCount()
	{
		return attributes.size();
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
	
	/**
	 * Get attribute's value
	 * 
	 * @param name attribute's name
	 * @return attribute's value
	 */
	public String getAttribute(String name)
	{
		return attributes.get(name);
	}

	/**
	 * @return the attributes
	 */
	public Map<String, String> getAttributes()
	{
		return attributes;
	}

	/**
	 * @return the parent
	 */
	public Situation getParent()
	{
		return parent;
	}
}
