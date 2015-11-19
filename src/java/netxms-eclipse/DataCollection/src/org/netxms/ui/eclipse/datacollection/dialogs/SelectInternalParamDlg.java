/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.datacollection.Messages;

/**
 * Dialog for selecting parameters with "Internal" source
 */
public class SelectInternalParamDlg extends AbstractSelectParamDlg
{
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
		list.add(new AgentParameter("ChildStatus(*)", Messages.get().SelectInternalParamDlg_DCI_ChildObjectStatus, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("ConditionStatus(*)", Messages.get().SelectInternalParamDlg_DCI_ConditionStatus, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("Dummy", Messages.get().SelectInternalParamDlg_DCI_Dummy, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("Status", Messages.get().SelectInternalParamDlg_DCI_Status, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		list.add(new AgentParameter("PingTime", Messages.get().SelectInternalParamDlg_PingTime_PrimaryIP, DataCollectionItem.DT_UINT)); //$NON-NLS-1$
		list.add(new AgentParameter("PingTime(*)", Messages.get().SelectInternalParamDlg_PingTime_Instance, DataCollectionItem.DT_UINT)); //$NON-NLS-1$
		
		if ((object instanceof Template) || (object instanceof AbstractNode))
		{
			list.add(new AgentParameter("Net.IP.NextHop(*)", Messages.get().SelectInternalParamDlg_DCI_NextHop, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
         list.add(new AgentParameter("NetSvc.ResponseTime(*)", "Network service {instance} response time", DataCollectionItem.DT_UINT)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).hasAgent()))
		{
			list.add(new AgentParameter("AgentStatus", Messages.get().SelectInternalParamDlg_DCI_AgentStatus, DataCollectionItem.DT_INT)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).isManagementServer()))
		{
			list.add(new AgentParameter("Server.AverageDBWriterQueueSize", Messages.get().SelectInternalParamDlg_DCI_AvgDBWriterQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AverageDBWriterQueueSize.IData", "Database writer's request queue (DCI data) for last minute", DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AverageDBWriterQueueSize.Other", "Database writer's request queue (other queries) for last minute", DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AverageDBWriterQueueSize.RawData", "Database writer's request queue (raw DCI data) for last minute", DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
			list.add(new AgentParameter("Server.AverageDCIQueuingTime", Messages.get().SelectInternalParamDlg_DCI_AvgDCIQueueTime, DataCollectionItem.DT_UINT)); //$NON-NLS-1$
			list.add(new AgentParameter("Server.AverageDCPollerQueueSize", Messages.get().SelectInternalParamDlg_DCI_AvgDCQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AverageSyslogProcessingQueueSize", Messages.get().SelectInternalParamDlg_SyslogProcessingQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AverageSyslogWriterQueueSize", Messages.get().SelectInternalParamDlg_SyslogWriterQueue, DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.Failed", "Failed DB queries", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.LongRunning", "Long running DB queries", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.NonSelect", "Non-SELECT DB queries", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.Select", "SELECT DB queries", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.Total", "Total DB queries", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DBWriter.Requests.IData", "DB writer requests (DCI data)", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DBWriter.Requests.Other", "DB writer requests (other queries)", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DBWriter.Requests.RawData", "DB writer requests (raw DCI data)", DataCollectionItem.DT_UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.ActiveRequests(*)", "Thread pool {instance}: active requests", DataCollectionItem.DT_INT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.CurrSize(*)", "Thread pool {instance}: current size", DataCollectionItem.DT_INT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.Load(*)", "Thread pool {instance}: current load", DataCollectionItem.DT_INT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.LoadAverage(*)", "Thread pool {instance}: load average (1 minute)", DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.LoadAverage5(*)", "Thread pool {instance}: load average (5 minutes)", DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.LoadAverage15(*)", "Thread pool {instance}: load average (15 minutes)", DataCollectionItem.DT_FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.MaxSize(*)", "Thread pool {instance}: maximum size", DataCollectionItem.DT_INT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.MinSize(*)", "Thread pool {instance}: minimum size", DataCollectionItem.DT_INT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.Usage(*)", "Thread pool {instance}: usage", DataCollectionItem.DT_INT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.TotalEventsProcessed", Messages.get().SelectInternalParamDlg_DCI_TotalEventsProcessed, DataCollectionItem.DT_UINT)); //$NON-NLS-1$
		}

		if ((object instanceof Template) || (object instanceof MobileDevice))
		{
			list.add(new AgentParameter("MobileDevice.BatteryLevel", Messages.get().SelectInternalParamDlg_DCI_BatteryLevel, DataCollectionItem.DT_INT)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.DeviceId", Messages.get().SelectInternalParamDlg_DCI_DeviceID, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.LastReportTime", Messages.get().SelectInternalParamDlg_DCI_LastReportTime, DataCollectionItem.DT_INT64)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.Model", Messages.get().SelectInternalParamDlg_DCI_Model, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.OS.Name", Messages.get().SelectInternalParamDlg_DCI_OSName, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.OS.Version", Messages.get().SelectInternalParamDlg_DCI_OSVersion, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.SerialNumber", Messages.get().SelectInternalParamDlg_DCI_SerialNumber, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.Vendor", Messages.get().SelectInternalParamDlg_DCI_Vendor, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.UserId", Messages.get().SelectInternalParamDlg_DCI_UserID, DataCollectionItem.DT_STRING)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).isWirelessController()))
		{
			list.add(new AgentParameter("WirelessController.AdoptedAPCount", Messages.get().SelectInternalParamDlg_AdoptedAPs, DataCollectionItem.DT_INT)); //$NON-NLS-1$
			list.add(new AgentParameter("WirelessController.TotalAPCount", Messages.get().SelectInternalParamDlg_TotalAPs, DataCollectionItem.DT_INT)); //$NON-NLS-1$
			list.add(new AgentParameter("WirelessController.UnadoptedAPCount", Messages.get().SelectInternalParamDlg_UnadoptedAPs, DataCollectionItem.DT_INT)); //$NON-NLS-1$
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
