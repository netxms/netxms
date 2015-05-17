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
package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objecttools.api.NodeInfo;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolExecutor;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolsCache;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.CommandBox;

/**
 * "Commands" element
 */
public class Commands extends OverviewPageElement
{
	private CommandBox commandBox;
	private Action actionWakeup;
	
	/**
	 * @param parent
	 * @param anchor
	 * @param objectTab
	 */
	public Commands(Composite parent, OverviewPageElement anchor, ObjectTab objectTab)
	{
		super(parent, anchor, objectTab);
		createActions();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		
		actionWakeup = new Action(Messages.get().Commands_ActionWakeup) {
			@Override
			public void run()
			{
				final AbstractObject object = getObject();
				new ConsoleJob(Messages.get().Commands_WakeupJobName, null, Activator.PLUGIN_ID, null) {
					@Override
					protected void runInternal(IProgressMonitor monitor) throws Exception
					{
						session.wakeupNode(object.getObjectId());
					}

					@Override
					protected String getErrorMessage()
					{
						return Messages.get().Commands_WakeupJobError;
					}
				}.start();
			}
		};
		actionWakeup.setImageDescriptor(Activator.getImageDescriptor("icons/wol.png")); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#getTitle()
	 */
	@Override
	protected String getTitle()
	{
		return Messages.get().Commands_Title;
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	protected void onObjectChange()
	{
		commandBox.deleteAll(false);
		if (getObject() instanceof AbstractNode)
		{
		   ObjectTool[] tools = ObjectToolsCache.getInstance().getTools();
		   for(final ObjectTool tool : tools)
		   {
		      if (((tool.getFlags() & ObjectTool.SHOW_IN_COMMANDS) == 0) || !tool.isApplicableForNode((AbstractNode)getObject()))
		         continue;

            final Set<NodeInfo> nodes = new HashSet<NodeInfo>(1);
            nodes.add(new NodeInfo((AbstractNode)getObject(), null));
            if (!ObjectToolExecutor.isToolAllowed(tool, nodes))
               continue;
		      
		      final Action action = new Action(tool.getCommandDisplayName()) {
               @Override
               public void run()
               {
                  ObjectToolExecutor.execute(nodes, tool);
               }
            };
            ImageDescriptor icon = ObjectToolsCache.getInstance().findIcon(tool.getId());
            if (icon != null)
               action.setImageDescriptor(icon);
	         commandBox.add(action, false);
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
