/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class NXCServiceRoot extends NXCObject
{
	/**
	 * @param msg Message to create object from
	 * @param session Associated client session
	 */
	public NXCServiceRoot(final NXCPMessage msg, final NXCSession session)
	{
		super(msg, session);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "ServiceRoot";
	}
}
