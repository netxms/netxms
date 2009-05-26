/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class NXCPolicyGroup extends NXCObject
{
	/**
	 * @param msg Message to create object from
	 * @param session Associated client session
	 */
	public NXCPolicyGroup(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "PolicyGroup";
	}
}
