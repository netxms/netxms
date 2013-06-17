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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.CommandBox;

/**
 * "Commands" element
 *
 */
public class Commands extends OverviewPageElement
{
	private CommandBox commandBox;
	private Action actionRestartAgent;
	private Action actionRestart;
	private Action actionShutdown;
	private Action actionWakeup;
	
	/**
	 * @param parent
	 * @param object
	 */
	public Commands(Composite parent, AbstractObject object)
	{
		super(parent, object);
		createActions();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		actionWakeup = new Action("Wakeup node using Wake-on-LAN") {
			@Override
			public void run()
			{
				final AbstractObject object = getObject();
				new ConsoleJob("Wakeup node", null, Activator.PLUGIN_ID, null) {
					@Override
					protected void runInternal(IProgressMonitor monitor) throws Exception
					{
						session.wakeupNode(object.getObjectId());
					}

					@Override
					protected String getErrorMessage()
					{
						return "Cannot send wake-on-LAN packet to node";
					}
				}.start();
			}
		};
		actionWakeup.setImageDescriptor(Activator.getImageDescriptor("icons/wol.png"));
		
		actionRestartAgent = new Action("Restart NetXMS agent") {
			@Override
			public void run()
			{
				final AbstractObject object = getObject();
				if (MessageDialogHelper.openQuestion(getShell(), "Confirmation", "Node " + object.getObjectName() + " will be rebooted. Are you sure?"))
				{
					new ConsoleJob("Initiate agent restart on node " + object.getObjectName(), null, Activator.PLUGIN_ID, null) {
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							session.executeAction(object.getObjectId(), "Agent.Restart");
						}
	
						@Override
						protected String getErrorMessage()
						{
							return "Cannot initiate agent restart on node " + object.getObjectName();
						}
					}.start();
				}
			}
		};
		actionRestartAgent.setImageDescriptor(Activator.getImageDescriptor("icons/restart.png"));
		
		actionRestart = new Action("Restart system") {
			@Override
			public void run()
			{
				final AbstractObject object = getObject();
				if (MessageDialogHelper.openQuestion(getShell(), "Confirmation", "Node " + object.getObjectName() + " will be rebooted. Are you sure?"))
				{
					new ConsoleJob("Initiate node restart", null, Activator.PLUGIN_ID, null) {
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							session.executeAction(object.getObjectId(), "System.Restart");
						}
	
						@Override
						protected String getErrorMessage()
						{
							return "Cannot initiate node restart";
						}
					}.start();
				}
			}
		};
		actionRestart.setImageDescriptor(Activator.getImageDescriptor("icons/restart.png"));
		
		actionShutdown = new Action("Shutdown system") {
			@Override
			public void run()
			{
				final AbstractObject object = getObject();
				if (MessageDialogHelper.openQuestion(getShell(), "Confirmation", "Node " + object.getObjectName() + " will be shut down. Are you sure?"))
				{
					new ConsoleJob("Initiate node shutdown", null, Activator.PLUGIN_ID, null) {
						@Override
						protected void runInternal(IProgressMonitor monitor) throws Exception
						{
							session.executeAction(object.getObjectId(), "System.Shutdown");
						}
	
						@Override
						protected String getErrorMessage()
						{
							return "Cannot initiate node shutdown";
						}
					}.start();
				}
			}
		};
		actionShutdown.setImageDescriptor(Activator.getImageDescriptor("icons/shutdown.png"));
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return "Commands";
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	void onObjectChange()
	{
		commandBox.deleteAll(false);
		if (getObject() instanceof Node)
		{
			commandBox.add(actionWakeup, false);
			if (((Node)getObject()).hasAgent())
			{
				commandBox.add(actionRestart, false);
				commandBox.add(actionShutdown, false);
			}
		}
		else if (getObject() instanceof Interface)
		{
			commandBox.add(actionWakeup, false);
		}
		commandBox.rebuild();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		commandBox = new CommandBox(parent, SWT.NONE);
		return commandBox;
	} 

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean isApplicableForObject(AbstractObject object)
	{
		return (object instanceof Node) || (object instanceof Interface);
	}
}
