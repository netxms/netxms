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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableColumn;
import org.netxms.client.InputField;
import org.netxms.client.objects.queries.ObjectQuery;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.dialogs.helpers.InputFieldLabelProvider;
import org.netxms.nxmc.modules.objecttools.dialogs.InputFieldEditDialog;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Object query edit dialog
 */
public class ObjectQueryEditDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectQueryEditDialog.class);

   private ObjectQuery query;
   private List<InputField> inputFields = new ArrayList<InputField>();
   private LabeledText name;
   private LabeledText description;
   private TableViewer inputFieldsViewer;
   private Button buttonUp;
   private Button buttonDown;
   private Button buttonAdd;
   private Button buttonEdit;
   private Button buttonRemove;
   private ScriptEditor source;

   /**
    * @param parentShell parent shell
    * @param query query to edit or null to create new
    */
   public ObjectQueryEditDialog(Shell parentShell, ObjectQuery query)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.query = query;
      if (query != null)
         inputFields.addAll(query.getInputFields());
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText((query != null) ? i18n.tr("Edit Object Query") : i18n.tr("Create Object Query"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Name"));
      name.setText((query != null) ? query.getName() : "");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = false;
      gd.widthHint = 300;
      name.setLayoutData(gd);

      Composite sourceGroup = new Composite(dialogArea, SWT.NONE);
      layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = WidgetHelper.INNER_SPACING;
      sourceGroup.setLayout(layout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 3;
      sourceGroup.setLayoutData(gd);

      new Label(sourceGroup, SWT.NONE).setText(i18n.tr("Source"));

      source = new ScriptEditor(sourceGroup, SWT.BORDER, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL,  
            "Columns can be defined as global variables or as \"with\" statement variables.\n\nVariables:\n\t$INPUT\t\tmap with all input values where array elements are indexed by field name like: $INPUT[\"Field1\"];\n\tAll object attributes are available without dereference, so e.g. $node->primaryHostName can be referred just as primaryHostName\n\nReturn value: true if currently filtered object should be included in resulting set");
      source.setText((query != null) ? query.getSource() : "");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 600;
      gd.heightHint = 500;
      source.setLayoutData(gd);

      description = new LabeledText(dialogArea, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.WRAP);
      description.setLabel(i18n.tr("Description"));
      description.setText((query != null) ? query.getDescription() : "");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = false;
      gd.heightHint = 140;
      description.setLayoutData(gd);

      Group inputFieldsGroup = new Group(dialogArea, SWT.NONE);
      inputFieldsGroup.setText(i18n.tr("Input Fields"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      inputFieldsGroup.setLayoutData(gd);

      layout = new GridLayout();
      layout.numColumns = 2;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      inputFieldsGroup.setLayout(layout);

      inputFieldsViewer = new TableViewer(inputFieldsGroup, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      setupInputFieldsViewer();
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      inputFieldsViewer.getControl().setLayoutData(gd);

      Composite buttonsLeft = new Composite(inputFieldsGroup, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonsLeft.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gd);

      buttonUp = new Button(buttonsLeft, SWT.PUSH);
      buttonUp.setText(i18n.tr("&Up"));
      buttonUp.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveInputFieldUp();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonUp.setLayoutData(rd);
      buttonUp.setEnabled(false);

      buttonDown = new Button(buttonsLeft, SWT.PUSH);
      buttonDown.setText(i18n.tr("Dow&n"));
      buttonDown.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveInputFieldDown();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonDown.setLayoutData(rd);
      buttonDown.setEnabled(false);

      Composite buttonsRight = new Composite(inputFieldsGroup, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonsRight.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gd);

      buttonAdd = new Button(buttonsRight, SWT.PUSH);
      buttonAdd.setText(i18n.tr("&Add..."));
      buttonAdd.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addField();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);

      buttonEdit = new Button(buttonsRight, SWT.PUSH);
      buttonEdit.setText("&Edit...");
      buttonEdit.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editInputField();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);
      buttonEdit.setEnabled(false);

      buttonRemove = new Button(buttonsRight, SWT.PUSH);
      buttonRemove.setText(i18n.tr("&Delete"));
      buttonRemove.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeInputField();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);
      buttonRemove.setEnabled(false);

      return dialogArea;
   }

   /**
    * Setup viewer for input fields
    */
   private void setupInputFieldsViewer()
   {
      TableColumn column = new TableColumn(inputFieldsViewer.getTable(), SWT.LEFT);
      column.setText(i18n.tr("Name"));
      column.setWidth(200);

      column = new TableColumn(inputFieldsViewer.getTable(), SWT.LEFT);
      column.setText(i18n.tr("Type"));
      column.setWidth(90);

      column = new TableColumn(inputFieldsViewer.getTable(), SWT.LEFT);
      column.setText(i18n.tr("Display name"));
      column.setWidth(200);

      inputFieldsViewer.getTable().setHeaderVisible(true);

      WidgetHelper.restoreColumnSettings(inputFieldsViewer.getTable(), "ObjectQueryInputFields");
      inputFieldsViewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(inputFieldsViewer.getTable(), "ObjectQueryInputFields");
         }
      });

      inputFieldsViewer.setContentProvider(new ArrayContentProvider());
      inputFieldsViewer.setLabelProvider(new InputFieldLabelProvider());
      inputFieldsViewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((InputField)e1).getSequence() - ((InputField)e2).getSequence();
         }
      });
      inputFieldsViewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = inputFieldsViewer.getStructuredSelection();
            buttonEdit.setEnabled(selection.size() == 1);
            buttonRemove.setEnabled(selection.size() > 0);
            buttonUp.setEnabled(selection.size() == 1);
            buttonDown.setEnabled(selection.size() == 1);
         }
      });
      inputFieldsViewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editInputField();
         }
      });
      inputFieldsViewer.setInput(inputFields.toArray());
   }

   /**
    * Add new field
    */
   private void addField()
   {
      InputField f = new InputField("Field" + Integer.toString(inputFields.size() + 1));
      InputFieldEditDialog dlg = new InputFieldEditDialog(getShell(), true, f);
      if (dlg.open() == Window.OK)
      {
         if (inputFieldNameIsUnique(f.getName()))
         {
            f.setSequence(inputFields.size());
            inputFields.add(f);
            inputFieldsViewer.setInput(inputFields.toArray());
            inputFieldsViewer.setSelection(new StructuredSelection(f));
         }
      }
   }

   /**
    * Check if input field name is unique
    * 
    * @param name field name to check
    * @return true if given name is unique
    */
   private boolean inputFieldNameIsUnique(String name)
   {
      for(InputField f : inputFields)
         if (f.getName().equalsIgnoreCase(name))
            return false;
      return true;
   }

   /**
    * Edit input field
    */
   private void editInputField()
   {
      IStructuredSelection selection = inputFieldsViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      InputFieldEditDialog dlg = new InputFieldEditDialog(getShell(), false, (InputField)selection.getFirstElement());
      if (dlg.open() == Window.OK)
      {
         inputFieldsViewer.update(selection.getFirstElement(), null);
      }
   }

   /**
    * Remove selected input field(s)
    */
   @SuppressWarnings("unchecked")
   private void removeInputField()
   {
      IStructuredSelection selection = inputFieldsViewer.getStructuredSelection();
      Iterator<InputField> it = selection.iterator();
      while(it.hasNext())
      {
         inputFields.remove(it.next());
      }
      inputFieldsViewer.setInput(inputFields.toArray());
   }

   /**
    * Move selected field up
    */
   private void moveInputFieldUp()
   {
      IStructuredSelection selection = inputFieldsViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      InputField f = (InputField)selection.getFirstElement();
      if (f.getSequence() > 0)
      {
         updateInputFieldSequence(f.getSequence() - 1, 1);
         f.setSequence(f.getSequence() - 1);
         inputFieldsViewer.refresh();
      }
   }

   /**
    * Move selected field down
    */
   private void moveInputFieldDown()
   {
      IStructuredSelection selection = inputFieldsViewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      InputField f = (InputField)selection.getFirstElement();
      if (f.getSequence() < inputFields.size() - 1)
      {
         updateInputFieldSequence(f.getSequence() + 1, -1);
         f.setSequence(f.getSequence() + 1);
         inputFieldsViewer.refresh();
      }
   }

   /**
    * @param curr
    * @param delta
    */
   private void updateInputFieldSequence(int curr, int delta)
   {
      for(InputField f : inputFields)
      {
         if (f.getSequence() == curr)
         {
            f.setSequence(curr + delta);
            break;
         }
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (query == null)
      {
         query = new ObjectQuery(name.getText().trim(), description.getText().trim(), source.getText());
      }
      else
      {
         query.setName(name.getText().trim());
         query.setDescription(description.getText().trim());
         query.setSource(source.getText());
      }
      query.setInputFileds(inputFields);
      super.okPressed();
   }

   /**
    * Get object query being edited.
    *
    * @return object query being edited
    */
   public ObjectQuery getQuery()
   {
      return query;
   }
}
