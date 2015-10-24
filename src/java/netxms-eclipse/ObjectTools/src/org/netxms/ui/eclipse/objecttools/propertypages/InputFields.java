/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objecttools.propertypages;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
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
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.objecttools.InputField;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.Messages;
import org.netxms.ui.eclipse.objecttools.dialogs.EditInputFieldDialog;
import org.netxms.ui.eclipse.objecttools.propertypages.helpers.InputFieldLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Input Fields" property page for object tool
 */
public class InputFields extends PropertyPage
{
	private ObjectToolDetails objectTool;
	private List<InputField> fields = new ArrayList<InputField>();
	private TableViewer viewer;
	private Button buttonAdd;
	private Button buttonEdit;
	private Button buttonRemove;
	private Button buttonUp;
	private Button buttonDown;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createControl(Composite parent)
	{
		noDefaultAndApplyButton();
		super.createControl(parent);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		objectTool = (ObjectToolDetails)getElement().getAdapter(ObjectToolDetails.class);
		for(InputField f : objectTool.getInputFields())
			fields.add(new InputField(f));

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalSpan = 2;
		gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT * 6;
		viewer.getTable().setLayoutData(gd);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new InputFieldLabelProvider());
		viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((InputField)e1).getSequence() - ((InputField)e2).getSequence();
         }
		});
		setupTableColumns();
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				buttonEdit.setEnabled(selection.size() == 1);
				buttonRemove.setEnabled(selection.size() > 0);
            buttonUp.setEnabled(selection.size() == 1);
            buttonDown.setEnabled(selection.size() == 1);
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editField();
			}
		});
		viewer.setInput(fields.toArray());

      Composite buttonsLeft = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonsLeft.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttonsLeft.setLayoutData(gd);
		
      buttonUp = new Button(buttonsLeft, SWT.PUSH);
      buttonUp.setText("&Up");
      buttonUp.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveFieldUp();
         }
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonUp.setLayoutData(rd);
      buttonUp.setEnabled(false);
      
      buttonDown = new Button(buttonsLeft, SWT.PUSH);
      buttonDown.setText("&Down");
      buttonDown.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveFieldDown();
         }
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonDown.setLayoutData(rd);
      buttonDown.setEnabled(false);
      
      Composite buttonsRight = new Composite(dialogArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonsRight.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttonsRight.setLayoutData(gd);

      buttonAdd = new Button(buttonsRight, SWT.PUSH);
      buttonAdd.setText(Messages.get().Columns_Add);
      buttonAdd.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

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
      buttonEdit.setText(Messages.get().Columns_Edit);
      buttonEdit.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editField();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);
      buttonEdit.setEnabled(false);

      buttonRemove = new Button(buttonsRight, SWT.PUSH);
      buttonRemove.setText(Messages.get().Columns_Delete);
      buttonRemove.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeField();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);
      buttonRemove.setEnabled(false);

      return dialogArea;
	}
	
	/**
	 * Setup table viewer columns
	 */
	private void setupTableColumns()
	{
		TableColumn column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setText("Name");
		column.setWidth(200);
		
		column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setText("Type");
		column.setWidth(90);
		
		column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setText("Display name");
		column.setWidth(200);
		
		viewer.getTable().setHeaderVisible(true);
		
		WidgetHelper.restoreColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InputFieldsPropertyPage"); //$NON-NLS-1$
	}

	/**
	 * Add new field
	 */
	private void addField()
	{
		InputField f = new InputField("Field" + Integer.toString(fields.size() + 1));
		EditInputFieldDialog dlg = new EditInputFieldDialog(getShell(), true, f);
		if (dlg.open() == Window.OK)
		{
		   if (nameIsUnique(f.getName()))
		   {
		      f.setSequence(fields.size());
   			fields.add(f);
   			viewer.setInput(fields.toArray());
   			viewer.setSelection(new StructuredSelection(f));
		   }
		}
	}
	
	/**
	 * Check if field name is unique
	 * 
	 * @param name
	 * @return
	 */
	private boolean nameIsUnique(String name)
   {
	   for(InputField f : fields)
	      if (f.getName().equalsIgnoreCase(name))
	         return false;
      return true;
   }

   /**
	 * Edit column
	 */
	private void editField()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		EditInputFieldDialog dlg = new EditInputFieldDialog(getShell(), false, (InputField)selection.getFirstElement());
		if (dlg.open() == Window.OK)
		{
		   viewer.update(selection.getFirstElement(), null);
		}
	}
	
	/**
	 * Remove selected column(s)
	 */
	@SuppressWarnings("unchecked")
	private void removeField()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<InputField> it = selection.iterator();
		while(it.hasNext())
		{
			fields.remove(it.next());
		}
		viewer.setInput(fields.toArray());
	}
	
	/**
	 * Move selected field up 
	 */
	private void moveFieldUp()
	{
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      InputField f = (InputField)selection.getFirstElement();
      if (f.getSequence() > 0)
      {
         updateSequence(f.getSequence() - 1, 1);
         f.setSequence(f.getSequence() - 1);
         viewer.refresh();
      }
	}
	
   /**
    * Move selected field down 
    */
   private void moveFieldDown()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;
      
      InputField f = (InputField)selection.getFirstElement();
      if (f.getSequence() < fields.size() - 1)
      {
         updateSequence(f.getSequence() + 1, -1);
         f.setSequence(f.getSequence() + 1);
         viewer.refresh();
      }
   }
   
   /**
    * @param curr
    * @param delta
    */
   private void updateSequence(int curr, int delta)
   {
      for(InputField f : fields)
      {
         if (f.getSequence() == curr)
         {
            f.setSequence(curr + delta);
            break;
         }
      }
   }
   
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		objectTool.setInputFields(fields);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InputFieldsPropertyPage"); //$NON-NLS-1$
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performCancel()
	 */
	@Override
	public boolean performCancel()
	{
		WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InputFieldsPropertyPage"); //$NON-NLS-1$
		return super.performCancel();
	}
}
