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
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.Map.Entry;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.jface.viewers.TreeViewer;
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
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;
import org.netxms.ui.eclipse.logviewer.Activator;
import org.netxms.ui.eclipse.logviewer.dialogs.helpers.FilterTreeContentProvider;
import org.netxms.ui.eclipse.logviewer.dialogs.helpers.FilterTreeLabelProvider;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.FilterTreeElement;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.OrderingColumnEditingSupport;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.OrderingListLabelProvider;
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
	private Button buttonAddAndSet;
	private Button buttonAddOrSet;
	private Button buttonAddOperation;
	private Button buttonRemove;
	
	private HashSet<OrderingColumn> orderingColumns;
	private TableViewer orderingList;
	private Button buttonAddOrderingColumn;
	private Button buttonMoveUp;
	private Button buttonMoveDown;
	private Button buttonRemoveOrderingColumn;
	
	private Action actionAddColumn;
	private Action actionAddCondition;
	private Action actionAddAndGroup;
	private Action actionAddOrGroup;
	private Action actionRemoveColumn;
	private Action actionRemoveOrderingColumn;
	
	/**
	 * @param parentShell
	 */
	public QueryBuilder(Shell parentShell, Log logHandle, LogFilter filter)
	{
		super(parentShell);
		this.logHandle = logHandle;
		this.filter = filter;
		createInternalTree(filter);
		
		List<OrderingColumn> originalColumns = filter.getOrderingColumns();
		orderingColumns = new HashSet<OrderingColumn>(originalColumns.size());
		for(OrderingColumn c : originalColumns)
		{
			orderingColumns.add(new OrderingColumn(c));
		}
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
		dialogArea.setLayout(layout);
		
		final Composite filterTreeArea = createFilterTree(dialogArea);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		filterTreeArea.setLayoutData(gd);
		
		final Composite orderingListArea = createOrderingList(dialogArea);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		orderingListArea.setLayoutData(gd);
		
		createActions();
		
		return dialogArea;
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionAddColumn = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				addColumn();
			}
		};
		actionAddColumn.setText("Add &column");
		
		actionAddAndGroup = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				addFilterSet(ColumnFilter.AND);
			}
		};
		actionAddAndGroup.setText("Add &AND filter set");
		
		actionAddOrGroup = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				addFilterSet(ColumnFilter.OR);
			}
		};
		actionAddOrGroup.setText("Add &OR filter set");
		
		actionAddCondition = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				addCondition();
			}
		};
		actionAddCondition.setText("Add co&ndition");
		
		actionRemoveColumn = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				removeFilterTreeElement();
			}
		};
		actionRemoveColumn.setText("&Remove");
		actionRemoveColumn.setImageDescriptor(Activator.getImageDescriptor("icons/remove.png"));

		actionRemoveOrderingColumn = new Action() {
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				removeOrderingColumn();
			}
		};
		actionRemoveOrderingColumn.setText("&Remove");
		actionRemoveOrderingColumn.setImageDescriptor(Activator.getImageDescriptor("icons/remove.png"));
	}

	/**
	 * Create filter tree control
	 * 
	 * @param parent Parent widget
	 */
	private Composite createFilterTree(Composite parent)
	{
		Group group = new Group(parent, SWT.NONE);
		group.setText("Filter");
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		group.setLayout(layout);
		
		/* filter tree */
		filterTree = new TreeViewer(group, SWT.SINGLE | SWT.BORDER);
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
		Composite buttons = new Composite(group, SWT.NONE);
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
		buttonAddColumn.setText("Add &column");
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
		RowData rd = new RowData();
		rd.width = WidgetHelper.WIDE_BUTTON_WIDTH_HINT;
		buttonAddColumn.setLayoutData(rd);

		buttonAddAndSet = new Button(buttons, SWT.PUSH);
		buttonAddAndSet.setText("Add &AND group");
		buttonAddAndSet.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addFilterSet(ColumnFilter.AND);
			}
		});
		
		buttonAddOrSet = new Button(buttons, SWT.PUSH);
		buttonAddOrSet.setText("Add &OR group");
		buttonAddOrSet.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addFilterSet(ColumnFilter.OR);
			}
		});
		
		buttonAddOperation = new Button(buttons, SWT.PUSH);
		buttonAddOperation.setText("Add co&ndition");
		buttonAddOperation.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addCondition();
			}
		});
		
		buttonRemove = new Button(buttons, SWT.PUSH);
		buttonRemove.setText("&Remove");
		buttonRemove.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeFilterTreeElement();
			}
		});
		
		createFilterPopupMenu();
		
		return group;
	}
	
	/**
	 * Create pop-up menu for ordering list
	 */
	private void createFilterPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillFilterContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(filterTree.getControl());
		filterTree.getControl().setMenu(menu);
	}
	
	/**
	 * Fill context menu for ordering column list
	 * 
	 * @param mgr Menu manager
	 */
	private void fillFilterContextMenu(final IMenuManager mgr)
	{
		final IStructuredSelection selection = (IStructuredSelection)filterTree.getSelection();
		if (selection.isEmpty())
			return;
		
		FilterTreeElement element = (FilterTreeElement)selection.getFirstElement();
		switch(element.getType())
		{
			case FilterTreeElement.ROOT:
				mgr.add(actionAddColumn);
				break;
			case FilterTreeElement.COLUMN:
				mgr.add(actionAddAndGroup);
				mgr.add(actionAddOrGroup);
				mgr.add(actionAddCondition);
				mgr.add(new Separator());
				mgr.add(actionRemoveColumn);
				break;
			case FilterTreeElement.FILTER:
				final ColumnFilter cf = (ColumnFilter)element.getObject();
				if (cf.getType() == ColumnFilter.SET)
				{
					mgr.add(actionAddAndGroup);
					mgr.add(actionAddOrGroup);
					mgr.add(actionAddCondition);
					mgr.add(new Separator());
				}
				mgr.add(actionRemoveColumn);
				break;
		}
	}
	
	/**
	 * Create list control containing ordering columns
	 * 
	 * @param parent Parent widget
	 */
	private Composite createOrderingList(Composite parent)
	{
		Group group = new Group(parent, SWT.NONE);
		group.setText("Sorting");

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		group.setLayout(layout);

		/* column list */
		orderingList = new TableViewer(group, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.FULL_SELECTION);
		
		TableViewerColumn column = new TableViewerColumn(orderingList, SWT.LEFT);
		column.getColumn().setText("Column");
		column.getColumn().setWidth(200);

		column = new TableViewerColumn(orderingList, SWT.LEFT);
		column.getColumn().setText("Descending");
		column.getColumn().setWidth(60);
		column.setEditingSupport(new OrderingColumnEditingSupport(orderingList));
		
		orderingList.getTable().setLinesVisible(true);
		orderingList.getTable().setHeaderVisible(true);
		orderingList.setContentProvider(new ArrayContentProvider());
		orderingList.setLabelProvider(new OrderingListLabelProvider());
		orderingList.setInput(orderingColumns.toArray());

		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.heightHint = 100;
		orderingList.getControl().setLayoutData(gd);
		
		orderingList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				buttonRemoveOrderingColumn.setEnabled(!event.getSelection().isEmpty());
			}
		});
		
		/* buttons */
		Composite buttons = new Composite(group, SWT.NONE);
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
		
		buttonAddOrderingColumn = new Button(buttons, SWT.PUSH);
		buttonAddOrderingColumn.setText("&Add column");
		buttonAddOrderingColumn.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addOrderingColumn();
			}
		});
		RowData rd = new RowData();
		rd.width = WidgetHelper.WIDE_BUTTON_WIDTH_HINT;
		buttonAddOrderingColumn.setLayoutData(rd);

		buttonRemoveOrderingColumn = new Button(buttons, SWT.PUSH);
		buttonRemoveOrderingColumn.setText("R&emove");
		buttonRemoveOrderingColumn.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeOrderingColumn();
			}
		});
		buttonRemoveOrderingColumn.setEnabled(false);
		
		createOrderingPopupMenu();

		return group;
	}
	
	/**
	 * Create pop-up menu for ordering list
	 */
	private void createOrderingPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener()
		{
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillOrderingContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(orderingList.getControl());
		orderingList.getControl().setMenu(menu);
	}
	
	/**
	 * Fill context menu for ordering column list
	 * 
	 * @param mgr Menu manager
	 */
	private void fillOrderingContextMenu(final IMenuManager mgr)
	{
		final IStructuredSelection selection = (IStructuredSelection)orderingList.getSelection();
		if (!selection.isEmpty())
			mgr.add(actionRemoveOrderingColumn);
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
	private void addFilterSet(int operation)
	{
		IStructuredSelection selection = (IStructuredSelection)filterTree.getSelection();
		FilterTreeElement element = (FilterTreeElement)selection.getFirstElement();
		if (selection.isEmpty() || (element.getType() == FilterTreeElement.ROOT))
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select column or filter set");
			return;
		}
	
		ColumnFilter cf = new ColumnFilter();
		cf.setOperation(operation);
		FilterTreeElement newElement;
		if (element.getType() == FilterTreeElement.COLUMN)
		{
			FilterTreeElement[] childs = element.getChilds();
			newElement = new FilterTreeElement(FilterTreeElement.FILTER, cf, element);
			for(int i = 0; i < childs.length; i++)
				childs[i].changeParent(newElement);
		}
		else
		{
			final ColumnFilter currFilter = (ColumnFilter)element.getObject();
			if (currFilter.getType() == ColumnFilter.SET)
			{
				newElement = new FilterTreeElement(FilterTreeElement.FILTER, cf, element);
			}
			else
			{
				newElement = new FilterTreeElement(FilterTreeElement.FILTER, cf, element.getParent());
				element.changeParent(newElement);
			}
		}
		elements.add(newElement);
		filterTree.refresh();
		filterTree.setSelection(new StructuredSelection(newElement), true);
	}
	
	/**
	 * Add condition to filter tree
	 */
	private void addCondition()
	{
		IStructuredSelection selection = (IStructuredSelection)filterTree.getSelection();
		FilterTreeElement element = (FilterTreeElement)selection.getFirstElement();
		if (selection.isEmpty() || (element.getType() == FilterTreeElement.ROOT))
		{
			MessageDialog.openWarning(getShell(), "Warning", "Please select column or filter set");
			return;
		}
		
		// Check if we can add condition under current element
		if (element.getType() == FilterTreeElement.COLUMN)
		{
			if (element.hasChilds())
			{
				MessageDialog.openWarning(getShell(), "Warning", "Please select column without conditions or condition set to add new condition");
				return;
			}
		}
		else
		{
			if (((ColumnFilter)element.getObject()).getType() != ColumnFilter.SET)
			{
				MessageDialog.openWarning(getShell(), "Warning", "Please select column without conditions or condition set to add new condition");
				return;
			}
		}

		// Find column information
		FilterTreeElement p = element;
		while(p.getType() != FilterTreeElement.COLUMN)
			p = p.getParent();
		final LogColumn logColumn = (LogColumn)p.getObject();
		
		AddConditionDialog dlg = new AddConditionDialog(getShell(), logColumn);
		if (dlg.open() == Window.OK)
		{
			FilterTreeElement condition = new FilterTreeElement(FilterTreeElement.FILTER, dlg.getFilter(), element);
			elements.add(condition);
			filterTree.refresh();
			filterTree.setSelection(new StructuredSelection(condition), true);
		}
	}
	
	/**
	 * Remove selected element from filter tree
	 */
	private void removeFilterTreeElement()
	{
		IStructuredSelection selection = (IStructuredSelection)filterTree.getSelection();
		FilterTreeElement element = (FilterTreeElement)selection.getFirstElement();
		if (selection.isEmpty() || (element.getType() == FilterTreeElement.ROOT))
		{
			MessageDialog.openWarning(getShell(), "Warning", "Only columns and filters can be deleted");
			return;
		}
		
		FilterTreeElement parent = element.getParent();
		parent.removeChild(element);
		filterTree.refresh();
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		generateFilter();
		super.okPressed();
	}
	
	/**
	 * Generate LogFilter object from built tree
	 */
	private void generateFilter()
	{
		optimizeTree(rootElement);
		
		filter = new LogFilter();
		FilterTreeElement[] columns = rootElement.getChilds();
		for(int i = 0; i < columns.length; i++)
		{
			ColumnFilter cf = buildColumnFilter(columns[i].getChilds()[0]);
			filter.setColumnFilter(((LogColumn)columns[i].getObject()).getName(), cf);
		}
		filter.setOrderingColumns(new ArrayList<OrderingColumn>(orderingColumns));
	}
	
	/**
	 * Build column filter object for given filter tree element
	 * 
	 * @param element Filter tree element
	 * @return Column filter object
	 */
	private ColumnFilter buildColumnFilter(FilterTreeElement element)
	{
		ColumnFilter cf = (ColumnFilter)element.getObject();
		if (cf.getType() != ColumnFilter.SET)
			return cf;	// Conditions already prepared
		
		ColumnFilter filterSet = new ColumnFilter();
		filterSet.setOperation(cf.getOperation());
		FilterTreeElement[] childs = element.getChilds();
		for(int i = 0; i < childs.length; i++)
		{
			filterSet.addSubFilter(buildColumnFilter(childs[i]));
		}
		return filterSet;
	}
	
	/**
	 * Remove unused element from subtree
	 * 
	 * @param root Root element
	 */
	private void optimizeTree(FilterTreeElement root)
	{
		FilterTreeElement[] childs = root.getChilds();
		for(int i = 0; i < childs.length; i++)
		{
			optimizeTree(childs[i]);
			if (!childs[i].hasChilds())
			{
				if (childs[i].getType() == FilterTreeElement.FILTER)
				{
					if (((ColumnFilter)childs[i].getObject()).getType() == ColumnFilter.SET)
						root.removeChild(childs[i]);  // Filter set without subfilters
				}
				else
				{
					// Column without filters, remove
					root.removeChild(childs[i]);
				}
			}
			else if (childs[i].getNumChilds() == 1)
			{
				if ((childs[i].getType() == FilterTreeElement.FILTER) &&
				    (((ColumnFilter)childs[i].getObject()).getType() == ColumnFilter.SET))
				{
					// Filter set with only one subfilter is meaningless
					FilterTreeElement subFilter = childs[i].getChilds()[0];
					root.removeChild(childs[i]);
					subFilter.changeParent(root);
				}
			}
		}
	}
	
	/**
	 * Add new ordering column to the list
	 */
	private void addOrderingColumn()
	{
		ColumnSelectionDialog dlg = new ColumnSelectionDialog(getShell(), logHandle);
		if (dlg.open() == Window.OK)
		{
			final OrderingColumn column = new OrderingColumn(dlg.getSelectedColumn());
			if (!orderingColumns.contains(column))
			{
				orderingColumns.add(column);
				orderingList.setInput(orderingColumns.toArray());
			}
			else
			{
				orderingList.setSelection(new StructuredSelection(column));
			}
		}
	}
	
	/**
	 * Remove ordering column
	 */
	@SuppressWarnings("unchecked")
	private void removeOrderingColumn()
	{
		IStructuredSelection selection = (IStructuredSelection)orderingList.getSelection();
		if (!selection.isEmpty())
		{
			Iterator<OrderingColumn> it = selection.iterator();
			while(it.hasNext())
			{
				orderingColumns.remove(it.next());
			}
			orderingList.setInput(orderingColumns.toArray());
		}
	}
}
