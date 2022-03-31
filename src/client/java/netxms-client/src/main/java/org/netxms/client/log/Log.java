/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.log;

import java.io.IOException;
import java.util.Collection;
import java.util.LinkedHashMap;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;

/**
 * Log handle for accessing log on management server
 */
public class Log
{
   private NXCSession session;
   private int handle;
   private String name;
   private LinkedHashMap<String, LogColumn> columns;
   private String recordIdColumn;
   private String objectIdColumn;
   private long numRecords;   // Number of records ready after successful query()
   private boolean hasDetailFields;

   /**
    * Create log object from server's reply to CMD_LOG_OPEN.
    *
    * @param session Client session
    * @param msg NXCP message with server's reply
    * @param name log name
    */
   public Log(NXCSession session, NXCPMessage msg, String name)
   {
      this.session = session;
      this.name = name;
      handle = msg.getFieldAsInt32(NXCPCodes.VID_LOG_HANDLE);
      recordIdColumn = msg.getFieldAsString(NXCPCodes.VID_RECORD_ID_COLUMN);
      objectIdColumn = msg.getFieldAsString(NXCPCodes.VID_OBJECT_ID_COLUMN);
      hasDetailFields = msg.getFieldAsBoolean(NXCPCodes.VID_HAS_DETAIL_FIELDS);

      int count = msg.getFieldAsInt32(NXCPCodes.VID_NUM_COLUMNS);
      columns = new LinkedHashMap<String, LogColumn>(count);
      long baseId = NXCPCodes.VID_COLUMN_INFO_BASE;
      for(int i = 0; i < count; i++, baseId += 10)
      {
         final LogColumn c = new LogColumn(msg, baseId);
         columns.put(c.getName(), c);
      }
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
    * Check if this log has additional fields with detailed information.
    *
    * @return true if this log has additional fields with detailed information
    */
   public boolean hasDetailFields()
   {
      return hasDetailFields;
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
    * @return Column description or null if column with given name does not exist
    */
   public String getColumnDescription(String columnName)
   {
      LogColumn col = columns.get(columnName);
      return (col != null) ? col.getDescription() : null;
   }

   /**
    * Get column object by column name.
    *
    * @param columnName Column name
    * @return Column object or null if column with given name does not exist
    */
   public LogColumn getColumn(String columnName)
   {
      return columns.get(columnName);
   }

   /**
    * Get index of given column
    * 
    * @param columnName column name
    * @return index of given column or -1 if such column does not exist
    */
   public int getColumnIndex(String columnName)
   {
      int index = 0;
      for(LogColumn c : columns.values())
      {
         if (c.getName().equals(columnName))
            return index;
         index++;
      }
      return -1;
   }

   /**
    * Get name of column holding unique record ID.
    * 
    * @return name of column holding unique record ID
    */
   public String getRecordIdColumnName()
   {
      return recordIdColumn;
   }

   /**
    * Get index of column holding unique record ID.
    * 
    * @return index of column holding unique record ID
    */
   public int getRecordIdColumnIndex()
   {
      return getColumnIndex(recordIdColumn);
   }

   /**
    * Get name of column holding ID of related NetXMS object.
    * 
    * @return name of column holding ID of related NetXMS object or null
    */
   public String getObjectIdColumnName()
   {
      return objectIdColumn;
   }

   /**
    * Get index of column holding ID of related NetXMS object.
    * 
    * @return index of column holding ID of related NetXMS object or -1 if this log does not have related objects
    */
   public int getObjectIdColumnIndex()
   {
      return (objectIdColumn != null) ? getColumnIndex(objectIdColumn) : -1;
   }

   /**
    * Send query to server
    *
    * @param filter Log filter
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void query(LogFilter filter) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_QUERY_LOG);
      msg.setFieldInt32(NXCPCodes.VID_LOG_HANDLE, handle);
      filter.fillMessage(msg);
      session.sendMessage(msg);
      final NXCPMessage response = session.waitForRCC(msg.getMessageId(), 1800000);
      numRecords = response.getFieldAsInt64(NXCPCodes.VID_NUM_ROWS);
   }

   /**
    * Retrieve log data from server. You must first call query() to prepare data on server.
    *
    * @param startRow start row to retrieve
    * @param rowCount number of rows to retrieve
    * @return data set
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table retrieveData(long startRow, long rowCount) throws IOException, NXCException
   {
      return retrieveData(startRow, rowCount, false);
   }

   /**
    * Retrieve log data from server. You must first call query() to prepare data on server.
    *
    * @param startRow start row to retrieve
    * @param rowCount number of rows to retrieve
    * @param refresh if set to true, server will reload data from database instead of using cache
    * @return data set
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public Table retrieveData(long startRow, long rowCount, boolean refresh) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_GET_LOG_DATA);
      msg.setFieldInt32(NXCPCodes.VID_LOG_HANDLE, handle);
      msg.setFieldInt64(NXCPCodes.VID_START_ROW, startRow);
      msg.setFieldInt64(NXCPCodes.VID_NUM_ROWS, rowCount);
      msg.setFieldInt16(NXCPCodes.VID_FORCE_RELOAD, refresh ? 1 : 0);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId(), 1800000);
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
    * Get details for specific log record. Details object will contain values for all columns marked as "details column".
    * 
    * @param recordId log record ID
    * @return log record details or null if this log does not have additional fields with detailed information
    * @throws IOException if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public LogRecordDetails getRecordDetails(long recordId) throws IOException, NXCException
   {
      if (!hasDetailFields)
         return null;
      
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_GET_LOG_RECORD_DETAILS);
      msg.setFieldInt32(NXCPCodes.VID_LOG_HANDLE, handle);
      msg.setFieldInt64(NXCPCodes.VID_RECORD_ID, recordId);
      session.sendMessage(msg);
      NXCPMessage response = session.waitForRCC(msg.getMessageId());
      return new LogRecordDetails(recordId, response);
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
      msg.setFieldInt32(NXCPCodes.VID_LOG_HANDLE, handle);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
      handle = -1;
   }

   /**
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      final StringBuilder sb = new StringBuilder();
      sb.append("Log");
      sb.append("{session=").append(session);
      sb.append(", handle=").append(handle);
      sb.append(", name='").append(name).append('\'');
      sb.append(", columns=").append(columns);
      sb.append(", numRecords=").append(numRecords);
      sb.append(", hasDetailFields=").append(hasDetailFields);
      sb.append('}');
      return sb.toString();
   }
}
