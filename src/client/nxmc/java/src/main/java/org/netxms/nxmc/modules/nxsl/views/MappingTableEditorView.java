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
package org.netxms.nxmc.modules.nxsl.views;

import java.util.Comparator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
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
import org.netxms.client.NXCSession;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.views.helpers.NaturalOrderComparator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.views.helpers.MappingTableEntryLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Mapping table editor
 */
public class MappingTableEditorView extends ConfigurationView
{
	public static final int COLUMN_KEY = 0;
	public static final int COLUMN_VALUE = 1;
	public static final int COLUMN_DESCRIPTION = 2;

   private final I18n i18n = LocalizationHelper.getI18n(MappingTableEditorView.class);

	private int mappingTableId;
	private MappingTable mappingTable;
	private NXCSession session;
   private GridTableViewer viewer;
	private boolean modified = false;
	private Action actionNewRow;
	private Action actionDelete;
	private Action actionSave;

   /**
    * Create new mapping table editor view.
    *
    * @param mappingTableId mapping table ID
    * @param mappingTableName mapping table name
    */
   public MappingTableEditorView(int mappingTableId, String mappingTableName)
   {
      super(mappingTableName, ResourceManager.getImageDescriptor("icons/config-views/script-editor.png"), "objects.mapping-tables.editor." + Integer.toString(mappingTableId), false);
      this.mappingTableId = mappingTableId;
      this.session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
      viewer = new GridTableViewer(parent, SWT.V_SCROLL | SWT.H_SCROLL);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new MappingTableEntryLabelProvider());

      Grid grid = viewer.getGrid();
      grid.setHeaderVisible(true);
      grid.setCellSelectionEnabled(true);

      GridColumn column = new GridColumn(grid, SWT.NONE);
      column.setText(i18n.tr("Key"));
      column.setWidth(200);

      column = new GridColumn(grid, SWT.NONE);
      column.setText(i18n.tr("Value"));
      column.setWidth(200);

      column = new GridColumn(grid, SWT.NONE);
      column.setText(i18n.tr("Comments"));
      column.setWidth(400);

      viewer.setColumnProperties(new String[] { "key", "value", "comments" });
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
		createContextMenu();
   }

	/**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * Create actions
    */
	private void createActions()
	{
      actionNewRow = new Action(i18n.tr("&New row"), SharedIcons.ADD_OBJECT) {
			@Override
			public void run()
			{
				addNewRow();
			}
		};
		actionNewRow.setEnabled(false);
      addKeyBinding("M1+N", actionNewRow);

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
			@Override
			public void run()
			{
				deleteRows();
			}
		};
		actionDelete.setEnabled(false);
      addKeyBinding("M1+D", actionDelete);

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
			@Override
			public void run()
			{
            save();
			}
		};
		actionSave.setEnabled(false);
      addKeyBinding("M1+S", actionSave);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionNewRow);
		manager.add(actionSave);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionNewRow);
		manager.add(actionSave);
	}

	/**
    * Create context menu for rows
    */
	private void createContextMenu()
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
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
      final MappingTable t = new MappingTable(mappingTable);
      t.getData().remove(t.getData().size() - 1); // Remove pseudo-row at the end
      new Job(i18n.tr("Updating mapping table"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateMappingTable(t);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  setModified(false);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update mapping table");
         }
      }.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Reload Content"), i18n.tr("Any unsaved changes will be lost. Are you sure?")))
            return;
      }

      new Job(i18n.tr("Loading mapping table"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
                  viewer.setInput(mappingTable.getData());
						actionNewRow.setEnabled(true);
						setModified(false);
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot load mapping table");
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
