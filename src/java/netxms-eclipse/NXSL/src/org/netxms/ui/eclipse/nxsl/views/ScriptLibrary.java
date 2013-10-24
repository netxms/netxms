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
package org.netxms.ui.eclipse.nxsl.views;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
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
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.scripts.Script;
import org.netxms.api.client.scripts.ScriptLibraryManager;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.nxsl.Activator;
import org.netxms.ui.eclipse.nxsl.Messages;
import org.netxms.ui.eclipse.nxsl.dialogs.CreateScriptDialog;
import org.netxms.ui.eclipse.nxsl.views.helpers.ScriptComparator;
import org.netxms.ui.eclipse.nxsl.views.helpers.ScriptLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Script library view
 */
public class ScriptLibrary extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.nxsl.views.ScriptLibrary"; //$NON-NLS-1$
	
	// Columns
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;

	private static final String TABLE_CONFIG_PREFIX = "ScriptLibrary"; //$NON-NLS-1$
	
	private ScriptLibraryManager scriptLibraryManager;
	private SortableTableViewer viewer;
	private RefreshAction actionRefresh;
	private Action actionNew;
	private Action actionEdit;
	private Action actionRename;
	private Action actionDelete;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		scriptLibraryManager = (ScriptLibraryManager)ConsoleSharedData.getSession();
		
		parent.setLayout(new FillLayout());
		
		final String[] names = { Messages.ScriptLibrary_ColumnId, Messages.ScriptLibrary_ColumnName };
		final int[] widths = { 70, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ScriptLabelProvider());
		viewer.setComparator(new ScriptComparator());
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				if (selection != null)
				{
					actionEdit.setEnabled(selection.size() == 1);
					actionRename.setEnabled(selection.size() == 1);
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

		refreshScriptList();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
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
				refreshScriptList();
			}
		};

		actionNew = new Action(Messages.ScriptLibrary_New, Activator.getImageDescriptor("icons/new.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				createNewScript();
			}
		};

		actionEdit = new Action(Messages.ScriptLibrary_Edit, Activator.getImageDescriptor("icons/edit.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				editScript();
			}
		};
		actionEdit.setEnabled(false);

		actionRename = new Action(Messages.ScriptLibrary_Rename) {
			@Override
			public void run()
			{
				renameScript();
			}
		};
		actionRename.setEnabled(false);

		actionDelete = new Action(Messages.ScriptLibrary_Delete, Activator.getImageDescriptor("icons/delete.png")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				deleteScript();
			}
		};
		actionDelete.setEnabled(false);
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
		manager.add(actionRename);
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
		manager.add(actionEdit);
		manager.add(actionDelete);
		manager.add(new Separator());
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
		Menu menu = menuMgr.createContextMenu(viewer.getTable());
		viewer.getTable().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionNew);
		mgr.add(actionEdit);
		mgr.add(actionRename);
		mgr.add(actionDelete);
	}

	/**
	 * Reload script list from server
	 */
	private void refreshScriptList()
	{
		new ConsoleJob(Messages.ScriptLibrary_LoadJobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.ScriptLibrary_LoadJobError;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<Script> library = scriptLibraryManager.getScriptLibrary();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(library.toArray());
					}
				});
			}
		}.start();
	}
	
	/**
	 * Create new script
	 */
	private void createNewScript()
	{
		final CreateScriptDialog dlg = new CreateScriptDialog(getSite().getShell(), null);
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.ScriptLibrary_CreateJobTitle, this, Activator.PLUGIN_ID, null) {
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					final long id = scriptLibraryManager.modifyScript(0, dlg.getName(), ""); //$NON-NLS-1$
					runInUIThread(new Runnable()
					{
						@Override
						public void run()
						{
							Object[] input = (Object[])viewer.getInput();
							List<Script> list = new ArrayList<Script>(input.length);
							for(Object o : input)
								list.add((Script)o);
							final Script script = new Script(id, dlg.getName(), ""); //$NON-NLS-1$
							list.add(script);
							viewer.setInput(list.toArray());
							viewer.setSelection(new StructuredSelection(script));
							actionEdit.run();
						}
					});
				}

				@Override
				protected String getErrorMessage()
				{
					return Messages.ScriptLibrary_CreateJobError;
				}
			}.start();
		}
	}
	
	/**
	 * Edit script
	 */
	private void editScript()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Script script = (Script)selection.getFirstElement();
		try
		{
			getSite().getPage().showView(ScriptEditorView.ID, Long.toString(script.getId()), IWorkbenchPage.VIEW_ACTIVATE);
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(getSite().getWorkbenchWindow().getShell(), Messages.ScriptLibrary_Error, String.format(Messages.ScriptLibrary_EditScriptError, e.getMessage()));
		}
	}

	/**
	 * Edit script
	 */
	private void renameScript()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		final Script script = (Script)selection.getFirstElement();
		final CreateScriptDialog dlg = new CreateScriptDialog(getSite().getShell(), script.getName());
		if (dlg.open() == Window.OK)
		{
			new ConsoleJob(Messages.ScriptLibrary_RenameJobTitle, this, Activator.PLUGIN_ID, null)
			{
				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					scriptLibraryManager.renameScript(script.getId(), dlg.getName());
					runInUIThread(new Runnable()
					{
						@Override
						public void run()
						{
							Object[] input = (Object[])viewer.getInput();
							List<Script> list = new ArrayList<Script>(input.length);
							for(Object o : input)
							{
								if (((Script)o).getId() != script.getId())
									list.add((Script)o);
							}
							final Script newScript = new Script(script.getId(), dlg.getName(), script.getSource());
							list.add(newScript);
							viewer.setInput(list.toArray());
							viewer.setSelection(new StructuredSelection(newScript));
						}
					});
				}
				
				@Override
				protected String getErrorMessage()
				{
					return Messages.ScriptLibrary_RenameJobError;
				}
			}.start();
		}
	}

	/**
	 * Delete selected script(s)
	 */
	@SuppressWarnings("rawtypes")
	private void deleteScript()
	{
		if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.ScriptLibrary_Confirmation, Messages.ScriptLibrary_ConfirmationText))
			return;
		
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		
		new ConsoleJob(Messages.ScriptLibrary_DeleteJobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				Iterator it = selection.iterator();
				while(it.hasNext())
				{
					Script script = (Script)it.next();
					scriptLibraryManager.deleteScript(script.getId());
				}
			}

			/* (non-Javadoc)
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
			@Override
			protected void jobFinalize()
			{
				refreshScriptList();
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.ScriptLibrary_DeleteJobError;
			}
		}.start();
	}
}
