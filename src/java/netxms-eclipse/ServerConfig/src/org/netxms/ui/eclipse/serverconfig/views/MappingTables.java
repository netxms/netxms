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
package org.netxms.ui.eclipse.serverconfig.views;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.Session;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.api.client.mt.MappingTableDescriptor;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.CreateMappingTableDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.MappingTableListComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.MappingTableListLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * List of mapping tables
 */
public class MappingTables extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.MappingTables"; //$NON-NLS-1$

	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_FLAGS = 2;
	public static final int COLUMN_DESCRIPTION = 3;
	
	private Session session;
	private SessionListener listener;
	private Map<Integer, MappingTableDescriptor> mappingTables = new HashMap<Integer, MappingTableDescriptor>();
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionNewTable;
	private Action actionEditTable;
	private Action actionDeleteTables;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final int[] widths = { 80, 160, 80, 400 };
		final String[] names = { Messages.get().MappingTables_ColID, Messages.get().MappingTables_ColName, Messages.get().MappingTables_ColFlags, Messages.get().MappingTables_ColDescription };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new MappingTableListLabelProvider());
		viewer.setComparator(new MappingTableListComparator());
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editTable();
			}
		});
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				actionEditTable.setEnabled(selection.size() == 1);
				actionDeleteTables.setEnabled(selection.size() > 0);
			}
		});
		
		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		WidgetHelper.restoreTableViewerSettings(viewer, settings, "MappingTablesList"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, settings, "MappingTablesList"); //$NON-NLS-1$
			}
		});
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = ConsoleSharedData.getSession();
		refresh(0);

		final Display display = getSite().getShell().getDisplay();
		listener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == SessionNotification.MAPPING_TABLE_UPDATED)
				{
					final Integer id = (int)n.getSubCode();
					display.asyncExec(new Runnable() {
						@Override
						public void run()
						{
							refresh(id);
						}
					});
				}
				else if (n.getCode() == SessionNotification.MAPPING_TABLE_DELETED)
				{
					final Integer id = (int)n.getSubCode();
					display.asyncExec(new Runnable() {
						@Override
						public void run()
						{
							mappingTables.remove(id);
							viewer.setInput(mappingTables.values().toArray());
						}
					});
				}
			}
		};
		session.addListener(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((listener != null) && (session != null))
			session.removeListener(listener);
		super.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refresh(0);
			}
		};
		
		actionNewTable = new Action(Messages.get().MappingTables_NewTable, SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createNewTable();
			}
		};
		
		actionEditTable = new Action(Messages.get().MappingTables_Edit, SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editTable();
			}
		};
		actionEditTable.setEnabled(false);
		
		actionDeleteTables = new Action(Messages.get().MappingTables_Delete, SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteTables();
			}
		};
		actionDeleteTables.setEnabled(false);
	}
	
	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * @param manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionNewTable);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNewTable);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Create pop-up menu for variable list
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
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
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(actionNewTable);
		mgr.add(actionEditTable);
		mgr.add(actionDeleteTables);
	}
	
	/**
	 * Refresh
	 */
	private void refresh(final int tableId)
	{
		new ConsoleJob(Messages.get().MappingTables_ReloadJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<MappingTableDescriptor> tables = session.listMappingTables();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (tableId > 0)
						{
							for(MappingTableDescriptor d : tables)
							{
								if (d.getId() == tableId)
								{
									MappingTableDescriptor od = mappingTables.get(tableId);
									if (!d.equals(od))
										mappingTables.put(d.getId(), d);
									break;
								}
							}
						}
						else
						{
							mappingTables.clear();
							for(MappingTableDescriptor d : tables)
								mappingTables.put(d.getId(), d);
						}
						viewer.setInput(mappingTables.values().toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().MappingTables_ReloadJobError;
			}
		}.start();
	}
	
	/**
	 * Create new table
	 */
	private void createNewTable()
	{
		final CreateMappingTableDialog dlg = new CreateMappingTableDialog(getSite().getShell());
		if (dlg.open() != Window.OK)
			return;
		
		new ConsoleJob(Messages.get().MappingTables_CreateJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Integer assignedId = session.createMappingTable(dlg.getName(), dlg.getDescription(), 0);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						// descriptor may already be presented if creation notification
						// was already processed
						MappingTableDescriptor d = mappingTables.get(assignedId);
						if (d == null)
						{
							d = new MappingTableDescriptor(assignedId, dlg.getName(), dlg.getDescription(), 0);
							mappingTables.put(assignedId, d);
						}
						viewer.setInput(mappingTables.values().toArray());
						viewer.setSelection(new StructuredSelection(d));
						editTable();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				// TODO Auto-generated method stub
				return null;
			}
		}.start();
	}
	
	/**
	 * Edit selected table
	 */
	private void editTable()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if ((selection == null) || (selection.size() != 1))
			return;
		
		MappingTableDescriptor d = (MappingTableDescriptor)selection.getFirstElement();
		try
		{
			getSite().getPage().showView(MappingTableEditor.ID, Integer.toString(d.getId()), IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(getSite().getShell(), Messages.get().MappingTables_Error, String.format(Messages.get().MappingTables_ErrorOpeningView, e.getLocalizedMessage()));
		}
	}
	
	/**
	 * Delete selected tables
	 */
	private void deleteTables()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if ((selection == null) || (selection.size() == 0))
			return;

		if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.get().MappingTables_DeleteConfirmation, Messages.get().MappingTables_DeleteConfirmationText))
			return;
		
		final List<Integer> tables = new ArrayList<Integer>(selection.size());
		for(Object o : selection.toList())
		{
			if (o instanceof MappingTableDescriptor)
				tables.add(((MappingTableDescriptor)o).getId());
		}
		new ConsoleJob(Messages.get().MappingTables_DeleteJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(Integer id : tables)
					session.deleteMappingTable(id);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(Integer id : tables)
							mappingTables.remove(id);
						viewer.setInput(mappingTables.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().MappingTables_DeleteJobError;
			}
		}.start();
	}
}
