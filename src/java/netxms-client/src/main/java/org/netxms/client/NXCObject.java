/**
 * Generic NetXMS object class 
 */

package org.netxms.client;

import java.util.HashSet;
import java.util.Iterator;
import java.net.InetAddress;
import org.netxms.base.*;

public class NXCObject
{
	// Object classes
	public static final int OBJECT_GENERIC        = 0;
	public static final int OBJECT_SUBNET         = 1;
	public static final int OBJECT_NODE           = 2;
	public static final int OBJECT_INTERFACE      = 3;
	public static final int OBJECT_NETWORK        = 4;
	public static final int OBJECT_CONTAINER      = 5;
	public static final int OBJECT_ZONE           = 6;
	public static final int OBJECT_SERVICEROOT    = 7;
	public static final int OBJECT_TEMPLATE       = 8;
	public static final int OBJECT_TEMPLATEGROUP  = 9;
	public static final int OBJECT_TEMPLATEROOT   = 10;
	public static final int OBJECT_NETWORKSERVICE = 11;
	public static final int OBJECT_VPNCONNECTOR   = 12;
	public static final int OBJECT_CONDITION      = 13;
	public static final int OBJECT_CLUSTER        = 14;
	
	// Generic object attributes
	private long objectId = 0;
	private String objectName;
	private int status;
	private InetAddress primaryIP;
	private String comments;
	private HashSet<Long> parents = new HashSet<Long>(0);
	private HashSet<Long> childs = new HashSet<Long>(0);


	/*
	 * Create object from NXCP message
	 * @param msg Message to create object from
	 */
	
	NXCObject(final NXCPMessage msg)
	{
		int i, count;
		long id;
		
		objectId = msg.getVariableAsInteger(NXCPCodes.VID_OBJECT_ID);
		objectName = msg.getVariableAsString(NXCPCodes.VID_OBJECT_NAME);
		primaryIP = msg.getVariableAsInetAddress(NXCPCodes.VID_IP_ADDRESS);
		
		// Parents
		count = msg.getVariableAsInteger(NXCPCodes.VID_PARENT_CNT);
		for(i = 0, id = NXCPCodes.VID_PARENT_ID_BASE; i < count; i++, id++)
		{
			parents.add(msg.getVariableAsInt64(id));
		}

		// Childs
		count = msg.getVariableAsInteger(NXCPCodes.VID_CHILD_CNT);
		for(i = 0, id = NXCPCodes.VID_CHILD_ID_BASE; i < count; i++, id++)
		{
			childs.add(msg.getVariableAsInt64(id));
		}
	}


	/**
	 * @return Iterator for list of parent objects
	 */
	public Iterator<Long> getParents()
	{
		return parents.iterator();
	}


	/**
	 * @return Iterator for list of child objects
	 */
	public Iterator<Long> getChilds()
	{
		return childs.iterator();
	}


	/**
	 * @return the comments
	 */
	public String getComments()
	{
		return comments;
	}


	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}


	/**
	 * @return the objectName
	 */
	public String getObjectName()
	{
		return objectName;
	}


	/**
	 * @return the primaryIP
	 */
	public InetAddress getPrimaryIP()
	{
		return primaryIP;
	}


	/**
	 * @return the status
	 */
	public int getStatus()
	{
		return status;
	}
}
