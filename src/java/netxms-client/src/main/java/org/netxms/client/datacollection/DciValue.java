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

import java.util.Date;

import org.netxms.base.NXCPMessage;

/**
 * DCI value
 */
public abstract class DciValue
{
	private long id;					// DCI id
	private long nodeId;				// related node object id
	private long templateDciId;	// related template DCI ID
	private String name;				// name
	private String description;	// description
	private String value;			// value
	private int source;				// data source (agent, SNMP, etc.)
	private int dataType;
	private int status;				// status (active, disabled, etc.)
	private int errorCount;
	private int dcObjectType;		// Data collection object type (item, table, etc.)
	private Date timestamp;
	private Threshold activeThreshold;
	
	/**
	 * Factory method to create correct DciValue subclass from NXCP message.
	 * 
	 * @param nodeId owning node ID
	 * @param msg NXCP message
	 * @param base Base variable ID for value object
	 * @return DciValue object
	 */
	public static DciValue createFromMessage(long nodeId, NXCPMessage msg, long base)
	{
		int type = msg.getVariableAsInteger(base + 8);
		switch(type)
		{
			case DataCollectionObject.DCO_TYPE_ITEM:
				return new SimpleDciValue(nodeId, msg, base);
			case DataCollectionObject.DCO_TYPE_TABLE:
				return new TableDciValue(nodeId, msg, base);
			default:
				return null;
		}
	}
	
	/**
	 * Constructor for creating DciValue from NXCP message
	 * 
	 * @param nodeId owning node ID
	 * @param msg NXCP message
	 * @param base Base variable ID for value object
	 */
	protected DciValue(long nodeId, NXCPMessage msg, long base)
	{
		long var = base;
	
		this.nodeId = nodeId;
		id = msg.getVariableAsInt64(var++);
		name = msg.getVariableAsString(var++);
		description = msg.getVariableAsString(var++);
		source = msg.getVariableAsInteger(var++);
		dataType = msg.getVariableAsInteger(var++);
		value = msg.getVariableAsString(var++);
		timestamp = new Date(msg.getVariableAsInt64(var++) * 1000);
		status = msg.getVariableAsInteger(var++);
		dcObjectType = msg.getVariableAsInteger(var++);
		errorCount = msg.getVariableAsInteger(var++);
		templateDciId = msg.getVariableAsInt64(var++);
		if (msg.getVariableAsBoolean(var++))
			activeThreshold = new Threshold(msg, var);
		else
			activeThreshold = null;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
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
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @return the source
	 */
	public int getSource()
	{
		return source;
	}

	/**
	 * @return the dataType
	 */
	public int getDataType()
	{
		return dataType;
	}

	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}

	/**
	 * @return the timestamp
	 */
	public Date getTimestamp()
	{
		return timestamp;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the activeThreshold
	 */
	public Threshold getActiveThreshold()
	{
		return activeThreshold;
	}

	/**
	 * @return the dcObjectType
	 */
	public int getDcObjectType()
	{
		return dcObjectType;
	}

	/**
	 * @return the errorCount
	 */
	public int getErrorCount()
	{
		return errorCount;
	}

	/**
	 * @return the templateDciId
	 */
	public final long getTemplateDciId()
	{
		return templateDciId;
	}
}
