/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.net.InetAddress;

import org.netxms.base.NXCPMessage;

/**
 * This class represents single cluster resource
 */
public class ClusterResource
{
	private long id;
	private String name;
	private InetAddress virtualAddress;
	private long currentOwner;
	
	/**
	 * Create cluster resource object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId Base variable ID
	 */
	protected ClusterResource(NXCPMessage msg, long baseId)
	{
		id = msg.getVariableAsInt64(baseId);
		name = msg.getVariableAsString(baseId + 1);
		virtualAddress = msg.getVariableAsInetAddress(baseId + 2);
		currentOwner = msg.getVariableAsInt64(baseId + 3);
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ClusterResource(ClusterResource src)
	{
		id = src.id;
		name = src.name;
		virtualAddress = src.virtualAddress;
		currentOwner = src.currentOwner;
	}
	
	/**
	 * Create new cluster resource object.
	 * 
	 * @param id
	 * @param name
	 * @param virtualAddress
	 */
	public ClusterResource(long id, String name, InetAddress virtualAddress)
	{
		this.id = id;
		this.name = name;
		this.virtualAddress = virtualAddress;
		this.currentOwner = 0;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the virtualAddress
	 */
	public InetAddress getVirtualAddress()
	{
		return virtualAddress;
	}

	/**
	 * @return the currentOwner
	 */
	public long getCurrentOwner()
	{
		return currentOwner;
	}

	/**
	 * @param name the name to set
	 */
	public void setName(String name)
	{
		this.name = name;
	}

	/**
	 * @param virtualAddress the virtualAddress to set
	 */
	public void setVirtualAddress(InetAddress virtualAddress)
	{
		this.virtualAddress = virtualAddress;
	}
}
