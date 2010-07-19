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
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.ui.eclipse.actionmanager.Activator;
import org.netxms.ui.eclipse.actionmanager.dialogs.EditActionDlg;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionComparator;
import org.netxms.ui.eclipse.actionmanager.views.helpers.ActionLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;
import org.netxms.ui.eclipse.tools.SortableTableViewer;

/**
 * Action configuration view
 *
 */
public class ActionManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.actionmanager.views.ActionManager";
	
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_TYPE = 1;
	public static final int COLUMN_RCPT = 2;
	public static final int COLUMN_SUBJ = 3;
	public static final int COLUMN_DATA = 4;
	
	private SortableTableViewer viewer;
	private NXCSession session;
	private NXCListener sessionListener;
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
		session = NXMCSharedData.getInstance().getSession();
		
		parent.setLayout(new FillLayout());
		
		final String[] columnNames = { "Name", "Type", "Recipient", "Subject", "Data" };
		final int[] columnWidths = { 150, 90, 100, 120, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
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
		
		makeActions();
		contributeToActionBars();
		createPopupMenu();
		
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(NXCNotification n)
			{
				processSessionNotification(n);
			}
		};
		session.addListener(sessionListener);
		
		refreshActionList();
	}

	/**
	 * Process client session notifications
	 * @param n
	 */
	private void processSessionNotification(NXCNotification n)
	{
		switch(n.getCode())
		{
			case NXCNotification.ACTION_CREATED:
				synchronized(actions)
				{
					actions.put(n.getSubCode(), (ServerAction)n.getObject());
				}
				updateActionsList();
				break;
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
		if (sessionListener != null)
			session.removeListener(sessionListener);
		super.dispose();
	}
	
	/**
	 * Refresh action list
	 */
	private void refreshActionList()
	{
		new ConsoleJob("Load actions configuration", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot load actions configuration from server";
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
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void makeActions()
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
		actionNew.setText("&New action...");
		
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
		actionEdit.setText("&Properties...");
		
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
		actionDelete.setText("&Delete");
		actionDelete.setImageDescriptor(Activator.getImageDescriptor("icons/delete.png"));
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

		// Create menu.
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
		ServerAction action = new ServerAction(0);
		EditActionDlg dlg = new EditActionDlg(getSite().getShell(), action);
		if (dlg.open() == Window.OK)
		{
			
		}
	}
	
	/**
	 * Edit currently selected action
	 */
	private void editAction()
	{
		
	}
	
	/**
	 * Delete selected actions
	 */
	private void deleteActions()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		final Object[] objects = selection.toArray();
		new ConsoleJob("Delete actions", this, Activator.PLUGIN_ID, Activator.PLUGIN_ID) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot delete action";
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
		new UIJob("Update actions list") {
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
}
