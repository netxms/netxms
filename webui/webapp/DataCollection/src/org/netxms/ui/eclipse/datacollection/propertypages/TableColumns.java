/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.propertypages;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.IInputValidator;
import org.eclipse.jface.dialogs.InputDialog;
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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
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
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectEditor;
import org.netxms.ui.eclipse.datacollection.api.DataCollectionObjectListener;
import org.netxms.ui.eclipse.datacollection.dialogs.EditColumnDialog;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.TableColumnLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Columns" property page for table DCI
 */
public class TableColumns extends PropertyPage
{
	private static final String COLUMN_SETTINGS_PREFIX = "TableColumns.ColumnList"; //$NON-NLS-1$
	
	private DataCollectionObjectEditor editor;
	private DataCollectionTable dci;
	private List<ColumnDefinition> columns;
	private TableViewer columnList;
	private Button addButton;
	private Button modifyButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		editor = (DataCollectionObjectEditor)getElement().getAdapter(DataCollectionObjectEditor.class);
		dci = editor.getObjectAsTable();

		columns = new ArrayList<ColumnDefinition>();
		for(ColumnDefinition c : dci.getColumns())
			columns.add(new ColumnDefinition(c));
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      Composite columnListArea = new Composite(dialogArea, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      columnListArea.setLayoutData(gd);
      layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      columnListArea.setLayout(layout);
	
      new Label(columnListArea, SWT.NONE).setText(Messages.TableColumns_Columns);
      
      columnList = new TableViewer(columnListArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalSpan = 2;
      columnList.getControl().setLayoutData(gd);
      setupColumnList();
      columnList.setInput(columns.toArray());
      
      Composite leftButtons = new Composite(columnListArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      leftButtons.setLayoutData(gd);
      RowLayout buttonsLayout = new RowLayout(SWT.HORIZONTAL);
      buttonsLayout.marginBottom = 0;
      buttonsLayout.marginLeft = 0;
      buttonsLayout.marginRight = 0;
      buttonsLayout.marginTop = 0;
      buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.fill = true;
      buttonsLayout.pack = false;
      leftButtons.setLayout(buttonsLayout);

      upButton = new Button(leftButtons, SWT.PUSH);
      upButton.setText("&Up");
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
				moveSelectionUp();
			}
		});

