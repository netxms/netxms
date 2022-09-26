/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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

import java.util.Comparator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.ColumnViewerEditor;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent;
import org.eclipse.jface.viewers.ColumnViewerEditorActivationStrategy;
import org.eclipse.jface.viewers.ICellModifier;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.nebula.jface.gridviewer.GridTableViewer;
import org.eclipse.nebula.jface.gridviewer.GridViewerEditor;
import org.eclipse.nebula.widgets.grid.Grid;
import org.eclipse.nebula.widgets.grid.GridColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Item;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableEntry;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.Messages;
import org.netxms.ui.eclipse.serverconfig.views.helpers.MappingTableEntryLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.NaturalOrderComparator;

/**
 * Mapping table editor
 */
public class MappingTableEditor extends ViewPart implements ISaveablePart2
{
	public static final String ID = "org.netxms.ui.eclipse.serverconfig.views.MappingTableEditor"; //$NON-NLS-1$

	public static final int COLUMN_KEY = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_DESCRIPTION = 2;

	private int mappingTableId;
	private MappingTable mappingTable;
	private NXCSession session;
   private GridTableViewer viewer;
	private boolean modified = false;
	private Action actionNewRow;
	private Action actionDelete;
	private Action actionSave;
	private Action actionRefresh;

   /**
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
			throw new PartInitException("Internal error", e); //$NON-NLS-1$
		}
		if (mappingTableId <= 0)
			throw new PartInitException("Internal error"); //$NON-NLS-1$

		setPartName(String.format(Messages.get().MappingTableEditor_InitialPartName, mappingTableId));

		session = ConsoleSharedData.getSession();
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{
      viewer = new GridTableViewer(parent, SWT.V_SCROLL | SWT.H_SCROLL);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new MappingTableEntryLabelProvider());

      Grid grid = viewer.getGrid();
      grid.setHeaderVisible(true);
      grid.setCellSelectionEnabled(true);

      GridColumn column = new GridColumn(grid, SWT.NONE);
      column.setText(Messages.get().MappingTableEditor_ColKey);
      column.setWidth(200);

      column = new GridColumn(grid, SWT.NONE);
      column.setText(Messages.get().MappingTableEditor_ColValue);
      column.setWidth(200);

      column = new GridColumn(grid, SWT.NONE);
      column.setText(Messages.get().MappingTableEditor_ColComments);
      column.setWidth(400);

      viewer.setColumnProperties(new String[] { "key", "value", "comments" }); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
      CellEditor[] editors = new CellEditor[] { new MappingTableCellEditor(viewer.getGrid()), new MappingTableCellEditor(viewer.getGrid()), new MappingTableCellEditor(viewer.getGrid()) };
      viewer.setCellEditors(editors);
      viewer.setCellModifier(new CellModifier());
      ColumnViewerEditorActivationStrategy activationStrategy = new ColumnViewerEditorActivationStrategy(viewer) {
         protected boolean isEditorActivationEvent(ColumnViewerEditorActivationEvent event)
         {
            return event.eventType == ColumnViewerEditorActivationEvent.TRAVERSAL || event.eventType == ColumnViewerEditorActivationEvent.MOUSE_DOUBLE_CLICK_SELECTION ||
                  (event.eventType == ColumnViewerEditorActivationEvent.KEY_PRESSED && (event.keyCode == SWT.CR || Character.isLetterOrDigit(event.character)));
         }
      };
      GridViewerEditor.create(viewer, activationStrategy,
            ColumnViewerEditor.TABBING_HORIZONTAL | ColumnViewerEditor.TABBING_MOVE_TO_ROW_NEIGHBOR | ColumnViewerEditor.TABBING_VERTICAL | ColumnViewerEditor.KEYBOARD_ACTIVATION);

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
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
			contextService.activateContext("org.netxms.ui.eclipse.serverconfig.context.MappingTableEditor"); //$NON-NLS-1$
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
					if (!MessageDialogHelper.openQuestion(getSite().getShell(), Messages.get().MappingTableEditor_RefreshConfirmation, Messages.get().MappingTableEditor_RefreshConfirmationText))
						return;
				refresh();
			}
		};

		actionNewRow = new Action(Messages.get().MappingTableEditor_NewRow, SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				addNewRow();
			}
		};
		actionNewRow.setEnabled(false);
		actionNewRow.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.add_new_row"); //$NON-NLS-1$
		handlerService.activateHandler(actionNewRow.getActionDefinitionId(), new ActionHandler(actionNewRow));

		actionDelete = new Action(Messages.get().MappingTableEditor_Delete, SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteRows();
			}
		};
		actionDelete.setEnabled(false);
		actionDelete.setActionDefinitionId("org.netxms.ui.eclipse.serverconfig.commands.delete_rows"); //$NON-NLS-1$
		handlerService.activateHandler(actionDelete.getActionDefinitionId(), new ActionHandler(actionDelete));

		actionSave = new Action(Messages.get().MappingTableEditor_Save, SharedIcons.SAVE) {
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

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
      viewer.getGrid().setFocus();
	}

   /**
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

   /**
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
	@Override
	public void doSaveAs()
	{
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
	@Override
	public boolean isDirty()
	{
		return modified;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return modified;
	}

   /**
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
		new ConsoleJob(Messages.get().MappingTableEditor_LoadJobName, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final MappingTable t = session.getMappingTable(mappingTableId);
            t.getData().sort(new Comparator<MappingTableEntry>() {
               @Override
               public int compare(MappingTableEntry e1, MappingTableEntry e2)
               {
                  return NaturalOrderComparator.compare(e1.getKey(), e2.getKey());
               }
            });
            t.getData().add(new MappingTableEntry(null, null, null)); // pseudo-entry at the end
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						mappingTable = t;
						setPartName(String.format(Messages.get().MappingTableEditor_PartName, mappingTable.getName()));
                  viewer.setInput(mappingTable.getData());
						actionNewRow.setEnabled(true);
						setModified(false);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().MappingTableEditor_LoadJobError;
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

      MappingTableEntry e = mappingTable.getData().get(mappingTable.getData().size() - 1);
		viewer.setSelection(new StructuredSelection(e));
		viewer.editElement(e, COLUMN_KEY);
	}

	/**
	 * Delete selected rows
	 */
	private void deleteRows()
	{
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
			return;

		for(Object o : selection.toList())
			mappingTable.getData().remove(o);

      viewer.refresh();
		setModified(true);
	}

