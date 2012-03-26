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

/**
 * This class represents single graph item (DCI)
 *
 */
public class GraphItem
{
	private long nodeId;
	private long dciId;
	private int source;
	private int dataType;
	private String name;
	private String description;

	/**
	 * Create graph item object with default values
	 */
	public GraphItem()
	{
		nodeId = 0;
		dciId = 0;
		source = DataCollectionObject.AGENT;
		dataType = DataCollectionItem.DT_STRING;
		name = "<noname>";
		description = "<noname>";
	}

	/**
	 * Constructor which sets all fields
	 * 
	 * @param nodeId
	 * @param dciId
	 * @param source
	 * @param dataType
	 * @param name
	 * @param description
	 */
	public GraphItem(long nodeId, long dciId, int source, int dataType, String name, String description)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
		this.source = source;
		this.dataType = dataType;
		this.name = name;
		this.description = description;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @param nodeId the nodeId to set
	 */
	public void setNodeId(long nodeId)
	{
		this.nodeId = nodeId;
	}

	/**
	 * @return the dciId
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @param dciId the dciId to set
	 */
	public void setDciId(long dciId)
	{
		this.dciId = dciId;
	}

	/**
	 * @return the source
	 */
	public int getSource()
	{
		return source;
	}

	/**
	 * @param source the source to set
	 */
	public void setSource(int source)
	{
		this.source = source;
	}

	/**
	 * @return the dataType
	 */
	public int getDataType()
	{
		return dataType;
	}

	/**
	 * @param dataType the dataType to set
	 */
	public void setDataType(int dataType)
	{
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
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @param description the description to set
	 */
	public void setDescription(String description)
	{
		this.description = description;
	}
}
