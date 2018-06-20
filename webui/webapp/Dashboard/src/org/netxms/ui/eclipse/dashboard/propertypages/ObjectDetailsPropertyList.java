/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.propertypages;

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
import org.eclipse.jface.viewers.TableViewer;
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
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.EditObjectPropertyDialog;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.ObjectPropertiesLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectDetailsConfig.ObjectProperty;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Properties for "Object details" dashboard element
 */
public class ObjectDetailsPropertyList extends PropertyPage
{
   private ObjectDetailsConfig config;
   private List<ObjectProperty> properties;
   private TableViewer viewer;
   private Button addButton;
   private Button editButton;
   private Button deleteButton;
   private Button upButton;
   private Button downButton;
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = (ObjectDetailsConfig)getElement().getAdapter(ObjectDetailsConfig.class);
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.horizontalSpacing = WidgetHelper.BUTTON_WIDTH_HINT / 2;
      dialogArea.setLayout(layout);
      
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Properties to display");
      
      final String[] names = { "Name", "Display name", "Type" };
      final int[] widths = { 150, 250, 90 };
      viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.getTable().setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ObjectPropertiesLabelProvider());

      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      gd.widthHint = 300;
      gd.heightHint = 300;
      viewer.getControl().setLayoutData(gd);
      
      properties = new ArrayList<ObjectProperty>();
      if (config.getProperties() != null)
      {
         for(ObjectProperty p : config.getProperties())
            if (p != null)
               properties.add(new ObjectProperty(p));
      }
      viewer.setInput(properties.toArray());
      
      /* buttons on left side */
      Composite leftButtons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginLeft = 0;
      leftButtons.setLayout(buttonLayout);
      leftButtons.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, false, false));
      
      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText(Messages.get().EmbeddedDashboard_Up);
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
      downButton.setText(Messages.get().EmbeddedDashboard_Down);
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
      rightButtons.setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, false));

      addButton = new Button(rightButtons, SWT.PUSH);
      addButton.setText(Messages.get().EmbeddedDashboard_Add);
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
            addProperty();
         }
      });
      
      editButton = new Button(rightButtons, SWT.PUSH);
      editButton.setText(Messages.get().DashboardElements_Edit);
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
            editProperty();
         }
      });
      
      deleteButton = new Button(rightButtons, SWT.PUSH);
      deleteButton.setText(Messages.get().EmbeddedDashboard_Delete);
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
            deletePropertiess();
         }
      });
      deleteButton.setEnabled(false);
      
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            editButton.setEnabled(selection.size() == 1);
            deleteButton.setEnabled(selection.size() > 0);
            upButton.setEnabled(selection.size() == 1);
            downButton.setEnabled(selection.size() == 1);
         }
      });
      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editProperty();
         }
      });
      
      return dialogArea;
   }

   /**
    * Move selected element down
    */
   protected void moveDown()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() == 1)
      {
         Object element = selection.getFirstElement();
         int index = properties.indexOf(element);
         if ((index < properties.size() - 1) && (index >= 0))
         {
            Collections.swap(properties, index + 1, index);
            viewer.setInput(properties.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      }
   }

   /**
    * Move selected element up
    */
   protected void moveUp()
   {
      final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() == 1)
      {
         Object element = selection.getFirstElement();
         int index = properties.indexOf(element);
         if (index > 0)
         {
            Collections.swap(properties, index - 1, index);
            viewer.setInput(properties.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      }
   }

   /**
    * Delete selected properties from list
    */
   protected void deletePropertiess()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      for(Object o : selection.toList())
         properties.remove(o);
      viewer.setInput(properties.toArray());
   }

   /**
    * Add property to list
    */
   protected void addProperty()
   {
      EditObjectPropertyDialog dlg = new EditObjectPropertyDialog(getShell(), new ObjectProperty());
      if (dlg.open() == Window.OK)
      {
         properties.add(dlg.getProperty());
         viewer.setInput(properties.toArray());
      }
   }

   /**
    * Edit selected property
    */
   protected void editProperty()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      EditObjectPropertyDialog dlg = new EditObjectPropertyDialog(getShell(), (ObjectProperty)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         viewer.refresh();
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      config.setProperties(properties);
      return true;
   }
}
