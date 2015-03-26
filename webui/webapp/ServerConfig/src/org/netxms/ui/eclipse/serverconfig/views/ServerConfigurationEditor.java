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
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.IDialogSettings;
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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.servermanager.ServerManager;
import org.netxms.api.client.servermanager.ServerVariable;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.dialogs.VariableEditDialog;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ServerVariableComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ServerVariablesFilter;
import org.netxms.ui.eclipse.serverconfig.views.helpers.ServerVariablesLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.FilterText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Editor for server configuration variables
 */
public class ServerConfigurationEditor extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.view.server_config"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "ServerConfigJob"; //$NON-NLS-1$
		
	private SortableTableViewer viewer;
	private ServerManager session;
	private Map<String, ServerVariable> varList;
	private boolean filterEnabled = false;
   private Composite content;
   private FilterText filterText;
   private ServerVariablesFilter filter;
	
	private Action actionAdd;
	private Action actionEdit;
	private Action actionDelete;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	private Action actionRefresh;
	private Action actionShowFilter;
	
	// Columns
	public static final int COLUMN_NAME = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_NEED_RESTART = 2;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createPartControl(Composite parent)
	{
	   content = new Composite(parent, SWT.NONE);
      content.setLayout(new FormLayout());
	   
	   // Create filter area
      filterText = new FilterText(content, SWT.NONE);
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });
	   
		final String[] names = { Messages.get().ServerConfigurationEditor_ColName, Messages.get().ServerConfigurationEditor_ColValue, Messages.get().ServerConfigurationEditor_ColRestart };
		final int[] widths = { 200, 150, 80 };
		viewer = new SortableTableViewer(content, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ServerVariablesLabelProvider());
		viewer.setComparator(new ServerVariableComparator());
		filter = new ServerVariablesFilter();
      viewer.addFilter(filter);
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
		
		// Setup layout
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterText);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      viewer.getTable().setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterText.setLayoutData(fd);
      
      filterText.setCloseAction(actionShowFilter);
		
		final IDialogSettings settings = Activator.getDefault().getDialogSettings();
		WidgetHelper.restoreTableViewerSettings(viewer, settings, "ServerConfigurationEditor"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, settings, "ServerConfigurationEditor"); //$NON-NLS-1$
			}
		});

		// Create the help context id for the viewer's control
		PlatformUI.getWorkbench().getHelpSystem().setHelp(viewer.getControl(), "org.netxms.nxmc.serverconfig.viewer"); //$NON-NLS-1$
		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		// Set initial focus to filter input line
      if (filterEnabled)
         filterText.setFocus();
      else
         enableFilter(false); // Will hide filter area correctly
		
		session = (ServerManager)ConsoleSharedData.getSession();
		refresh();
	}
	
	/**
	 * Refresh viewer
	 */
	public void refresh()
	{
		new ConsoleJob(Messages.get().ServerConfigurationEditor_LoadJobName, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().ServerConfigurationEditor_LoadJobError;
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
		manager.add(actionExportAllToCsv);
      manager.add(actionShowFilter);		
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionAdd);
		manager.add(actionExportAllToCsv);
      manager.add(actionShowFilter);      
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
	   final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
	   
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionAdd = new Action(Messages.get().ServerConfigurationEditor_ActionCreate, SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				addVariable();
			}
		};
		
		actionEdit = new Action(Messages.get().ServerConfigurationEditor_ActionEdit, SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editVariable();
			}
		};
		actionEdit.setEnabled(false);
		
      actionDelete = new Action(Messages.get().ServerConfigurationEditor_ActionDelete, SharedIcons.DELETE_OBJECT) {
      @Override
         public void run()
         {
            deleteVariables();
         }
      };
      actionDelete.setEnabled(false);
		
		actionShowFilter = new Action("Show filter", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
            enableFilter(!filterEnabled);
            actionShowFilter.setChecked(filterEnabled);
			}
		};
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
      actionShowFilter.setChecked(filterEnabled);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.datacollection.commands.show_dci_filter"); //$NON-NLS-1$
      final ActionHandler showFilterHandler = new ActionHandler(actionShowFilter);
      handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), showFilterHandler);
		
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}
	
	  /**
    * Enable or disable filter
    * 
    * @param enable New filter state
    */
   private void enableFilter(boolean enable)
   {
      filterEnabled = enable;
      filterText.setVisible(filterEnabled);
      FormData fd = (FormData)viewer.getTable().getLayoutData();
      fd.top = enable ? new FormAttachment(filterText) : new FormAttachment(0, 0);
      content.layout();
      if (enable)
      {
         filterText.setFocus();
      }
      else
      {
         filterText.setText(""); //$NON-NLS-1$
         onFilterModify();
      }
   }
   
   /**
    * Handler for filter modification
    */
   private void onFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
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
		mgr.add(new Separator());
		mgr.add(actionExportToCsv);
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
			new ConsoleJob(Messages.get().ServerConfigurationEditor_CreateJobName, this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().ServerConfigurationEditor_CreateJobError;
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
			new ConsoleJob(Messages.get().ServerConfigurationEditor_ModifyJobName, this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return Messages.get().ServerConfigurationEditor_ModifyJobError;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					session.setServerVariable(dlg.getVarName(), dlg.getVarValue());
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							refresh();
						}
					});
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
		
		if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.get().ServerConfigurationEditor_DeleteConfirmation, Messages.get().ServerConfigurationEditor_DeleteConfirmationText))
			return;
		
		final List<String> names = new ArrayList<String>(selection.size());
		for(Object o : selection.toList())
		{
			if (o instanceof ServerVariable)
				names.add(((ServerVariable)o).getName());
		}
		new ConsoleJob(Messages.get().ServerConfigurationEditor_DeleteJobName, this, Activator.PLUGIN_ID, ServerConfigurationEditor.JOB_FAMILY) {
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
				return Messages.get().ServerConfigurationEditor_DeleteJobError;
			}
		}.start();
	}
}
