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
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ClusterSyncNetwork(ClusterSyncNetwork src)
	{
		this.subnetAddress = src.subnetAddress;
		this.subnetMask = src.subnetMask;
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

	/**
	 * @param subnetAddress the subnetAddress to set
	 */
	public void setSubnetAddress(InetAddress subnetAddress)
	{
		this.subnetAddress = subnetAddress;
	}

	/**
	 * @param subnetMask the subnetMask to set
	 */
	public void setSubnetMask(InetAddress subnetMask)
	{
		this.subnetMask = subnetMask;
	}
}
