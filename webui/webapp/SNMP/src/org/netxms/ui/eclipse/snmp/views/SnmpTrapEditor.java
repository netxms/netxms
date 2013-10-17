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
package org.netxms.ui.eclipse.snmp.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.TrapConfigurationDialog;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpTrapComparator;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpTrapLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * SNMP trap configuration editor
 *
 */
public class SnmpTrapEditor extends ViewPart implements SessionListener
{
	public static final String ID = "org.netxms.ui.eclipse.snmp.views.SnmpTrapEditor"; //$NON-NLS-1$
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_TRAP_OID = 1;
	public static final int COLUMN_EVENT = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	
	private static final String TABLE_CONFIG_PREFIX = "SnmpTrapEditor"; //$NON-NLS-1$
	
	private SortableTableViewer viewer;
	private NXCSession session;
	private RefreshAction actionRefresh;
	private Action actionNew;
	private Action actionEdit;
	private Action actionDelete;
	private Map<Long, SnmpTrap> traps = new HashMap<Long, SnmpTrap>();

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		parent.setLayout(new FillLayout());
		
		final String[] columnNames = { Messages.SnmpTrapEditor_ColID, Messages.SnmpTrapEditor_ColOID, Messages.SnmpTrapEditor_ColEvent, Messages.SnmpTrapEditor_ColDescription };
		final int[] columnWidths = { 70, 200, 100, 200 };
		viewer = new SortableTableViewer(parent, columnNames, columnWidths, COLUMN_ID, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SnmpTrapLabelProvider());
		viewer.setComparator(new SnmpTrapComparator());
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
		refreshTrapList();
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
				refreshTrapList();
			}
		};
		
		actionNew = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				createTrap();
			}
		};
		actionNew.setText(Messages.SnmpTrapEditor_NewMapping);
		actionNew.setImageDescriptor(Activator.getImageDescriptor("icons/new.png")); //$NON-NLS-1$
		
		actionEdit = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				editTrap();
			}
		};
		actionEdit.setText(Messages.SnmpTrapEditor_Properties);
		actionEdit.setImageDescriptor(Activator.getImageDescriptor("icons/edit.png")); //$NON-NLS-1$
		
		actionDelete = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				deleteTraps();
			}
		};
		actionDelete.setText(Messages.SnmpTrapEditor_Delete);
		actionDelete.setImageDescriptor(Activator.getImageDescriptor("icons/delete.png")); //$NON-NLS-1$
	}
	
	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		switch(n.getCode())
		{
			case NXCNotification.TRAP_CONFIGURATION_CREATED:
			case NXCNotification.TRAP_CONFIGURATION_MODIFIED:
				synchronized(traps)
				{
					traps.put(n.getSubCode(), (SnmpTrap)n.getObject());
				}
				updateTrapList();
				break;
			case NXCNotification.TRAP_CONFIGURATION_DELETED:
				synchronized(traps)
				{
					traps.remove(n.getSubCode());
				}
				updateTrapList();
				break;
		}
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
	 * Refresh trap list
	 */
	private void refreshTrapList()
	{
		new ConsoleJob(Messages.SnmpTrapEditor_LoadJob_Title, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.SnmpTrapEditor_LoadJob_Error;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				List<SnmpTrap> list = session.getSnmpTrapsConfiguration();
				synchronized(traps)
				{
					traps.clear();
					for(SnmpTrap t : list)
					{
						traps.put(t.getId(), t);
					}
				}
				updateTrapList();
			}	
		}.start();
	}

	/**
	 * Update trap list
	 */
	private void updateTrapList()
	{
		viewer.getControl().getDisplay().asyncExec(new Runnable() {
			@Override
			public void run()
			{
				synchronized(traps)
				{
					viewer.setInput(traps.values().toArray());
				}
			}
		});
	}

	/**
	 * Delete selected traps
	 */
	protected void deleteTraps()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		final Object[] objects = selection.toArray();
		new ConsoleJob(Messages.SnmpTrapEditor_DeleteJob_Title, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.SnmpTrapEditor_DeleteJob_Error;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int i = 0; i < objects.length; i++)
				{
					session.deleteSnmpTrapConfiguration(((SnmpTrap)objects[i]).getId());
				}
			}
		}.start();
	}

	/**
	 * Edit selected trap
	 */
	protected void editTrap()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final SnmpTrap trap = (SnmpTrap)selection.getFirstElement();
		TrapConfigurationDialog dlg = new TrapConfigurationDialog(getViewSite().getShell(), trap);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.SnmpTrapEditor_ModifyJob_Title, this, Activator.PLUGIN_ID, null) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.SnmpTrapEditor_ModifyJob_Error;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.modifySnmpTrapConfiguration(trap);
				}
			}.start();
		}
	}

	/**
	 * Create new trap
	 */
	protected void createTrap()
	{
		final SnmpTrap trap = new SnmpTrap();
		trap.setEventCode(500);		// SYS_UNMATCHED_SNMP_TRAP
		TrapConfigurationDialog dlg = new TrapConfigurationDialog(getViewSite().getShell(), trap);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.SnmpTrapEditor_CreateJob_Title, this, Activator.PLUGIN_ID, null) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.SnmpTrapEditor_CreateJob_Error;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					trap.setId(session.createSnmpTrapConfiguration());
					session.modifySnmpTrapConfiguration(trap);
				}
			}.start();
		}
	}
}
