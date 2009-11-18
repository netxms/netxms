/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * @author Victor
 *
 */
public class PolicyRoot extends GenericObject
{
	/**
	 * @param msg Message to create object from
	 * @param session Associated client session
	 */
	public PolicyRoot(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "PolicyRoot";
	}
}
