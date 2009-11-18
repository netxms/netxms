/**
 * 
 */
package org.netxms.client.objects;

import java.net.InetAddress;
import org.netxms.base.*;
import org.netxms.client.NXCSession;


/**
 * @author victor
 *
 */
public class Subnet extends GenericObject
{
	private InetAddress subnetMask;

	/**
	 * @param msg
	 */
	public Subnet(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		subnetMask = msg.getVariableAsInetAddress(NXCPCodes.VID_IP_NETMASK);
	}

	
	/**
	 * @return Subnet mask
	 */
	public InetAddress getSubnetMask()
	{
		return subnetMask;
	}


	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Subnet";
	}
}
