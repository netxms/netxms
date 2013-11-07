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
package org.netxms.ui.eclipse.actionmanager.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.Activator;
import org.netxms.ui.eclipse.actionmanager.Messages;
import org.netxms.ui.eclipse.actionmanager.dialogs.EditActionDlg;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionComparator;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionLabelProvider;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Action configuration view
 *
 */
public class ActionManager extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.actionmanager.views.ActionManager"; //$NON-NLS-1$
	
	private static final String TABLE_CONFIG_PREFIX = "ActionList"; //$NON-NLS-1$
	
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_RCPT = 2;
	public static final int COLUMN_SUBJ = 3;
	public static final int COLUMN_DATA = 4;
	
	private SortableTableViewer viewer;
	private NXCSession session;
	private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
	private Action actionRefresh;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		parent.setLayout(new FillLayout());
		
		final String[] columnNames = { Messages.ActionManager_ColumnName, Messages.ActionManager_ColumnType, Messages.ActionManager_ColumnRcpt, Messages.ActionManager_ColumnSubj, Messages.ActionManager_ColumnData };
		final int[] columnWidths = { 150, 90, 100, 120, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ActionLabelProvider());
		viewer.setComparator(new ActionComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener()
		{
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
					actionDelete.setEnabled(selection.size() > 0);
				}
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionEdit.run();
			}
		});
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session.addListener(this);
		
		refreshActionList();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		session.removeListener(this);
		super.dispose();
	}
	
	/**
	 * Refresh action list
	 */
	private void refreshActionList()
	{
		new ConsoleJob(Messages.ActionManager_LoadJobName, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.ActionManager_LoadJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				List<ServerAction> actions = session.getActions();
				synchronized(ActionManager.this.actions)
				{
					ActionManager.this.actions.clear();
					for(ServerAction a : actions)
					{
						ActionManager.this.actions.put(a.getId(), a);
					}
				}
				
				updateActionsList();
			}
		}.start();
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNew);
		manager.add(actionDelete);
		manager.add(actionEdit);
		manager.add(new Separator());
		manager.add(actionRefresh);
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
				refreshActionList();
			}
		};
		
		actionNew = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				createAction();
			}
		};
		actionNew.setText(Messages.ActionManager_ActionNew);
		actionNew.setImageDescriptor(SharedIcons.ADD_OBJECT);
		
		actionEdit = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				editAction();
			}
		};
		actionEdit.setText(Messages.ActionManager_ActionProperties);
		actionEdit.setImageDescriptor(SharedIcons.EDIT);
		
		actionDelete = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				deleteActions();
			}
		};
		actionDelete.setText(Messages.ActionManager_ActionDelete);
		actionDelete.setImageDescriptor(SharedIcons.DELETE_OBJECT);
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionNew);
		mgr.add(actionDelete);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(actionEdit);
	}
	
	/**
	 * Create new action
	 */
	private void createAction()
	{
		final ServerAction action = new ServerAction(0);
		final EditActionDlg dlg = new EditActionDlg(getSite().getShell(), action, true);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.ActionManager_CreateJobName, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.ActionManager_CreateJobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					long id = session.createAction(action.getName());
					action.setId(id);
					session.modifyAction(action);
				}
			}.start();
		}
	}
	
	/**
	 * Edit currently selected action
	 */
	private void editAction()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final ServerAction action = (ServerAction)selection.getFirstElement();
		final EditActionDlg dlg = new EditActionDlg(getSite().getShell(), action, false);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.ActionManager_UpdateJobName, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.ActionManager_UodateJobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.modifyAction(action);
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected actions
	 */
	private void deleteActions()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.isEmpty())
			return;
		
		if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.ActionManager_Confirmation, Messages.ActionManager_ConfirmDelete))
			return;
		
		final Object[] objects = selection.toArray();
		new ConsoleJob(Messages.ActionManager_DeleteJobName, this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.ActionManager_DeleteJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
					session.deleteAction(((ServerAction)objects[i]).getId());
				}
			}
		}.start();
	}

	/**
	 * Update actions list 
	 */
	private void updateActionsList()
	{
		new UIJob(viewer.getControl().getDisplay(), Messages.ActionManager_UiUpdateJobName) {
			@Override
			public IStatus runInUIThread(IProgressMonitor monitor)
			{
				synchronized(ActionManager.this.actions)
				{
					viewer.setInput(ActionManager.this.actions.values().toArray());
				}
				return Status.OK_STATUS;
			}
		}.schedule();
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.ISessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		switch(n.getCode())
		{
			case NXCNotification.ACTION_CREATED:
			case NXCNotification.ACTION_MODIFIED:
				synchronized(actions)
				{
					actions.put(n.getSubCode(), (ServerAction)n.getObject());
				}
				updateActionsList();
				break;
			case NXCNotification.ACTION_DELETED:
				synchronized(actions)
				{
					actions.remove(n.getSubCode());
				}
				updateActionsList();
				break;
		}
	}
}
