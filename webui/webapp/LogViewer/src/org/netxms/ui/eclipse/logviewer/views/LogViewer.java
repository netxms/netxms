/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.logviewer.views;

import java.util.Collection;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.commands.ActionHandler;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.logviewer.Activator;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.logviewer.views.helpers.LogLabelProvider;
import org.netxms.ui.eclipse.logviewer.widgets.FilterBuilder;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Universal log viewer
 */
public class LogViewer extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.logviewer.view.log_viewer"; //$NON-NLS-1$
	public static final String JOB_FAMILY = "LogViewerJob"; //$NON-NLS-1$
	
	private static final int PAGE_SIZE = 400;
		
	private NXCSession session;
	private FilterBuilder filterBuilder;
	private TableViewer viewer;
	private String logName;
	private Log logHandle;
	private LogFilter filter;
	private Image titleImage = null;
	private RefreshAction actionRefresh;
	private Action actionExecute;
	private Action actionClearFilter;
	private Action actionShowFilter;
	private Action actionGetMoreData;
	private Action actionCopyToClipboard;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	private Table resultSet;
	private boolean noData = false;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		logName = site.getSecondaryId();
		setPartName(Messages.getString(Messages.LogViewer_2 + logName));
		final ImageDescriptor img = Activator.getImageDescriptor("icons/" + logName + ".png"); //$NON-NLS-1$ //$NON-NLS-2$
		if (img != null)
		{
			titleImage = img.createImage();
			this.setTitleImage(titleImage);
		}
		filter = new LogFilter();

		// Initiate loading of user manager plugin if it was not loaded before
		Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
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
		viewer.getTable().addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if (logHandle != null)
					WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "LogViewer." + logHandle.getName()); //$NON-NLS-1$
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
		contributeToActionBars();
		createPopupMenu();
		
		new ConsoleJob(Messages.LogViewer_7 + logName + Messages.LogViewer_8, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.LogViewer_9 + logName + Messages.LogViewer_10;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				logHandle = session.openServerLog(logName);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						setupLogViewer();
					}
				});
			}
		}.start();
		
		activateContext();
		
		filterBuilder.setExecuteAction(actionExecute);
		filterBuilder.setCloseAction(new Action() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				actionShowFilter.setChecked(false);
				showFilter(false);
			}
		});
	}
	
	/**
	 * Activate context
	 */
	private void activateContext()
	{
		IContextService contextService = (IContextService)getSite().getService(IContextService.class);
		if (contextService != null)
		{
			contextService.activateContext("org.netxms.ui.eclipse.logviewer.context.LogViewer"); //$NON-NLS-1$
		}
	}

	/**
	 * Estimate required column width
	 */
	private int estimateColumnWidth(final LogColumn lc)
	{
		switch(lc.getType())
		{
			case LogColumn.LC_INTEGER:
				return 80;
			case LogColumn.LC_TIMESTAMP:
				return 120;
			case LogColumn.LC_SEVERITY:
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
		}
		WidgetHelper.restoreColumnSettings(table, Activator.getDefault().getDialogSettings(), "LogViewer." + logHandle.getName()); //$NON-NLS-1$
		viewer.setLabelProvider(new LogLabelProvider(logHandle));
		filterBuilder.setLogHandle(logHandle);
	}

	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionExecute);
		manager.add(actionClearFilter);
		manager.add(actionShowFilter);
		manager.add(new Separator());
		manager.add(actionGetMoreData);
		manager.add(new Separator());
		manager.add(actionExportAllToCsv);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionExecute);
		manager.add(actionClearFilter);
		manager.add(new Separator());
		manager.add(actionGetMoreData);
		manager.add(new Separator());
		manager.add(actionExportAllToCsv);
		manager.add(new Separator());
		manager.add(actionRefresh);
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
			private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * 
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(final IMenuManager mgr)
	{
		mgr.add(actionCopyToClipboard);
		mgr.add(actionExportToCsv);
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
		
		actionRefresh = new RefreshAction(this) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				refreshData();
			}
		};
		actionRefresh.setEnabled(false);

		actionExecute = new Action(Messages.LogViewer_ActionExec, SharedIcons.EXECUTE) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				doQuery();
			}
		};
		actionExecute.setActionDefinitionId("org.netxms.ui.eclipse.logviewer.commands.execute"); //$NON-NLS-1$
		handlerService.activateHandler(actionExecute.getActionDefinitionId(), new ActionHandler(actionExecute));

		actionClearFilter = new Action(Messages.LogViewer_ActionClearFilter, SharedIcons.CLEAR_LOG) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				filterBuilder.clearFilter();
			}
		};

		actionGetMoreData = new Action(Messages.LogViewer_ActionGetMoreData, Activator.getImageDescriptor("icons/get_more_data.png")) { //$NON-NLS-2$
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				getMoreData();
			}
		};
		actionGetMoreData.setEnabled(false);
		actionGetMoreData.setActionDefinitionId("org.netxms.ui.eclipse.logviewer.commands.get_more_data"); //$NON-NLS-1$
		handlerService.activateHandler(actionGetMoreData.getActionDefinitionId(), new ActionHandler(actionGetMoreData));

		actionShowFilter = new Action(Messages.LogViewer_ActionShowFilter, Action.AS_CHECK_BOX) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				showFilter(actionShowFilter.isChecked());
			}
		};
		actionShowFilter.setChecked(true);
      actionShowFilter.setActionDefinitionId("org.netxms.ui.eclipse.logviewer.commands.show_filter"); //$NON-NLS-1$
		handlerService.activateHandler(actionShowFilter.getActionDefinitionId(), new ActionHandler(actionShowFilter));
		
		actionCopyToClipboard = new Action(Messages.LogViewer_ActionCopy, SharedIcons.COPY) {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				copySelectionToClipboard();
			}
		};
      actionCopyToClipboard.setActionDefinitionId("org.netxms.ui.eclipse.library.commands.copy"); //$NON-NLS-1$
		handlerService.activateHandler(actionCopyToClipboard.getActionDefinitionId(), new ActionHandler(actionCopyToClipboard));
		
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
	}
	
	/**
	 * Prepare filter and execute query on server
	 */
	private void doQuery()
	{
		actionRefresh.setEnabled(false);
		actionGetMoreData.setEnabled(false);
		filter = filterBuilder.createFilter();
		new ConsoleJob(Messages.LogViewer_QueryJob, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.LogViewer_QueryJobError + logName;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
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
						actionRefresh.setEnabled(true);
					}
				});
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
		
		new ConsoleJob(Messages.LogViewer_GetDataJob, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.LogViewer_QueryError + logName;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
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
		}.start();
	}

	/**
	 * Refresh existing dataset
	 */
	private void refreshData()
	{
		new ConsoleJob(Messages.LogViewer_RefreshJob, this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return Messages.LogViewer_RefreshError + logName;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
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
		}.start();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if (titleImage != null)
			titleImage.dispose();
		super.dispose();
	}
	
	/**
	 * @param show
	 */
	private void showFilter(boolean show)
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
			final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n"; //$NON-NLS-1$ //$NON-NLS-2$
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
}
