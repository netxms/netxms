/**
 * 
 */
package org.netxms.client;

/**
 * Data for representing object on map
 * @author Victor
 *
 */
public class NXCMapObjectData
{
	public static final int NO_POSITION = -1;
	
	private long objectId;
	private int x;
	private int y;
	
	/**
	 * Object with position
	 */
	public NXCMapObjectData(long objectId, int x, int y)
	{
		this.objectId = objectId;
		this.x = x;
		this.y = y;
	}

	/**
	 * Unpositioned object
	 */
	public NXCMapObjectData(long objectId)
	{
		this.objectId = objectId;
		this.x = NO_POSITION;
		this.y = NO_POSITION;
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/**
	 * @return the x
	 */
	public int getX()
	{
		return x;
	}

	/**
	 * @return the y
	 */
	public int getY()
	{
		return y;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object arg0)
	{
		if (arg0 instanceof NXCMapObjectData)
			return ((NXCMapObjectData)arg0).objectId == this.objectId;
		return super.equals(arg0);
	}
}
