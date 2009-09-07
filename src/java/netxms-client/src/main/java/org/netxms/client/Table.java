/**
 * 
 */
package org.netxms.client;

import java.util.ArrayList;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class Table
{
	private ArrayList<String> columns;
	private ArrayList<ArrayList<String>> data;
	
	/**
	 * Create empty table
	 */
	public Table()
	{
		columns = new ArrayList<String>(0);
		data = new ArrayList<ArrayList<String>>(0);
	}
	
	/**
	 * Create table from data in NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public Table(final NXCPMessage msg)
	{
		int columnCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_COLS);
		columns = new ArrayList<String>(columnCount);
		long varId = NXCPCodes.VID_TABLE_COLUMN_INFO_BASE;
		for(int i = 0; i < columnCount; i++, varId += 9)
			columns.add(msg.getVariableAsString(varId++));
		
		int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_ROWS);
		data = new ArrayList<ArrayList<String>>(rowCount);
		varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			ArrayList<String> row = new ArrayList<String>(columnCount);
			for(int j = 0; j < columnCount; j++)
				row.add(msg.getVariableAsString(varId++));
			data.add(row);
		}
	}
	
	/**
	 * Add data from additional messages
	 */
	public void addDataFromMessage(final NXCPMessage msg)
	{
		int rowCount = msg.getVariableAsInteger(NXCPCodes.VID_TABLE_NUM_ROWS);
		long varId = NXCPCodes.VID_TABLE_DATA_BASE;
		for(int i = 0; i < rowCount; i++)
		{
			ArrayList<String> row = new ArrayList<String>(columns.size());
			for(int j = 0; j < columns.size(); j++)
				row.add(msg.getVariableAsString(varId++));
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
	 * Get column name
	 * 
	 * @param column Column index (zero-based)
	 * @return Column name
	 * @throws IndexOutOfBoundsException if column index is out of range (column < 0 || column >= getColumnCount())
	 */
	public String getColumnName(int column) throws IndexOutOfBoundsException
	{
		return columns.get(column);
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
	public String getCell(int row, int column) throws IndexOutOfBoundsException
	{
		ArrayList<String> rowData = data.get(row);
		return rowData.get(column);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	public String toString()
	{
		final StringBuilder sb = new StringBuilder();
		sb.append("Table");
		sb.append("{columns=").append(columns);
		sb.append(", data=").append(data);
		sb.append('}');
		return sb.toString();
	}
}
