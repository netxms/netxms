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
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * Represents NetXMS agent's table
 */
public class AgentTable
{
	private String name;
	private String description;
	private String instanceColumn;
	
	/**
	 * Create agent table info from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected AgentTable(final NXCPMessage msg, final long baseId)
	{
		name = msg.getVariableAsString(baseId);
		instanceColumn = msg.getVariableAsString(baseId + 1);
		description = msg.getVariableAsString(baseId + 2);
	}
	
	/**
	 * Create agent table info from scratch.
	 * 
	 * @param name
	 * @param description
	 * @param instanceColumn
	 */
	public AgentTable(String name, String description, String instanceColumn)
	{
		this.name = name;
		this.description = description;
		this.instanceColumn = instanceColumn;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the instanceColumn
	 */
	public final String getInstanceColumn()
	{
		return instanceColumn;
	}
}
