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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * This class represents NetXMS TEMPLATE objects.
 * 
 * @author Victor
 *
 */
public class Template extends GenericObject
{
	private int version;
	private boolean autoApplyEnabled;
	private String autoApplyFilter;

	/**
	 * @param msg
	 * @param session
	 */
	public Template(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		version = msg.getVariableAsInteger(NXCPCodes.VID_VERSION);
		autoApplyEnabled = msg.getVariableAsBoolean(NXCPCodes.VID_AUTO_APPLY);
		autoApplyFilter = msg.getVariableAsString(NXCPCodes.VID_APPLY_FILTER);
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
		return autoApplyEnabled;
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
}
