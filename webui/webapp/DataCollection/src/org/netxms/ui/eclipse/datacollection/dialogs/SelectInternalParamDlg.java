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
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.datacollection.Messages;

/**
 * @author victor
 *
 */
public class SelectInternalParamDlg extends AbstractSelectParamDlg
{
	private static final long serialVersionUID = 1L;

	/**
	 * @param parentShell
	 * @param nodeId
	 */
	public SelectInternalParamDlg(Shell parentShell, long nodeId)
	{
		super(parentShell, nodeId, false);
	}

	/**
	 * Fill parameter list
	 */
	protected void fillParameterList()
	{
		ArrayList<AgentParameter> list = new ArrayList<AgentParameter>(10);

		// Internal parameters common for all nodes
		list.add(new AgentParameter("ChildStatus(*)", Messages.SelectInternalParamDlg_DCI_ChildObjectStatus, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("ConditionStatus(*)", Messages.SelectInternalParamDlg_DCI_ConditionStatus, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("Dummy", Messages.SelectInternalParamDlg_DCI_Dummy, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("Status", Messages.SelectInternalParamDlg_DCI_Status, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		
		if ((object instanceof Template) || (object instanceof Node))
		{
			list.add(new AgentParameter("Net.IP.NextHop(*)", Messages.SelectInternalParamDlg_DCI_NextHop, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof Node) && ((Node)object).hasAgent()))
		{
			list.add(new AgentParameter("AgentStatus", Messages.SelectInternalParamDlg_DCI_AgentStatus, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof Node) && ((Node)object).isManagementServer()))
		{
			list.add(new AgentParameter("Server.AverageConfigurationPollerQueueSize", Messages.SelectInternalParamDlg_DCI_AvgConfPollerQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
			list.add(new AgentParameter("AverageDBWriterQueueSize", Messages.SelectInternalParamDlg_DCI_AvgDBWriterQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
			list.add(new AgentParameter("AverageDCIQueuingTime", Messages.SelectInternalParamDlg_DCI_AvgDCIQueueTime, DataCollectionItem.DT_UINT)); //$NON-NLS-1$
			list.add(new AgentParameter("AverageDCPollerQueueSize", Messages.SelectInternalParamDlg_DCI_AvgDCQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
			list.add(new AgentParameter("AverageStatusPollerQueueSize", Messages.SelectInternalParamDlg_DCI_AvgStatusPollerQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
		}

		if ((object instanceof Template) || (object instanceof MobileDevice))
		{
			list.add(new AgentParameter("MobileDevice.BatteryLevel", Messages.SelectInternalParamDlg_DCI_BatteryLevel, DataCollectionItem.DT_INT)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.DeviceId", Messages.SelectInternalParamDlg_DCI_DeviceID, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.LastReportTime", Messages.SelectInternalParamDlg_DCI_LastReportTime, DataCollectionItem.DT_INT64)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.Model", Messages.SelectInternalParamDlg_DCI_Model, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.OS.Name", Messages.SelectInternalParamDlg_DCI_OSName, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.OS.Version", Messages.SelectInternalParamDlg_DCI_OSVersion, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.SerialNumber", Messages.SelectInternalParamDlg_DCI_SerialNumber, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.Vendor", Messages.SelectInternalParamDlg_DCI_Vendor, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.UserId", Messages.SelectInternalParamDlg_DCI_UserID, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
		}
		
		viewer.setInput(list.toArray());
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#getConfigurationPrefix()
	 */
	@Override
	protected String getConfigurationPrefix()
	{
		return "SelectInternalParamDlg"; //$NON-NLS-1$
	}
}
