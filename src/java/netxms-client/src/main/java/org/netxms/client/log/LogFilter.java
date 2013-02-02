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
package org.netxms.client.log;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.Map.Entry;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Log filter
 */
public class LogFilter
{
	private HashMap<String, ColumnFilter> columnFilters;
	private List<OrderingColumn> orderingColumns;
	
	/**
	 * Create empty log filter
	 */
	public LogFilter()
	{
		columnFilters = new HashMap<String, ColumnFilter>();
		orderingColumns = new ArrayList<OrderingColumn>();
	}
	
	/**
	 * Set filter for column
	 * 
	 * @param column Column name
	 * @param filter Filter
	 */
	public void setColumnFilter(String column, ColumnFilter filter)
	{
		columnFilters.put(column, filter);
	}
	
	/**
	 * Get filter currently set for column
	 * 
	 * @param column Column name
	 * @return Current column filter or null if filter is not set
	 */
	public ColumnFilter getColumnFilter(String column)
	{
		return columnFilters.get(column);
	}
	
	/**
	 * Remove filter for given column
	 * 
	 * @param column Column name
	 */
	public void clearColumnFilter(String column)
	{
		columnFilters.remove(column);
	}
	
	/**
	 * Get all column filters.
	 * 
	 * @return all configured column filters
	 */
	public Set<Entry<String, ColumnFilter>> getColumnFilters()
	{
		return columnFilters.entrySet();
	}

	/**
	 * @return the orderingColumns
	 */
	public List<OrderingColumn> getOrderingColumns()
	{
		return orderingColumns;
	}

	/**
	 * @param orderingColumns the orderingColumns to set
	 */
	public void setOrderingColumns(List<OrderingColumn> orderingColumns)
	{
		this.orderingColumns = orderingColumns;
	}

	/**
	 * Fill NXCP message with filter's data.
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setVariableInt32(NXCPCodes.VID_NUM_FILTERS, columnFilters.size());
		long varId = NXCPCodes.VID_COLUMN_FILTERS_BASE;
		for(final Entry<String, ColumnFilter> e : columnFilters.entrySet())
		{
			msg.setVariable(varId++, e.getKey());
			varId += e.getValue().fillMessage(msg, varId);
		}
		
		if (orderingColumns != null)
		{
			msg.setVariableInt32(NXCPCodes.VID_NUM_ORDERING_COLUMNS, orderingColumns.size());
			varId = NXCPCodes.VID_ORDERING_COLUMNS_BASE;
			for(final OrderingColumn c : orderingColumns)
			{
				msg.setVariable(varId++, c.getName());
				msg.setVariableInt16(varId++, c.isDescending() ? 1 : 0);
			}
		}
		else
		{
			msg.setVariableInt32(NXCPCodes.VID_NUM_ORDERING_COLUMNS, 0);
		}
	}
}
