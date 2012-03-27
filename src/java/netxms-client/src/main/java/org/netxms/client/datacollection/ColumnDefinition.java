/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
	private String name;
	private int dataType;
	private SnmpObjectId snmpObjectId;
	private String transformationScript;

	/**
	 * Create new column definition.
	 * 
	 * @param name column name
	 */
	public ColumnDefinition(String name)
	{
		this.name = name;
		dataType = DataCollectionObject.DT_STRING;
		snmpObjectId = null;
		transformationScript = null;
	}
	
	/**
	 * Create column definition from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected ColumnDefinition(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId);
		dataType = msg.getVariableAsInteger(baseId + 1);
		transformationScript = msg.getVariableAsString(baseId + 2);
		long[] oid = msg.getVariableAsUInt32Array(baseId + 3);
		snmpObjectId = (oid != null) ? new SnmpObjectId(oid) : null;
	}

	/**
	 * Fill NXCP message with column's data
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		msg.setVariable(baseId, name);
		msg.setVariableInt16(baseId + 1, dataType);
		if ((transformationScript != null) && !transformationScript.isEmpty())
			msg.setVariable(baseId + 2, transformationScript);
		if (snmpObjectId != null)
			snmpObjectId.setNXCPVariable(msg, baseId + 3);
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
	 * @return the transformationScript
	 */
	public String getTransformationScript()
	{
		return transformationScript;
	}

	/**
	 * @param transformationScript the transformationScript to set
	 */
	public void setTransformationScript(String transformationScript)
	{
		this.transformationScript = transformationScript;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
