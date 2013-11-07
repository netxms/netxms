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
package org.netxms.client.datacollection;

import org.netxms.base.NXCPMessage;

/**
 * DCI information used in condition objects
 *
 */
public class ConditionDciInfo
{
	private long nodeId;
	private long dciId;
	private int function;
	private int polls;
	
	/**
	 * @param nodeId
	 * @param dciId
	 * @param function
	 * @param polls
	 */
	public ConditionDciInfo(long nodeId, long dciId, int function, int polls)
	{
		this.nodeId = nodeId;
		this.dciId = dciId;
		this.function = function;
		this.polls = polls;
	}

	/**
	 * Create DCI information from NXCP message
	 * 
	 * @param msg
	 * @param baseId
	 */
	public ConditionDciInfo(NXCPMessage msg, long baseId)
	{
		dciId = msg.getVariableAsInt64(baseId);
		nodeId = msg.getVariableAsInt64(baseId + 1);
		function = msg.getVariableAsInteger(baseId + 2);
		polls = msg.getVariableAsInteger(baseId + 3);
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src source object
	 */
	public ConditionDciInfo(ConditionDciInfo src)
	{
		dciId = src.dciId;
		nodeId = src.nodeId;
		function = src.function;
		polls = src.polls;
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the dciId
	 */
	public long getDciId()
	{
		return dciId;
	}

	/**
	 * @return the function
	 */
	public int getFunction()
	{
		return function;
	}

	/**
	 * @return the polls
	 */
	public int getPolls()
	{
		return polls;
	}

	/**
	 * @param function the function to set
	 */
	public void setFunction(int function)
	{
		this.function = function;
	}

	/**
	 * @param polls the polls to set
	 */
	public void setPolls(int polls)
	{
		this.polls = polls;
	}
}
