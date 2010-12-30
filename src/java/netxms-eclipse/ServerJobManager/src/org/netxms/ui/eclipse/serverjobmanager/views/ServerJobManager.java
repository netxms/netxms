/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.serverjobmanager.views;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCServerJob;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverjobmanager.Activator;
import org.netxms.ui.eclipse.serverjobmanager.views.helpers.ServerJobComparator;
import org.netxms.ui.eclipse.serverjobmanager.views.helpers.ServerJobLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Server job manager - provides view to manage server-side jobs
 */
public class ServerJobManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverjobmanager.view.server_job_manager";
	public static final String JOB_FAMILY = "ServerJobManagerJob";
		
	// Columns
	public static final int COLUMN_STATUS = 0;
	public static final int COLUMN_USER = 1;
	public static final int COLUMN_NODE = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	public static final int COLUMN_PROGRESS = 4;
	public static final int COLUMN_MESSAGE = 5;

	private static final String TABLE_CONFIG_PREFIX = "ServerJobManager";
	
	private SortableTableViewer viewer;
	private NXCSession session = null;
	private NXCListener clientListener = null;
	
	private RefreshAction actionRefresh;
	private Action actionRestartJob;
	private Action actionCancelJob;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Status", "Initiator", "Node", "Description", "Progress", "Message" };
		final int[] widths = { 80, 100, 150, 250, 100, 300 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ServerJobLabelProvider());
		viewer.setComparator(new ServerJobComparator());
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});

		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionCancelJob.setEnabled(selection.size()> 0);
				}
			}
		});
		
		// Create listener for notifications received from server via client library
		clientListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() != NXCNotification.JOB_CHANGE)
					return;
				refreshJobList();
			}
		};
		session.addListener(clientListener);

		refreshJobList();
	}

	/**
	 * Contribute actions to action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pulldown menu
	 * @param manager menu manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRestartJob);
		manager.add(actionCancelJob);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		//manager.add(actionRestartJob);
		manager.add(actionCancelJob);
		manager.add(actionRefresh);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}
	
	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionRestartJob);
		mgr.add(actionCancelJob);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				refreshJobList();
			}
		};
		
		actionCancelJob = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				cancelServerJob();
			}

		};
		actionCancelJob.setText("Cancel");
		actionCancelJob.setImageDescriptor(Activator.getImageDescriptor("icons/cancel.png"));
		actionCancelJob.setEnabled(false);
		
		actionRestartJob = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
			}
		};
		actionRestartJob.setText("Restart");
		actionRestartJob.setEnabled(false);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((session != null) && (clientListener != null))
			session.removeListener(clientListener);
		super.dispose();
	}
	
	/**
	 * Refresh job list
	 */
	private void refreshJobList()
	{
		new ConsoleJob("Refresh server job list", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final NXCServerJob[] jobList = session.getServerJobList();
				new UIJob("Update job list") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						// Remember current selection
						IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
						Object[] selectedObjects = selection.toArray();
						
						viewer.setInput(jobList);
						
						// Build new list of selected jobs - add object to selection if
						// object with same id was selected before
						List<NXCServerJob> selectedJobs = new ArrayList<NXCServerJob>(selectedObjects.length);
						for(int i = 0; i < selectedObjects.length; i++)
						{
							for(int j = 0; j < jobList.length; j++)
							{
								if (((NXCServerJob)selectedObjects[i]).getId() == jobList[j].getId())
								{
									selectedJobs.add(jobList[j]);
									break;
								}
							}
						}
						viewer.setSelection(new StructuredSelection(selectedJobs));
						return Status.OK_STATUS;
					}
				}.schedule();
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get job list from server";
			}
		}.start();
	}
	
	/**
	 * Cancel server job
	 */
	private void cancelServerJob()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		new ConsoleJob("Cancel server jobs", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@SuppressWarnings("rawtypes")
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				Iterator it = selection.iterator();
				while(it.hasNext())
				{
					Object object = it.next();
					if (object instanceof NXCServerJob)
					{
						final NXCServerJob jobObject = (NXCServerJob)object;
						session.cancelServerJob(jobObject.getId());
					}
					else
					{
						throw new NXCException(RCC.INTERNAL_ERROR);
					}
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot cancel server job";
			}

			@Override
			protected void jobFinalize()
			{
				refreshJobList();
			}
		}.start();
	}
}
