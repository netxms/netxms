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
import org.netxms.client.constants.Severity;

/**
 * DCI value
 */
public abstract class DciValue
{
	protected long id;					// DCI id
	protected long nodeId;				// related node object id
	private long templateDciId;	// related template DCI ID
	protected String name;				// name
	protected String description;	// description
	protected String value;			// value
	protected int source;				// data source (agent, SNMP, etc.)
	protected int dataType;
	protected int status;				// status (active, disabled, etc.)
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
		int type = msg.getFieldAsInt32(base + 8);
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
	 * Simple constructor for DciValue
	 */
	protected DciValue()
   {
   }
	
	/**
	 * Constructor for creating DciValue from NXCP message
	 * 
	 * @param nodeId owning node ID
	 * @param msg NXCP message
	 * @param base Base field ID for value object
	 */
	protected DciValue(long nodeId, NXCPMessage msg, long base)
	{
		long fieldId = base;
	
		this.nodeId = nodeId;
		id = msg.getFieldAsInt64(fieldId++);
		name = msg.getFieldAsString(fieldId++);
		description = msg.getFieldAsString(fieldId++);
		source = msg.getFieldAsInt32(fieldId++);
		dataType = msg.getFieldAsInt32(fieldId++);
		value = msg.getFieldAsString(fieldId++);
		timestamp = msg.getFieldAsDate(fieldId++);
		status = msg.getFieldAsInt32(fieldId++);
		dcObjectType = msg.getFieldAsInt32(fieldId++);
		errorCount = msg.getFieldAsInt32(fieldId++);
		templateDciId = msg.getFieldAsInt64(fieldId++);
		if (msg.getFieldAsBoolean(fieldId++))
			activeThreshold = new Threshold(msg, fieldId);
		else
			activeThreshold = null;
	}
	
   /**
	 * Returns formated DCI value or string with format error and correct type of DCI value;
	 * 
	 * @param formatString the string into which will be placed DCI value 
	 * @return The format
	 */
	public String format(String formatString)
	{
	   return new DataFormatter(formatString, dataType).format(value);
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

   /**
    * Get severity of active threshold
    * 
    * @return severity of active threshold or NORMAL if there are no active thresholds
    */
   public Severity getThresholdSeverity()
   {
      return (activeThreshold != null) ? activeThreshold.getCurrentSeverity() : Severity.NORMAL;
   }
}
