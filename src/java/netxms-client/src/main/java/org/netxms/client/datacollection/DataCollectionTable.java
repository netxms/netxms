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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Tabular data collection object
 */
public class DataCollectionTable extends DataCollectionObject
{
	private String instanceColumn;
	
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
