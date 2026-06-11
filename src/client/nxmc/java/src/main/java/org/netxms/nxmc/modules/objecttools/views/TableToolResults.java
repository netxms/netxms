/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools.views;

import java.util.ArrayList;
import java.util.Arrays;
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
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.InputField;
import org.netxms.client.Table;
import org.netxms.client.constants.InputFieldType;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableContentProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableItemComparator;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.TableLabelProvider;
import org.netxms.nxmc.modules.objects.ObjectContext;
import org.netxms.nxmc.modules.objects.dialogs.InputFieldEntryDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.xnap.commons.i18n.I18n;

/**
 * Display results of table tool execution
 */
public class TableToolResults extends ObjectToolResultView
{
   private final I18n i18n = LocalizationHelper.getI18n(TableToolResults.class);

	private SortableTableViewer viewer;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
   private Map<String, String> inputValues;
   private List<String> maskedFields;
   private boolean askInputValues;

   /**
    * Constructor
    *
    * @param node target node
    * @param tool tool information
    * @param inputValues input values collected from the user (may be empty)
    * @param maskedFields list of input field names that must be masked in audit log / title (may be empty)
    */
   public TableToolResults(ObjectContext node, ObjectTool tool, Map<String, String> inputValues, List<String> maskedFields)
   {
      super(node, tool, ResourceManager.getImageDescriptor("icons/object-tools/table.png"), false);
      this.inputValues = inputValues;
      this.maskedFields = maskedFields;
	}

   /**
    * Clone constructor
    */
   protected TableToolResults()
   {
      super();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      TableToolResults view = (TableToolResults)super.cloneView();
      view.inputValues = inputValues;
      view.maskedFields = maskedFields;
      return view;
   }

   /**
    * @see org.netxms.nxmc.modules.objecttools.views.ObjectToolResultView#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      if (inputValues != null)
      {
         List<String> keys = new ArrayList<>(inputValues.size());
         List<String> values = new ArrayList<>(inputValues.size());
         for(Map.Entry<String, String> e : inputValues.entrySet())
         {
            keys.add(e.getKey());
            if ((maskedFields != null) && maskedFields.contains(e.getKey()))
               values.add("");
            else
               values.add(e.getValue() != null ? e.getValue() : "");
         }
         memento.set("inputValues.keys", keys);
         memento.set("inputValues.values", values);
      }
      if (maskedFields != null)
         memento.set("maskedFields", maskedFields);
   }

   /**
    * Unlike AbstractCommandResultView, which restores a captured stdout snapshot and never re-runs the
    * tool, table tools always re-execute against the agent / SNMP target on refresh. So if any of the
    * collected input fields were masked, their values were cleared in saveState() and we must re-prompt
    * the user before the next execution can succeed.
    *
    * @see org.netxms.nxmc.modules.objecttools.views.ObjectToolResultView#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      List<String> keys = memento.getAsStringList("inputValues.keys");
      List<String> values = memento.getAsStringList("inputValues.values");
      if ((keys != null) && (values != null) && (keys.size() == values.size()))
      {
         inputValues = new HashMap<>(keys.size());
         for(int i = 0; i < keys.size(); i++)
            inputValues.put(keys.get(i), values.get(i));
      }
      maskedFields = memento.getAsStringList("maskedFields");
      askInputValues = (maskedFields != null) && !maskedFields.isEmpty();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
	{
		viewer = new SortableTableViewer(parent, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new TableContentProvider());
		viewer.setLabelProvider(new TableLabelProvider());

		createActions();
		createPopupMenu();
		refreshTable();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.MenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
		Action resetAction = viewer.getResetColumnOrderAction();
		if (resetAction != null)
		   manager.add(resetAction);
		Action showAllAction = viewer.getShowAllColumnsAction();
		if (showAllAction != null)
		   manager.add(showAllAction);
      Action autoSizeAction = viewer.getAutoSizeColumnsAction();
      if (autoSizeAction != null)
      {
         manager.add(new Separator());
         manager.add(autoSizeAction);
      }
		manager.add(actionExportAllToCsv);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExportAllToCsv);
	}

	/**
	 * Create pop-up menu
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
		manager.add(actionExportToCsv);
	}

   /**
    * Re-prompt user for masked input values after view restoration.
    *
    * @return true if values are ready (either no prompt needed or user supplied them); false if user canceled
    */
   private boolean restoreUserInputFields()
   {
      if (!askInputValues)
         return true;

      final InputField[] fields = tool.getInputFields();
      if (fields.length > 0)
      {
         Arrays.sort(fields, (f1, f2) -> (f1.getSequence() - f2.getSequence()));
         InputFieldEntryDialog dlg = new InputFieldEntryDialog(getWindow().getShell(), tool.getDisplayName(), fields, inputValues);
         if (dlg.open() != Window.OK)
            return false;  // askInputValues stays true so the next refresh re-prompts

         inputValues = dlg.getValues();
         if (maskedFields == null)
            maskedFields = new ArrayList<>();
         else
            maskedFields.clear();
         for(int i = 0; i < fields.length; i++)
         {
            if (fields[i].getType() == InputFieldType.PASSWORD)
               maskedFields.add(fields[i].getName());
         }
      }
      askInputValues = false;
      return true;
   }

	/**
	 * Refresh table
	 */
	public void refreshTable()
	{
      if (!restoreUserInputFields())
         return;

		viewer.setInput(null);
		new Job(String.format(i18n.tr("Load data for table tool %s"), tool.getName()), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table table = session.executeTableTool(tool.getId(), object.object.getObjectId(), inputValues, maskedFields);
            runInUIThread(() -> {
               if (!table.getTitle().isEmpty())
                  setName(object.object.getObjectName() + ": " + table.getTitle()); //$NON-NLS-1$
               updateViewer(table);
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return String.format(i18n.tr("Cannot get data for table tool %s"), tool.getName());
			}
		}.start();
	}

	/**
	 * Update viewer with fresh table data
	 *
	 * @param table table
	 */
	private void updateViewer(final Table table)
	{
		if (!viewer.isInitialized())
		{
			String[] names = table.getColumnDisplayNames();
			final int[] widths = new int[names.length];
			Arrays.fill(widths, 100);
			viewer.createColumns(names, widths, 0, SWT.UP);
			viewer.enablePersistence("TableToolResults." + Long.toString(tool.getId())); //$NON-NLS-1$
			viewer.setComparator(new TableItemComparator(table.getColumnDataTypes()));
			updateMenu(); // Column management actions become available only after columns are created
		}
		((TableLabelProvider)viewer.getLabelProvider()).setColumns(table.getColumns());
		viewer.setInput(table);
	}
}
