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
package org.netxms.ui.eclipse.logviewer.views;

import java.util.Collection;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ExpandBar;
import org.eclipse.swt.widgets.ExpandItem;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCAccessListElement;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.log.Log;
import org.netxms.client.log.LogColumn;
import org.netxms.client.log.LogFilter;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.logviewer.Activator;
import org.netxms.ui.eclipse.logviewer.Messages;
import org.netxms.ui.eclipse.logviewer.dialogs.QueryBuilder;
import org.netxms.ui.eclipse.logviewer.views.helpers.LogLabelProvider;
import org.netxms.ui.eclipse.logviewer.widgets.FilterBuilder;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.RefreshAction;

/**
 * @author Victor
 *
 */
public class LogViewer extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.logviewer.view.log_viewer";
	public static final String JOB_FAMILY = "LogViewerJob";
	
	private static final int PAGE_SIZE = 400;
	private static final int FIRST_PAGE = 0;
	private static final int LAST_PAGE = 255;
	private static final int PREV_PAGE = -1;
	private static final int NEXT_PAGE = 1;
		
	private NXCSession session;
	private FilterBuilder filterBuilder;
	private TableViewer viewer;
	private String logName;
	private Log logHandle;
	private LogFilter filter;
	private Image titleImage = null;
	private RefreshAction actionRefresh;
	private Action actionDoQuery;
	private Action actionGoFirstPage;
	private Action actionGoLastPage;
	private Action actionGoNextPage;
	private Action actionGoPrevPage;
	private Table resultSet;
	private long currentPosition;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = NXMCSharedData.getInstance().getSession();
		logName = site.getSecondaryId();
		setPartName(Messages.getString("LogViewer_" + logName));
		final ImageDescriptor img = Activator.getImageDescriptor("icons/" + logName + ".png");
		if (img != null)
		{
			titleImage = img.createImage();
			this.setTitleImage(titleImage);
		}
		filter = new LogFilter();

		// Initiate loading of user manager plugin if it was not loaded before
		Platform.getAdapterManager().loadAdapter(new NXCAccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter");
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
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

		makeActions();
		contributeToActionBars();
		
		new ConsoleJob("Open log \"" + logName + "\"", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot open server log file \"" + logName + "\"";
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				logHandle = session.openServerLog(logName);
				new UIJob("Setup log viewer for \"" + logName + "\" log")
				{
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						setupLogViewer();
						return Status.OK_STATUS;
					}		
				}.schedule();
			}
		}.start();
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
		viewer.setLabelProvider(new LogLabelProvider(logHandle));
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
		manager.add(actionDoQuery);
		manager.add(new Separator());
		manager.add(actionGoFirstPage);
		manager.add(actionGoPrevPage);
		manager.add(actionGoNextPage);
		manager.add(actionGoLastPage);
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
		manager.add(actionGoFirstPage);
		manager.add(actionGoPrevPage);
		manager.add(actionGoNextPage);
		manager.add(actionGoLastPage);
		manager.add(new Separator());
		manager.add(actionDoQuery);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create actions
	 */
	private void makeActions()
	{
		actionRefresh = new RefreshAction()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
			}
		};

		actionDoQuery = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				doQuery();
			}
		};
		actionDoQuery.setText("New &query");
		actionDoQuery.setImageDescriptor(Activator.getImageDescriptor("icons/do_query.png"));

		actionGoFirstPage = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				goToPage(FIRST_PAGE);
			}
		};
		actionGoFirstPage.setText("&First page");
		actionGoFirstPage.setImageDescriptor(Activator.getImageDescriptor("icons/first_page.png"));

		actionGoLastPage = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				goToPage(LAST_PAGE);
			}
		};
		actionGoLastPage.setText("&Last page");
		actionGoLastPage.setImageDescriptor(Activator.getImageDescriptor("icons/last_page.png"));

		actionGoNextPage = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				goToPage(NEXT_PAGE);
			}
		};
		actionGoNextPage.setText("&Next page");
		actionGoNextPage.setImageDescriptor(Activator.getImageDescriptor("icons/next_page.png"));

		actionGoPrevPage = new Action()
		{
			/* (non-Javadoc)
			 * @see org.eclipse.jface.action.Action#run()
			 */
			@Override
			public void run()
			{
				goToPage(PREV_PAGE);
			}
		};
		actionGoPrevPage.setText("&Previous page");
		actionGoPrevPage.setImageDescriptor(Activator.getImageDescriptor("icons/prev_page.png"));
	}
	
	/**
	 * Prepare filter and execute query on server
	 */
	private void doQuery()
	{
		final QueryBuilder dlg = new QueryBuilder(getSite().getShell(), logHandle, filter);
		if (dlg.open() == Window.OK)
		{
			filter = dlg.getFilter();
			new ConsoleJob("Query server log", this, Activator.PLUGIN_ID, JOB_FAMILY) {
				@Override
				protected String getErrorMessage()
				{
					return "Cannot query server log " + logName;
				}

				@Override
				protected void runInternal(IProgressMonitor monitor) throws Exception
				{
					logHandle.query(filter);
					currentPosition = 0;
					resultSet = logHandle.retrieveData(currentPosition, PAGE_SIZE);
					new UIJob("Update log viewer") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							viewer.setInput(resultSet.getAllRows());
							return Status.OK_STATUS;
						}
					}.schedule();
				}
			}.start();
		}
	}
	
	/**
	 * Go to specified page
	 * 
	 * @param page Page to go to (one of the FIRST_PAGE, LAST_PAGE, NEXT_PAGE, PREV_PAGE)
	 */
	private void goToPage(int page)
	{
		switch(page)
		{
			case FIRST_PAGE:
				currentPosition = 0;
				break;
			case LAST_PAGE:
				currentPosition = logHandle.getNumRecords() - PAGE_SIZE;
				if (currentPosition < 0)
					currentPosition = 0;
				break;
			case NEXT_PAGE:
				if (logHandle.getNumRecords() - currentPosition > PAGE_SIZE)
					currentPosition += PAGE_SIZE;
				break;
			case PREV_PAGE:
				currentPosition -= PAGE_SIZE;
				if (currentPosition < 0)
					currentPosition = 0;
				break;
		}
		
		new ConsoleJob("Get log page from server", this, Activator.PLUGIN_ID, JOB_FAMILY) {
			@Override
			protected String getErrorMessage()
			{
				return "Cannot query server log " + logName;
			}

			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				resultSet = logHandle.retrieveData(currentPosition, PAGE_SIZE);
				new UIJob("Update log viewer") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						viewer.setInput(resultSet.getAllRows());
						return Status.OK_STATUS;
					}
				}.schedule();
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
}
