/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import java.util.ArrayList;

import org.eclipse.swt.widgets.Shell;
import org.netxms.client.AgentParameter;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Template;

/**
 * @author victor
 *
 */
public class SelectInternalParamDlg extends AbstractSelectParamDlg
{
	public SelectInternalParamDlg(Shell parentShell, long nodeId)
	{
		super(parentShell, nodeId);
	}

	/**
	 * Fill parameter list
	 */
	protected void fillParameterList()
	{
		ArrayList<AgentParameter> list = new ArrayList<AgentParameter>(10);

		// Internal parameters common for all nodes
		list.add(new AgentParameter("ChildStatus(*)", "Status of child object {instance}", DataCollectionItem.DT_INT));
		list.add(new AgentParameter("ConditionStatus(*)", "Status of condition object {instance}", DataCollectionItem.DT_INT));
		list.add(new AgentParameter("Dummy", "Dummy value", DataCollectionItem.DT_INT));
		list.add(new AgentParameter("Net.IP.NextHop(*)", "Next routing hop for IP address {instance}", DataCollectionItem.DT_STRING));
		list.add(new AgentParameter("Status", "Status", DataCollectionItem.DT_INT));
		
		if ((object instanceof Template) || ((Node)object).hasAgent())
		{
			list.add(new AgentParameter("AgentStatus", "Status of NetXMS agent", DataCollectionItem.DT_INT));
		}
		
		if ((object instanceof Template) || ((Node)object).isManagementServer())
		{
			list.add(new AgentParameter("Server.AverageConfigurationPollerQueueSize", "Average length of configuration poller queue for last minute", DataCollectionItem.DT_FLOAT));
			list.add(new AgentParameter("AverageDBWriterQueueSize", "Average length of database writer's request queue for last minute", DataCollectionItem.DT_FLOAT));
			list.add(new AgentParameter("AverageDCIQueuingTime", "Average time to queue DCI for polling for last minute", DataCollectionItem.DT_UINT));
			list.add(new AgentParameter("AverageDCPollerQueueSize", "Average length of data collection poller's request queue for last minute", DataCollectionItem.DT_FLOAT));
			list.add(new AgentParameter("AverageStatusPollerQueueSize", "Average length of status poller queue for last minute", DataCollectionItem.DT_FLOAT));
		}
		
		viewer.setInput(list.toArray());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#getConfigurationPrefix()
	 */
	@Override
	protected String getConfigurationPrefix()
	{
		return "SelectInternalParamDlg";
	}
}
