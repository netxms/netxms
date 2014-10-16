/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Raden Solutions
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
import org.netxms.client.datacollection.DataCollectionObject;

/**
 * Generic class for holding data in tabular format. Table has named columns. All data stored as strings.
 */
public class Table
{
	private int source;
	private String title;
	private List<TableColumnDefinition> columns;
	private List<TableRow> data;
	private boolean extendedFormat;

	/**
	 * Create empty table
	 */
	public Table()
	{
		title = "untitled";
		source = DataCollectionObject.AGENT;
		columns = new ArrayList<TableColumnDefinition>(0);
		data = new ArrayList<TableRow>(0);
		extendedFormat = false;
	}

	/**
	 * Create table from data in NXCP message
	 *
	 * @param msg NXCP message
	 */
	public Table(final NXCPMessage msg)
	{
		title = msg.getVariableAsString(NXCPCodes.VID_TABLE_TITLE);
		source = msg.getVariableAsInteger(NXCPCodes.VID_DCI_SOURCE_TYPE);
		
		final int columnCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_COLS);
		columns = new ArrayList<TableColumnDefinition>(columnCount);
		long varId = NXCPCodes.VID_TABLE_COLUMN_INFO_BASE;
		for(int i = 0; i < columnCount; i++, varId += 10L)
		{
			columns.add(new TableColumnDefinition(msg, varId));
		}

