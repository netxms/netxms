/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	public ThresholdViolationSummary(NXCPMessage msg, long baseId)
	{
		nodeId = msg.getFieldAsInt64(baseId);
		int count = msg.getFieldAsInt32(baseId + 1);
		dciList = new ArrayList<DciValue>(count);
		long fieldId = baseId + 2;
		for(int i = 0; i < count; i++)
		{
         DciValue v = DciValue.createFromMessage(msg, fieldId);
         if (v.getActiveThreshold() != null)
            dciList.add(v);
			fieldId += 50;
		}
	}

	/**
	 * Get owning node ID
	 * 
	 * @return owning node ID
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
	 * @return current most critical severity level from active thresholds
	 */
	public Severity getCurrentSeverity()
	{
		Severity severity = Severity.NORMAL;
		for(DciValue v : dciList)
		{
			if (v.getActiveThreshold().getCurrentSeverity().compareTo(severity) > 0)
				severity = v.getActiveThreshold().getCurrentSeverity();
		}
		return severity;
	}
}
