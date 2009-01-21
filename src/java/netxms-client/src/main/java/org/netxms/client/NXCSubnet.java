/**
 * 
 */
package org.netxms.client;

import java.net.InetAddress;
import org.netxms.base.*;


/**
 * @author victor
 *
 */
public class NXCSubnet extends NXCObject
{
	private InetAddress subnetMask;

	/**
	 * @param msg
	 */
	public NXCSubnet(NXCPMessage msg, NXCSession session)
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
}
