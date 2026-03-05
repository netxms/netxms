/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.jface.viewers.CheckboxCellEditor;
import org.eclipse.jface.viewers.ColumnViewer;
import org.eclipse.jface.viewers.ComboBoxCellEditor;
import org.eclipse.jface.viewers.EditingSupport;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.jface.viewers.TextCellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Table;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.helpers.ColumnPreviewLabelProvider;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for creating table DCI from SNMP
 */
public class CreateSnmpTableDciDialog extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateSnmpTableDciDialog.class);

   private LabeledText textDescription;
   private LabeledText textInterval;
   private LabeledText textRetention;
   private TableViewer columnViewer;

   private String description;
   private int pollingInterval;
   private int retentionTime;
   private List<ColumnDefinition> columns;
   private Set<ColumnDefinition> selectedColumns;
   private String dciName;

   /**
    * Create dialog.
    *
    * @param parentShell parent shell
    * @param initialDescription initial description text
    * @param columns list of column definitions to display
    * @param dciName OID that will be used as the DCI metric name
    */
   public CreateSnmpTableDciDialog(Shell parentShell, String initialDescription, List<ColumnDefinition> columns, String dciName)
   {
      super(parentShell);
      this.description = initialDescription;
      this.columns = columns;
      this.selectedColumns = new HashSet<ColumnDefinition>(columns);
      this.dciName = dciName;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create SNMP Table DCI"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      dialogArea.setLayout(layout);

      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel(i18n.tr("Description"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 500;
      textDescription.setLayoutData(gd);
      textDescription.setText(description);

      Composite optionsGroup = new Composite(dialogArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      optionsGroup.setLayoutData(gd);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      optionsGroup.setLayout(layout);

      textInterval = new LabeledText(optionsGroup, SWT.NONE);
      textInterval.setLabel(i18n.tr("Polling interval (seconds)"));
      textInterval.setText("60"); //$NON-NLS-1$
      textInterval.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textInterval.setLayoutData(gd);

      textRetention = new LabeledText(optionsGroup, SWT.NONE);
      textRetention.setLabel(i18n.tr("Retention time (days)"));
      textRetention.setText("30"); //$NON-NLS-1$
      textRetention.getTextControl().setTextLimit(5);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textRetention.setLayoutData(gd);

      // Columns preview table
      columnViewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      Table table = columnViewer.getTable();
      table.setHeaderVisible(true);
      table.setLinesVisible(true);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 200;
      table.setLayoutData(gd);

      TableViewerColumn column = new TableViewerColumn(columnViewer, SWT.CENTER);
      column.getColumn().setText(i18n.tr("Include"));
      column.getColumn().setWidth(60);
      column.setEditingSupport(new IncludeEditingSupport(columnViewer));

      column = new TableViewerColumn(columnViewer, SWT.LEFT);
      column.getColumn().setText(i18n.tr("Name"));
      column.getColumn().setWidth(150);
      column.setEditingSupport(new NameEditingSupport(columnViewer));

      column = new TableViewerColumn(columnViewer, SWT.LEFT);
      column.getColumn().setText(i18n.tr("Data Type"));
      column.getColumn().setWidth(100);
      column.setEditingSupport(new DataTypeEditingSupport(columnViewer));

      column = new TableViewerColumn(columnViewer, SWT.LEFT);
      column.getColumn().setText(i18n.tr("SNMP OID"));
      column.getColumn().setWidth(200);

      column = new TableViewerColumn(columnViewer, SWT.CENTER);
      column.getColumn().setText(i18n.tr("Instance"));
      column.getColumn().setWidth(70);
      column.setEditingSupport(new InstanceEditingSupport(columnViewer));

      columnViewer.setContentProvider(new ArrayContentProvider());
      columnViewer.setLabelProvider(new ColumnPreviewLabelProvider(selectedColumns, dciName));
      columnViewer.setInput(columns);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      try
      {
         pollingInterval = Integer.parseInt(textInterval.getText());
         if ((pollingInterval < 1) || (pollingInterval > 10000))
            throw new NumberFormatException();
      }
      catch(NumberFormatException e)
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter polling interval as integer in range 1 .. 10000"));
         return;
      }

      try
      {
         retentionTime = Integer.parseInt(textRetention.getText());
         if ((retentionTime < 0) || (retentionTime > 10000))
            throw new NumberFormatException();
      }
      catch(NumberFormatException e)
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Please enter retention time as integer in range 0 .. 10000"));
         return;
      }

      description = textDescription.getText().trim();
      super.okPressed();
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @return the pollingInterval
    */
   public int getPollingInterval()
   {
      return pollingInterval;
   }

   /**
    * @return the retentionTime
    */
   public int getRetentionTime()
   {
      return retentionTime;
   }

   /**
    * @return only the columns selected by the user
    */
   public List<ColumnDefinition> getSelectedColumns()
   {
      List<ColumnDefinition> result = new ArrayList<ColumnDefinition>();
      for(ColumnDefinition c : columns)
         if (selectedColumns.contains(c))
            result.add(c);
      return result;
   }

   /**
    * Editing support for Include column
    */
   private class IncludeEditingSupport extends EditingSupport
   {
      private CellEditor editor;

      public IncludeEditingSupport(ColumnViewer viewer)
      {
         super(viewer);
         editor = new CheckboxCellEditor(null, SWT.CHECK | SWT.READ_ONLY);
      }

      @Override
      protected boolean canEdit(Object element)
      {
         return true;
      }

      @Override
      protected CellEditor getCellEditor(Object element)
      {
         return editor;
      }

      @Override
      protected Object getValue(Object element)
      {
         return Boolean.valueOf(selectedColumns.contains(element));
      }

      @Override
      protected void setValue(Object element, Object value)
      {
         if ((Boolean)value)
            selectedColumns.add((ColumnDefinition)element);
         else
            selectedColumns.remove(element);
         getViewer().update(element, null);
      }
   }

   /**
    * Editing support for Name column
    */
   private class NameEditingSupport extends EditingSupport
   {
      private CellEditor editor;

      public NameEditingSupport(ColumnViewer viewer)
      {
         super(viewer);
         editor = new TextCellEditor(((TableViewer)viewer).getTable());
      }

      @Override
      protected boolean canEdit(Object element)
      {
         return true;
      }

      @Override
      protected CellEditor getCellEditor(Object element)
      {
         return editor;
      }

      @Override
      protected Object getValue(Object element)
      {
         return ((ColumnDefinition)element).getName();
      }

      @Override
      protected void setValue(Object element, Object value)
      {
         String name = ((String)value).trim();
         if (!name.isEmpty())
         {
            ((ColumnDefinition)element).setName(name);
            ((ColumnDefinition)element).setDisplayName(name);
            getViewer().update(element, null);
         }
      }
   }

   /**
    * Editing support for Data Type column
    */
   private class DataTypeEditingSupport extends EditingSupport
   {
      private CellEditor editor;

      public DataTypeEditingSupport(ColumnViewer viewer)
      {
         super(viewer);
         String[] typeNames = new String[EditColumnDialog.TYPES.length];
         for(int i = 0; i < EditColumnDialog.TYPES.length; i++)
            typeNames[i] = DataCollectionDisplayInfo.getDataTypeName(EditColumnDialog.TYPES[i]);
         editor = new ComboBoxCellEditor(((TableViewer)viewer).getTable(), typeNames, SWT.READ_ONLY);
      }

      @Override
      protected boolean canEdit(Object element)
      {
         return true;
      }

      @Override
      protected CellEditor getCellEditor(Object element)
      {
         return editor;
      }

      @Override
      protected Object getValue(Object element)
      {
         return EditColumnDialog.getDataTypePosition(((ColumnDefinition)element).getDataType());
      }

      @Override
      protected void setValue(Object element, Object value)
      {
         ((ColumnDefinition)element).setDataType(EditColumnDialog.TYPES[(Integer)value]);
         getViewer().update(element, null);
      }
   }

   /**
    * Editing support for Instance column
    */
   private class InstanceEditingSupport extends EditingSupport
   {
      private CellEditor editor;

      public InstanceEditingSupport(ColumnViewer viewer)
      {
         super(viewer);
         editor = new CheckboxCellEditor(null, SWT.CHECK | SWT.READ_ONLY);
      }

      @Override
      protected boolean canEdit(Object element)
      {
         return true;
      }

      @Override
      protected CellEditor getCellEditor(Object element)
      {
         return editor;
      }

      @Override
      protected Object getValue(Object element)
      {
         return Boolean.valueOf(((ColumnDefinition)element).isInstanceColumn());
      }

      @Override
      protected void setValue(Object element, Object value)
      {
         ((ColumnDefinition)element).setInstanceColumn((Boolean)value);
         getViewer().update(element, null);
      }
   }

}
