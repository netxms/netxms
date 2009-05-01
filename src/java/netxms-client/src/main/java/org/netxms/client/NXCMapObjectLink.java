/**
 * 
 */
package org.netxms.client;

/**
 * Represents link between two objects on map
 * 
 * @author Victor
 *
 */
public class NXCMapObjectLink
{
	// Link types
	public static final int NORMAL = 0;
	public static final int VPN = 1;
	
	private int linkType;
	private long object1;
	private long object2;
	private String port1;
	private String port2;

	/**
	 * @param linkType
	 * @param object1
	 * @param object2
	 * @param port1
	 * @param port2
	 */
	public NXCMapObjectLink(int linkType, long object1, long object2, String port1, String port2)
	{
		this.linkType = linkType;
		this.object1 = object1;
		this.object2 = object2;
		this.port1 = port1;
		this.port2 = port2;
	}

	/**
	 * @param linkType
	 * @param object1
	 * @param object2
	 * @param port1
	 * @param port2
	 */
	public NXCMapObjectLink(int linkType, long object1, long object2)
	{
		this.linkType = linkType;
		this.object1 = object1;
		this.object2 = object2;
		this.port1 = "";
		this.port2 = "";
	}

	/**
	 * @return the linkType
	 */
	public int getLinkType()
	{
		return linkType;
	}

	/**
	 * @return the object1
	 */
	public long getObject1()
	{
		return object1;
	}

	/**
	 * @return the object2
	 */
	public long getObject2()
	{
		return object2;
	}

	/**
	 * @return the port1
	 */
	public String getPort1()
	{
		return port1;
	}

	/**
	 * @return the port2
	 */
	public String getPort2()
	{
		return port2;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object obj)
	{
		if (obj instanceof NXCMapObjectLink)
			return (((NXCMapObjectLink)obj).object1 == this.object1) &&
			       (((NXCMapObjectLink)obj).object2 == this.object2);
		return super.equals(obj);
	}
}
