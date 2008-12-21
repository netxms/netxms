/**
 *  NetXMS network interface object representation
 */
package org.netxms.client;

import java.net.InetAddress;
import org.netxms.base.*;

/**
 * @author Victor
 *
 */
public class NXCInterface extends NXCObject
{
	private InetAddress subnetMask;
	private int ifIndex;
	private int ifType;
	private byte[] macAddress;
	private int requiredPollCount;
	
	/**
	 * @param msg
	 */
	public NXCInterface(NXCPMessage msg)
	{
		super(msg);
		
		subnetMask = msg.getVariableAsInetAddress(NXCPCodes.VID_IP_NETMASK);
		ifIndex = msg.getVariableAsInteger(NXCPCodes.VID_IF_INDEX);
		ifType = msg.getVariableAsInteger(NXCPCodes.VID_IF_TYPE);
		macAddress = msg.getVariableAsBinary(NXCPCodes.VID_MAC_ADDR);
		if (macAddress == null)
			macAddress = new byte[6];
		requiredPollCount = msg.getVariableAsInteger(NXCPCodes.VID_REQUIRED_POLLS);
	}

	/**
	 * @return Interface subnet mask
	 */
	public InetAddress getSubnetMask()
	{
		return subnetMask;
	}

	/**
	 * @return Interface index
	 */
	public int getIfIndex()
	{
		return ifIndex;
	}

	/**
	 * @return Interface type
	 */
	public int getIfType()
	{
		return ifType;
	}

	/**
	 * @return Interface MAC address
	 */
	public byte[] getMacAddress()
	{
		return macAddress;
	}

	/**
	 * @return Number of polls required to change interface status
	 */
	public int getRequiredPollCount()
	{
		return requiredPollCount;
	}
}
