/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.dialogs;

import java.util.ArrayList;
import java.util.Set;
import java.util.Map.Entry;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.ui.eclipse.logviewer.dialogs.helpers.FilterTreeContentProvider;
import org.netxms.ui.eclipse.logviewer.dialogs.helpers.FilterTreeElement;
import org.netxms.ui.eclipse.logviewer.dialogs.helpers.FilterTreeLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class QueryBuilder extends Dialog
{
	private Log logHandle;
	private LogFilter filter;
	private TreeViewer filterTree;
	private ArrayList<FilterTreeElement> elements;
	private FilterTreeElement rootElement;
	private Button buttonAddColumn;
	private Button buttonAddSet;
	private Button buttonAddOperation;
	private Button buttonRemove;
	
	/**
	 * @param parentShell
	 */
	public QueryBuilder(Shell parentShell, Log logHandle, LogFilter filter)
	{
		super(parentShell);
		this.logHandle = logHandle;
		this.filter = filter;
		createInternalTree(filter);
	}
	
	/**
	 * Create internal tree-like representation of log filter
	 * 
	 * @param filter Initial log filter
	 */
	private void createInternalTree(final LogFilter filter)
	{
		elements = new ArrayList<FilterTreeElement>();
		
		rootElement = new FilterTreeElement(FilterTreeElement.ROOT, null, null);
		elements.add(rootElement);
		
		final Set<Entry<String, ColumnFilter>> columnFilters = filter.getColumnFilters();
		for(final Entry<String, ColumnFilter> cf : columnFilters)
		{
			final LogColumn lc = logHandle.getColumn(cf.getKey());
			if (lc != null)
			{
				final FilterTreeElement column = new FilterTreeElement(FilterTreeElement.COLUMN, lc, rootElement);
				addColumnFilter(column, cf.getValue());
			}
		}
	}

	/**
	 * Add column filter to given tree element
	 * 
	 * @param parent Tree element
	 * @param filter Filter to add
	 */
	private void addColumnFilter(final FilterTreeElement parent, final ColumnFilter filter)
	{
		FilterTreeElement entry = new FilterTreeElement(FilterTreeElement.FILTER, filter, parent);
		if (filter.getType() == ColumnFilter.SET)
		{
			for(final ColumnFilter f : filter.getSubFilters())
			{
				addColumnFilter(entry, f);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		/* filter tree */
		filterTree = new TreeViewer(dialogArea, SWT.SINGLE | SWT.BORDER);
		filterTree.setContentProvider(new FilterTreeContentProvider());
		filterTree.setLabelProvider(new FilterTreeLabelProvider());
		// We cannot simply pass rootElement to setInput(). Note from IStructuredContentProvider.getElements(): 
		// For instances where the viewer is displaying a tree containing a single 'root' element 
		// it is still necessary that the 'input' does not return itself from this method. This leads to 
		// recursion issues (see bug 9262). 
		ArrayList<FilterTreeElement> input = new ArrayList<FilterTreeElement>(1);
		input.add(rootElement);
		filterTree.setInput(input);
		
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.widthHint = 400;
		gd.heightHint = 300;
		filterTree.getControl().setLayoutData(gd);
		
		/* buttons */
		Composite buttons = new Composite(dialogArea, SWT.NONE);
		RowLayout buttonLayout = new RowLayout();
		buttonLayout.type = SWT.VERTICAL;
		buttonLayout.fill = true;
		buttonLayout.pack = false;
		buttonLayout.marginTop = 0;
		buttonLayout.spacing = WidgetHelper.OUTER_SPACING;
		buttons.setLayout(buttonLayout);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		buttons.setLayoutData(gd);
		
		buttonAddColumn = new Button(buttons, SWT.PUSH);
		buttonAddColumn.setText("Add &column filter");
		buttonAddColumn.addSelectionListener(new SelectionListener() {
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

		buttonAddSet = new Button(buttons, SWT.PUSH);
		buttonAddSet.setText("Add filter &set");
		buttonAddSet.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addFilterSet();
			}
		});
		
		buttonAddOperation = new Button(buttons, SWT.PUSH);
		buttonAddOperation.setText("Add &condition");
		buttonAddOperation.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
			}
		});
		
		buttonRemove = new Button(buttons, SWT.PUSH);
		buttonRemove.setText("&Remove");
		
		return dialogArea;
	}
	
	/**
	 * Add column to filter
	 */
	private void addColumn()
	{
		ColumnSelectionDialog dlg = new ColumnSelectionDialog(getShell(), logHandle);
		if (dlg.open() == Window.OK)
		{
			// Check if selected column already has filters
			for(FilterTreeElement e : elements)
			{
				if ((e.getType() == FilterTreeElement.COLUMN) &&
				    (((LogColumn)e.getObject()).getName().equals(dlg.getSelectedColumn().getName())))
				{
					filterTree.setSelection(new StructuredSelection(e), true);
					return;	// Column already added
				}
			}
			
			// Add column
			FilterTreeElement column = new FilterTreeElement(FilterTreeElement.COLUMN, dlg.getSelectedColumn(), rootElement);
			elements.add(column);
			filterTree.refresh();
			filterTree.setSelection(new StructuredSelection(column), true);
		}
	}
	
	/**
	 * Add filter set
	 */
	private void addFilterSet()
	{
		IStructuredSelection selection = (IStructuredSelection)filterTree.getSelection();
		if (selection.isEmpty())
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select column or filter set");
			return;
		}
		
		FilterTreeElement element = (FilterTreeElement)selection.getFirstElement();
		if (element.getType() == FilterTreeElement.ROOT)
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select column or filter set");
			return;
		}
		
		
	}

	/**
	 * Get filter created by builder
	 * 
	 * @return the filter
	 */
	public LogFilter getFilter()
	{
		return filter;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Log Query Builder");
	}
}
