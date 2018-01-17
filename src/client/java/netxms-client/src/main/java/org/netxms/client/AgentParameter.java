/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
import org.netxms.client.constants.DataType;

/**
 * Represents NetXMS agent's parameter
 */
public class AgentParameter
{
	private String name;
	private String description;
	private DataType dataType;
	
	/**
	 * Create agent parameter from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base field ID
	 */
	protected AgentParameter(final NXCPMessage msg, final long baseId)
	{
		name = msg.getFieldAsString(baseId);
		description = msg.getFieldAsString(baseId + 1);
		dataType = DataType.getByValue(msg.getFieldAsInt32(baseId + 2));
	}
	
	/**
	 * Create agent parameter info from scratch.
	 * 
	 * @param name parameter name
	 * @param description parameter description
	 * @param dataType parameter data type
	 */
	public AgentParameter(String name, String description, DataType dataType)
	{
		this.name = name;
		this.description = description;
		this.dataType = dataType;
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
	 * @return the dataType
	 */
	public DataType getDataType()
	{
		return dataType;
	}
}
