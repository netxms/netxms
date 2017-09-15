/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client.maps.elements;

import org.netxms.base.NXCPMessage;

/**
 * Network map element representing NetXMS object
 *
 */
public class NetworkMapObject extends NetworkMapElement
{
	private long objectId;
	
	/**
	 * @param msg
	 * @param baseId
	 */
	protected NetworkMapObject(NXCPMessage msg, long baseId)
	{
		super(msg, baseId);
		objectId = msg.getFieldAsInt64(baseId + 10);
	}
	
	/**
	 * Create new object element
	 * 
	 * @param id element ID
	 * @param objectId NetXMS object ID
	 */
	public NetworkMapObject(long id, long objectId)
	{
		super(id);
		this.objectId = objectId;
		type = MAP_ELEMENT_OBJECT;
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.maps.elements.NetworkMapElement#fillMessage(org.netxms.base.NXCPMessage, long)
	 */
	@Override
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		super.fillMessage(msg, baseId);
		msg.setFieldInt32(baseId + 10, (int)objectId);
	}

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "NetworkMapObject [objectId=" + objectId + ", id=" + id + ", type=" + type + ", x=" + x + ", y=" + y + "]";
   }
}