	/**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      modified = false;
      firePropertyChange(PROP_DIRTY);

      super.dispose();
   }

   /**
    * Cell modifier class
    */
	private class CellModifier implements ICellModifier
	{
      /**
       * @see org.eclipse.jface.viewers.ICellModifier#modify(java.lang.Object, java.lang.String, java.lang.Object)
       */
		@Override
		public void modify(Object element, String property, Object value)
		{
			if (element instanceof Item)
				element = ((Item)element).getData();

			MappingTableEntry e = (MappingTableEntry)element;
         boolean pseudoEntry = (e.getKey() == null) && (e.getValue() == null) && (e.getDescription() == null);
			boolean changed = false;
			if (property.equals("key")) //$NON-NLS-1$
			{
            if ((e.getKey() == null) || !e.getKey().equals(value))
				{
					e.setKey((String)value);
					changed = true;
				}
			}
			else if (property.equals("value")) //$NON-NLS-1$
			{
            if ((e.getValue() == null) || !e.getValue().equals(value))
				{
					e.setValue((String)value);
					changed = true;
				}
			}
			else if (property.equals("comments")) //$NON-NLS-1$
			{
            if ((e.getDescription() == null) || !e.getDescription().equals(value))
				{
					e.setDescription((String)value);
					changed = true;
				}
			}

			if (changed)
			{
            if (pseudoEntry)
            {
               if (!e.isEmpty())
               {
                  if (e.getKey() == null)
                     e.setKey("");
                  if (e.getValue() == null)
                     e.setValue("");
                  if (e.getDescription() == null)
                     e.setDescription("");
                  mappingTable.getData().add(new MappingTableEntry(null, null, null)); // add new pseudo-entry at the end
               }
               else
               {
                  e.setKey(null);
                  e.setValue(null);
                  e.setDescription(null);
               }
            }
				viewer.refresh();
				setModified(true);
			}
		}

      /**
       * @see org.eclipse.jface.viewers.ICellModifier#getValue(java.lang.Object, java.lang.String)
       */
		@Override
		public Object getValue(Object element, String property)
		{
			MappingTableEntry e = (MappingTableEntry)element;
			if (property.equals("key")) //$NON-NLS-1$
				return e.getKey();
			if (property.equals("value")) //$NON-NLS-1$
				return e.getValue();
			if (property.equals("comments")) //$NON-NLS-1$
				return e.getDescription();
			return null;
		}

      /**
       * @see org.eclipse.jface.viewers.ICellModifier#canModify(java.lang.Object, java.lang.String)
       */
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
			super(Messages.get().MappingTableEditor_SaveJobName, MappingTableEditor.this, Activator.PLUGIN_ID, null);
		}

      /**
       * @see org.netxms.ui.eclipse.jobs.ConsoleJob#runInternal(org.eclipse.core.runtime.IProgressMonitor)
       */
		@Override
		protected void runInternal(IProgressMonitor monitor) throws Exception
		{
         mappingTable.getData().remove(mappingTable.getData().size() - 1); // Remove pseudo-row at the end
			session.updateMappingTable(mappingTable);
			runInUIThread(new Runnable() {				
				@Override
				public void run()
				{
					setModified(false);
				}
			});
		}

      /**
       * @see org.netxms.ui.eclipse.jobs.ConsoleJob#getErrorMessage()
       */
		@Override
		protected String getErrorMessage()
		{
			return Messages.get().MappingTableEditor_SaveJobError;
		}
	}

   /**
    * Custom text cell editor
    */
   private static class MappingTableCellEditor extends TextCellEditor
   {
      public MappingTableCellEditor(Composite parent)
      {
         super(parent);
      }

      /**
       * @see org.eclipse.jface.viewers.TextCellEditor#doSetValue(java.lang.Object)
       */
      @Override
      protected void doSetValue(Object value)
      {
         super.doSetValue((value != null) ? value : "");
      }

      /**
       * @see org.eclipse.jface.viewers.CellEditor#activate(org.eclipse.jface.viewers.ColumnViewerEditorActivationEvent)
       */
      @Override
      public void activate(ColumnViewerEditorActivationEvent activationEvent)
      {
         super.activate(activationEvent);
         if ((activationEvent.eventType == ColumnViewerEditorActivationEvent.KEY_PRESSED) && Character.isLetterOrDigit(activationEvent.character))
         {
            ((Text)getControl()).setText(new String(new char[] { activationEvent.character }));
         }
      }

      /**
       * @see org.eclipse.jface.viewers.TextCellEditor#doSetFocus()
       */
      @Override
      protected void doSetFocus()
      {
         super.doSetFocus();
         ((Text)getControl()).clearSelection();
      }
   }
}
