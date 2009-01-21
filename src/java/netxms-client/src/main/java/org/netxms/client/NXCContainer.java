/**
 * 
 */
package org.netxms.client;

import org.netxms.base.*;

/**
 * @author Victor
 *
 */
public class NXCContainer extends NXCObject
{
	private int category;
	
	/**
	 * @param msg
	 */
	public NXCContainer(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		category = msg.getVariableAsInteger(NXCPCodes.VID_CATEGORY);
	}

	/**
	 * @return the category
	 */
	public int getCategory()
	{
		return category;
	}
}
