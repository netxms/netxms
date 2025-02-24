/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.networkmaps.propertypages;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.configs.MapLinkDataSource;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectDciDialog;
import org.netxms.nxmc.modules.networkmaps.dialogs.DataSourceEditDlg;
import org.netxms.nxmc.modules.networkmaps.propertypages.helpers.DciListLabelProvider;
import org.netxms.nxmc.modules.networkmaps.views.helpers.LinkEditor;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * DCI network map link data source property page
 */
public class LinkDataSources extends LinkPropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(LinkDataSources.class);

   public static final int COLUMN_POSITION = 0;
   public static final int COLUMN_NODE = 1;
   public static final int COLUMN_METRIC = 2;
   public static final int COLUMN_FORMAT = 3;
   public static final int COLUMN_LOCATION = 4;
   public static final int COLUMN_SOURCE = 5;

   private DciListLabelProvider labelProvider;
   private SortableTableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;
   private Button upButton;
   private Button downButton;
   private List<MapLinkDataSource> dciList = null;

   /**
    * Create new page for given link editor.
    *
    * @param linkEditor link editor
    */
   public LinkDataSources(LinkEditor linkEditor)
   {
      super(linkEditor, LocalizationHelper.getI18n(LinkDataSources.class).tr("Data Sources"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      Composite dialogArea = (Composite)super.createContents(parent);

      dciList = linkEditor.getDciList();

      labelProvider = new DciListLabelProvider(dciList, linkEditor);
      labelProvider.resolveDciNames(dciList);

      final String[] columnNames = { i18n.tr("Position"), i18n.tr("Node"), i18n.tr("Metric"), i18n.tr("Format"), i18n.tr("Location"), i18n.tr("Source") };
      final int[] columnWidths = { 40, 130, 200, 150, 100, 100 };
      viewer = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI
            | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(labelProvider);
      viewer.disableSorting();
      viewer.setInput(dciList.toArray());

      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.heightHint = 0;
      gridData.horizontalSpan = 2;
      viewer.getControl().setLayoutData(gridData);

      /* buttons on left side */
      Composite leftButtons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginLeft = 0;
      leftButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gridData);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(i18n.tr("&Up"));
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      upButton.setLayoutData(rd);
      upButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveUp();
         }
      });
      upButton.setEnabled(false);

      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText(i18n.tr("Dow&n"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      downButton.setLayoutData(rd);
      downButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveDown();
         }
      });
      downButton.setEnabled(false);

      /* buttons on right side */
      Composite rightButtons = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      rightButtons.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gridData);

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText(i18n.tr("&Add..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addButton.setLayoutData(rd);
      addButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addItem();
         }
      });

      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText(i18n.tr("Edit..."));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editButton.setLayoutData(rd);
      editButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editItem();
         }
      });
      editButton.setEnabled(false);

      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText(i18n.tr("&Delete"));
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteItems();
         }
      });
      deleteButton.setEnabled(false);

      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editButton.notifyListeners(SWT.Selection, new Event());
         }
      });

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();
            editButton.setEnabled(selection.size() == 1);
            deleteButton.setEnabled(selection.size() > 0);
            upButton.setEnabled(selection.size() == 1);
            downButton.setEnabled(selection.size() == 1);
         }
      });

      return dialogArea;
   }

   /**
    * @see org.netxms.nxmc.modules.networkmaps.propertypages.LinkPropertyPage#setupLayout(org.eclipse.swt.layout.GridLayout)
    */
   @Override
   protected void setupLayout(GridLayout layout)
   {
      layout.numColumns = 2;
   }

   /**
    * Add new item
    */
   private void addItem()
   {
      SelectDciDialog dlg = new SelectDciDialog(getShell(), 0);
      if (dlg.open() == Window.OK)
      {
         List<DciValue> selection = dlg.getSelection();
         List<MapLinkDataSource> select = new ArrayList<MapLinkDataSource>();
         for(DciValue item : selection)
         {
            MapLinkDataSource dci = new MapLinkDataSource(item);
            select.add(dci);
            labelProvider.addCacheEntry(dci.getNodeId(), dci.getDciId(), item.getDescription());

            dciList.add(dci);
            
         }
         viewer.setInput(dciList.toArray());
         viewer.setSelection(new StructuredSelection(select));
      }
   }

   /**
    * Edit selected item
    */
   private void editItem()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      MapLinkDataSource dci = (MapLinkDataSource)selection.getFirstElement();
      if (dci == null)
         return;

      DataSourceEditDlg dlg = new DataSourceEditDlg(getShell(), dci, linkEditor);
      if (dlg.open() == Window.OK)
      {
         labelProvider.addCacheEntry(dci.getNodeId(), dci.getDciId(), dlg.getDciDescription());
         viewer.update(dci, null);
      }
   }

   /**
    * Delete selected item(s)
    */
   private void deleteItems()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      for(Object o : selection.toList())
         dciList.remove(o);
      viewer.setInput(dciList.toArray());
   }

   /**
    * Move selected item up
    */
   private void moveUp()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1)
      {
         Object element = selection.getFirstElement();
         int index = dciList.indexOf(element);
         if (index > 0)
         {
            Collections.swap(dciList, index - 1, index);
            viewer.setInput(dciList.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      }
   }

   /**
    * Move selected item down
    */
   private void moveDown()
   {
      final IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() == 1)
      {
         Object element = selection.getFirstElement();
         int index = dciList.indexOf(element);
         if ((index < dciList.size() - 1) && (index >= 0))
         {
            Collections.swap(dciList, index + 1, index);
            viewer.setInput(dciList.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      }
   }

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
   {
      linkEditor.setDciList(dciList);
      linkEditor.setModified();
      return true;
   }
}
