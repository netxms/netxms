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
		objectId = msg.getVariableAsInt64(baseId + 10);
	}

	/**
	 * @return the objectId
	 */
	public long getObjectId()
	{
		return objectId;
	}
}
