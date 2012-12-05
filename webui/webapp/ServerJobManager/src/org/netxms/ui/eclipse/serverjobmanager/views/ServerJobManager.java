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
package org.netxms.ui.eclipse.serverjobmanager.views;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerJob;
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
	public static final String ID = "org.netxms.ui.eclipse.serverjobmanager.views.ServerJobManager";
	public static final String JOB_FAMILY = "ServerJobManagerJob";
		
	// Columns
	public static final int COLUMN_STATUS = 0;
	public static final int COLUMN_USER = 1;
	public static final int COLUMN_NODE = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	public static final int COLUMN_PROGRESS = 4;
	public static final int COLUMN_MESSAGE = 5;

	private static final String TABLE_CONFIG_PREFIX = "ServerJobManager";
	
	private static final int CANCEL_JOB = 0;
	private static final int HOLD_JOB = 1;
	private static final int UNHOLD_JOB = 2;
	
	private SortableTableViewer viewer;
	private NXCSession session = null;
	private NXCListener clientListener = null;
	
	private RefreshAction actionRefresh;
	private Action actionRestartJob;
	private Action actionCancelJob;
	private Action actionHoldJob;
	private Action actionUnholdJob;
	
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
			private static final long serialVersionUID = 1L;

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
					actionCancelJob.setEnabled(selection.size() > 0);
					actionHoldJob.setEnabled(selection.size() > 0);
					actionUnholdJob.setEnabled(selection.size() > 0);
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
				refreshJobList(false);
			}
		};
		session.addListener(clientListener);

		refreshJobList(false);
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
		manager.add(actionHoldJob);
		manager.add(actionUnholdJob);
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
		menuMgr.addMenuListener(new IMenuListener() {
			private static final long serialVersionUID = 1L;

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
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionRestartJob);
		manager.add(actionCancelJob);
		manager.add(new Separator());
		manager.add(actionHoldJob);
		manager.add(actionUnholdJob);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				refreshJobList(true);
			}
		};
		
		actionCancelJob = new Action("&Cancel") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				cancelServerJob();
			}
		};
		actionCancelJob.setImageDescriptor(Activator.getImageDescriptor("icons/cancel.png"));
		actionCancelJob.setEnabled(false);
		
		actionHoldJob = new Action("&Hold") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				holdServerJob();
			}
		};
		actionHoldJob.setImageDescriptor(Activator.getImageDescriptor("icons/hold.gif"));
		actionHoldJob.setEnabled(false);
		
		actionUnholdJob = new Action("&Unhold") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				unholdServerJob();
			}
		};
		actionUnholdJob.setImageDescriptor(Activator.getImageDescriptor("icons/unhold.gif"));
		actionUnholdJob.setEnabled(false);
		
		actionRestartJob = new Action("&Restart") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
			}
		};
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
	private void refreshJobList(boolean userInitiated)
	{
		ConsoleJob job = new ConsoleJob("Refresh server job list", this, Activator.PLUGIN_ID, JOB_FAMILY, getSite().getShell().getDisplay()) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final ServerJob[] jobList = session.getServerJobList();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						// Remember current selection
						IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
						Object[] selectedObjects = selection.toArray();
						
						viewer.setInput(jobList);
						
						// Build new list of selected jobs - add object to selection if
						// object with same id was selected before
						List<ServerJob> selectedJobs = new ArrayList<ServerJob>(selectedObjects.length);
						for(int i = 0; i < selectedObjects.length; i++)
						{
							for(int j = 0; j < jobList.length; j++)
							{
								if (((ServerJob)selectedObjects[i]).getId() == jobList[j].getId())
								{
									selectedJobs.add(jobList[j]);
									break;
								}
							}
						}
						viewer.setSelection(new StructuredSelection(selectedJobs));
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot get job list from server";
			}
		};
		job.setUser(userInitiated);
		job.start();
	}
	
	/**
	 * Cancel server job
	 */
	private void cancelServerJob()
	{
		doJobAction("Cancel", CANCEL_JOB);
	}
	
	/**
	 * Hold server job
	 */
	private void holdServerJob()
	{
		doJobAction("Hold", HOLD_JOB);
	}
	
	/**
	 * Unhold server job
	 */
	private void unholdServerJob()
	{
		doJobAction("Unhold", UNHOLD_JOB);
	}
	
	/**
	 * Do job action: cancel, hold, unhold
	 * 
	 * @param actionName
	 * @param actionId
	 */
	private void doJobAction(final String actionName, final int actionId)
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		new ConsoleJob(actionName + " server jobs", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@SuppressWarnings("rawtypes")
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				Iterator it = selection.iterator();
				while(it.hasNext())
				{
					Object object = it.next();
					if (object instanceof ServerJob)
					{
						final ServerJob jobObject = (ServerJob)object;
						switch(actionId)
						{
							case CANCEL_JOB: 
								session.cancelServerJob(jobObject.getId());
								break;
							case HOLD_JOB: 
								session.holdServerJob(jobObject.getId());
								break;
							case UNHOLD_JOB: 
								session.unholdServerJob(jobObject.getId());
								break;
							default:
								throw new NXCException(RCC.INTERNAL_ERROR);
						}
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
				return "Cannot " + actionName.toLowerCase() + " server job";
			}

			@Override
			protected void jobFinalize()
			{
				refreshJobList(false);
			}
		}.start();
	}
}