		final int totalRowCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_ROWS);
		data = new ArrayList<TableRow>(totalRowCount);

		extendedFormat = msg.getVariableAsBoolean(NXCPCodes.VID_TABLE_EXTENDED_FORMAT);
		final int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_NUM_ROWS);
		varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			final TableRow row = new TableRow(columnCount);
			if (extendedFormat)
			{
			   row.setObjectId(msg.getVariableAsInt64(varId++));
			   varId += 9;
			}
			for(int j = 0; j < columnCount; j++)
			{
            row.get(j).setValue(msg.getVariableAsString(varId++));
			   if (extendedFormat)
			   {
			      int status = msg.getVariableAsInteger(varId++);
	            row.get(j).setStatus((status == 65535) ? -1 : status);
	            varId += 8;
			   }
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
			final TableRow row = new TableRow(columns.size());
         if (extendedFormat)
         {
            row.setObjectId(msg.getVariableAsInt64(varId++));
            varId += 9;
         }
         for(int j = 0; j < columns.size(); j++)
         {
             row.get(j).setValue(msg.getVariableAsString(varId++));
             if (extendedFormat)
             {
                row.get(j).setStatus(msg.getVariableAsInteger(varId++));
                varId += 8;
             }
         }
			data.add(row);
		}
	}

	/**
	 * Fill NXCP message with table's data
	 * 
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		msg.setVariable(NXCPCodes.VID_TABLE_TITLE, title);
		msg.setVariableInt16(NXCPCodes.VID_TABLE_EXTENDED_FORMAT, extendedFormat ? 1 : 0);
		
		msg.setVariableInt32(NXCPCodes.VID_TABLE_NUM_COLS, columns.size());
		long varId = NXCPCodes.VID_TABLE_COLUMN_INFO_BASE;
		for(TableColumnDefinition c : columns)
		{
			c.fillMessage(msg, varId);
			varId += 10;
		}
		
		msg.setVariableInt32(NXCPCodes.VID_TABLE_NUM_ROWS, data.size());
		varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < data.size(); i++)
		{
			varId = data.get(i).fillMessage(msg, varId, extendedFormat);
		}
	}

	/**
	 * Get number of columns in table
	 *
	 * @return Number of columns
	 */
	public int getColumnCount()
	{
		return columns.size();
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
	 * Get column definition
	 *
	 * @param column Column index (zero-based)
	 * @return Column name
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	public TableColumnDefinition getColumnDefinition(final int column) throws IndexOutOfBoundsException
	{
		return columns.get(column);
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
		return columns.get(column).getName();
	}

	/**
	 * Get column display name
	 *
	 * @param column Column index (zero-based)
	 * @return Column name
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	public String getColumnDisplayName(final int column) throws IndexOutOfBoundsException
	{
		return columns.get(column).getDisplayName();
	}

	/**
	 * Get column format
	 *
	 * @param column Column index (zero-based)
	 * @return Column format
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	@Deprecated
	public int getColumnFormat(final int column) throws IndexOutOfBoundsException
	{
		return columns.get(column).getDataType();
	}

	/**
	 * Get column index by name
	 *
	 * @param name Column name
	 * @return 0-based column index or -1 if column with given name does not exist
	 */
	public int getColumnIndex(final String name)
	{
		for(int i = 0; i < columns.size(); i++)
			if (columns.get(i).getName().equalsIgnoreCase(name))
				return i;
		return -1;
	}

	/**
	 * Get names of all columns
	 * 
	 * @return array of column names
	 */
	public TableColumnDefinition[] getColumns()
	{
		return columns.toArray(new TableColumnDefinition[columns.size()]);
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
	public String getCellValue(final int row, final int column) throws IndexOutOfBoundsException
	{
		return data.get(row).get(column).getValue();
	}

   /**
    * @param row
    * @param column
    * @return
    * @throws IndexOutOfBoundsException
    */
   public TableCell getCell(final int row, final int column) throws IndexOutOfBoundsException
   {
      return data.get(row).get(column);
   }
	
	/**
	 * Get row.
	 *
	 * @param row Row index (zero-based)
	 * @return table row
	 * @throws IndexOutOfBoundsException if row index is out of range (row < 0 || row >= getRowCount())
	 */
	public TableRow getRow(final int row) throws IndexOutOfBoundsException
	{
		return data.get(row);
	}
	
	/**
	 * Get all rows.
	 * 
	 * @return Array of all rows in a table
	 */
	public TableRow[] getAllRows()
	{
	   return data.toArray(new TableRow[data.size()]);
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
		sb.append("{columns=").append(columns);
		sb.append(", data=").append(data);
		sb.append('}');
		return sb.toString();
	}

	/**
	 * Append all records from given table to this table. Source table must have same column set.
	 * 
	 * @param src source table
	 */
	public void addAll(Table src)
	{
	   for(TableRow r : src.data)
	      data.add(new TableRow(r));
	}

	/**
	 * Add new row
	 */
	public void addRow()
	{
		data.add(new TableRow(columns.size()));
	}
	
	/**
	 * Set cell value
	 * 
	 * @param row
	 * @param col
	 * @param value
	 */
	public void setCell(int row, int col, String value)
	{
		if ((row >= 0) && (row < data.size()) && (col >= 0) && (col < columns.size()))
			data.get(row).get(col).setValue(value);
	}

	/**
	 * @return the source
	 */
	public int getSource()
	{
		return source;
	}

	/**
	 * @param source the source to set
	 */
	public void setSource(int source)
	{
		this.source = source;
	}

	/**
	 * Get display names of all columns
	 * 
	 * @return
	 */
	public String[] getColumnDisplayNames()
	{
		String[] names = new String[columns.size()];
		for(int i = 0; i < names.length; i++)
			names[i] = columns.get(i).getDisplayName();
		return names;
	}

	/**
	 * Get display names of all columns
	 * 
	 * @return
	 */
	public int[] getColumnDataTypes()
	{
		int[] types = new int[columns.size()];
		for(int i = 0; i < types.length; i++)
			types[i] = columns.get(i).getDataType();
		return types;
	}

   /**
    * @return the extendedFormat
    */
   public boolean isExtendedFormat()
   {
      return extendedFormat;
   }

   /**
    * @param extendedFormat the extendedFormat to set
    */
   public void setExtendedFormat(boolean extendedFormat)
   {
      this.extendedFormat = extendedFormat;
   }
}
