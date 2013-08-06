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

/**
 * Condition for table DCI threshold
 */
public class TableCondition
{
	private String column;
	private int operation;
	private String value;
	
	/**
	 * Create condition from NXCP message
	 * 
	 * @param msg
	 * @param baseId
	 */
	protected TableCondition(NXCPMessage msg, long baseId)
	{
		column = msg.getVariableAsString(baseId);
		operation = msg.getVariableAsInteger(baseId + 1);
		value = msg.getVariableAsString(baseId + 2);
	}

	/**
	 * @return the column
	 */
	public String getColumn()
	{
		return column;
	}

	/**
	 * @param column the column to set
	 */
	public void setColumn(String column)
	{
		this.column = column;
	}

	/**
	 * @return the operation
	 */
	public int getOperation()
	{
		return operation;
	}

	/**
	 * @param operation the operation to set
	 */
	public void setOperation(int operation)
	{
		this.operation = operation;
	}

	/**
	 * @return the value
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * @param value the value to set
	 */
	public void setValue(String value)
	{
		this.value = value;
	}
}
