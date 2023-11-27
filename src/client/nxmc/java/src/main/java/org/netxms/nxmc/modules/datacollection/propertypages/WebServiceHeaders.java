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
package org.netxms.nxmc.modules.datacollection.propertypages;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
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
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.HeaderEditDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Headers" property page for web service definition configuration
 */
public class WebServiceHeaders extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(WebServiceHeaders.class);

   private WebServiceDefinition definition;
   private Map<String, String> headers;
   private TableViewer viewer;
   private Button buttonAdd;
   private Button buttonEdit;
   private Button buttonDelete;

   /**
    * Constructor
    */
   public WebServiceHeaders(WebServiceDefinition definition)
   {
      super(LocalizationHelper.getI18n(WebServiceHeaders.class).tr("Headers"));
      noDefaultAndApplyButton();
      this.definition = definition;
      this.headers = new HashMap<String, String>(definition.getHeaders());
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
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION);
      setupViewer();
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 450;
      viewer.getTable().setLayoutData(gd);

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
      buttonAdd.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addHeader();
         }
      });

      buttonEdit = new Button(buttonArea, SWT.PUSH);
      buttonEdit.setText(i18n.tr("&Edit..."));
      buttonEdit.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editHeader();
         }
      });
      buttonEdit.setEnabled(false);

      buttonDelete = new Button(buttonArea, SWT.PUSH);
      buttonDelete.setText(i18n.tr("&Delete"));
      buttonDelete.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteHeaders();
         }
      });
      buttonDelete.setEnabled(false);

      return dialogArea;
   }

   /**
    * Add new header
    */
   private void addHeader()
   {
      HeaderEditDialog dlg = new HeaderEditDialog(getShell(), null, null);
      if (dlg.open() == Window.OK)
      {
         headers.put(dlg.getName(), dlg.getValue());
         viewer.refresh();
      }
   }

   /**
    * Edit header
    */
   @SuppressWarnings("unchecked")
   private void editHeader()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Entry<String, String> e = (Entry<String, String>)selection.getFirstElement();
      HeaderEditDialog dlg = new HeaderEditDialog(getShell(), e.getKey(), e.getValue());
      if (dlg.open() == Window.OK)
      {
         headers.put(dlg.getName(), dlg.getValue());
         viewer.refresh();
      }
   }

   /**
    * Delete selected headers
    */
   @SuppressWarnings("unchecked")
   private void deleteHeaders()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      for(Object o : selection.toList())
         headers.remove(((Entry<String, String>)o).getKey());
      viewer.refresh();
   }

   /**
    * Save dialog settings
    */
   private void saveSettings()
   {
      WidgetHelper.saveColumnSettings(viewer.getTable(), "WebServiceHeaders");
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
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
   {
      definition.getHeaders().clear();
      for(Entry<String, String> e : headers.entrySet())
         definition.setHeader(e.getKey(), e.getValue());
      saveSettings();
      return true;
   }

   /**
    * Setup viewer
    */
   private void setupViewer()
   {
      Table table = viewer.getTable();
      table.setHeaderVisible(true);

      TableColumn tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Name"));
      tc.setWidth(200);

      tc = new TableColumn(table, SWT.LEFT);
      tc.setText(i18n.tr("Value"));
      tc.setWidth(200);

      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new HeaderLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @SuppressWarnings("unchecked")
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            String v1 = ((Entry<String, String>)e1).getKey();
            String v2 = ((Entry<String, String>)e2).getKey();
            return v1.compareToIgnoreCase(v2);
         }
      });
      viewer.setInput(headers.entrySet());

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editHeader();
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            buttonEdit.setEnabled(selection.size() == 1);
            buttonDelete.setEnabled(!selection.isEmpty());
         }
      });

      WidgetHelper.restoreColumnSettings(table, "WebServiceHeaders");
   }

   /**
    * Label provide for header list
    */
   private class HeaderLabelProvider extends LabelProvider implements ITableLabelProvider
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
      @SuppressWarnings("unchecked")
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         return columnIndex == 0 ? ((Entry<String, String>)element).getKey() : ((Entry<String, String>)element).getValue();
      }
   }
}
