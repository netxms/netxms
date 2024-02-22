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
package org.netxms.ui.eclipse.networkmaps.objecttabs.helpers;

import org.netxms.client.maps.elements.NetworkMapElement;

/**
 * Cluster resource displayed on map.
 */
public class ClusterResourceMapElement extends NetworkMapElement
{
	private Object data;
	
	/**
	 * Create resource object from scratch.
	 *
	 * @param id element ID
	 * @param type resource type (currently the only supported type is <code>CLUSTER_RESOURCE</code>)
	 * @param data user-defined resource data
	 */
   public ClusterResourceMapElement(long id, Object data)
	{
		super(id);
		this.data = data;
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
