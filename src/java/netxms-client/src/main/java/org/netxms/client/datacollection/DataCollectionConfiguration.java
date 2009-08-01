/**
 * 
 */
package org.netxms.client.datacollection;

import java.io.IOException;
import java.util.HashMap;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;

/**
 * Data collection configuration for node
 * 
 * @author Victor Kirhenshtein
 *
 */
public class DataCollectionConfiguration
{
	private NXCSession session;
	private long nodeId;
	private HashMap<Long, DataCollectionItem> items;
	private boolean isOpen = false;

	/**
	 * Create empty data collection configuration.
	 * 
	 * @param nodeId
	 */
	public DataCollectionConfiguration(final NXCSession session, final long nodeId)
	{
		this.session = session;
		this.nodeId = nodeId;
		items = new HashMap<Long, DataCollectionItem>(0);
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
			final NXCPMessage response = session.waitForMessage(NXCPCodes.CMD_DCI_DATA, msg.getMessageId());
			if (response.isEndOfSequence())
				break;
			
			DataCollectionItem item = new DataCollectionItem(response);
			items.put(item.getId(), item);
		}
		isOpen = true;
	}
	
	/**
	 * Close data collection configuration.
	 * 
	 * @throws IOException
	 * @throws NXCException
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
	public DataCollectionItem[] getItems()
	{
		return items.values().toArray(new DataCollectionItem[items.size()]);
	}
	
	/**
	 * Find data collection item by ID.
	 *  
	 * @param id DCI ID
	 * @return Data collection item or <b>null</b> if item with given ID does not exist
	 */
	public DataCollectionItem findItem(long id)
	{
		return items.get(id);
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
}