      downButton = new Button(leftButtons, SWT.PUSH);
      downButton.setText("Do&wn");
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
				moveSelectionDown();
			}
		});

      Composite buttons = new Composite(columnListArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      buttons.setLayoutData(gd);
      buttonsLayout = new RowLayout(SWT.HORIZONTAL);
      buttonsLayout.marginBottom = 0;
      buttonsLayout.marginLeft = 0;
      buttonsLayout.marginRight = 0;
      buttonsLayout.marginTop = 0;
      buttonsLayout.spacing = WidgetHelper.OUTER_SPACING;
      buttonsLayout.fill = true;
      buttonsLayout.pack = false;
      buttons.setLayout(buttonsLayout);
      
      addButton = new Button(buttons, SWT.PUSH);
      addButton.setText(Messages.TableColumns_Add);
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
				addColumn();
			}
		});
      
      modifyButton = new Button(buttons, SWT.PUSH);
      modifyButton.setText(Messages.TableColumns_Edit);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      modifyButton.setLayoutData(rd);
      modifyButton.setEnabled(false);
      modifyButton.addSelectionListener(new SelectionListener() {
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
      
      deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText(Messages.TableColumns_Delete);
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      deleteButton.setLayoutData(rd);
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				deleteColumns();
			}
      });
      
      /*** Selection change listener for column list ***/
      columnList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				deleteButton.setEnabled(selection.size() > 0);
				if (selection.size() == 1)
				{
					modifyButton.setEnabled(true);
					upButton.setEnabled(columns.indexOf(selection.getFirstElement()) > 0);
					downButton.setEnabled(columns.indexOf(selection.getFirstElement()) < columns.size() - 1);
				}
				else
				{
					modifyButton.setEnabled(false);
					upButton.setEnabled(false);
					downButton.setEnabled(false);
				}
			}
      });
      
      columnList.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editColumn();
			}
      });
      
      final DataCollectionObjectListener listener = new DataCollectionObjectListener() {
			@Override
			public void onSelectItem(int origin, String name, String description, int dataType)
			{
			}

			@Override
			public void onSelectTable(int origin, String name, String description)
			{
				if (origin == DataCollectionObject.AGENT)
					updateColumnsFromAgent(name);
			}
		};
		dialogArea.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				editor.removeListener(listener);
			}
		});
		editor.addListener(listener);

      return dialogArea;
	}

	/**
	 * Setup threshold list control
	 */
	private void setupColumnList()
	{
		Table table = columnList.getTable();
		table.setLinesVisible(true);
		table.setHeaderVisible(true);
		
		TableColumn column = new TableColumn(table, SWT.LEFT);
		column.setText(Messages.TableColumns_Name);
		column.setWidth(150);
		
		column = new TableColumn(table, SWT.LEFT);
		column.setText(Messages.TableColumns_DisplayName);
		column.setWidth(150);
		
		column = new TableColumn(table, SWT.LEFT);
		column.setText(Messages.TableColumns_Type);
		column.setWidth(80);
		
		column = new TableColumn(table, SWT.LEFT);
		column.setText("Instance");
		column.setWidth(50);
		
		column = new TableColumn(table, SWT.LEFT);
		column.setText("Aggregation");
		column.setWidth(80);
		
		column = new TableColumn(table, SWT.LEFT);
		column.setText(Messages.TableColumns_OID);
		column.setWidth(200);
		
		WidgetHelper.restoreColumnSettings(table, Activator.getDefault().getDialogSettings(), COLUMN_SETTINGS_PREFIX);
		
		columnList.setContentProvider(new ArrayContentProvider());
		columnList.setLabelProvider(new TableColumnLabelProvider());
	}

	/**
	 * Delete selected columns
	 */
	@SuppressWarnings("unchecked")
	private void deleteColumns()
	{
		final IStructuredSelection selection = (IStructuredSelection)columnList.getSelection();
		if (!selection.isEmpty())
		{
			Iterator<ColumnDefinition> it = selection.iterator();
			while(it.hasNext())
			{
				columns.remove(it.next());
			}
	      columnList.setInput(columns.toArray());
		}
	}
	
	/**
	 * Edit selected threshold
	 */
	private void editColumn()
	{
		final IStructuredSelection selection = (IStructuredSelection)columnList.getSelection();
		if (selection.size() == 1)
		{
			final ColumnDefinition column = (ColumnDefinition)selection.getFirstElement();
			EditColumnDialog dlg = new EditColumnDialog(getShell(), column);
			if (dlg.open() == Window.OK)
			{
				columnList.update(column, null);
			}
		}
	}
	
	/**	
	 * Add new threshold
	 */
	private void addColumn()
	{
		final InputDialog idlg = new InputDialog(getShell(), Messages.TableColumns_NewColumn, Messages.TableColumns_ColumnName, "", new IInputValidator() { //$NON-NLS-1$
			@Override
			public String isValid(String newText)
			{
				if (newText.trim().isEmpty())
					return Messages.TableColumns_WarningText;
				return null;
			}
		});
		if (idlg.open() == Window.OK)
		{
			final ColumnDefinition column = new ColumnDefinition(idlg.getValue(), idlg.getValue());
			final EditColumnDialog dlg = new EditColumnDialog(getShell(), column);
			if (dlg.open() == Window.OK)
			{
				columns.add(column);
		      columnList.setInput(columns.toArray());
		      columnList.setSelection(new StructuredSelection(column));
			}
		}
	}
	
	/**
	 * Move selected element up 
	 */
	private void moveSelectionUp()
	{
		final IStructuredSelection selection = (IStructuredSelection)columnList.getSelection();
		if (selection.size() != 1)
			return;
		
		final ColumnDefinition column = (ColumnDefinition)selection.getFirstElement();
		int index = columns.indexOf(column);
		if (index > 0)
		{
			Collections.swap(columns, index, index - 1);
			columnList.setInput(columns.toArray());
			columnList.setSelection(new StructuredSelection(column));
		}
	}
	
	/**
	 * Move selected element down
	 */
	private void moveSelectionDown()
	{
		final IStructuredSelection selection = (IStructuredSelection)columnList.getSelection();
		if (selection.size() != 1)
			return;
		
		final ColumnDefinition column = (ColumnDefinition)selection.getFirstElement();
		int index = columns.indexOf(column);
		if (index < columns.size() - 1)
		{
			Collections.swap(columns, index, index + 1);
			columnList.setInput(columns.toArray());
			columnList.setSelection(new StructuredSelection(column));
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		dci.getColumns().clear();
		dci.getColumns().addAll(columns);
		editor.modify();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		saveSettings();
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		saveSettings();
		applyChanges(false);
		return true;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performCancel()
	 */
	@Override
	public boolean performCancel()
	{
		saveSettings();
		return true;
	}
	
	/**
	 * Save settings
	 */
	private void saveSettings()
	{
		WidgetHelper.saveColumnSettings(columnList.getTable(), Activator.getDefault().getDialogSettings(), COLUMN_SETTINGS_PREFIX);
	}
	
	/**
	 * Update columns from real table
	 */
	private void updateColumnsFromAgent(final String name)
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		ConsoleJob job = new ConsoleJob(Messages.TableColumns_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				try
				{
					final org.netxms.client.Table table = session.queryAgentTable(dci.getNodeId(), name);
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							columns.clear();
							for(int i = 0; i < table.getColumnCount(); i++)
							{
								ColumnDefinition c = new ColumnDefinition(table.getColumnName(i), table.getColumnDisplayName(i));
								c.setDataType(table.getColumnDefinition(i).getDataType());
								c.setInstanceColumn(table.getColumnDefinition(i).isInstanceColumn());
								columns.add(c);
							}
							columnList.setInput(columns.toArray());
						}
					});
				}
				catch(Exception e)
				{
					// TODO: add logging
					e.printStackTrace();
				}
			}
			
			@Override
			protected String getErrorMessage()
			{
				return null;
			}
		};
		job.setUser(false);
		job.start();
	}
}
