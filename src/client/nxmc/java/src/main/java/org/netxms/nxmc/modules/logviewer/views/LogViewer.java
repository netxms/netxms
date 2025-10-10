/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logviewer.views;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
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
import org.netxms.client.log.OrderingColumn;
import org.netxms.client.xml.XMLTools;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.ExportToCsvAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.views.ViewWithContext;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logviewer.LogDescriptorRegistry;
import org.netxms.nxmc.modules.logviewer.LogRecordDetailsViewer;
import org.netxms.nxmc.modules.logviewer.LogRecordDetailsViewerRegistry;
import org.netxms.nxmc.modules.logviewer.views.helpers.LogLabelProvider;
import org.netxms.nxmc.modules.logviewer.widgets.FilterBuilder;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Universal log viewer
 */
public class LogViewer extends ViewWithContext
{
	private static final int PAGE_SIZE = 400;

   private final I18n i18n = LocalizationHelper.getI18n(LogViewer.class);
   private static final Logger logger = LoggerFactory.getLogger(LogViewer.class);

   protected NXCSession session = Registry.getSession();
	protected TableViewer viewer;

   private FilterBuilder filterBuilder;
	private String logName;
	private Log logHandle;
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
    * Internal constructor used for cloning
    */
   protected LogViewer()
   {
      super(null, null, null, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#cloneView()
    */
   @Override
   public View cloneView()
   {
      LogViewer view = (LogViewer)super.cloneView();
      view.logName = logName;
      view.recordDetailsViewer = recordDetailsViewer;
      return view;
   }

   /**
    * Create new log viewer view
    *
    * @param viewName view name
    * @param logName log name
    */
   public LogViewer(String viewName, String logName, String additionalId)
   {
      super(viewName, ResourceManager.getImageDescriptor("icons/log-viewer/" + logName + ".png"), "LogViewer." + logName + additionalId, false);
      this.logName = logName;
      recordDetailsViewer = LogRecordDetailsViewerRegistry.get(logName);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public void createContent(Composite parent)
	{
      FormLayout layout = new FormLayout();
		parent.setLayout(layout);

		/* create filter builder */
		filterBuilder = new FilterBuilder(parent, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		filterBuilder.setLayoutData(gd);

		/* create viewer */
		viewer = new TableViewer(parent, SWT.MULTI | SWT.FULL_SELECTION);
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

      enableRefresh(false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      LogViewer origin = (LogViewer)view;
      delayedQueryFilter = origin.filterBuilder.createFilter();
      openServerLog(true);
      super.postClone(view);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      openServerLog(false);
   }
   
   /**
    * Open server log
    */
   protected void openServerLog(boolean onClone)
   {
      new Job(String.format(i18n.tr("Opening server log \"%s\""), logName), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Log handle = session.openServerLog(logName);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  logHandle = handle;
                  setupLogViewer(onClone);
                  if (delayedQueryFilter != null)
                  {
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
	 * @param onClone 
	 */
	protected void setupLogViewer(boolean onClone)
	{
      org.eclipse.swt.widgets.Table table = viewer.getTable();
      Collection<LogColumn> columns = logHandle.getColumns();
      LogFilter filter = delayedQueryFilter != null ? delayedQueryFilter : new LogFilter();
      LogColumn orderingColumn = null;
      for(final LogColumn lc : columns)
      {
         TableColumn column = new TableColumn(table, SWT.LEFT);
         column.setText(lc.getDescription());
         column.setData(lc);
         column.setWidth(estimateColumnWidth(lc));
         if (!onClone)
         {
            if ((lc.getType() == LogColumn.LC_INTEGER) && ((lc.getFlags() & LogColumn.LCF_RECORD_ID) != 0))
            {
               orderingColumn = lc;
            }
            else if ((orderingColumn == null) && (lc.getType() == LogColumn.LC_TIMESTAMP))
            {
               orderingColumn = lc;
            }
         }
      }

      WidgetHelper.restoreColumnSettings(table, "LogViewer." + logHandle.getName());
		viewer.setLabelProvider(createLabelProvider(logHandle));
		filterBuilder.setLogHandle(logHandle);
      if (onClone)
      {
         filterBuilder.setFilter(delayedQueryFilter);
      }
      else
      {
         if (orderingColumn != null)
         {
            List<OrderingColumn> orderingColumns = new ArrayList<OrderingColumn>(1);
            orderingColumns.add(new OrderingColumn(orderingColumn.getName(), orderingColumn.getDescription(), true));
            filter.setOrderingColumns(orderingColumns);
         }
         filterBuilder.setFilter(filter);         
      }
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
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(org.eclipse.jface.action.IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
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
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
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
      addKeyBinding("F9", actionExecute);

      actionClearFilter = new Action(i18n.tr("&Clear filter"), SharedIcons.CLEAR_LOG) {
			@Override
			public void run()
			{
				filterBuilder.clearFilter();
			}
		};
      addKeyBinding("M1+E", actionClearFilter);

      actionGetMoreData = new Action(i18n.tr("Get &more data"), ResourceManager.getImageDescriptor("icons/log-viewer/get-more-data.png")) {
			@Override
			public void run()
			{
				getMoreData();
			}
		};
		actionGetMoreData.setEnabled(false);
      addKeyBinding("M2+F9", actionGetMoreData);

      actionShowFilter = new Action(i18n.tr("Show &filter"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				showFilter(actionShowFilter.isChecked());
			}
		};
      actionShowFilter.setImageDescriptor(SharedIcons.FILTER);
		actionShowFilter.setChecked(true);
      addKeyBinding("M1+F2", actionShowFilter);

      actionCopyToClipboard = new Action(i18n.tr("&Copy to clipboard"), SharedIcons.COPY) {
			@Override
			public void run()
			{
				copySelectionToClipboard();
			}
		};
      addKeyBinding("M1+C", actionCopyToClipboard);

		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);

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
      onQueryStart();
		final LogFilter filter = filterBuilder.createFilter();
      new Job(String.format(i18n.tr("Querying server log \"%s\""), logName), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				logHandle.query(filter);
				final Table data = logHandle.retrieveData(0, PAGE_SIZE);
            runInUIThread(() -> {
               resultSet = data;
               viewer.setInput(resultSet.getAllRows());
               noData = (resultSet.getRowCount() < PAGE_SIZE);
				});
			}

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot query server log \"%s\""), logName);
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> onQueryComplete());
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

      onQueryStart();
      new Job(String.format(i18n.tr("Querying server log \"%s\""), logName), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table data = logHandle.retrieveData(resultSet.getRowCount(), PAGE_SIZE);
            runInUIThread(() -> {
               resultSet.addAll(data);
               viewer.setInput(resultSet.getAllRows());
               noData = (data.getRowCount() < PAGE_SIZE);
				});
			}

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot query server log \"%s\""), logName);
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> onQueryComplete());
         }
		}.start();
	}

