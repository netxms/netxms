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

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Tabular data collection object
 */
public class DataCollectionTable extends DataCollectionObject
{
	private String instanceColumn;
	private List<ColumnDefinition> columns;
	
	/**
	 * Create data collection object from NXCP message.
	 * 
	 * @param owner
	 * @param msg
	 */
	public DataCollectionTable(DataCollectionConfiguration owner, NXCPMessage msg)
	{
		super(owner, msg);
		instanceColumn = msg.getVariableAsString(NXCPCodes.VID_INSTANCE);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_COLUMNS);
		columns = new ArrayList<ColumnDefinition>(count);
		long varId = NXCPCodes.VID_DCI_COLUMN_BASE;
		for(int i = 0; i < count; i++)
		{
			columns.add(new ColumnDefinition(msg, varId));
			varId += 10;
		}
	}

	/**
	 * Constructor for new data collection objects.
	 * 
	 * @param owner
	 * @param id
	 */
	public DataCollectionTable(DataCollectionConfiguration owner, long id)
	{
		super(owner, id);
		instanceColumn = null;
		columns = new ArrayList<ColumnDefinition>(0);
	}

	/**
	 * Fill NXCP message with item's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		super.fillMessage(msg);
		
		msg.setVariableInt16(NXCPCodes.VID_DCOBJECT_TYPE, DCO_TYPE_TABLE);
		msg.setVariable(NXCPCodes.VID_INSTANCE, instanceColumn);
		msg.setVariableInt32(NXCPCodes.VID_NUM_COLUMNS, columns.size());
		long varId = NXCPCodes.VID_DCI_COLUMN_BASE;
		for(int i = 0; i < columns.size(); i++)
		{
			columns.get(i).fillMessage(msg, varId);
			varId += 10;
		}
	}
	
	/**
	 * @return the instanceColumn
	 */
	public String getInstanceColumn()
	{
		return instanceColumn;
	}

	/**
	 * @param instanceColumn the instanceColumn to set
	 */
	public void setInstanceColumn(String instanceColumn)
	{
		this.instanceColumn = instanceColumn;
	}
}
