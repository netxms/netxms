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
import org.eclipse.ui.IActionDelegate;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IViewReference;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ASN;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.CreateSnmpDciDialog;
import org.netxms.ui.eclipse.datacollection.views.DataCollectionEditor;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.shared.MibCache;

/**
 * "Create DCI from SNMP walk result" action
 */
public class CreateSnmpDci implements IObjectActionDelegate
{
	private Shell shell;
	private ViewPart viewPart;
	private List<SnmpValue> objects = new ArrayList<SnmpValue>();

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
		if (objects.size() == 0)
			return;
		
		MibObject mibObject = MibCache.findObject(objects.get(0).getName(), false);
		String description = (mibObject != null) ? mibObject.getName() : objects.get(0).getName();
		if (objects.size() > 1)
			description += " @@instance@@"; //$NON-NLS-1$
		
		final CreateSnmpDciDialog dlg = new CreateSnmpDciDialog(shell, description);
		if (dlg.open() != Window.OK)
			return;
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		final List<SnmpValue> values = new ArrayList<SnmpValue>(objects);
		
		// Get set of nodes
		final Set<AbstractNode> nodes = new HashSet<AbstractNode>();
		for(SnmpValue v : values)
		{
			AbstractNode node = (AbstractNode)session.findObjectById(v.getNodeId(), AbstractNode.class);
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
		
		new ConsoleJob(Messages.get().CreateSnmpDci_JobTitle, viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().CreateSnmpDci_ErrorMessage;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				monitor.beginTask(Messages.get().CreateSnmpDci_TaskTitle, values.size());
				for(SnmpValue v : values)
				{
					final String description = dlg.getDescription().replaceAll("@@instance@@", Long.toString(v.getObjectId().getIdFromPos(v.getObjectId().getLength() - 1))); //$NON-NLS-1$
					createDci(session, v, description, dlg.getPollingInterval(), 
							dlg.getRetentionTime(), dlg.getDeltaCalculation(), lockRequired);
					monitor.worked(1);
				}
				monitor.done();
			}
		}.start();
	}

	/**
	 * @param session
	 * @param value
	 * @param description
	 * @param pollingInterval
	 * @param retentionTime
	 * @param deltaCalculation
	 * @param lockRequired
	 * @throws Exception
	 */
	private static void createDci(NXCSession session, SnmpValue value, String description, int pollingInterval,
			int retentionTime, int deltaCalculation, Map<Long, Boolean> lockRequired) throws Exception
	{
		AbstractNode node = (AbstractNode)session.findObjectById(value.getNodeId(), AbstractNode.class);
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
		dci.setOrigin(DataCollectionItem.SNMP);
		dci.setDataType(dciTypeFromAsnType(value.getType()));
		dci.setStatus(DataCollectionItem.ACTIVE);
		dci.setDescription(description);
		dci.setDeltaCalculation(deltaCalculation);
		dci.setName(value.getName());
		
		dcc.modifyObject(dci);
			
		if (lockRequired.get(node.getObjectId()))
		{
			dcc.close();
		}
	}
	
	/**
	 * Convert ASN.1 identifier type to DCI data type
	 * 
	 * @param type ASN.1 identifier type
	 * @return DCI data type
	 */
	private static int dciTypeFromAsnType(int type)
	{
		switch(type)
		{
			case ASN.COUNTER32:
			case ASN.GAUGE32:
			case ASN.UINTEGER32:
				return DataCollectionItem.DT_UINT;
			case ASN.COUNTER64:
				return DataCollectionItem.DT_UINT64;
			case ASN.INTEGER:
				return DataCollectionItem.DT_INT;
			default:
				return DataCollectionItem.DT_STRING;
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
				if (o instanceof SnmpValue)
					objects.add((SnmpValue)o);
			}
			
			action.setEnabled(objects.size() > 0);
		}
		else
		{
			action.setEnabled(false);
		}
	}
}
