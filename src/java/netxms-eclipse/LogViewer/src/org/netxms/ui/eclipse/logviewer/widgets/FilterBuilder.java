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
package org.netxms.ui.eclipse.logviewer.widgets;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MenuEvent;
import org.eclipse.swt.events.MenuListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.Form;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.OrderingColumn;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.ColumnSelectionHandler;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.FilterTreeElement;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.OrderingColumnEditingSupport;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.OrderingListLabelProvider;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Log filter builder control
 */
public class FilterBuilder extends Composite
{
	private Log logHandle = null;
	private List<FilterTreeElement> elements = new ArrayList<FilterTreeElement>();
	private FilterTreeElement rootElement = null;
	private List<OrderingColumn> orderingColumns = new ArrayList<OrderingColumn>();
	private FormToolkit toolkit;
	private Form form;
	private Section condition;
	private Section ordering;
	private TableViewer orderingList;
	private Action actionExecute;
	private Action actionClose;
	private Menu columnSelectionMenu = null;

	/**
	 * @param parent
	 * @param style
	 */
	public FilterBuilder(Composite parent, int style)
	{
		super(parent, style);
		
		rootElement = new FilterTreeElement(FilterTreeElement.ROOT, null, null);
		elements.add(rootElement);
		
		setLayout(new FillLayout());
		
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createForm(this);
		form.setText("Filter");
		form.getToolBarManager().add(new Action("&Execute", SharedIcons.EXECUTE) {
			@Override
			public void run()
			{
				if (actionExecute != null)
					actionExecute.run();
			} 
		});
		form.getToolBarManager().add(new Action("&Clear filter", SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				clearFilter();
			} 
		});
		form.getToolBarManager().add(new Action("Close", SharedIcons.CLOSE) {
			@Override
			public void run()
			{
				if (actionClose != null)
					actionClose.run();
			} 
		});
		form.getToolBarManager().update(true);

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 3;
		form.getBody().setLayout(layout);
		
		createConditionSection();
		createOrderingSection();
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if (columnSelectionMenu != null)
					columnSelectionMenu.dispose();
			}
		});
	}
	
	/**
	 * Create condition section
	 */
	private void createConditionSection()
	{
		condition = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		condition.setText("Condition");
		TableWrapData twd = new TableWrapData();
		twd.grabHorizontal = true;
		twd.align = TableWrapData.FILL;
		condition.setLayoutData(twd);

		final Composite clientArea = toolkit.createComposite(condition);
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);
		condition.setClient(clientArea);
		
		final ImageHyperlink addColumnLink = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		addColumnLink.setText("Add column");
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
		addColumnLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addColumnToFilter(addColumnLink);
			}
		});
	}
	
	/**
	 * Create ordering section
	 */
	private void createOrderingSection()
	{
		ordering = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		ordering.setText("Ordering");
		TableWrapData twd = new TableWrapData();
		twd.grabHorizontal = false;
		twd.align = TableWrapData.FILL;
		ordering.setLayoutData(twd);
		
		final Composite clientArea = toolkit.createComposite(ordering);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		ordering.setClient(clientArea);

		orderingList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.FULL_SELECTION);
		toolkit.adapt(orderingList.getTable());
		
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
		gd.verticalSpan = 2;
		gd.heightHint = 60;
		orderingList.getControl().setLayoutData(gd);
		
		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText("Add");
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addSortingColumn();
			}
		});
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		
		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText("Remove");
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				removeSortingColumn();
			}
		});
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.setEnabled(false);
		
		orderingList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				linkRemove.setEnabled(!orderingList.getSelection().isEmpty());
			}
		});
	}

	/**
	 * @param action
	 */
	public void setExecuteAction(Action action)
	{
		actionExecute = action;
	}
	
	/**
	 * @param action
	 */
	public void setCloseAction(Action action)
	{
		actionClose = action;
	}
	
	/**
	 * Clear filter
	 */
	public void clearFilter()
	{
		
	}
	
	/**
	 * 
	 */
	private void addSortingColumn()
	{
		createColumnSelectionMenu(new ColumnSelectionHandler() {
			@Override
			public void columnSelected(LogColumn column)
			{
				final OrderingColumn orderingColumn = new OrderingColumn(column);
				if (!orderingColumns.contains(orderingColumn))
				{
					orderingColumns.add(orderingColumn);
					orderingList.setInput(orderingColumns.toArray());
				}
				else
				{
					orderingList.setSelection(new StructuredSelection(column));
				}
			}
		});
	}
	
	/**
	 * Remove sorting column(s) from list
	 */
	private void removeSortingColumn()
	{
		IStructuredSelection selection = (IStructuredSelection)orderingList.getSelection();
		for(Object o : selection.toList())
		{
			orderingColumns.remove(o);
		}
		orderingList.setInput(orderingColumns.toArray());
	}

	/**
	 * @param logHandle the logHandle to set
	 */
	public void setLogHandle(Log logHandle)
	{
		this.logHandle = logHandle;
		form.setText("Filter: " + logHandle.getName());
	}
	
	/**
	 * Add column to filter
	 */
	private void addColumnToFilter(final Control lastControl)
	{
		createColumnSelectionMenu(new ColumnSelectionHandler() {
			@Override
			public void columnSelected(LogColumn column)
			{
				// Check if selected column already has filters
				for(FilterTreeElement e : elements)
				{
					if ((e.getType() == FilterTreeElement.COLUMN) &&
					    (((LogColumn)e.getObject()).getName().equals(column.getName())))
					{
						return;	// Column already added
					}
				}
				
				// Add column
				final FilterTreeElement columnElement = new FilterTreeElement(FilterTreeElement.COLUMN, column, rootElement);
				elements.add(columnElement);
				
				final ColumnFilterEditor editor = new ColumnFilterEditor((Composite)condition.getClient(), 
						toolkit, column, columnElement, new Runnable() {
							@Override
							public void run()
							{
								rootElement.removeChild(columnElement);
								elements.remove(columnElement);
								/* TODO: remove all childs? */
								columnElement.getEditor().dispose();
								FilterBuilder.this.getParent().layout(true, true);
							}
						});
				editor.setFilterBuilder(FilterBuilder.this);
				editor.moveAbove(lastControl);
				GridData gd = new GridData();
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				editor.setLayoutData(gd);
				columnElement.setEditor(editor);
				
				FilterBuilder.this.getParent().layout(true, true);
			}
		});
	}
	
	/**
	 * Create menu for selecting log column
	 * 
	 * @param listener selection listener
	 */
	private void createColumnSelectionMenu(final ColumnSelectionHandler handler)
	{
		if (logHandle == null)
			return;
		
		if (columnSelectionMenu != null)
			columnSelectionMenu.dispose();
		
		columnSelectionMenu = new Menu(getShell(), SWT.POP_UP);
		getShell().setMenu(columnSelectionMenu);
		
		for(final LogColumn lc : logHandle.getColumns())
		{
			MenuItem item = new MenuItem(columnSelectionMenu, SWT.PUSH);
			item.setText(lc.getDescription());
			item.addSelectionListener(new SelectionListener() {
				@Override
				public void widgetSelected(SelectionEvent e)
				{
					handler.columnSelected(lc);
				}
				
				@Override
				public void widgetDefaultSelected(SelectionEvent e)
				{
					widgetSelected(e);
				}
			});
		}
		
		columnSelectionMenu.addMenuListener(new MenuListener() {
			@Override
			public void menuShown(MenuEvent e)
			{
			}
			
			@Override
			public void menuHidden(MenuEvent e)
			{
				getShell().setMenu(null);
			}
		});
		columnSelectionMenu.setVisible(true);
	}
}
