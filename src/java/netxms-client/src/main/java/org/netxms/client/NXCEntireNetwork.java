/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class NXCEntireNetwork extends NXCObject
{
	/**
	 * @param msg Message to create object from
	 * @param session Associated client session
	 */
	public NXCEntireNetwork(final NXCPMessage msg, final NXCSession session)
	{
		super(msg, session);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Network";
	}
}
