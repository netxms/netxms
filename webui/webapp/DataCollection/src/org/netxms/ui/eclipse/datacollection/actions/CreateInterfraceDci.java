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
package org.netxms.ui.eclipse.datacollection.actions;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IViewReference;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.AbstractNode;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.CreateInterfaceDciDialog;
import org.netxms.ui.eclipse.datacollection.dialogs.helpers.InterfaceDciInfo;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "Create DCI for interface" action
 */
public class CreateInterfraceDci implements IObjectActionDelegate
{
	private static final int IFDCI_IN_BYTES = 0;
	private static final int IFDCI_OUT_BYTES = 1;
	private static final int IFDCI_IN_PACKETS = 2;
	private static final int IFDCI_OUT_PACKETS = 3;
	private static final int IFDCI_IN_ERRORS = 4;
	private static final int IFDCI_OUT_ERRORS = 5;
	
	private Shell shell;
	private ViewPart viewPart;
	private List<Interface> objects = new ArrayList<Interface>();

	/**
	 * @see IObjectActionDelegate#setActivePart(IAction, IWorkbenchPart)
	 */
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		shell = targetPart.getSite().getShell();
		viewPart = (targetPart instanceof ViewPart) ? (ViewPart)targetPart : null;
	}

	/**
	 * @see IActionDelegate#run(IAction)
	 */
	public void run(IAction action)
	{
		final CreateInterfaceDciDialog dlg = new CreateInterfaceDciDialog(shell, (objects.size() == 1) ? objects.get(0) : null);
		if (dlg.open() == Window.OK)
		{
			final List<Interface> ifaces = new ArrayList<Interface>(objects);
			
			// Get set of nodes
			final Set<AbstractNode> nodes = new HashSet<AbstractNode>();
			for(Interface iface : ifaces)
			{
				AbstractNode node = iface.getParentNode();
				if (node != null)
				{
					nodes.add(node);
				}
			}
			
			// Check what nodes requires DCI list lock
			final Map<Long, Boolean> lockRequired = new HashMap<Long, Boolean>(nodes.size());
			final IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
			for(AbstractNode n : nodes)
			{
				IViewReference ref = page.findViewReference(DataCollectionEditor.ID, Long.toString(n.getObjectId()));
				lockRequired.put(n.getObjectId(), !((ref != null) && (ref.getView(false) != null)));
			}
			
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(Messages.get().CreateInterfraceDci_JobTitle, viewPart, Activator.PLUGIN_ID, null) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().CreateInterfraceDci_JobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					InterfaceDciInfo[] dciInfo = dlg.getDciInfo();
					monitor.beginTask(Messages.get().CreateInterfraceDci_TaskName, ifaces.size() * dciInfo.length);
					for(int i = 0; i < ifaces.size(); i++)
					{
						for(int j = 0; j < dciInfo.length; j++)
						{
							if (dciInfo[j].enabled)
							{
								createInterfaceDci(session, ifaces.get(i), j, dciInfo[j], dlg.getPollingInterval(), 
										dlg.getRetentionTime(), ifaces.size() > 1, lockRequired);
							}
							monitor.worked(1);
						}
					}
					monitor.done();
				}
			}.start();
		}
	}

	/**
	 * @param iface
	 * @param dciType
	 * @param dciInfo
	 * @throws Exception
	 */
	private static void createInterfaceDci(NXCSession session, Interface iface, int dciType, InterfaceDciInfo dciInfo, int pollingInterval,
			int retentionTime, boolean updateDescription, Map<Long, Boolean> lockRequired) throws Exception
	{
		AbstractNode node = iface.getParentNode();
		if (node == null)
			throw new NXCException(RCC.INTERNAL_ERROR);		
		
		DataCollectionConfiguration dcc;
		if (lockRequired.get(node.getObjectId()))
		{
			dcc = session.openDataCollectionConfiguration(node.getObjectId());
		}
		else
		{
			dcc = new DataCollectionConfiguration(session, node.getObjectId());
		}

		final DataCollectionItem dci = (DataCollectionItem)dcc.findItem(dcc.createItem(), DataCollectionItem.class);
		dci.setPollingInterval(pollingInterval);
		dci.setRetentionTime(retentionTime);
		if (node.hasAgent())
		{
			dci.setOrigin(DataCollectionItem.AGENT);
			if (node.isAgentIfXCountersSupported())
				dci.setDataType(((dciType != IFDCI_IN_ERRORS) && (dciType != IFDCI_OUT_ERRORS)) ? DataCollectionItem.DT_UINT64 : DataCollectionItem.DT_UINT);
			else
				dci.setDataType(DataCollectionItem.DT_UINT);
		}
		else
		{
			dci.setOrigin(DataCollectionItem.SNMP);
			if (node.isIfXTableSupported())
				dci.setDataType(((dciType != IFDCI_IN_ERRORS) && (dciType != IFDCI_OUT_ERRORS)) ? DataCollectionItem.DT_UINT64 : DataCollectionItem.DT_UINT);
			else
				dci.setDataType(DataCollectionItem.DT_UINT);
		}
		dci.setStatus(DataCollectionItem.ACTIVE);
		dci.setDescription(updateDescription ? dciInfo.description.replaceAll("@@ifName@@", iface.getObjectName()) : dciInfo.description); //$NON-NLS-1$
		dci.setDeltaCalculation(dciInfo.delta ? DataCollectionItem.DELTA_AVERAGE_PER_SECOND : DataCollectionItem.DELTA_NONE);
		
		if (dci.getOrigin() == DataCollectionItem.AGENT)
		{
			switch(dciType)
			{
				case IFDCI_IN_BYTES:
					dci.setName((node.isAgentIfXCountersSupported() ? "Net.Interface.BytesIn64(" : "Net.Interface.BytesIn(") + iface.getIfIndex() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
					break;
				case IFDCI_OUT_BYTES:
					dci.setName((node.isAgentIfXCountersSupported() ? "Net.Interface.BytesOut64(" : "Net.Interface.BytesOut(") + iface.getIfIndex() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
					break;
				case IFDCI_IN_PACKETS:
					dci.setName((node.isAgentIfXCountersSupported() ? "Net.Interface.PacketsIn64(" : "Net.Interface.PacketsIn(") + iface.getIfIndex() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
					break;
				case IFDCI_OUT_PACKETS:
					dci.setName((node.isAgentIfXCountersSupported() ? "Net.Interface.PacketsOut64(" : "Net.Interface.PacketsOut(") + iface.getIfIndex() + ")"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
					break;
				case IFDCI_IN_ERRORS:
					dci.setName("Net.Interface.InErrors(" + iface.getIfIndex() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
					break;
				case IFDCI_OUT_ERRORS:
					dci.setName("Net.Interface.OutErrors(" + iface.getIfIndex() + ")"); //$NON-NLS-1$ //$NON-NLS-2$
					break;
			}
		}
		else
		{
			switch(dciType)
			{
				case IFDCI_IN_BYTES:
					dci.setName((node.isIfXTableSupported() ? ".1.3.6.1.2.1.31.1.1.1.6." : ".1.3.6.1.2.1.2.2.1.10.") + iface.getIfIndex()); //$NON-NLS-1$ //$NON-NLS-2$
					break;
				case IFDCI_OUT_BYTES:
					dci.setName((node.isIfXTableSupported() ? ".1.3.6.1.2.1.31.1.1.1.10." : ".1.3.6.1.2.1.2.2.1.16.") + iface.getIfIndex()); //$NON-NLS-1$ //$NON-NLS-2$
					break;
				case IFDCI_IN_PACKETS:
					dci.setName((node.isIfXTableSupported() ? ".1.3.6.1.2.1.31.1.1.1.7." : ".1.3.6.1.2.1.2.2.1.11.") + iface.getIfIndex()); //$NON-NLS-1$ //$NON-NLS-2$
					break;
				case IFDCI_OUT_PACKETS:
					dci.setName((node.isIfXTableSupported() ? ".1.3.6.1.2.1.31.1.1.1.11." : ".1.3.6.1.2.1.2.2.1.17.") + iface.getIfIndex()); //$NON-NLS-1$ //$NON-NLS-2$
					break;
				case IFDCI_IN_ERRORS:
					dci.setName(".1.3.6.1.2.1.2.2.1.14." + iface.getIfIndex()); //$NON-NLS-1$
					break;
				case IFDCI_OUT_ERRORS:
					dci.setName(".1.3.6.1.2.1.2.2.1.20." + iface.getIfIndex()); //$NON-NLS-1$
					break;
			}
		}
		
		dcc.modifyObject(dci);
			
		if (lockRequired.get(node.getObjectId()))
		{
			dcc.close();
		}
	}
	
	/**
	 * @see IActionDelegate#selectionChanged(IAction, ISelection)
	 */
	public void selectionChanged(IAction action, ISelection selection)
	{
		objects.clear();
		if ((selection instanceof IStructuredSelection) &&
			 (((IStructuredSelection)selection).size() > 0))
		{
			for(Object o : ((IStructuredSelection)selection).toList())
			{
				if (o instanceof Interface)
					objects.add((Interface)o);
			}
			
			action.setEnabled(objects.size() > 0);
		}
		else
		{
			action.setEnabled(false);
		}
	}
}
