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

import java.util.Arrays;
import java.util.Comparator;
import org.netxms.base.NXCPMessage;

/**
 * Represents NetXMS agent's table
 */
public class AgentTable
{
	private String name;
	private String description;
	private String[] instanceColumns;
	private TableColumnDefinition[] columns;
	
	/**
	 * Create agent table info from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected AgentTable(final NXCPMessage msg, final long baseId)
	{
		name = msg.getVariableAsString(baseId + 1);
		description = msg.getVariableAsString(baseId + 2);
		
		String t = msg.getVariableAsString(baseId + 3);
		instanceColumns = (t != null) ? t.split("\\|") : new String[0];
		Arrays.sort(instanceColumns, new Comparator<String>() {
			@Override
			public int compare(String s1, String s2)
			{
				return s1.compareToIgnoreCase(s2);
			}
		});
		
		int count = msg.getVariableAsInteger(baseId) - 4;
		columns = new TableColumnDefinition[count];
		long varId = baseId + 4;
		for(int i = 0; i < count; i++)
		{
			columns[i] = new TableColumnDefinition(msg, varId);
			varId += 2;
		}
	}
	
	/**
	 * Create agent table info from scratch.
	 * 
	 * @param name
	 * @param description
	 * @param instanceColumn
	 */
	public AgentTable(String name, String description, String[] instanceColumns)
	{
		this.name = name;
		this.description = description;
		this.instanceColumns = instanceColumns;
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
	public final String[] getInstanceColumns()
	{
		return instanceColumns;
	}
	
	/**
	 * Get all instance columns as comma separated list
	 * 
	 * @return
	 */
	public String getInstanceColumnsAsList()
	{
		if (instanceColumns.length == 0)
			return "";
		StringBuilder sb = new StringBuilder();
		sb.append(instanceColumns[0]);
		for(int i = 1; i < instanceColumns.length; i++)
		{
			sb.append(", ");
			sb.append(instanceColumns[i]);
		}
		return sb.toString();
	}
	
	/**
	 * @return the columns
	 */
	public TableColumnDefinition[] getColumns()
	{
		return columns;
	}

	/**
	 * Table column definition
	 */
	public class TableColumnDefinition
	{
		public String name;
		public int dataType;
		
		TableColumnDefinition(NXCPMessage msg, long baseId)
		{
			name = msg.getVariableAsString(baseId);
			dataType = msg.getVariableAsInteger(baseId + 1);
		}
	}
}
