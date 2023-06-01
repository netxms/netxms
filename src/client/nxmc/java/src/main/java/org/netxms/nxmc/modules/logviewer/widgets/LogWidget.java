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

import java.util.Collection;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.TableRow;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.client.log.LogRecordDetails;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.LogRecordDetailsViewerRegistry;
import org.netxms.nxmc.modules.logviewer.views.helpers.LogLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Universal log viewer
 */
public class LogWidget extends Composite
{
	private static final int PAGE_SIZE = 400;

   private final I18n i18n = LocalizationHelper.getI18n(LogWidget.class);

   protected NXCSession session = Registry.getSession();
	protected TableViewer viewer;

   private View view;
   private FilterBuilder filterBuilder;
	private String logName;
	private Log logHandle;
	private LogFilter filter;
   private LogFilter delayedQueryFilter = null;
   private LogRecordDetailsViewer recordDetailsViewer;
	private Table resultSet;
	private boolean noData = false;
   private Action actionExecute;
   private Action actionClearFilter;
   private Action actionShowFilter;
   private Action actionGetMoreData;
   private Action actionCopyToClipboard;
   private Action actionExportToCsv;
   private Action actionExportAllToCsv;
   private Action actionShowDetails;

   /**
    * Create log view widget
    * 
    * @param view
    * @param parent
    * @param style
    * @param logName
    */
   public LogWidget(View view, Composite parent, int style, String logName)
   {
      super(parent, style);
      this.view = view;
      this.logName = logName;
      filter = new LogFilter();
      recordDetailsViewer = LogRecordDetailsViewerRegistry.get(logName);
      

      FormLayout layout = new FormLayout();
      setLayout(layout);

      /* create filter builder */
      filterBuilder = new FilterBuilder(this, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      filterBuilder.setLayoutData(gd);

      /* create viewer */
      viewer = new TableViewer(this, SWT.MULTI | SWT.FULL_SELECTION);
      org.eclipse.swt.widgets.Table table = viewer.getTable();
      table.setLinesVisible(true);
      table.setHeaderVisible(true);
      viewer.setContentProvider(new ArrayContentProvider());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      viewer.getControl().setLayoutData(gd);
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            showRecordDetails();
         }
      });
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (logHandle != null)
               WidgetHelper.saveColumnSettings(viewer.getTable(), "LogViewer." + logHandle.getName());
         }
      });

      /* setup layout */
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(filterBuilder);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      table.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      filterBuilder.setLayoutData(fd);

      createActions();
      createPopupMenu();
   }

   /**
    * Post clone 
    * @param view
    */
   public void postClone(LogWidget origin)
   {
      delayedQueryFilter = origin.filterBuilder.createFilter();
      openServerLog();
   }
   
   /**
    * Open server log
    */
   public void openServerLog()
   {
      new Job(String.format(i18n.tr("Opening server log \"%s\""), logName), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Log handle = session.openServerLog(logName);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  logHandle = handle;
                  setupLogViewer();
                  if (delayedQueryFilter != null)
                  {
                     filterBuilder.setFilter(delayedQueryFilter);
                     delayedQueryFilter = null;
                     doQuery();
                  }
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot open server log \"%s\""), logName);
         }
      }.start();
      
   }

   /**
    * Estimate required column width
    */
	protected int estimateColumnWidth(final LogColumn lc)
	{
		switch(lc.getType())
		{
			case LogColumn.LC_INTEGER:
				return 80;
			case LogColumn.LC_TIMESTAMP:
				return 120;
			case LogColumn.LC_SEVERITY:
				return 100;
			case LogColumn.LC_COMPLETION_STATUS:
				return 100;
			case LogColumn.LC_OBJECT_ID:
				return 150;
			case LogColumn.LC_TEXT:
				return 250;
			default:
				return 100;
		}
	}

	/**
	 * Setup log viewer after successful log open
	 */
	private void setupLogViewer()
	{
		org.eclipse.swt.widgets.Table table = viewer.getTable();
		Collection<LogColumn> columns = logHandle.getColumns();
		for(final LogColumn lc : columns)
		{
			TableColumn column = new TableColumn(table, SWT.LEFT);
			column.setText(lc.getDescription());
			column.setData(lc);
			column.setWidth(estimateColumnWidth(lc));
			if (lc.getType() == LogColumn.LC_TIMESTAMP)
			{
			   filterBuilder.addOrderingColumn(lc, true);
			}
		}
      WidgetHelper.restoreColumnSettings(table, "LogViewer." + logHandle.getName());
		viewer.setLabelProvider(createLabelProvider(logHandle));
		filterBuilder.setLogHandle(logHandle);
		filter = filterBuilder.createFilter();
	}

	/**
	 * Create label provider
	 * 
	 * @return
	 */
	protected ITableLabelProvider createLabelProvider(Log logHandle)
	{
	   return new LogLabelProvider(logHandle, viewer);
	}

	/**
	 * Fill local menu
	 * 
	 * @param manager
	 */
   public void fillLocalMenu(IMenuManager manager)
	{
		manager.add(actionExecute);
      manager.add(actionGetMoreData);
      manager.add(new Separator());
      manager.add(actionExportAllToCsv);
      manager.add(new Separator());
		manager.add(actionClearFilter);
		manager.add(actionShowFilter);
	}

   /**
    * Fill local tool bar
    * 
    * @param manager
    */
   public void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionExecute);
      manager.add(actionGetMoreData);
      manager.add(new Separator());
      manager.add(actionExportAllToCsv);
      manager.add(new Separator());
      manager.add(actionShowFilter);
		manager.add(actionClearFilter);
	}

	/**
	 * Create pop-up menu for user list
	 */
	private void createPopupMenu()
	{
		// Create menu manager
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
      if (recordDetailsViewer != null)
      {
         mgr.add(actionShowDetails);
         mgr.add(new Separator());
      }
		mgr.add(actionCopyToClipboard);
		mgr.add(actionExportToCsv);
	}

	/**
	 * Create actions
	 */
	protected void createActions()
	{
      actionExecute = new Action(i18n.tr("&Execute query"), SharedIcons.EXECUTE) {
			@Override
			public void run()
			{
				doQuery();
			}
		};
      view.addKeyBinding("F9", actionExecute);

      actionClearFilter = new Action(i18n.tr("&Clear filter"), SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				filterBuilder.clearFilter();
			}
		};
		view.addKeyBinding("M1+E", actionClearFilter);

      actionGetMoreData = new Action(i18n.tr("Get &more data"), ResourceManager.getImageDescriptor("icons/log-viewer/get-more-data.png")) {
			@Override
			public void run()
			{
				getMoreData();
			}
		};
		actionGetMoreData.setEnabled(false);
		view.addKeyBinding("M2+F9", actionGetMoreData);

      actionShowFilter = new Action(i18n.tr("Show &filter"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				showFilter(actionShowFilter.isChecked());
			}
		};
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
		actionShowFilter.setChecked(true);
		view.addKeyBinding("M1+F2", actionShowFilter);

      actionCopyToClipboard = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
			@Override
			public void run()
			{
				copySelectionToClipboard();
			}
		};
		view.addKeyBinding("M1+C", actionCopyToClipboard);

		actionExportToCsv = new ExportToCsvAction(view, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(view, viewer, false);

      actionShowDetails = new Action("Show &details") {
         @Override
         public void run()
         {
            showRecordDetails();
         }
      };
	}

	/**
	 * Query log with given filter
	 * 
	 * @param filter
	 */
	public void queryWithFilter(LogFilter filter)
	{
	   if (logHandle != null)
	   {
	      filterBuilder.setFilter(filter);
	      doQuery();
	   }
	   else
	   {
	      delayedQueryFilter = filter;
	   }
	}

	/**
	 * Prepare filter and execute query on server
	 */
	private void doQuery()
	{
		actionGetMoreData.setEnabled(false);
		filter = filterBuilder.createFilter();
      new Job(String.format(i18n.tr("Querying server log \"%s\""), logName), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				logHandle.query(filter);
				final Table data = logHandle.retrieveData(0, PAGE_SIZE);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						resultSet = data;
						viewer.setInput(resultSet.getAllRows());
						noData = (resultSet.getRowCount() < PAGE_SIZE);
						actionGetMoreData.setEnabled(!noData);
					}
				});
			}

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot query server log \"%s\""), logName);
         }
      }.start();
	}
	
	/**
	 * Get more data from server
	 */
	private void getMoreData()
	{
		if (noData)
			return;	// we already know that there will be no more data

      new Job(String.format(i18n.tr("Querying server log \"%s\""), logName), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table data = logHandle.retrieveData(resultSet.getRowCount(), PAGE_SIZE);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						resultSet.addAll(data);
						viewer.setInput(resultSet.getAllRows());
						noData = (data.getRowCount() < PAGE_SIZE);
						actionGetMoreData.setEnabled(!noData);
					}
				});
			}

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot query server log \"%s\""), logName);
         }
		}.start();
	}

	/**
	 * Refresh 
	 */
   public void refresh()
	{
      new Job(String.format(i18n.tr("Querying server log \"%s\""), logName), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table data = logHandle.retrieveData(0, resultSet.getRowCount(), true);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						resultSet = data;
						viewer.setInput(resultSet.getAllRows());
					}
				});
			}

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot query server log \"%s\""), logName);
         }
      }.start();
	}

   /**
    * Show details for selected record
    */
   private void showRecordDetails()
   {
      if (recordDetailsViewer == null)
         return;

      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() != 1)
         return;

      final TableRow record = (TableRow)selection.getFirstElement();
      final long recordId = record.getValueAsLong(logHandle.getRecordIdColumnIndex());

      new Job(i18n.tr("Getting log record details"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final LogRecordDetails recordDetails = logHandle.getRecordDetails(recordId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  recordDetailsViewer.showRecordDetails(recordDetails, record, logHandle, view);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get details for selected record");
         }
      }.start();
   }

	/**
	 * @param show
	 */
	protected void showFilter(boolean show)
	{
		filterBuilder.setVisible(show);
		FormData fd = (FormData)viewer.getTable().getLayoutData();
		fd.top = show ? new FormAttachment(filterBuilder) : new FormAttachment(0, 0);
		viewer.getTable().getParent().layout();
		if (show)
			filterBuilder.setFocus();
	}

	/**
	 * Copy selection in the list to clipboard
	 */
	private void copySelectionToClipboard()
	{
		TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length > 0)
		{
			StringBuilder sb = new StringBuilder();
         final String newLine = WidgetHelper.getNewLineCharacters();
			for(int i = 0; i < selection.length; i++)
			{
				if (i > 0)
					sb.append(newLine);
				sb.append(selection[i].getText(0));
				for(int j = 1; j < viewer.getTable().getColumnCount(); j++)
				{
					sb.append('\t');
					sb.append(selection[i].getText(j));
				}
			}
			WidgetHelper.copyToClipboard(sb.toString());
		}
	}

	/**
	 * @return
	 */
	public TableViewer getViewer()
	{
	   return viewer;
	}

	/**
	 * @return
	 */
	protected Table getResultSet()
	{
	   return resultSet;
	}

	/**
	 * @param columnName
	 * @return
	 */
	protected int getColumnIndex(String columnName)
	{
	   if (resultSet == null)
	      return -1;
	   return resultSet.getColumnIndex(columnName);
	}
}
