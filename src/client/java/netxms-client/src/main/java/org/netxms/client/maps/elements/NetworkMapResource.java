/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
 * Resource (anything related to nodes but not NetXMS object - like cluster resources) displayed on map.
 */
public class NetworkMapResource extends NetworkMapElement
{
	public static final int CLUSTER_RESOURCE = 1;
	
	private int type;
	private Object data;
	
	/**
	 * Create from NXCP message.
	 *
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	public NetworkMapResource(NXCPMessage msg, long baseId)
	{
		super(msg, baseId);
		type = msg.getFieldAsInt32(baseId + 10);
		data = null;
	}

	/**
	 * Create resource object from scratch.
	 *
	 * @param id element ID
	 * @param type resource type (currently the only supported type is <code>CLUSTER_RESOURCE</code>)
	 * @param data user-defined resource data
	 */
	public NetworkMapResource(long id, int type, Object data)
	{
		super(id);
		this.type = type;
		this.data = data;
	}

	/**
	 * @see org.netxms.client.maps.elements.NetworkMapElement#fillMessage(org.netxms.base.NXCPMessage, long)
	 */
	@Override
	public void fillMessage(NXCPMessage msg, long baseId)
	{
		super.fillMessage(msg, baseId);
		msg.setFieldInt32(baseId + 10, type);
	}

	/**
	 * Get resource type.
	 *
	 * @return resource type
	 */
	public int getType()
	{
		return type;
	}

	/**
	 * Get user-defined resource data.
	 *
	 * @return resource data
	 */
	public Object getData()
	{
		return data;
	}
}
