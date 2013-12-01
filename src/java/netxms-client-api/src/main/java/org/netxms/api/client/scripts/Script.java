/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 NetXMS Team
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
package org.netxms.api.client.scripts;

/**
 * NXSL script object
 *
 */
public class Script
{
	private long id;
	private String name;
	private String source;
	
	/**
	 * Create script object with given ID and source code.
	 * 
	 * @param id
	 * @param source
	 */
	public Script(long id, String name, String source)
	{
		this.id = id;
		this.name = (name != null) ? name : ("[" + Long.toString(id) + "]");
		this.source = source;
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the source
	 */
	public String getSource()
	{
		return source;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}
}
