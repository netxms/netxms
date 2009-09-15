/**
 * 
 */
package org.netxms.client.log;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class LogColumn
{
	private String name;
	private String description;
	private int type;
	
	/**
	 * Create log column object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected LogColumn(NXCPMessage msg, long baseId)
	{
		name = msg.getVariableAsString(baseId);
		type = msg.getVariableAsInteger(baseId + 1);
		description = msg.getVariableAsString(baseId + 2);
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}
}
