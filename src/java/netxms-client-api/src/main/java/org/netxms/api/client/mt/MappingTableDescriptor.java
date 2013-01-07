/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.api.client.mt;

import org.netxms.base.NXCPMessage;

/**
 * Information about mapping table without actual mapping data
 */
public class MappingTableDescriptor
{
	private int id;
	private String name;
	private String description;
	private int flags;

	/**
	 * Create from NXCP message.
	 * 
	 * @param msg
	 * @param baseId
	 */
	public MappingTableDescriptor(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInteger(baseId);
		name = msg.getVariableAsString(baseId + 1);
		description = msg.getVariableAsString(baseId + 2);
		flags = msg.getVariableAsInteger(baseId + 3);
	}
	
	/**
	 * @param id
	 * @param name
	 * @param description
	 * @param flags
	 */
	public MappingTableDescriptor(int id, String name, String description, int flags)
	{
		this.id = id;
		this.name = name;
		this.description = description;
		this.flags = flags;
	}

	/**
	 * @return the id
	 */
	public final int getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public final String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public final String getDescription()
	{
		return description;
	}

	/**
	 * @return the flags
	 */
	public final int getFlags()
	{
		return flags;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		final int prime = 31;
		int result = 1;
		result = prime * result + ((description == null) ? 0 : description.hashCode());
		result = prime * result + flags;
		result = prime * result + id;
		result = prime * result + ((name == null) ? 0 : name.hashCode());
		return result;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		MappingTableDescriptor other = (MappingTableDescriptor)obj;
		if (description == null)
		{
			if (other.description != null)
				return false;
		}
		else if (!description.equals(other.description))
			return false;
		if (flags != other.flags)
			return false;
		if (id != other.id)
			return false;
		if (name == null)
		{
			if (other.name != null)
				return false;
		}
		else if (!name.equals(other.name))
			return false;
		return true;
	}
}
