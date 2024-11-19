/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2022 Raden Solutions
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
package org.netxms.nxmc.base.widgets;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.base.dialogs.KeyValuePairEditDialog;
import org.netxms.nxmc.base.widgets.helpers.KeyValuePairLabelProvider;
import org.netxms.nxmc.tools.ElementLabelComparator;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Editor for generic set of key/value pairs
 */
public class KeyValueSetEditor extends Composite
{
   private SortableTableViewer viewer;
   private Button buttonAdd;
   private Button buttonEdit;
   private Button buttonRemove;
   private String keyLabel;
   private String valueLabel;
   private Map<String, String> content = new HashMap<String, String>();

   /**
    * Create new editor.
    *
    * @param parent parent composite
    * @param style widget style
    */
   public KeyValueSetEditor(Composite parent, int style, String keyLabel, String valueLabel)
   {
      super(parent, style);
      this.keyLabel = keyLabel;
      this.valueLabel = valueLabel;

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      this.setLayout(layout);

      final String[] setColumnNames = { keyLabel, valueLabel };
      final int[] setColumnWidths = { 150, 300 };
      viewer = new SortableTableViewer(this, setColumnNames, setColumnWidths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new KeyValuePairLabelProvider());
      viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
      viewer.setInput(content.entrySet().toArray());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            int size = viewer.getStructuredSelection().size();
            buttonEdit.setEnabled(size == 1);
            buttonRemove.setEnabled(size > 0);
         }
      });
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editEntry();
         }
      });

      GridData gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      viewer.getControl().setLayoutData(gd);

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

      buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText("&Add...");
      buttonAdd.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addEntry();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);

      buttonEdit = new Button(buttons, SWT.PUSH);
      buttonEdit.setText("&Edit...");
      buttonEdit.addSelectionListener(new SelectionAdapter() {
        @Override
        public void widgetSelected(SelectionEvent e)
        {
           editEntry();
        }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);
      buttonEdit.setEnabled(false);

      buttonRemove = new Button(buttons, SWT.PUSH);
      buttonRemove.setText("&Delete");
      buttonRemove.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            deleteEntry();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);
      buttonRemove.setEnabled(false);

      gd = new GridData();
      gd.verticalAlignment = GridData.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.grabExcessHorizontalSpace = true;
      viewer.getControl().setLayoutData(gd);
   }

   /**
    * Add new entry
    */
   private void addEntry()
   {
      KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getShell(), null, null, true, true, keyLabel, valueLabel);
      if (dlg.open() == Window.OK)
      {
         content.put(dlg.getKey(), dlg.getValue());
         viewer.setInput(content.entrySet().toArray());
      }
   }
   
   /**
    * Edit selected entry
    */
   @SuppressWarnings("unchecked")
   private void editEntry()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Entry<String, String> attr = (Entry<String, String>)selection.getFirstElement();
      KeyValuePairEditDialog dlg = new KeyValuePairEditDialog(getShell(), attr.getKey(), attr.getValue(), true, false, keyLabel, valueLabel);
      if (dlg.open() == Window.OK)
      {
         content.put(dlg.getKey(), dlg.getValue());
         viewer.setInput(content.entrySet().toArray());
      }
   }

   /**
    * Delete selected entries
    */
   private void deleteEntry()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      Iterator<?> it = selection.iterator();
      if (it.hasNext())
      {
         while(it.hasNext())
         {
            @SuppressWarnings("unchecked")
            Entry<String, String> e = (Entry<String, String>) it.next();
            content.remove(e.getKey());
         }
         viewer.setInput(content.entrySet().toArray());
      }
   }

   /**
    * Add all provided key/value pairs to editor's content (existing keys will be replaced).
    * 
    * @param entries set of key/value pairs to add
    */
   public void addAll(Map<String, String> entries)
   {
      content.putAll(entries);
      viewer.setInput(entries.entrySet().toArray());
   }

   /**
    * Replace current content with provided one. Editor will use provided map as new content, any changes to it will be reflected in
    * the editor.
    * 
    * @param entries new set of entries
    */
   public void setContent(Map<String, String> entries)
   {
      this.content = entries;
   }

   /**
    * Get editor content. Changes to returned map will change content of the editor.
    */
   public Map<String, String> getContent()
   {
      return content;
   }
}
