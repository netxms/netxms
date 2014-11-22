/**
 * 
 */
package org.netxms.client;

import java.net.InetAddress;
import org.netxms.base.NXCPMessage;

/**
 * Element of IP address list. Can represent either subnet or address range.
 */
public class IpAddressListElement
{
	public static final int SUBNET = 0;
	public static final int RANGE = 1;
	
	private int type;
	private InetAddress addr1;
	private InetAddress addr2;
	
	/**
	 * Create new element
	 * 
	 * @param type
	 * @param addr1
	 * @param addr2
	 */
	public IpAddressListElement(int type, InetAddress addr1, InetAddress addr2)
	{
		this.type = type;
		this.addr1 = addr1;
		this.addr2 = addr2;
	}

	/**
	 * Create element from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base variable ID
	 */
	protected IpAddressListElement(NXCPMessage msg, long baseId)
	{
		type = msg.getFieldAsInt32(baseId);
		addr1 = msg.getFieldAsInetAddress(baseId + 1);
		addr2 = msg.getFieldAsInetAddress(baseId + 2);
	}

	/**
	 * @return the type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * Get first address (subnet base address or range start)
	 * 
	 * @return the addr1
	 */
	public InetAddress getAddr1()
	{
		return addr1;
	}

	/**
	 * Get second address (subnet mask or range last address)
	 * 
	 * @return the addr2
	 */
	public InetAddress getAddr2()
	{
		return addr2;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#toString()
	 */
	@Override
	public String toString()
	{
		StringBuilder sb = new StringBuilder();
		sb.append(addr1.getHostAddress());
		sb.append((type == SUBNET) ? '/' : '-');
		sb.append(addr2.getHostAddress());
		return sb.toString();
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		final int prime = 31;
		int result = 1;
		result = prime * result + ((addr1 == null) ? 0 : addr1.hashCode());
		result = prime * result + ((addr2 == null) ? 0 : addr2.hashCode());
		result = prime * result + type;
		return result;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		IpAddressListElement other = (IpAddressListElement)obj;
		if (addr1 == null)
		{
			if (other.addr1 != null)
				return false;
		}
		else if (!addr1.equals(other.addr1))
			return false;
		if (addr2 == null)
		{
			if (other.addr2 != null)
				return false;
		}
		else if (!addr2.equals(other.addr2))
			return false;
		if (type != other.type)
			return false;
		return true;
	}
}
