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
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * Container object
 */
public class Container extends GenericObject
{
	public static final int CF_AUTO_BIND = 0x000001;
	public static final int CF_AUTO_UNBIND = 0x000002;
	
	private int category;
	private int flags;
	private String autoBindFilter;
	
	/**
	 * @param msg
	 */
	public Container(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		category = msg.getVariableAsInteger(NXCPCodes.VID_CATEGORY);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		autoBindFilter = msg.getVariableAsString(NXCPCodes.VID_AUTOBIND_FILTER);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
	 */
	@Override
	public boolean isAllowedOnMap()
	{
		return true;
	}

	/**
	 * @return the category
	 */
	public int getCategory()
	{
		return category;
	}

	/**
	 * @return true if automatic bind is enabled
	 */
	public boolean isAutoBindEnabled()
	{
		return (flags & CF_AUTO_BIND) != 0;
	}

	/**
	 * @return true if automatic unbind is enabled
	 */
	public boolean isAutoUnbindEnabled()
	{
		return (flags & CF_AUTO_UNBIND) != 0;
	}

	/**
	 * @return Filter script for automatic bind
	 */
	public String getAutoBindFilter()
	{
		return autoBindFilter;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.objects.GenericObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Container";
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}
}
