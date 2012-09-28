/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 NetXMS Team
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

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Generic class for holding data in tabular format. Table has named columns. All data stored as strings.
 */
public class Table
{
	private String title;
	private String instanceColumn;
	private List<String> columnNames;
	private List<Integer> columnFormats;
	private List<List<String>> data;

	/**
	 * Create empty table
	 */
	public Table()
	{
		title = "untitled";
		instanceColumn = null;
		columnNames = new ArrayList<String>(0);
		columnFormats = new ArrayList<Integer>(0);
		data = new ArrayList<List<String>>(0);
	}

	/**
	 * Create table from data in NXCP message
	 *
	 * @param msg NXCP message
	 */
	public Table(final NXCPMessage msg)
	{
		title = msg.getVariableAsString(NXCPCodes.VID_TABLE_TITLE);
		instanceColumn = msg.getVariableAsString(NXCPCodes.VID_INSTANCE_COLUMN);
		
		final int columnCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_COLS);
		columnNames = new ArrayList<String>(columnCount);
		columnFormats = new ArrayList<Integer>(columnCount);
		long varId = NXCPCodes.VID_TABLE_COLUMN_INFO_BASE;
		for(int i = 0; i < columnCount; i++, varId += 8L)
		{
			columnNames.add(msg.getVariableAsString(varId++));
			columnFormats.add(msg.getVariableAsInteger(varId++));
		}

		final int totalRowCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_ROWS);
		data = new ArrayList<List<String>>(totalRowCount);

		final int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ROWS);
		varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			final List<String> row = new ArrayList<String>(columnCount);
			for(int j = 0; j < columnCount; j++)
			{
				row.add(msg.getVariableAsString(varId++));
			}
			data.add(row);
		}
	}

	/**
	 * Add data from additional messages
	 * @param msg
	 */
	public void addDataFromMessage(final NXCPMessage msg)
	{
		final int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ROWS);
		long varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			final List<String> row = new ArrayList<String>(columnNames.size());
         //noinspection ForLoopReplaceableByForEach
         for(int j = 0; j < columnNames.size(); j++)
         {
             row.add(msg.getVariableAsString(varId++));
         }
			data.add(row);
		}
	}

	/**
	 * Get number of columns in table
	 *
	 * @return Number of columns
	 */
	public int getColumnCount()
	{
		return columnNames.size();
	}

	/**
	 * Get number of rows in table
	 *
	 * @return Number of rows
	 */
	public int getRowCount()
	{
		return data.size();
	}

	/**
	 * Get column name
	 *
	 * @param column Column index (zero-based)
	 * @return Column name
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	public String getColumnName(final int column) throws IndexOutOfBoundsException
	{
		return columnNames.get(column);
	}

	/**
	 * Get column format
	 *
	 * @param column Column index (zero-based)
	 * @return Column format
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	public int getColumnFormat(final int column) throws IndexOutOfBoundsException
	{
		return columnFormats.get(column);
	}

	/**
	 * Get column index by name
	 *
	 * @param name Column name
	 * @return 0-based column index or -1 if column with given name does not exist
	 */
	public int getColumnIndex(final String name)
	{
		return columnNames.indexOf(name);
	}

	/**
	 * Get names of all columns
	 * 
	 * @return array of column names
	 */
	public String[] getColumnNames()
	{
		return columnNames.toArray(new String[columnNames.size()]);
	}
	
	/**
	 * Get formats of all columns
	 * 
	 * @return
	 */
	public Integer[] getColumnFormats()
	{
		return columnFormats.toArray(new Integer[columnFormats.size()]);
	}

	/**
	 * Get cell value at given row and column
	 *
	 * @param row Row index (zero-based)
	 * @param column Column index (zero-based)
	 * @return Data from given cell
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 *         or row index is out of range (row < 0 || row >= getRowCount())
	 */
	public String getCell(final int row, final int column) throws IndexOutOfBoundsException
	{
		final List<String> rowData = data.get(row);
		return rowData.get(column);
	}

	/**
	 * Get row.
	 *
	 * @param row Row index (zero-based)
	 * @return List of all values for given row
	 * @throws IndexOutOfBoundsException if row index is out of range (row < 0 || row >= getRowCount())
	 */
	public List<String> getRow(final int row) throws IndexOutOfBoundsException
	{
		return data.get(row);
	}
	
	/**
	 * Get all rows as an array of List<String>. Method returns Object[] because
	 * Java forbids creation of generic arrays.
	 * 
	 * @return Array of all rows in a table
	 */
	public Object[] getAllRows()
	{
		Object[] rows = new Object[data.size()];
		for(int i = 0; i < rows.length; i++)
			rows[i] = data.get(i);
		return rows;
	}

	/**
	 * @return the title
	 */
	public String getTitle()
	{
		return title;
	}

	/**
	 * @param title the title to set
	 */
	public void setTitle(String title)
	{
		this.title = title;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		final StringBuilder sb = new StringBuilder();
		sb.append("Table");
		sb.append("{columns=").append(columnNames);
		sb.append(", data=").append(data);
		sb.append('}');
		return sb.toString();
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
