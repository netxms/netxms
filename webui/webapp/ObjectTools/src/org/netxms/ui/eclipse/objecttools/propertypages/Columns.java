/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.client.objecttools.ObjectToolTableColumn;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.dialogs.EditColumnDialog;
import org.netxms.ui.eclipse.objecttools.propertypages.helpers.ToolColumnLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Columns" property page for object tool
 */
public class Columns extends PropertyPage
{
	private static final long serialVersionUID = 1L;

	private ObjectToolDetails objectTool;
	private List<ObjectToolTableColumn> columns = new ArrayList<ObjectToolTableColumn>();
	private TableViewer viewer;
	private Button buttonAdd;
	private Button buttonEdit;
	private Button buttonRemove;

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
		for(ObjectToolTableColumn tc : objectTool.getColumns())
			columns.add(new ObjectToolTableColumn(tc));

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);
		
		viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		viewer.getTable().setLayoutData(gd);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new ToolColumnLabelProvider(objectTool.getType() == ObjectTool.TYPE_TABLE_SNMP));
		setupTableColumns();
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
				buttonEdit.setEnabled(selection.size() == 1);
				buttonRemove.setEnabled(selection.size() > 0);
			}
		});
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editColumn();
			}
		});
		viewer.setInput(columns.toArray());
		
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttons.setLayoutData(gd);

      buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText("&Add...");
      buttonAdd.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addColumn();
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);
		
      buttonEdit = new Button(buttons, SWT.PUSH);
      buttonEdit.setText("&Edit...");
      buttonEdit.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				editColumn();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonEdit.setLayoutData(rd);

      buttonRemove = new Button(buttons, SWT.PUSH);
      buttonRemove.setText("&Delete");
      buttonRemove.addSelectionListener(new SelectionListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeColumn();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);

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
		column.setText("Format");
		column.setWidth(90);
		
		column = new TableColumn(viewer.getTable(), SWT.LEFT);
		column.setText(objectTool.getType() == ObjectTool.TYPE_TABLE_SNMP ? "OID" : "Index");
		column.setWidth(200);
		
		viewer.getTable().setHeaderVisible(true);
		
		WidgetHelper.restoreColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "ColumnsPropertyPage");
	}

	/**
	 * Add new column
	 */
	private void addColumn()
	{
		ObjectToolTableColumn tc = new ObjectToolTableColumn("Column " + Integer.toString(columns.size() + 1));
		EditColumnDialog dlg = new EditColumnDialog(getShell(), true, objectTool.getType() == ObjectTool.TYPE_TABLE_SNMP, tc);
		if (dlg.open() == Window.OK)
		{
			columns.add(tc);
			viewer.setInput(columns.toArray());
			viewer.setSelection(new StructuredSelection(tc));
		}
	}
	
	/**
	 * Edit column
	 */
	private void editColumn()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() != 1)
			return;
		
		EditColumnDialog dlg = new EditColumnDialog(getShell(), false,
				objectTool.getType() == ObjectTool.TYPE_TABLE_SNMP, (ObjectToolTableColumn)selection.getFirstElement());
		dlg.open();
		viewer.update(selection.getFirstElement(), null);
	}
	
	/**
	 * Remove selected column(s)
	 */
	@SuppressWarnings("unchecked")
	private void removeColumn()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		Iterator<ObjectToolTableColumn> it = selection.iterator();
		while(it.hasNext())
		{
			columns.remove(it.next());
		}
		viewer.setInput(columns.toArray());
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		objectTool.setColumns(columns);
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
		WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "ColumnsPropertyPage");
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performCancel()
	 */
	@Override
	public boolean performCancel()
	{
		WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "ColumnsPropertyPage");
		return super.performCancel();
	}
}