	/**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      if (resultSet == null)
         return;

      onQueryStart();
      new Job(String.format(i18n.tr("Querying server log \"%s\""), logName), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final Table data = logHandle.retrieveData(0, resultSet.getRowCount(), true);
            runInUIThread(() -> {
               resultSet = data;
               viewer.setInput(resultSet.getAllRows());
               noData = (data.getRowCount() < PAGE_SIZE);
				});
			}

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Cannot query server log \"%s\""), logName);
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFinalize()
          */
         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> onQueryComplete());
         }
      }.start();
	}

   /**
    * Handles query start
    */
   private void onQueryStart()
   {
      actionExecute.setEnabled(false);
      actionGetMoreData.setEnabled(false);
      enableRefresh(false);
   }

   /**
    * Handles query completion
    */
   private void onQueryComplete()
   {
      actionExecute.setEnabled(true);
      actionGetMoreData.setEnabled(!noData);
      enableRefresh(true);
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

      new Job(i18n.tr("Getting log record details"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final LogRecordDetails recordDetails = logHandle.getRecordDetails(recordId);
            runInUIThread(() -> {
               recordDetailsViewer.showRecordDetails(recordDetails, record, logHandle, LogViewer.this);
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
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
	   if (!viewer.getControl().isDisposed())
	      viewer.getControl().setFocus();
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
	protected TableViewer getViewer()
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

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      return true;
   } 

   /**
    * @see org.netxms.nxmc.base.views.View#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getName();
   }

   /**
    * Memento to load context
    * 
    * @param memento
    */
   @Override
   public Object restoreContext(Memento memento)
   {      
      return null;
   }

   /**
    * @return the logName
    */
   public String getLogName()
   {
      return logName;
   }   

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("logName", logName);
      LogFilter filter = filterBuilder.createFilter();
      try
      {
         memento.set("filter", XMLTools.serialize(filter));
      }
      catch(Exception e)
      {
         logger.error("Failed to serialize filter", e);
         memento.set("filter", "");
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      logName = memento.getAsString("logName");
      recordDetailsViewer = LogRecordDetailsViewerRegistry.get(logName);
      setName(Registry.getSingleton(LogDescriptorRegistry.class).get(getLogName()).getViewTitle());
      setImage(ResourceManager.getImageDescriptor("icons/log-viewer/" + logName + ".png"));
      try
      {
         delayedQueryFilter = XMLTools.createFromXml(LogFilter.class, memento.getAsString("filter"));
      }
      catch(Exception e)
      {
         logger.error("Failed to load filter", e);
         throw new ViewNotRestoredException(i18n.tr("Failed to load filter"), e);
      }
   }
}
