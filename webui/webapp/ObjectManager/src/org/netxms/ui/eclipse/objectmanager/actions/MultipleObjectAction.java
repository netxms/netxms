/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.Iterator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Abstract base class for actions operating on multiple objects
 */
public abstract class MultipleObjectAction implements IObjectActionDelegate
{
	private IStructuredSelection selection;
	private IWorkbenchWindow window;
	private IWorkbenchPart part;
	
	/**
	 * Returns job description (like "Deleting object ").
	 * 
	 * @return Job description
	 */
	abstract protected String formatJobDescription(AbstractObject object);
	
	/**
	 * Returns error message prefix (like "Cannot delete object ")
	 * 
	 * @return Error message prefix
	 */
	abstract protected String formatErrorMessage(AbstractObject object, Display display);
	
	/**
	 * Run action on given object.
	 * 
	 * @throws Exception
	 */
	abstract protected void runObjectAction(final NXCSession session, final AbstractObject object) throws Exception;
	
	/**
	 * Hook for possible confirmation. Default implementation always returns true.
	 * 
	 * @return true if action confirmed by user
	 */
	protected boolean confirm()
	{
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		window = targetPart.getSite().getWorkbenchWindow();
		part = targetPart;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public final void run(IAction action)
	{
		if (selection == null)
			return;
		
		if (!confirm())
			return;
		
		Iterator<AbstractObject> it = selection.iterator();
		while(it.hasNext())
		{
			final AbstractObject object = it.next();
			final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
			new ConsoleJob(formatJobDescription(object), part, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					runObjectAction(session, object);
				}
				
				@Override
				protected String getErrorMessage()
				{
					return formatErrorMessage(object, getDisplay());
				}
			}.start();
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public final void selectionChanged(IAction action, ISelection selection)
	{
		if (selection instanceof IStructuredSelection)
		{
			this.selection = (IStructuredSelection)selection;
			action.setEnabled(this.selection.size() > 0);
		}
		else
		{
			action.setEnabled(false);
		}
	}

	/**
	 * @return the selection
	 */
	protected IStructuredSelection getSelection()
	{
		return selection;
	}

	/**
	 * @return the window
	 */
	protected IWorkbenchWindow getWindow()
	{
		return window;
	}
}
