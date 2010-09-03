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
package org.netxms.client.datacollection;

/**
 * This class represents single graph item (DCI)
 *
 */
public class GraphItem
{
	private long nodeId;
	private long dciId;
	private int source;
	private int dataType;
	private String name;
	private String description;

	/**
	 * Create graph item object with default values
	 */
	public GraphItem()
	{
		nodeId = 0;
		dciId = 0;
		source = DataCollectionItem.AGENT;
	}
}
