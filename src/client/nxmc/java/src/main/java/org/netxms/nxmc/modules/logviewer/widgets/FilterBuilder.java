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
package org.netxms.nxmc.modules.logviewer.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.TableViewerColumn;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MenuEvent;
import org.eclipse.swt.events.MenuListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.MenuItem;
import org.netxms.client.constants.ColumnFilterSetOperation;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.OrderingColumn;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.base.widgets.helpers.ExpansionEvent;
import org.netxms.nxmc.base.widgets.helpers.ExpansionListener;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.widgets.helpers.ColumnSelectionHandler;
import org.netxms.nxmc.modules.logviewer.widgets.helpers.OrderingColumnEditingSupport;
import org.netxms.nxmc.modules.logviewer.widgets.helpers.OrderingListLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Log filter builder control
 */
public class FilterBuilder extends Composite
{
   private final I18n i18n = LocalizationHelper.getI18n(FilterBuilder.class);

	private Log logHandle = null;
	private Map<String, ColumnFilterEditor> columns = new HashMap<String, ColumnFilterEditor>();
	private List<OrderingColumn> orderingColumns = new ArrayList<OrderingColumn>();
   private ScrolledComposite scroller;
   private Composite content;
	private Section condition;
	private Section ordering;
   private ImageHyperlink addColumnLink;
	private TableViewer orderingList;
	private Menu columnSelectionMenu = null;

	/**
	 * @param parent
	 * @param style
	 */
	public FilterBuilder(Composite parent, int style)
	{
		super(parent, style);

		setLayout(new FillLayout());

      scroller = new ScrolledComposite(this, SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            content.layout(true, true);
            scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
         }
      });

      content = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      content.setLayout(layout);
      content.setBackground(content.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      scroller.setContent(content);

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
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
      Point size = content.computeSize(wHint, hHint, changed);
      if (size.y > 600)
         size.y = 600;
		return size;
	}

	/**
	 * Create condition section
	 */
	private void createConditionSection()
	{
      condition = new Section(content, i18n.tr("Condition"), true);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      condition.setLayoutData(gd);
      condition.addExpansionListener(new ExpansionListener() {
         @Override
         public void expansionStateChanged(ExpansionEvent e)
         {
            FilterBuilder.this.updateLayout();
         }
      });

      final Composite clientArea = condition.getClient();
		GridLayout layout = new GridLayout();
		clientArea.setLayout(layout);

      addColumnLink = new ImageHyperlink(clientArea, SWT.NONE);
      addColumnLink.setText(i18n.tr("Add column"));
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
      addColumnLink.setBackground(clientArea.getBackground());
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
      ordering = new Section(content, i18n.tr("Ordering"), true);
      GridData gd = new GridData();
      gd.grabExcessVerticalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      gd.horizontalAlignment = SWT.FILL;
      gd.widthHint = 400;
      ordering.setLayoutData(gd);
      ordering.addExpansionListener(new ExpansionListener() {
         @Override
         public void expansionStateChanged(ExpansionEvent e)
         {
            FilterBuilder.this.updateLayout();
         }
      });

      final Composite clientArea = ordering.getClient();
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);

		orderingList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL | SWT.FULL_SELECTION);

		TableViewerColumn column = new TableViewerColumn(orderingList, SWT.LEFT);
      column.getColumn().setText(i18n.tr("Column"));
		column.getColumn().setWidth(200);

		column = new TableViewerColumn(orderingList, SWT.LEFT);
      column.getColumn().setText(i18n.tr("Descending"));
		column.getColumn().setWidth(60);
		column.setEditingSupport(new OrderingColumnEditingSupport(orderingList));

		orderingList.getTable().setLinesVisible(true);
		orderingList.getTable().setHeaderVisible(true);
		orderingList.setContentProvider(new ArrayContentProvider());
		orderingList.setLabelProvider(new OrderingListLabelProvider());
		orderingList.setInput(orderingColumns.toArray());

      gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
		gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 2;
		orderingList.getControl().setLayoutData(gd);

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add"));
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addOrderingColumn();
			}
		});
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);

      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
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
	 * Clear filter
	 */
	public void clearFilter()
	{
		orderingColumns.clear();
		orderingList.setInput(orderingColumns.toArray());
		
		for(ColumnFilterEditor e : columns.values())
			e.dispose();
		columns.clear();
		
		updateLayout();
	}
	
	/**
	 * Add ordering column to filter
	 * @param column
	 */
	public void addOrderingColumn(LogColumn column, boolean descending)
	{
	   final OrderingColumn orderingColumn = new OrderingColumn(column);
      if (!orderingColumns.contains(orderingColumn))
      {
         orderingColumn.setDescending(descending);
         orderingColumns.add(orderingColumn);
         orderingList.setInput(orderingColumns.toArray());
      }
	}
	
   /**
    * Add ordering column
    */
	private void addOrderingColumn()
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
               FilterBuilder.this.updateLayout();
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
      updateLayout();
	}

	/**
	 * @param logHandle the logHandle to set
	 */
	public void setLogHandle(Log logHandle)
	{
		this.logHandle = logHandle;
      updateLayout();
	}
	
	/**
	 * Add column to filter
	 */
	private void addColumnToFilter(final Control lastControl)
	{
		createColumnSelectionMenu(new ColumnSelectionHandler() {
			@Override
			public void columnSelected(final LogColumn column)
			{
				// Check if selected column already has filters
				if (columns.get(column.getName()) != null)
					return;	// Column already added

            createColumnFilterEditor(column, lastControl, null);
				FilterBuilder.this.updateLayout();
			}
		});
	}

   /**
    * Create column filter editor
    * 
    * @param column
    * @param lastControl
    * @param initialFilter
    */
   private void createColumnFilterEditor(final LogColumn column, final Control lastControl, ColumnFilter initialFilter)
   {
      final ColumnFilterEditor editor = new ColumnFilterEditor((Composite)condition.getClient(), 
            column, (initialFilter != null) ? initialFilter.getOperation() : ColumnFilterSetOperation.AND, new Runnable() {
               @Override
               public void run()
               {
                  columns.remove(column.getName());
                  FilterBuilder.this.updateLayout();
               }
            });
      editor.attachFilterBuilder(FilterBuilder.this, initialFilter);
      editor.moveAbove(lastControl);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      editor.setLayoutData(gd);

      columns.put(column.getName(), editor);
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
	
	/**
	 * @return
	 */
	public LogFilter createFilter()
	{
		LogFilter filter = new LogFilter();
		for(Entry<String, ColumnFilterEditor> e : columns.entrySet())
		{
			ColumnFilter cf = e.getValue().buildFilterTree();
			if (cf != null)
			{
				filter.setColumnFilter(e.getKey(), cf);
			}
		}
		filter.setOrderingColumns(new ArrayList<OrderingColumn>(orderingColumns));
		return filter;
	}

	/**
	 * Update layout after internal change
	 */
	public void updateLayout()
	{
      getParent().layout(true, true);
      scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
	}

   /**
    * Update filter builder from existing filter
    * 
    * @param filter
    */
   public void setFilter(LogFilter filter)
   {
      clearFilter();

      for(Entry<String, ColumnFilter> cf : filter.getColumnFilters())
      {
         createColumnFilterEditor(logHandle.getColumn(cf.getKey()), addColumnLink, cf.getValue());
      }

      orderingColumns.addAll(filter.getOrderingColumns());
      orderingList.setInput(orderingColumns.toArray());

      updateLayout();
   }
}
