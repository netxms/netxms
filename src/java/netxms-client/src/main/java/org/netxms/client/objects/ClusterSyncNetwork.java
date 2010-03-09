/**
 * 
 */
package org.netxms.client.objects;

import java.net.InetAddress;

/**
 * Cluster sync network
 *
 */
public class ClusterSyncNetwork
{
	private InetAddress subnetAddress;
	private InetAddress subnetMask;
	
	/**
	 * Create cluster sync network object
	 * 
	 * @param subnetAddress Subnet address
	 * @param subnetMask Subnet mask
	 */
	public ClusterSyncNetwork(InetAddress subnetAddress, InetAddress subnetMask)
	{
		this.subnetAddress = subnetAddress;
		this.subnetMask = subnetMask;
	}

	/**
	 * @return the subnetAddress
	 */
	public InetAddress getSubnetAddress()
	{
		return subnetAddress;
	}

	/**
	 * @return the subnetMask
	 */
	public InetAddress getSubnetMask()
	{
		return subnetMask;
	}
}
