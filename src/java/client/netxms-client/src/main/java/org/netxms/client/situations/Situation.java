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
 */package org.netxms.client.situations;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Situation object
 */
public class Situation
{
	private long id;
	private String name;
	private String comments;
	private List<SituationInstance> instances;
	
	/**
	 * Create situation object from NXCP message
	 * 
	 * @param msg
	 */
	public Situation(NXCPMessage msg)
	{
		id = msg.getFieldAsInt64(NXCPCodes.VID_SITUATION_ID);
		name = msg.getFieldAsString(NXCPCodes.VID_NAME);
		comments = msg.getFieldAsString(NXCPCodes.VID_COMMENTS);
		
		int count = msg.getFieldAsInt32(NXCPCodes.VID_INSTANCE_COUNT);
		instances = new ArrayList<SituationInstance>(count);
		long varId = NXCPCodes.VID_INSTANCE_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			final SituationInstance si = new SituationInstance(this, msg, varId);
			instances.add(si);
			varId += si.getAttributeCount() * 2 + 2;
		}
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
	 * @return the comments
	 */
	public String getComments()
	{
		return comments;
	}

	/**
	 * @return the instances
	 */
	public List<SituationInstance> getInstances()
	{
		return instances;
	}
}
