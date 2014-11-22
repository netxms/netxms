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
 * Generic agent policy object
 *
 */
public class AgentPolicy extends GenericObject
{
	private int version;
	private int policyType;
	private String description;
	
	/**
	 * @param msg
	 * @param session
	 */
	public AgentPolicy(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		policyType = msg.getFieldAsInt32(NXCPCodes.VID_POLICY_TYPE);
		version = msg.getFieldAsInt32(NXCPCodes.VID_VERSION);
		description = msg.getFieldAsString(NXCPCodes.VID_DESCRIPTION);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "AgentPolicy";
	}

	/**
	 * @return the version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @return the policyType
	 */
	public int getPolicyType()
	{
		return policyType;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}
}
