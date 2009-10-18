/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.maps;

/**
 * Data for representing object on map
 * @author Victor
 *
 */
public class NetworkMapObjectData
{
	public static final int NO_POSITION = -1;
	
	private long objectId;
	private int x;
	private int y;
	
	/**
	 * Object with position
	 */
	public NetworkMapObjectData(long objectId, int x, int y)
	{
		this.objectId = objectId;
		this.x = x;
		this.y = y;
	}

	/**
	 * Unpositioned object
	 */
	public NetworkMapObjectData(long objectId)
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
		if (arg0 instanceof NetworkMapObjectData)
			return ((NetworkMapObjectData)arg0).objectId == this.objectId;
		return super.equals(arg0);
	}
}
