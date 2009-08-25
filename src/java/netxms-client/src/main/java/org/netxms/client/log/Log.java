/**
 * 
 */
package org.netxms.client.log;

import java.util.Collection;
import java.util.HashMap;

import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;

/**
 * Log handle for accessing log on management server
 * 
 * @author Victor Kirhenshtein
 *
 */
public class Log
{
	private NXCSession session;
	private int handle;
	private String name;
	private HashMap<String, LogColumn> columns;
	private long numRecords;	// Number of records ready after successful query() 
	
	/**
	 * Create log object from server's reply to CMD_LOG_OPEN.
	 * 
	 * @param session Client session
	 * @param msg NXCP message with server's reply
	 */
	public Log(NXCSession session, NXCPMessage msg)
	{
		this.session = session;
	}

	/**
	 * Get log name.
	 * 
	 * @return Log name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * Get column information
	 * 
	 * @return the columns
	 */
	public Collection<LogColumn> getColumns()
	{
		return columns.values();
	}
	
	/**
	 * Get description for given column name.
	 * 
	 * @param columnName Column name
	 * @return Column description
	 */
	public String getColumnDescription(String columnName)
	{
		LogColumn col = columns.get(columnName);
		return (col != null) ? col.getDescription() : null;
	}
	
	/**
	 * Send query to server
	 * 
	 * @param filter Log filter
	 */
	public void query(LogFilter filter)
	{
		
	}
	
	/**
	 * Retrieve log data from server. You must first call query() to prepare data on server.
	 * 
	 * @param startRow
	 * @param rowCount
	 * @return
	 */
	public Table retrieveData(long startRow, long rowCount)
	{
		return null;
	}

	/**
	 * Get number of records available on server after successful query() call.
	 * 
	 * @return Number of log records available
	 */
	public long getNumRecords()
	{
		return numRecords;
	}
	
	/**
	 * Close log
	 */
	public void close()
	{
		
	}
}
