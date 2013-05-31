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
package org.netxms.ui.eclipse.datacollection.views;

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
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.views.helpers.SummaryTableComparator;
import org.netxms.ui.eclipse.datacollection.views.helpers.SummaryTableLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Manager of DCI summary tables
 */
@SuppressWarnings("restriction")
public class SummaryTableManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.datacollection.views.SummaryTableManager"; //$NON-NLS-1$
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_MENU_PATH = 1;
	public static final int COLUMN_TITLE = 2;
	
	private SortableTableViewer viewer;
	private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
	private SessionListener listener = null;
	private Map<Integer, DciSummaryTableDescriptor> descriptors = new HashMap<Integer, DciSummaryTableDescriptor>();
	
	private Action actionRefresh;
	private Action actionCreate;
	private Action actionEdit;
	private Action actionDelete;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.SummaryTableManager_ID, Messages.SummaryTableManager_MenuPath, Messages.SummaryTableManager_Title };
		final int[] widths = { 90, 250, 200 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_MENU_PATH, SWT.UP, SortableTableViewer.DEFAULT_STYLE);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SummaryTableLabelProvider());
		viewer.setComparator(new SummaryTableComparator());
		
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				actionEdit.setEnabled(selection.size() == 1);
				actionDelete.setEnabled(selection.size() > 0);
			}
		});
		
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editSummaryTable();
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();

		refresh();
		
		listener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == NXCNotification.DCI_SUMMARY_TABLE_DELETED)
				{
					final int id = (int)n.getSubCode();
					getSite().getShell().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							if (descriptors.remove(id) != null)
								viewer.setInput(descriptors.values().toArray());
						}
					});
				}
				else if (n.getCode() == NXCNotification.DCI_SUMMARY_TABLE_UPDATED)
				{
					getSite().getShell().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							refresh();
						}
					});
				}
			}
		};
		session.addListener(listener);
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
	 * Refresh viewer
	 */
	private void refresh()
	{
		new ConsoleJob("Get configured DCI summary tables", this, Activator.PLUGIN_ID, null) { //$NON-NLS-1$
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<DciSummaryTableDescriptor> list = session.listDciSummaryTables();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						descriptors.clear();
						for(DciSummaryTableDescriptor d : list)
							descriptors.put(d.getId(), d);
						viewer.setInput(descriptors.values().toArray());
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.SummaryTableManager_JobError;
			}
		}.start();
	}

	/**
	 * 
	 */
	private void createActions()
	{
		// create refresh action
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refresh();
			}
		};

		// create add action
		actionCreate = new Action(Messages.SummaryTableManager_ActionNew, SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				createSummaryTable();
			}
		};

		// create edit action
		actionEdit = new Action(Messages.SummaryTableManager_ActionEdit, SharedIcons.EDIT) {
			@Override
			public void run()
			{
				editSummaryTable();
			}
		};
		actionEdit.setEnabled(false);

		// create delete action
		actionDelete = new Action(Messages.SummaryTableManager_ActionDelete, SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteSelection();
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
	 * @param manager Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionCreate);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionCreate);
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
					mgr.add(actionCreate);
					mgr.add(actionEdit);
					mgr.add(actionDelete);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Create new loyalty program
	 */
	private void createSummaryTable()
	{
		DciSummaryTable t = new DciSummaryTable("", ""); //$NON-NLS-1$ //$NON-NLS-2$
		PropertyDialog dlg = PropertyDialog.createDialogOn(getSite().getShell(), null, t);
		if (dlg != null)
		{
			dlg.getShell().setText(Messages.SummaryTableManager_TitleCreate);
			dlg.open();
			if (t.getId() != 0)
			{
				// was saved to server
				DciSummaryTableDescriptor d = new DciSummaryTableDescriptor(t);
				descriptors.put(d.getId(), d);
				viewer.setInput(descriptors.values().toArray());
				viewer.setSelection(new StructuredSelection(d));
			}
		}
	}

	/**
	 * Edit existing dataset
	 */
	private void editSummaryTable()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		final DciSummaryTableDescriptor d = (DciSummaryTableDescriptor)selection.getFirstElement();
		new ConsoleJob(Messages.SummaryTableManager_ReadJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciSummaryTable t = session.getDciSummaryTable(d.getId());
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						PropertyDialog dlg = PropertyDialog.createDialogOn(getSite().getShell(), null, t);
						dlg.getShell().setText(Messages.SummaryTableManager_TitleEdit);
						dlg.open();
						d.updateFromTable(t);
						viewer.update(d, null);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.SummaryTableManager_ReadJobError;
			}
		}.start();
	}
	
	/**
	 * Delete selected datasets
	 */
	private void deleteSelection()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 0)
			return;
		
		if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.SummaryTableManager_ConfirmDelete, Messages.SummaryTableManager_Confirmation))
			return;
	
		final int[] idList = new int[selection.size()];
		int i = 0;
		for(Object o : selection.toList())
			idList[i++] = ((DciSummaryTableDescriptor)o).getId();
		
		new ConsoleJob(Messages.SummaryTableManager_DeleteJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				for(int id : idList)
					session.deleteDciSummaryTable(id);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						for(int id : idList)
							descriptors.remove(id);
						viewer.setInput(descriptors.values().toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.SummaryTableManager_DeleteJobError;
			}
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((session != null) && (listener != null))
			session.removeListener(listener);
		super.dispose();
	}
}
