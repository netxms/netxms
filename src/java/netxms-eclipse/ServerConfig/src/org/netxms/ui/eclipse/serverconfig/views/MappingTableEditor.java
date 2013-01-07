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

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.Session;
import org.netxms.api.client.mt.MappingTable;
import org.netxms.api.client.mt.MappingTableEntry;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.MappingTableEntryComparator;
import org.netxms.ui.eclipse.serverconfig.views.helpers.MappingTableEntryLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Mapping table editor
 */
public class MappingTableEditor extends ViewPart implements ISaveablePart2
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.MappingTableEditor";
	
	public static final int COLUMN_KEY = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_DESCRIPTION = 2;
	
	private int mappingTableId;
	private MappingTable mappingTable;
	private Session session;
	private SortableTableViewer viewer;
	private boolean modified = false;
	private Action actionNewRow;
	private Action actionDelete;
	private Action actionSave;
	private Action actionRefresh;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		try
		{
			mappingTableId = Integer.parseInt(site.getSecondaryId());
		}
		catch(Exception e)
		{
			throw new PartInitException("Internal error", e);
		}
		if (mappingTableId <= 0)
			throw new PartInitException("Internal error");
		
		setPartName("Mapping Table - [" + mappingTableId + "]");
		
		session = ConsoleSharedData.getSession();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final int[] widths = { 200, 200, 400 };
		final String[] names = { "Key", "Value", "Comments" };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_KEY, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new MappingTableEntryLabelProvider());
		viewer.setComparator(new MappingTableEntryComparator());
		
		viewer.setColumnProperties(new String[] { "key", "value", "comments" });
		CellEditor[] editors = new CellEditor[] { new TextCellEditor(viewer.getTable()), new TextCellEditor(viewer.getTable()), new TextCellEditor(viewer.getTable()) };
		viewer.setCellEditors(editors);
		viewer.setCellModifier(new CellModifier());
		
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				actionDelete.setEnabled(selection.size() > 0);
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		activateContext();
		
		refresh();
	}
	
	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.serverconfig.context.MappingTableEditor");
		}
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
				
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				if (modified)
					if (!MessageDialog.openQuestion(getSite().getShell(), "Refresh Confirmation", "This will destroy unsaved changes. Are you sure?"))
						return;
				refresh();
			}
		};
		
		actionNewRow = new Action("&New row", SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				addNewRow();
			}
		};
		actionNewRow.setEnabled(false);
		actionNewRow.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.add_new_row");
		handlerService.activateHandler(actionNewRow.getActionDefinitionId(), new ActionHandler(actionNewRow));
		
		actionDelete = new Action("&Delete", SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteRows();
			}
		};
		actionDelete.setEnabled(false);
		actionDelete.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.delete_rows");
		handlerService.activateHandler(actionDelete.getActionDefinitionId(), new ActionHandler(actionDelete));
		
		actionSave = new Action("&Save", SharedIcons.SAVE) {
			@Override
			public void run()
			{
				new SaveJob().start();
			}
		};
		actionSave.setEnabled(false);
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
		manager.add(actionNewRow);
		manager.add(actionSave);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * @param manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNewRow);
		manager.add(actionSave);
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
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionNewRow);
		manager.add(actionDelete);
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
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		SaveJob job = new SaveJob();
		job.start();
		try
		{
			job.join();
		}
		catch(InterruptedException e)
		{
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSaveAs()
	 */
	@Override
	public void doSaveAs()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isDirty()
	 */
	@Override
	public boolean isDirty()
	{
		return modified;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
	 */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
	 */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return modified;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart2#promptToSaveOnClose()
	 */
	@Override
	public int promptToSaveOnClose()
	{
		return DEFAULT;
	}
	
	/**
	 * Refresh content
	 */
	private void refresh()
	{
		new ConsoleJob("Load mapping table", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final MappingTable t = session.getMappingTable(mappingTableId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						mappingTable = t;
						setPartName("Mapping Table - " + mappingTable.getName());
						viewer.setInput(mappingTable.getData().toArray());
						actionNewRow.setEnabled(true);
						setModified(false);
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot load mapping table content from server";
			}
		}.start();
	}
	
	/**
	 * Set or clear "modified" flag
	 * 
	 * @param m
	 */
	private void setModified(boolean m)
	{
		if (modified == m)
			return;
		
		modified = m;
		actionSave.setEnabled(m);
		firePropertyChange(PROP_DIRTY);
	}
	
	/**
	 * Add new row
	 */
	private void addNewRow()
	{
		if (mappingTable == null)
			return;
		
		MappingTableEntry e = new MappingTableEntry("", "", "");
		mappingTable.getData().add(e);
		viewer.setInput(mappingTable.getData().toArray());
		viewer.setSelection(new StructuredSelection(e));
		setModified(true);
		viewer.editElement(e, COLUMN_KEY);
	}
	
	/**
	 * Delete selected rows
	 */
	private void deleteRows()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 0)
			return;
		
		for(Object o : selection.toList())
			mappingTable.getData().remove(o);
		
		viewer.setInput(mappingTable.getData().toArray());
		setModified(true);
	}
	
	/**
	 * Cell modifier class
	 */
	private class CellModifier implements ICellModifier
	{
		@Override
		public void modify(Object element, String property, Object value)
		{
			if (element instanceof Item)
				element = ((Item)element).getData();
			
			MappingTableEntry e = (MappingTableEntry)element;
			boolean changed = false;
			if (property.equals("key"))
			{
				if (!e.getKey().equals((String)value))
				{
					e.setKey((String)value);
					changed = true;
				}
			}
			else if (property.equals("value"))
			{
				if (!e.getKey().equals((String)value))
				{
					e.setValue((String)value);
					changed = true;
				}
			}
			else if (property.equals("comments"))
			{
				if (!e.getKey().equals((String)value))
				{
					e.setDescription((String)value);
					changed = true;
				}
			}
			
			if (changed)
			{
				viewer.refresh();
				setModified(true);
			}
		}

		@Override
		public Object getValue(Object element, String property)
		{
			MappingTableEntry e = (MappingTableEntry)element;
			if (property.equals("key"))
				return e.getKey();
			if (property.equals("value"))
				return e.getValue();
			if (property.equals("comments"))
				return e.getDescription();
			return null;
		}

		@Override
		public boolean canModify(Object element, String property)
		{
			return true;
		}
	}

	/**
	 * Job for saving table
	 */
	private class SaveJob extends ConsoleJob
	{
		public SaveJob()
		{
			super("Save mapping table", MappingTableEditor.this, Activator.PLUGIN_ID, null);
		}

		/* (non-Javadoc)
		 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#runInternal(org.eclipse.core.runtime.IProgressMonitor)
		 */
		@Override
		protected void runInternal(IProgressMonitor monitor) throws Exception
		{
			session.updateMappingTable(mappingTable);
			runInUIThread(new Runnable() {				
				@Override
				public void run()
				{
					setModified(false);
				}
			});
		}

		/* (non-Javadoc)
		 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#getErrorMessage()
		 */
		@Override
		protected String getErrorMessage()
		{
			return "Cannot save mapping table";
		}
	}
}
