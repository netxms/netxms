/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.snmp.propertypages;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpTrap;
import org.netxms.client.snmp.SnmpTrapParameterMapping;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.snmp.dialogs.ParameterMappingEditDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Parameters" property page for SNMP trap configuration
 */
public class SnmpTrapParameters extends PropertyPage
{
   private static final String PARAMLIST_TABLE_SETTINGS = "TrapConfigurationDialog.ParamList";

   private I18n i18n = LocalizationHelper.getI18n(SnmpTrapParameters.class);
   private SnmpTrap trap;
   private List<SnmpTrapParameterMapping> pmap;
   private TableViewer paramList;
   private Button buttonAdd;
   private Button buttonEdit;
   private Button buttonDelete;
   private Button buttonUp;
   private Button buttonDown;

   /**
    * Create page
    * 
    * @param trap SNMP trap object to edit
    */
   public SnmpTrapParameters(SnmpTrap trap)
   {
      super("Parameters");
      noDefaultAndApplyButton();
      this.trap = trap;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      paramList = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 300;
      paramList.getTable().setLayoutData(gd);
      setupParameterList();
      
      Composite buttonArea = new Composite(dialogArea, SWT.NONE);
      RowLayout btnLayout = new RowLayout();
      btnLayout.type = SWT.VERTICAL;
      btnLayout.marginBottom = 0;
      btnLayout.marginLeft = 0;
      btnLayout.marginRight = 0;
      btnLayout.marginTop = 0;
      btnLayout.fill = true;
      btnLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonArea.setLayout(btnLayout);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      buttonArea.setLayoutData(gd);
      
      buttonAdd = new Button(buttonArea, SWT.PUSH);
      buttonAdd.setText(i18n.tr("&Add..."));
      buttonAdd.setLayoutData(new RowData(WidgetHelper.BUTTON_WIDTH_HINT, SWT.DEFAULT));
      buttonAdd.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addParameter();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      
      buttonEdit = new Button(buttonArea, SWT.PUSH);
      buttonEdit.setText(i18n.tr("&Edit..."));
      buttonEdit.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editParameter();
         }
         
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      buttonEdit.setEnabled(false);
      
      buttonDelete = new Button(buttonArea, SWT.PUSH);
      buttonDelete.setText(i18n.tr("&Delete"));
      buttonDelete.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteParameters();
         }
      });
      buttonDelete.setEnabled(false);

      buttonUp = new Button(buttonArea, SWT.PUSH);
      buttonUp.setText(i18n.tr("Move &up"));

      buttonDown = new Button(buttonArea, SWT.PUSH);
      buttonDown.setText(i18n.tr("Move &down"));

      return dialogArea;
   }

   /**
    * Add new parameter mapping
    */
   private void addParameter()
   {
      SnmpTrapParameterMapping pm = new SnmpTrapParameterMapping(new SnmpObjectId());
      ParameterMappingEditDialog dlg = new ParameterMappingEditDialog(getShell(), pm);
      if (dlg.open() == Window.OK)
      {
         pmap.add(pm);
         paramList.setInput(pmap.toArray());
         paramList.setSelection(new StructuredSelection(pm));
      }
   }

   /**
    * Edit currently selected parameter mapping
    */
   private void editParameter()
   {
      IStructuredSelection selection = paramList.getStructuredSelection();
      if (selection.size() != 1)
         return;

      SnmpTrapParameterMapping pm = (SnmpTrapParameterMapping)selection.getFirstElement();
      ParameterMappingEditDialog dlg = new ParameterMappingEditDialog(getShell(), pm);
      if (dlg.open() == Window.OK)
      {
         paramList.update(pm, null);
      }
   }

   /**
    * Delete selected parameters
    */
   @SuppressWarnings("unchecked")
   private void deleteParameters()
   {
      IStructuredSelection selection = paramList.getStructuredSelection();
      if (selection.isEmpty())
         return;
      
      Iterator<SnmpTrapParameterMapping> it = selection.iterator();
      while(it.hasNext())
         pmap.remove(it.next());
      
      paramList.setInput(pmap.toArray());
   }

   /**
    * Setup parameter mapping list
    */
   private void setupParameterList()
   {
      Table table = paramList.getTable();
      table.setHeaderVisible(true);
      
      TableColumn tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Number"));
      tc.setWidth(90);
      
      tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Parameter"));
      tc.setWidth(200);
      
      pmap = new ArrayList<SnmpTrapParameterMapping>(trap.getParameterMapping());
      
      paramList.setContentProvider(new ArrayContentProvider());
      paramList.setLabelProvider(new ParameterMappingLabelProvider());
      paramList.setInput(pmap.toArray());

      WidgetHelper.restoreColumnSettings(table, PARAMLIST_TABLE_SETTINGS);
      
      paramList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editParameter();
         }
      });

      paramList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = paramList.getStructuredSelection();
            buttonEdit.setEnabled(selection.size() == 1);
            buttonDelete.setEnabled(!selection.isEmpty());
         }
      });
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      WidgetHelper.saveColumnSettings(paramList.getTable(), PARAMLIST_TABLE_SETTINGS);
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      trap.getParameterMapping().clear();
      trap.getParameterMapping().addAll(pmap);
      saveSettings();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performCancel()
    */
   @Override
   public boolean performCancel()
   {
      if (isControlCreated())
         saveSettings();
      return super.performCancel();
   }

   /**
    * Label provider for parameter mapping list
    */
   private class ParameterMappingLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         SnmpTrapParameterMapping pm = (SnmpTrapParameterMapping)element;
         switch(columnIndex)
         {
            case 0: // position
               return Integer.toString(pmap.indexOf(pm) + 2);
            case 1: // OID or position in trap
               return (pm.getType() == SnmpTrapParameterMapping.BY_OBJECT_ID) ? pm.getObjectId().toString() : (i18n.tr("POS:") + Integer.toString(pm.getPosition()));
         }
         return null;
      }
   }
}
