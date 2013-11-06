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
package org.netxms.ui.eclipse.objectmanager.actions;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.dialogs.SetInterfaceExpStateDlg;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * "Create DCI for interface" action
 */
public class ChangeInterfaceExpectedState implements IObjectActionDelegate
{
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
		if (objects.size() == 0)
			return;
		
		SetInterfaceExpStateDlg dlg = new SetInterfaceExpStateDlg(shell);
		if (dlg.open() != Window.OK)
			return;
		
		final long[] idList = new long[objects.size()];
		for(int i = 0; i < idList.length; i++)
			idList[i] = objects.get(i).getObjectId();
		final int newState = dlg.getExpectedState();
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Update expected state for interfaces", viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < idList.length; i++)
				{
					NXCObjectModificationData md = new NXCObjectModificationData(idList[i]);
					md.setExpectedState(newState);
					session.modifyObject(md);
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot update expected state for interface object";
			}
		}.start();
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
