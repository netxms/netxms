/**
 * 
 */
package org.netxms.client.log;

import java.io.IOException;
import java.util.Collection;
import java.util.HashMap;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCException;
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
		handle = msg.getVariableAsInteger(NXCPCodes.VID_LOG_HANDLE);
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
	 * @throws IOException 
	 * @throws NXCException 
	 */
	public Table retrieveData(long startRow, long rowCount) throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_GET_LOG_DATA);
		msg.setVariableInt32(NXCPCodes.VID_LOG_HANDLE, handle);
		msg.setVariableInt64(NXCPCodes.VID_START_ROW, startRow);
		msg.setVariableInt64(NXCPCodes.VID_NUM_ROWS, startRow);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
		return session.receiveTable(msg.getMessageId(), NXCPCodes.CMD_LOG_DATA);
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
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void close() throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_CLOSE_SERVER_LOG);
		msg.setVariableInt32(NXCPCodes.VID_LOG_HANDLE, handle);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
		handle = -1;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#finalize()
	 */
	@Override
	protected void finalize() throws Throwable
	{
		if (handle != -1)
			close();
		super.finalize();
	}
}
