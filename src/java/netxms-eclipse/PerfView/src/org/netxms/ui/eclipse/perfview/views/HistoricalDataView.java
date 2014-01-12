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
package org.netxms.ui.eclipse.perfview.views;

import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.MobileDevice;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.perfview.Activator;
import org.netxms.ui.eclipse.perfview.Messages;
import org.netxms.ui.eclipse.perfview.dialogs.HistoricalDataSelectionDialog;
import org.netxms.ui.eclipse.perfview.views.helpers.HistoricalDataLabelProvider;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Historical data view
 */
public class HistoricalDataView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.perfview.views.HistoricalDataView"; //$NON-NLS-1$
	
	private NXCSession session;
	private long nodeId;
	private long dciId;
	private String nodeName;
	private SortableTableViewer viewer;
	private Date timeFrom = null;
	private Date timeTo = null;
	private int recordLimit = 4096;
	private boolean updateInProgress = false;
	private Action actionRefresh;
	private Action actionSelectRange;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		
		session = (NXCSession)ConsoleSharedData.getSession();
		
		// Secondary ID must by in form nodeId&dciId
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
		if (parts.length != 2)
			throw new PartInitException("Internal error"); //$NON-NLS-1$
		
		nodeId = Long.parseLong(parts[0]);
		AbstractObject object = session.findObjectById(nodeId);
		if ((object == null) || (!(object instanceof AbstractNode) && !(object instanceof MobileDevice) && !(object instanceof Cluster)))
			throw new PartInitException(Messages.get().HistoricalDataView_InvalidObjectID);
		nodeName = object.getObjectName();
		
		dciId = Long.parseLong(parts[1]);
		
		setPartName(nodeName + ": [" + Long.toString(dciId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.get().HistoricalDataView_ColTimestamp, Messages.get().HistoricalDataView_ColValue };
		final int[] widths = { 150, 400 };
		viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new HistoricalDataLabelProvider());

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		refreshData();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshData();
			}
		};
		
		actionSelectRange = new Action(Messages.get().HistoricalDataView_8) {
			@Override
			public void run()
			{
				selectRange();
			}
		};
		
		actionExportToCsv = new ExportToCsvAction(this, viewer, true);
		actionExportAllToCsv = new ExportToCsvAction(this, viewer, false);
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
		manager.add(actionSelectRange);
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
		manager.add(actionExportAllToCsv);
		manager.add(actionRefresh);
	}

	/**
	 * Create pop-up menu
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param manager
	 */
	private void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionSelectRange);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/**
	 * Refresh data
	 */
	private void refreshData()
	{
		if (updateInProgress)
			return;
		updateInProgress = true;
		
		new ConsoleJob(Messages.get().HistoricalDataView_9, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final DciData data = session.getCollectedData(nodeId, dciId, timeFrom, timeTo, recordLimit);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						viewer.setInput(data.getValues());
						updateInProgress = false;
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().HistoricalDataView_10 + nodeName + Messages.get().HistoricalDataView_11 + Long.toString(dciId) + Messages.get().HistoricalDataView_12;
			}
		}.start();
	}
	
	/**
	 * Select data range for display
	 */
	private void selectRange()
	{
		HistoricalDataSelectionDialog dlg = new HistoricalDataSelectionDialog(getSite().getShell(), recordLimit, timeFrom, timeTo);
		if (dlg.open() == Window.OK)
		{
			recordLimit = dlg.getMaxRecords();
			timeFrom = dlg.getTimeFrom();
			timeTo = dlg.getTimeTo();
			refreshData();
		}
	}
}
