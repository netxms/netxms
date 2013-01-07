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
}
