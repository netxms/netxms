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
import org.netxms.ui.eclipse.dashboard.dialogs.EditToolDialog;
import org.netxms.ui.eclipse.dashboard.propertypages.helpers.ToolListLabelProvider;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectToolsConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectToolsConfig.Tool;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledSpinner;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Properties for "Object tools" dashboard element
 */
public class ObjectTools extends PropertyPage
{
   private ObjectToolsConfig config;
   private LabeledText title;
   private LabeledSpinner columns;
   private List<Tool> tools;
   private TableViewer viewer;
   private Button addButton;
   private Button deleteButton;
   private Button upButton;
   private Button downButton;
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      config = (ObjectToolsConfig)getElement().getAdapter(ObjectToolsConfig.class);
      
      Composite dialogArea = new Composite(parent, SWT.NONE);
      
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      dialogArea.setLayout(layout);
      
      title = new LabeledText(dialogArea, SWT.NONE);
      title.setLabel("Title");
      title.setText(config.getTitle());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      title.setLayoutData(gd);
      
      columns = new LabeledSpinner(dialogArea, SWT.NONE);
      columns.setLabel("Columns");
      columns.setRange(1, 32);
      columns.setSelection(config.getNumColumns());
      
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Tools");
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      label.setLayoutData(gd);
      
      viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ToolListLabelProvider());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 0;
      viewer.getTable().setLayoutData(gd);

      tools = new ArrayList<Tool>();
      if (config.getTools() != null)
      {
         for(Tool t : config.getTools())
            if (t != null)
               tools.add(new Tool(t));
      }
      viewer.setInput(tools.toArray());
      
      Composite buttonsArea = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      buttonsArea.setLayout(layout);
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      buttonsArea.setLayoutData(gd);

      /* buttons on left side */
      Composite leftButtons = new Composite(buttonsArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginLeft = 0;
      leftButtons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.grabExcessHorizontalSpace = true;
      leftButtons.setLayoutData(gd);
      
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
      Composite rightButtons = new Composite(buttonsArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      rightButtons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      rightButtons.setLayoutData(gd);

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
            addTool();
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
            deleteTools();
         }
      });
      deleteButton.setEnabled(false);
      
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
            deleteButton.setEnabled(selection.size() > 0);
            upButton.setEnabled(selection.size() == 1);
            downButton.setEnabled(selection.size() == 1);
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
         int index = tools.indexOf(element);
         if ((index < tools.size() - 1) && (index >= 0))
         {
            Collections.swap(tools, index + 1, index);
            viewer.setInput(tools.toArray());
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
         int index = tools.indexOf(element);
         if (index > 0)
         {
            Collections.swap(tools, index - 1, index);
            viewer.setInput(tools.toArray());
            viewer.setSelection(new StructuredSelection(element));
         }
      }
   }

   /**
    * Delete selected tools from list
    */
   protected void deleteTools()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      for(Object o : selection.toList())
         tools.remove(o);
      viewer.setInput(tools.toArray());
   }

   /**
    * Add tool to list
    */
   protected void addTool()
   {
      EditToolDialog dlg = new EditToolDialog(getShell(), new Tool());
      if (dlg.open() == Window.OK)
      {
         tools.add(dlg.getTool());
         viewer.setInput(tools.toArray());
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      config.setTitle(title.getText().trim());
      config.setNumColumns(columns.getSelection());
      config.setTools(tools.toArray(new Tool[tools.size()]));
      return true;
   }
}
