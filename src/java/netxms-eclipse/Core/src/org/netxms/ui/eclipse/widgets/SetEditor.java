/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 Raden Solutions
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
package org.netxms.ui.eclipse.widgets;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
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
import org.netxms.ui.eclipse.console.dialogs.SetEntryEditDialog;
import org.netxms.ui.eclipse.tools.ObjectLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.helpers.AttributeLabelProvider;

public class SetEditor extends Composite
{
   private SortableTableViewer viewerSetValue;
   private Button addSetValueButton;
   private Button editSetValueButton;
   private Button removeSetValueButton;
   private Map<String, String> pStorageSet = new HashMap<String, String>();

   public SetEditor(Composite parent, int style)
   {
      super(parent, style);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      this.setLayout(layout);
      
      final String[] setColumnNames = { "Key", "Value" };
      final int[] setColumnWidths = { 150, 200 };
      viewerSetValue = new SortableTableViewer(this, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewerSetValue.setContentProvider(new ArrayContentProvider());
      viewerSetValue.setLabelProvider(new AttributeLabelProvider());
      viewerSetValue.setComparator(new ObjectLabelComparator((ILabelProvider)viewerSetValue.getLabelProvider()));
      viewerSetValue.setInput(pStorageSet.entrySet().toArray());
      viewerSetValue.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = ((IStructuredSelection)viewerSetValue.getSelection()).size();
            editSetValueButton.setEnabled(size == 1);
            removeSetValueButton.setEnabled(size > 0);
         }
      });

      GridData gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      viewerSetValue.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(this, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginLeft = 0;
      buttonLayout.marginRight = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);

      addSetValueButton = new Button(buttons, SWT.PUSH);
      addSetValueButton.setText("Add");
      addSetValueButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addPStorageSetAction();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      addSetValueButton.setLayoutData(rd);

      editSetValueButton = new Button(buttons, SWT.PUSH);
      editSetValueButton.setText("Edit");
      editSetValueButton.addSelectionListener(new SelectionListener() {
        @Override
        public void widgetDefaultSelected(SelectionEvent e)
        {
           widgetSelected(e);
        }

        @Override
        public void widgetSelected(SelectionEvent e)
        {
           editPStorageSetAction();
        }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      editSetValueButton.setLayoutData(rd);
      editSetValueButton.setEnabled(false);
      
      removeSetValueButton = new Button(buttons, SWT.PUSH);
      removeSetValueButton.setText("Delete");
      removeSetValueButton.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deletePStorageSetAction();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      removeSetValueButton.setLayoutData(rd);
      removeSetValueButton.setEnabled(false);
      
      gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      viewerSetValue.getControl().setLayoutData(gd);
   }
   
   /**
    * Add new attribute
    */
   private void addPStorageSetAction()
   {
      SetEntryEditDialog dlg = new SetEntryEditDialog(getShell(), null, null, true, true);
      if (dlg.open() == Window.OK)
      {
         pStorageSet.put(dlg.getAtributeName(), dlg.getAttributeValue());
         viewerSetValue.setInput(pStorageSet.entrySet().toArray());
      }
   }
   
   /**
    * Edit set value
    */
   @SuppressWarnings("unchecked")
   private void editPStorageSetAction()
   {
      IStructuredSelection selection = (IStructuredSelection)viewerSetValue.getSelection();
      if (selection.size() != 1)
         return;
      
      Entry<String, String> attr = (Entry<String, String>)selection.getFirstElement();
      SetEntryEditDialog dlg = new SetEntryEditDialog(getShell(), attr.getKey(), attr.getValue(), true, false);
      if (dlg.open() == Window.OK)
      {
         pStorageSet.put(dlg.getAtributeName(), dlg.getAttributeValue());
         viewerSetValue.setInput(pStorageSet.entrySet().toArray());
      }
   }
   
   /**
    * Delete attribute(s) from list
    */
   private void deletePStorageSetAction()
   {
      IStructuredSelection selection = (IStructuredSelection)viewerSetValue.getSelection();
      Iterator<?> it = selection.iterator();
      if (it.hasNext())
      {
         while(it.hasNext())
         {
            @SuppressWarnings("unchecked")
            Entry<String, String> e = (Entry<String, String>) it.next();
            pStorageSet.remove(e.getKey());
         }
         viewerSetValue.setInput(pStorageSet.entrySet().toArray());
      }
   }

   /**
    * Put all new storage values
    * 
    * @param pStorageSet
    */
   public void putAll(Map<String, String> pStorageSet)
   {
      this.pStorageSet.putAll(pStorageSet);
      viewerSetValue.setInput(pStorageSet.entrySet().toArray());
   }

   /**
    * Put all new storage values
    * 
    * @param pStorageSet
    */
   public void setMap(Map<String, String> pStorageSet)
   {
      this.pStorageSet = pStorageSet;
   }

   /**
    * Get set
    */
   public Map<String, String> getSet()
   {
      return pStorageSet;
   }
}
