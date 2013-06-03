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
import org.eclipse.jface.dialogs.Dialog;
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
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.EditDciSummaryTableColumnDlg;
import org.netxms.ui.eclipse.datacollection.dialogs.SelectDciDialog;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.SummaryTableColumnLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Columns" page for DCI summary table
 */
public class SummaryTableColumns extends PropertyPage
{
	private static final String COLUMN_SETTINGS_PREFIX = "SummaryTableColumns.ColumnList"; //$NON-NLS-1$

	private DciSummaryTable table;
	private List<DciSummaryTableColumn> columns;
	private TableViewer viewer;
	private Button addButton;
	private Button modifyButton;
	private Button deleteButton;
	private Button upButton;
	private Button downButton;
	private Button importButton;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		table = (DciSummaryTable)getElement().getAdapter(DciSummaryTable.class);

		columns = new ArrayList<DciSummaryTableColumn>(table.getColumns().size());
		for(DciSummaryTableColumn c : table.getColumns())
			columns.add(new DciSummaryTableColumn(c));

		Composite dialogArea = new Composite(parent, SWT.NONE);

		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

		new Label(dialogArea, SWT.NONE).setText(Messages.SummaryTableColumns_Columns);

		viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalSpan = 2;
		viewer.getControl().setLayoutData(gd);
		setupColumnList();
		viewer.setInput(columns.toArray());

		Composite leftButtons = new Composite(dialogArea, SWT.NONE);
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
		upButton.setText(Messages.Thresholds_Up);
		upButton.setEnabled(false);
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

		downButton = new Button(leftButtons, SWT.PUSH);
		downButton.setText(Messages.Thresholds_Down);
		downButton.setEnabled(false);
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

		Composite buttons = new Composite(dialogArea, SWT.NONE);
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

		importButton = new Button(buttons, SWT.PUSH);
		importButton.setText(Messages.SummaryTableColumns_Import);
		RowData rd = new RowData();
		rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
		importButton.setLayoutData(rd);

		importButton.addSelectionListener(new SelectionListener() {

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				importColumns();
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

		addButton = new Button(buttons, SWT.PUSH);
		addButton.setText(Messages.Thresholds_Add);
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
		modifyButton.setText(Messages.Thresholds_Edit);
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
		deleteButton.setText(Messages.Thresholds_Delete);
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

		/*** Selection change listener for thresholds list ***/
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection selection = (IStructuredSelection)event.getSelection();
				int index = columns.indexOf(selection.getFirstElement());
				upButton.setEnabled((selection.size() == 1) && (index > 0));
				downButton.setEnabled((selection.size() == 1) && (index >= 0) && (index < columns.size() - 1));
				modifyButton.setEnabled(selection.size() == 1);
				deleteButton.setEnabled(selection.size() > 0);
			}
		});

		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				editColumn();
			}
		});

		return dialogArea;
	}

	/**
	 * Setup threshold list control
	 */
	private void setupColumnList()
	{
		Table table = viewer.getTable();
		table.setLinesVisible(true);
		table.setHeaderVisible(true);

		TableColumn column = new TableColumn(table, SWT.LEFT);
		column.setText(Messages.SummaryTableColumns_Name);
		column.setWidth(150);

		column = new TableColumn(table, SWT.LEFT);
		column.setText(Messages.SummaryTableColumns_DciName);
		column.setWidth(250);

		WidgetHelper.restoreColumnSettings(table, Activator.getDefault().getDialogSettings(), COLUMN_SETTINGS_PREFIX);

		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new SummaryTableColumnLabelProvider());
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);

		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.SummaryTableColumns_JobName, null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				synchronized(table)
				{
					table.getColumns().clear();
					table.getColumns().addAll(columns);
					int id = session.modifyDciSummaryTable(table);
					table.setId(id);
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.SummaryTableColumns_JobError;
			}

			/*
			 * (non-Javadoc)
			 * 
			 * @see org.netxms.ui.eclipse.jobs.ConsoleJob#jobFinalize()
			 */
			@Override
			protected void jobFinalize()
			{
				if (isApply)
				{
					runInUIThread(new Runnable() {
						@Override
						public void run()
						{
							SummaryTableColumns.this.setValid(true);
						}
					});
				}
			}
		}.start();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		saveSettings();
		applyChanges(false);
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		saveSettings();
		applyChanges(true);
	}

	/*
	 * (non-Javadoc)
	 * 
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
		WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), COLUMN_SETTINGS_PREFIX);
	}

	/**
	 * Move currently selected element up
	 */
	private void moveUp()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			final DciSummaryTableColumn column = (DciSummaryTableColumn)selection.getFirstElement();

			int index = columns.indexOf(column);
			if (index > 0)
			{
				Collections.swap(columns, index - 1, index);
				viewer.setInput(columns.toArray());
				viewer.setSelection(new StructuredSelection(column));
			}
		}
	}

	/**
	 * Move currently selected element down
	 */
	private void moveDown()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			final DciSummaryTableColumn column = (DciSummaryTableColumn)selection.getFirstElement();

			int index = columns.indexOf(column);
			if ((index < columns.size() - 1) && (index >= 0))
			{
				Collections.swap(columns, index + 1, index);
				viewer.setInput(columns.toArray());
				viewer.setSelection(new StructuredSelection(column));
			}
		}
	}

	/**
	 * Delete selected columns
	 */
	@SuppressWarnings("unchecked")
	private void deleteColumns()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (!selection.isEmpty())
		{
			Iterator<Threshold> it = selection.iterator();
			while(it.hasNext())
			{
				columns.remove(it.next());
			}
			viewer.setInput(columns.toArray());
		}
	}

	/**
	 * Edit selected column
	 */
	private void editColumn()
	{
		final IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		if (selection.size() == 1)
		{
			final DciSummaryTableColumn column = (DciSummaryTableColumn)selection.getFirstElement();
			EditDciSummaryTableColumnDlg dlg = new EditDciSummaryTableColumnDlg(getShell(), column);
			if (dlg.open() == Window.OK)
			{
				viewer.update(column, null);
			}
		}
	}

	/**
	 * Add new column
	 */
	private void addColumn()
	{
		DciSummaryTableColumn column = new DciSummaryTableColumn("", "", 0); //$NON-NLS-1$ //$NON-NLS-2$
		EditDciSummaryTableColumnDlg dlg = new EditDciSummaryTableColumnDlg(getShell(), column);
		if (dlg.open() == Window.OK)
		{
			columns.add(column);
			viewer.setInput(columns.toArray());
			viewer.setSelection(new StructuredSelection(column));
		}
	}

	/**
	 * Import Columns from node
	 */
	private void importColumns()
	{
		final SelectDciDialog dialog = new SelectDciDialog(getShell(), 0);
		dialog.setAllowTemplateItems(true);
		dialog.setEnableEmptySelection(false);
		if (dialog.open() == Dialog.OK)
		{
			final DciValue selection = dialog.getSelection();
			DciSummaryTableColumn column = new DciSummaryTableColumn(selection.getDescription(), selection.getName(), 0);
			columns.add(column);
			viewer.setInput(columns.toArray());
			viewer.setSelection(new StructuredSelection(column));
		}
	}
}
