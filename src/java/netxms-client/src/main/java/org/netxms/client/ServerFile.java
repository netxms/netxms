/**
 * 
 */
package org.netxms.client;

import java.util.Date;

import org.netxms.base.NXCPMessage;

/**
 * Represents information about file in server's file store
 *
 */
public class ServerFile
{
	private String name;
	private long size;
	private Date modifyicationTime;
	
	/**
	 * Create server file object from NXCP message.
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected ServerFile(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId);
		size = msg.getVariableAsInt64(baseId + 1);
		modifyicationTime = msg.getVariableAsDate(baseId + 2);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the size
	 */
	public long getSize()
	{
		return size;
	}

	/**
	 * @return the modifyicationTime
	 */
	public Date getModifyicationTime()
	{
		return modifyicationTime;
	}
}
