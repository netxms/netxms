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

import java.io.IOException;
import java.util.HashMap;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;

/**
 * Data collection configuration for node
 */
public class DataCollectionConfiguration
{
	private NXCSession session;
	private long nodeId;
	private HashMap<Long, DataCollectionObject> items;
	private boolean isOpen = false;
	private Object userData = null;

	/**
	 * Create empty data collection configuration.
	 * 
	 * @param nodeId
	 */
	public DataCollectionConfiguration(final NXCSession session, final long nodeId)
	{
		this.session = session;
		this.nodeId = nodeId;
		items = new HashMap<Long, DataCollectionObject>(0);
	}
	
	/**
	 * Open data collection configuration.
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void open() throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_GET_NODE_DCI_LIST);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
		
		while(true)
		{
			final NXCPMessage response = session.waitForMessage(NXCPCodes.CMD_NODE_DCI, msg.getMessageId());
			if (response.isEndOfSequence())
				break;

			int type = response.getVariableAsInteger(NXCPCodes.VID_DCOBJECT_TYPE);
			DataCollectionObject dco;
			switch(type)
			{
				case DataCollectionObject.DCO_TYPE_ITEM:
					dco = new DataCollectionItem(this, response);
					break;
				case DataCollectionObject.DCO_TYPE_TABLE:
					dco = new DataCollectionTable(this, response);
					break;
				default:
					dco = null;
					break;
			}
			if (dco != null)
				items.put(dco.getId(), dco);
		}
		isOpen = true;
	}
	
	/**
	 * Close data collection configuration.
	 * 
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void close() throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_UNLOCK_NODE_DCI_LIST);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
		items.clear();
		isOpen = false;
	}
	
	/**
	 * Get list of data collection items
	 * 
	 * @return List of data collection items
	 */
	public DataCollectionObject[] getItems()
	{
		return items.values().toArray(new DataCollectionItem[items.size()]);
	}
	
	/**
	 * Find data collection object by ID.
	 *  
	 * @param id DCI ID
	 * @return Data collection item or <b>null</b> if item with given ID does not exist
	 */
	public DataCollectionObject findItem(long id)
	{
		return items.get(id);
	}
	
	/**
	 * Find data collection object by ID.
	 *  
	 * @param id data collection object ID
	 * @param classFilter class filter for found object
	 * @return Data collection item or <b>null</b> if item with given ID does not exist
	 */
	public DataCollectionObject findItem(long id, Class<? extends DataCollectionObject> classFilter)
	{
		DataCollectionObject o = items.get(id);
		if (o == null)
			return null;
		return classFilter.isInstance(o) ? o : null;
	}
	
	/**
	 * Create new data collection object.
	 * 
	 * @return Identifier assigned to created item
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public long createItem() throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_CREATE_NEW_DCI);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		session.sendMessage(msg);
		final NXCPMessage response = session.waitForRCC(msg.getMessageId());
		long id = response.getVariableAsInt64(NXCPCodes.VID_DCI_ID);
		items.put(id, new DataCollectionItem(this, id));
		return id;
	}
	
	/**
	 * Modify data collection item.
	 * 
	 * @param itemId Data collection item's identifier
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void modifyItem(long itemId) throws IOException, NXCException
	{
		DataCollectionObject item = items.get(itemId);
		if (item == null)
			throw new NXCException(RCC.INVALID_DCI_ID);
		modifyItem(item);
	}

	/**
	 * Modify data collection item.
	 * 
	 * @param item Data collection item
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void modifyItem(DataCollectionObject item) throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_MODIFY_NODE_DCI);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		item.fillMessage(msg);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
	}
	
	/**
	 * Copy or move DCIs
	 * 
	 * @param destNodeId Destination node ID
	 * @param items List of data collection items to copy or move
	 * @param move Move flag - true if items need to be moved
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	private void copyItemsInternal(long destNodeId, long[] items, boolean move) throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_COPY_DCI);
		msg.setVariableInt32(NXCPCodes.VID_SOURCE_OBJECT_ID, (int)nodeId);
		msg.setVariableInt32(NXCPCodes.VID_DESTINATION_OBJECT_ID, (int)destNodeId);
		msg.setVariableInt16(NXCPCodes.VID_MOVE_FLAG, move ? 1 : 0);
		msg.setVariableInt32(NXCPCodes.VID_NUM_ITEMS, items.length);
		msg.setVariable(NXCPCodes.VID_ITEM_LIST, items);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
	}

	/**
	 * Copy data collection items.
	 * 
	 * @param destNodeId Destination node ID
	 * @param items List of data collection items to copy
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void copyItems(long destNodeId, long[] items) throws IOException, NXCException
	{
		copyItemsInternal(destNodeId, items, false);
	}

	/**
	 * Move data collection items.
	 * 
	 * @param destNodeId Destination node ID
	 * @param items List of data collection items to move
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void moveItems(long destNodeId, long[] items) throws IOException, NXCException
	{
		copyItemsInternal(destNodeId, items, true);
	}
	
	/**
	 * Clear collected data for given DCI.
	 * 
	 * @param itemId Data collection item ID
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void clearCollectedData(long itemId) throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_CLEAR_DCI_DATA);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		msg.setVariableInt32(NXCPCodes.VID_DCI_ID, (int)itemId);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
	}
	
	/**
	 * Set status of data collection items.
	 * 
	 * @param items Data collection items' identifiers
	 * @param status New status
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void setItemStatus(long[] items, int status) throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_SET_DCI_STATUS);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		msg.setVariableInt16(NXCPCodes.VID_DCI_STATUS, status);
		msg.setVariableInt32(NXCPCodes.VID_NUM_ITEMS, items.length);
		msg.setVariable(NXCPCodes.VID_ITEM_LIST, items);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
	}
	
	/**
	 * Delete data collection item.
	 * 
	 * @param itemId Data collection item identifier
	 * @throws IOException if socket I/O error occurs
	 * @throws NXCException if NetXMS server returns an error or operation was timed out
	 */
	public void deleteItem(long itemId) throws IOException, NXCException
	{
		NXCPMessage msg = session.newMessage(NXCPCodes.CMD_DELETE_NODE_DCI);
		msg.setVariableInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
		msg.setVariableInt32(NXCPCodes.VID_DCI_ID, (int)itemId);
		session.sendMessage(msg);
		session.waitForRCC(msg.getMessageId());
		items.remove(itemId);
	}
	
	/* (non-Javadoc)
	 * @see java.lang.Object#finalize()
	 */
	@Override
	protected void finalize() throws Throwable
	{
		if (isOpen)
			close();
		super.finalize();
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the userData
	 */
	public Object getUserData()
	{
		return userData;
	}

	/**
	 * @param userData the userData to set
	 */
	public void setUserData(Object userData)
	{
		this.userData = userData;
	}
}
