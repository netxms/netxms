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

import java.util.Map;

import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.dialogs.VariableEditDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ServerVariableComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ViewLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for server configuration variables
 */
public class ServerConfigurationEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.view.server_config";
	public static final String JOB_FAMILY = "ServerConfigJob";
		
	private TableViewer viewer;
	private NXCSession session;
	private Map<String, ServerVariable> varList;
	
	private Action actionAddVariable;
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
		viewer.setLabelProvider(new ViewLabelProvider());
		viewer.setComparator(new ServerVariableComparator());
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				if ((selection != null) && (selection.size() == 1))
				{
					ServerVariable var = (ServerVariable)selection.getFirstElement();
					if (var != null)
					{
						editVariable(var);
					}
				}
			}
		});

		// Create the help context id for the viewer's control
		PlatformUI.getWorkbench().getHelpSystem().setHelp(viewer.getControl(), "org.netxms.nxmc.serverconfig.viewer");
		makeActions();
		contributeToActionBars();
		createPopupMenu();
		
		session = (NXCSession)ConsoleSharedData.getSession();
		refreshViewer();
	}
	
	/**
	 * Refresh viewer
	 */
	public void refreshViewer()
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

	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
		manager.add(actionAddVariable);
	}

	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
		manager.add(actionAddVariable);
	}

	private void makeActions()
	{
		actionRefresh = new RefreshAction() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				refreshViewer();
			}
		};
		
		actionAddVariable = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				ServerConfigurationEditor.this.addVariable();
			}
		};
		actionAddVariable.setText("Add variable");
		actionAddVariable.setImageDescriptor(Activator.getImageDescriptor("icons/add_variable.png"));
	}

	/**
	 * Create pop-up menu for variable list
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
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
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
					refreshViewer();
				}
			}.start();
		}
	}
	
	/**
	 * Edit currently selected variable
	 * @param var
	 */
	public void editVariable(ServerVariable var)
	{
		final VariableEditDialog dlg = new VariableEditDialog(getSite().getShell(), var.getName(), var.getValue());
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob("Modify configuration variable", this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot create configuration variable";
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.setServerVariable(dlg.getVarName(), dlg.getVarValue());
					refreshViewer();
				}
			}.start();
		}
	}
}
