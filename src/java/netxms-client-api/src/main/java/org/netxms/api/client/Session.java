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
package org.netxms.api.client;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.List;

import org.netxms.api.client.mt.MappingTable;
import org.netxms.api.client.mt.MappingTableDescriptor;
import org.netxms.base.NXCPMessage;

/**
 * Base client session interface. All client APIs for NetXMS based products expected to provide this interface.
 */
public interface Session
{
	/**
	 * Add notification listener
	 * 
	 * @param lst Listener to add
	 */
	public abstract void addListener(SessionListener lst);

	/**
	 * Remove notification listener
	 * 
	 * @param lst Listener to remove
	 */
	public abstract void removeListener(SessionListener lst);

	/**
	 * Wait for message with specific code and id.
	 * 
	 * @param code
	 *           Message code
	 * @param id
	 *           Message id
	 * @param timeout
	 *           Wait timeout in milliseconds
	 * @return Message object
	 * @throws NetXMSClientException
	 *            if message was not arrived within timeout interval
	 */
	public abstract NXCPMessage waitForMessage(final int code, final long id, final int timeout) throws NetXMSClientException;

	/**
	 * Wait for message with specific code and id.
	 * 
	 * @param code
	 *           Message code
	 * @param id
	 *           Message id
	 * @return Message object
	 * @throws NetXMSClientException
	 *            if message was not arrived within timeout interval
	 */
	public abstract NXCPMessage waitForMessage(final int code, final long id) throws NetXMSClientException;

	/**
	 * Wait for CMD_REQUEST_COMPLETED message with given id using default timeout
	 * 
	 * @param id
	 *           Message id
	 * @return received message
	 * @throws NetXMSClientException
	 *            if message was not arrived within timeout interval or contains RCC other than RCC.SUCCESS
	 */
	public abstract NXCPMessage waitForRCC(final long id) throws NetXMSClientException;

	/**
	 * Wait for CMD_REQUEST_COMPLETED message with given id
	 * 
	 * @param id
	 *           Message id
	 * @param timeout Timeout in milliseconds
	 * @return received message
	 * @throws NetXMSClientException
	 *            if message was not arrived within timeout interval or contains RCC other than RCC.SUCCESS
	 */
	public abstract NXCPMessage waitForRCC(final long id, final int timeout) throws NetXMSClientException;

	/**
	 * Create new NXCP message with unique id
	 * 
	 * @param code
	 *           Message code
	 * @return New message object
	 */
	public abstract NXCPMessage newMessage(int code);

	/**
	 * Connect to the server.
	 * 
	 */
	public abstract void connect() throws IOException, UnknownHostException, NetXMSClientException;

	/**
	 * Disconnect from server.
	 * 
	 */
	public abstract void disconnect();

	/**
	 * Get receiver buffer size.
	 * 
	 * @return Current receiver buffer size in bytes.
	 */
	public abstract int getRecvBufferSize();

	/**
	 * Set receiver buffer size. This method should be called before connect(). It will not have any effect after
	 * connect().
	 * 
	 * @param recvBufferSize
	 *           Size of receiver buffer in bytes.
	 */
	public abstract void setRecvBufferSize(int recvBufferSize);

	/**
	 * Get server address
	 * 
	 * @return Server address
	 */
	public abstract String getServerAddress();

	/**
	 * Get login name of currently logged in user
	 * 
	 * @return login name of currently logged in user
	 */
	public abstract String getUserName();

	/**
	 * Get NetXMS server version.
	 * 
	 * @return Server version
	 */
	public abstract String getServerVersion();

	/**
	 * Get NetXMS server UID.
	 * 
	 * @return Server UID
	 */
	public abstract byte[] getServerId();

	/**
	 * @return the serverTimeZone
	 */
	public abstract String getServerTimeZone();

	/**
	 * @return the connClientInfo
	 */
	public abstract String getConnClientInfo();

	/**
	 * @param connClientInfo
	 *           the connClientInfo to set
	 */
	public abstract void setConnClientInfo(final String connClientInfo);

	/**
	 * Set command execution timeout.
	 * 
	 * @param commandTimeout
	 *           New command timeout
	 */
	public abstract void setCommandTimeout(final int commandTimeout);

	/**
	 * Get identifier of logged in user.
	 * 
	 * @return Identifier of logged in user
	 */
	public abstract int getUserId();

	/**
	 * Get system-wide rights of currently logged in user.
	 * 
	 * @return System-wide rights of currently logged in user
	 */

	public abstract int getUserSystemRights();

	/**
	 * @return the passwordExpired
	 */
	public abstract boolean isPasswordExpired();

	/**
	 * Set custom attribute for currently logged in user. Server will allow to change
	 * only attributes whose name starts with dot.
	 * 
	 * @param name Attribute's name
	 * @param value New attribute's value
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
	 */
	public abstract void setAttributeForCurrentUser(final String name, final String value) throws IOException, NetXMSClientException;

	/**
	 * Get custom attribute for currently logged in user. If attribute is not set, empty string will
	 * be returned.
	 * 
	 * @param name Attribute's name
	 * @return Attribute's value
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
	 */
	public abstract String getAttributeForCurrentUser(final String name) throws IOException, NetXMSClientException;
	
	/**
	 * Get connection state
	 * @return connection state
	 */
	public abstract boolean isConnected();

   /**
    * Send KEEPALIVE message. Return nothing is connection is fine, exception thrown otherwise
    * 
    * @return 
    * @throws IOException
    * @throws NXCException
    */
   public abstract void checkConnection() throws IOException, NetXMSClientException;
   
   /**
    * Get default date format provided by server
    * 
    * @return
    */
   public abstract String getDateFormat();

   /**
    * Get default time format provided by server
    * 
    * @return
    */
   public abstract String getTimeFormat();
   
   /**
    * Get list of all configured mapping tables.
    * 
    * @return
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
    */
   public abstract List<MappingTableDescriptor> listMappingTables() throws IOException, NetXMSClientException;
   
   /**
    * @param id
    * @return
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
    */
   public abstract MappingTable getMappingTable(int id) throws IOException, NetXMSClientException;
   
   /**
    * Create or update mapping table. If table ID is 0, new table will be created on server.
    * 
    * @param table mapping table
    * @return ID of new table object
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
    */
   public abstract int updateMappingTable(MappingTable table) throws IOException, NetXMSClientException;

   /**
    * Delete mapping table
    * 
    * @param id mapping table ID
	 * @throws IOException if socket I/O error occurs
	 * @throws NetXMSClientException if NetXMS server returns an error or operation was timed out
    */
   public abstract void deleteMappingTable(int id) throws IOException, NetXMSClientException;
}
