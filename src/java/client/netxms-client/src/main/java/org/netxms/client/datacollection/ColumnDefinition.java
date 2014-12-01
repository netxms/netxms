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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;
import org.netxms.client.snmp.SnmpObjectId;

/**
 * Column definition for data collection table
 */
public class ColumnDefinition
{
	public static final int TCF_DATA_TYPE_MASK          = 0x000F;
	public static final int TCF_AGGREGATE_FUNCTION_MASK = 0x0070;
	public static final int TCF_INSTANCE_COLUMN         = 0x0100;
	public static final int TCF_INSTANCE_LABEL_COLUMN   = 0x0200;

	private String name;
	private String displayName;
	private int flags;
	private SnmpObjectId snmpObjectId;

	/**
	 * Create new column definition.
	 * 
	 * @param name column name
	 */
	public ColumnDefinition(String name, String displayName)
	{
		this.name = name;
		this.displayName = displayName;
		flags = 0;
		setDataType(DataCollectionObject.DT_STRING);
		snmpObjectId = null;
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ColumnDefinition(ColumnDefinition src)
	{
		name = src.name;
		displayName = src.displayName;
		flags = src.flags;
		snmpObjectId = src.snmpObjectId;
	}
	
	/**
	 * Create column definition from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected ColumnDefinition(NXCPMessage msg, long baseId)
	{
		name = msg.getFieldAsString(baseId);
		flags = msg.getFieldAsInt32(baseId + 1);
		long[] oid = msg.getFieldAsUInt32Array(baseId + 2);
		snmpObjectId = (oid != null) ? new SnmpObjectId(oid) : null;
		displayName = msg.getFieldAsString(baseId + 3);
		if ((displayName == null) || (displayName.length() == 0))
			displayName = name;
	}

	/**
	 * Fill NXCP message with column's data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setField(baseId, name);
		msg.setFieldInt16(baseId + 1, flags);
		if (snmpObjectId != null)
			snmpObjectId.setNXCPVariable(msg, baseId + 2);
		msg.setField(baseId + 3, displayName);
	}
	
	/**
	 * @return the dataType
	 */
	public int getDataType()
	{
		return flags & TCF_DATA_TYPE_MASK;
	}

	/**
	 * @param dataType the dataType to set
	 */
	public void setDataType(int dataType)
	{
		flags = (flags & ~TCF_DATA_TYPE_MASK) | (dataType & TCF_DATA_TYPE_MASK);
	}

	/**
	 * @return the dataType
	 */
	public int getAggregationFunction()
	{
		return (flags & TCF_AGGREGATE_FUNCTION_MASK) >> 4;
	}

	/**
	 * @param function new aggregation function
	 */
	public void setAggregationFunction(int function)
	{
		flags = (flags & ~TCF_AGGREGATE_FUNCTION_MASK) | ((function << 4) & TCF_AGGREGATE_FUNCTION_MASK);
	}

	/**
	 * @return the snmpObjectId
	 */
	public SnmpObjectId getSnmpObjectId()
	{
		return snmpObjectId;
	}

	/**
	 * @param snmpObjectId the snmpObjectId to set
	 */
	public void setSnmpObjectId(SnmpObjectId snmpObjectId)
	{
		this.snmpObjectId = snmpObjectId;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the displayName
	 */
	public String getDisplayName()
	{
		return displayName;
	}

	/**
	 * @param displayName the displayName to set
	 */
	public void setDisplayName(String displayName)
	{
		this.displayName = displayName;
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}

	/**
	 * @param flags the flags to set
	 */
	public void setFlags(int flags)
	{
		this.flags = flags;
	}
	
	/**
	 * @return
	 */
	public boolean isInstanceColumn()
	{
		return (flags & TCF_INSTANCE_COLUMN) != 0;
	}
	
	/**
	 * @param isInstance
	 */
	public void setInstanceColumn(boolean isInstance)
	{
		if (isInstance)
			flags |= TCF_INSTANCE_COLUMN;
		else
			flags &= ~TCF_INSTANCE_COLUMN;
	}
	
	/**
	 * @return
	 */
	public boolean isInstanceLabelColumn()
	{
		return (flags & TCF_INSTANCE_LABEL_COLUMN) != 0;
	}
	
	/**
	 * @param isInstance
	 */
	public void setInstanceLabelColumn(boolean isInstanceLabel)
	{
		if (isInstanceLabel)
			flags |= TCF_INSTANCE_LABEL_COLUMN;
		else
			flags &= ~TCF_INSTANCE_LABEL_COLUMN;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}
}
