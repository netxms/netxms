/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
 * Push DCI data
 */
public class DciPushData
{
	public long nodeId;
	public String nodeName;
	public long dciId;
	public String dciName;
	public String value;
	
	/**
	 * @param nodeId
	 * @param dciId
	 * @param value
	 */
	public DciPushData(long nodeId, long dciId, String value)
	{
		this.nodeId = nodeId;
		this.nodeName = null;
		this.dciId = dciId;
		this.dciName = null;
		this.value = value;
	}

	/**
	 * @param nodeName
	 * @param dciName
	 * @param value
	 */
	public DciPushData(String nodeName, String dciName, String value)
	{
		this.nodeId = 0;
		this.nodeName = nodeName;
		this.dciId = 0;
		this.dciName = dciName;
		this.value = value;
	}
}
