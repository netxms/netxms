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
package org.netxms.client.datacollection;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPMessage;
import org.netxms.client.constants.Severity;

/**
 * Threshold violation summary for node
 */
public class ThresholdViolationSummary
{
	private long nodeId;
	private List<DciValue> dciList;
	
	/**
	 * Create from NXCP message.
	 * 
	 * @param msg
	 * @param baseId
	 */
	public ThresholdViolationSummary(NXCPMessage msg, long baseId)
	{
		nodeId = msg.getVariableAsInt64(baseId);
		int count = msg.getVariableAsInteger(baseId + 1);
		dciList = new ArrayList<DciValue>(count);
		long varId = baseId + 2;
		for(int i = 0; i < count; i++)
		{
			dciList.add(DciValue.createFromMessage(nodeId, msg, varId));
			varId += 50;
		}
	}

	/**
	 * @return the nodeId
	 */
	public long getNodeId()
	{
		return nodeId;
	}

	/**
	 * @return the dciList
	 */
	public List<DciValue> getDciList()
	{
		return dciList;
	}
	
	/**
	 * Get current most critical severity level from active thresholds.
	 * 
	 * @return
	 */
	public int getCurrentSeverity()
	{
		int severity = Severity.NORMAL;
		for(DciValue v : dciList)
		{
			if (v.getActiveThreshold().getCurrentSeverity() > severity)
				severity = v.getActiveThreshold().getCurrentSeverity();
		}
		return severity;
	}
}
