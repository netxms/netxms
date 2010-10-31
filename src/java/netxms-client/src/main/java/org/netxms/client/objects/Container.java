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
package org.netxms.client.objects;

import org.netxms.base.*;
import org.netxms.client.NXCSession;

/**
 * Container object
 *
 */
public class Container extends GenericObject
{
	private int category;
	private boolean autoBindEnabled;
	private String autoBindFilter;
	
	/**
	 * @param msg
	 */
	public Container(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		category = msg.getVariableAsInteger(NXCPCodes.VID_CATEGORY);
		autoBindEnabled = msg.getVariableAsBoolean(NXCPCodes.VID_ENABLE_AUTO_BIND);
		autoBindFilter = msg.getVariableAsString(NXCPCodes.VID_AUTO_BIND_FILTER);
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
		return autoBindEnabled;
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
}
