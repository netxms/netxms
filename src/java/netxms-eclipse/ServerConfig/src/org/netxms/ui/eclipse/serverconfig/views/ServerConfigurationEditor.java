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
package org.netxms.ui.eclipse.serverconfig.views;

import java.util.ArrayList;
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
import org.eclipse.jface.dialogs.MessageDialog;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.servermanager.ServerManager;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.VariableEditDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ServerVariableComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ServerVariablesLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for server configuration variables
 */
public class ServerConfigurationEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.view.server_config";
	public static final String JOB_FAMILY = "ServerConfigJob";
		
	private SortableTableViewer viewer;
	private ServerManager session;
	private Map<String, ServerVariable> varList;
	
	private Action actionAdd;
	private Action actionEdit;
	private Action actionDelete;
	private RefreshAction actionRefresh;
	
	// Columns
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_NEED_RESTART = 2;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Name", "Value", "Restart" };
		final int[] widths = { 200, 150, 80 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ServerVariablesLabelProvider());
		viewer.setComparator(new ServerVariableComparator());
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editVariable();
			}
		});
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				actionEdit.setEnabled(selection.size() == 1);
				actionDelete.setEnabled(selection.size() > 0);
			}
		});
		
		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		WidgetHelper.restoreTableViewerSettings(viewer, settings, "ServerConfigurationEditor");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, settings, "ServerConfigurationEditor");
			}
		});

		// Create the help context id for the viewer's control
		PlatformUI.getWorkbench().getHelpSystem().setHelp(viewer.getControl(), "org.netxms.nxmc.serverconfig.viewer");
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = (ServerManager)ConsoleSharedData.getSession();
		refresh();
	}
	
	/**
	 * Refresh viewer
	 */
	public void refresh()
	{
		new ConsoleJob("Load server configuration variables", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot load server configuration variables";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				varList = session.getServerVariables();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						synchronized(varList)
						{
							viewer.setInput(varList.values().toArray());
						}
					}
				});
			}
		}.start();
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
		manager.add(actionAdd);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionAdd);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionAdd = new Action("&Create new...", SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				addVariable();
			}
		};
		
		actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editVariable();
			}
		};
		actionEdit.setEnabled(false);
		
		actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteVariables();
			}
		};
		actionDelete.setEnabled(false);
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
		mgr.add(actionAdd);
		mgr.add(actionEdit);
		mgr.add(actionDelete);
	}

	/**
	 * Passing the focus request to the viewer's control.
	 */
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}
	
	/**
	 * Add new variable
	 */
	private void addVariable()
	{
		final VariableEditDialog dlg = new VariableEditDialog(getSite().getShell(), null, null);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob("Create configuration variable", this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot create configuration variable";
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.setServerVariable(dlg.getVarName(), dlg.getVarValue());
					refresh();
				}
			}.start();
		}
	}
	
	/**
	 * Edit currently selected variable
	 * @param var
	 */
	private void editVariable()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if ((selection == null) || (selection.size() != 1))
			return;
		
		final ServerVariable var = (ServerVariable)selection.getFirstElement();
		final VariableEditDialog dlg = new VariableEditDialog(getSite().getShell(), var.getName(), var.getValue());
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob("Modify configuration variable", this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot modify configuration variable";
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.setServerVariable(dlg.getVarName(), dlg.getVarValue());
					refresh();
				}
			}.start();
		}
	}
	
	/**
	 * Delete selected variables
	 */
	private void deleteVariables()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if ((selection == null) || (selection.size() == 0))
			return;
		
		if (!MessageDialog.openQuestion(getSite().getShell(), "Delete Confirmation", "Are you sure you want to delete selected configuration variables?"))
			return;
		
		final List<String> names = new ArrayList<String>(selection.size());
		for(Object o : selection.toList())
		{
			if (o instanceof ServerVariable)
				names.add(((ServerVariable)o).getName());
		}
		new ConsoleJob("Delete configuration variables", this, Activator.PLUGIN_ID, ServerConfigurationEditor.JOB_FAMILY) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(String n : names)
				{
					session.deleteServerVariable(n);
				}
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						refresh();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot delete configuration variable";
			}
		}.start();
	}
}
