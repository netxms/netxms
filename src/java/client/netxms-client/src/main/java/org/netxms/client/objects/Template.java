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
 * This class represents NetXMS TEMPLATE objects.
 */
public class Template extends GenericObject
{
	public static final int TF_AUTO_APPLY = 0x000001;
	public static final int TF_AUTO_REMOVE = 0x000002;
	
	private int version;
	private int flags;
	private String autoApplyFilter;

	/**
	 * @param msg
	 * @param session
	 */
	public Template(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		version = msg.getVariableAsInteger(NXCPCodes.VID_VERSION);
		flags = msg.getVariableAsInteger(NXCPCodes.VID_FLAGS);
		autoApplyFilter = msg.getVariableAsString(NXCPCodes.VID_AUTOBIND_FILTER);
	}

	/**
	 * @return template version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @return template version as string in form major.minor
	 */
	public String getVersionAsString()
	{
		return Integer.toString(version >> 16) + "." + Integer.toString(version & 0xFFFF);
	}

	/**
	 * @return true if automatic apply is enabled
	 */
	public boolean isAutoApplyEnabled()
	{
		return (flags & TF_AUTO_APPLY) != 0;
	}

	/**
	 * @return true if automatic removal is enabled
	 */
	public boolean isAutoRemoveEnabled()
	{
		return (flags & TF_AUTO_REMOVE) != 0;
	}

	/**
	 * @return Filter script for automatic apply
	 */
	public String getAutoApplyFilter()
	{
		return autoApplyFilter;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Template";
	}

	/**
	 * @return the flags
	 */
	public int getFlags()
	{
		return flags;
	}
}
