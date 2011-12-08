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
package org.netxms.ui.eclipse.agentmanager.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.packages.PackageInfo;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.agentmanager.Activator;
import org.netxms.ui.eclipse.agentmanager.views.helpers.PackageLabelProvider;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Agent package manager
 */
public class PackageManager extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.agentmanager.views.PackageManager";
	
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_VERSION = 2;
	public static final int COLUMN_PLATFORM = 3;
	public static final int COLUMN_FILE = 4;
	public static final int COLUMN_DESCRIPTION = 5;

	private List<PackageInfo> packageList = null;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionInstall;
	private Action actionRemove;
	private Action actionDeploy;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "ID", "Name", "Version", "Platform", "File", "Description" };
		final int[] widths = { 70, 120, 90, 120, 150, 400 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new PackageLabelProvider());

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getTable().setFocus();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refresh();
			}
		};
		
		actionInstall = new Action("&Install new package...") {
			@Override
			public void run()
			{
				installPackage();
			}
		};
		
		actionRemove = new Action("&Remove") {
			@Override
			public void run()
			{
				removePackage();
			}
		};
		
		actionDeploy = new Action("&Deploy to managed nodes...") {
			@Override
			public void run()
			{
				deployPackage();
			}
		};
	}

	/**
	 * Contribute actions to action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pulldown menu
	 * @param manager menu manager
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionInstall);
		manager.add(actionRemove);
		manager.add(actionDeploy);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
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
		menuMgr.addMenuListener(new IMenuListener()
		{
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
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionInstall);
		manager.add(actionRemove);
		manager.add(actionDeploy);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
	}
	
	/**
	 * Refresh package list
	 */
	private void refresh()
	{
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob("Load package list", this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final List<PackageInfo> list = session.getInstalledPackages();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						packageList = list;
						viewer.setInput(list.toArray());
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return "Cannot get package list from server";
			}
		}.start();
	}
	
	private void installPackage()
	{
		
	}
	
	private void removePackage()
	{
		
	}
	
	private void deployPackage()
	{
		
	}
}
