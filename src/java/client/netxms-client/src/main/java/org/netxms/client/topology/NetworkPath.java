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
package org.netxms.client.topology;

import java.util.ArrayList;
import java.util.List;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Network path
 */
public class NetworkPath
{
	private List<HopInfo> path;
	private boolean complete;
	
	/**
	 * Create network path object from NXCP message
	 * 
	 * @param msg NXCP message
	 */
	public NetworkPath(NXCPMessage msg)
	{
		complete = msg.getVariableAsBoolean(NXCPCodes.VID_IS_COMPLETE);
		int count = msg.getVariableAsInteger(NXCPCodes.VID_HOP_COUNT);
		path = new ArrayList<HopInfo>(count);
		long varId = NXCPCodes.VID_NETWORK_PATH_BASE;
		for(int i = 0; i < count; i++)
		{
			path.add(new HopInfo(msg, varId));
			varId += 10;
		}
	}

	/**
	 * @return the path
	 */
	public List<HopInfo> getPath()
	{
		return path;
	}

	/**
	 * @return the complete
	 */
	public boolean isComplete()
	{
		return complete;
	}
}
