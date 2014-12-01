/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.topology.views;

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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.FdbEntry;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.views.helpers.FDBComparator;
import org.netxms.ui.eclipse.topology.views.helpers.FDBLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Switch forwarding database view
 */
public class SwitchForwardingDatabaseView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.SwitchForwardingDatabaseView"; //$NON-NLS-1$
	
	public static final int COLUMN_MAC_ADDRESS = 0;
	public static final int COLUMN_PORT = 1;
	public static final int COLUMN_INTERFACE = 2;
   public static final int COLUMN_VLAN = 3;
	public static final int COLUMN_NODE = 4;
	
	private NXCSession session;
	private long rootObject;
	private SortableTableViewer viewer;
	private Action actionRefresh;
	private Action actionExportToCsv;
	private Action actionExportAllToCsv;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		try
		{
			rootObject = Long.parseLong(site.getSecondaryId());
		}
		catch(NumberFormatException e)
		{
			rootObject = 0;
		}

		session = (NXCSession)ConsoleSharedData.getSession();
		setPartName(String.format(Messages.get().SwitchForwardingDatabaseView_Title, session.getObjectName(rootObject)));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.get().SwitchForwardingDatabaseView_ColMacAddr, Messages.get().SwitchForwardingDatabaseView_ColPort, Messages.get().SwitchForwardingDatabaseView_ConIface, Messages.get().SwitchForwardingDatabaseView_ColVlan, Messages.get().SwitchForwardingDatabaseView_ColNode };
		final int[] widths = { 180, 100, 200, 100, 250 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_MAC_ADDRESS, SWT.DOWN, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new FDBLabelProvider());
		viewer.setComparator(new FDBComparator());
		
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "SwitchForwardingDatabase"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "SwitchForwardingDatabase"); //$NON-NLS-1$
			}
		});

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		refresh();
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
				refresh();
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
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
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
		viewer.getTable().setFocus();
	}

	/**
	 * Refresh content
	 */
	private void refresh()
	{
	   new ConsoleJob(Messages.get().SwitchForwardingDatabaseView_JobTitle, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<FdbEntry> fdb = session.getSwitchForwardingDatabase(rootObject);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  viewer.setInput(fdb.toArray());
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().SwitchForwardingDatabaseView_JobError, session.getObjectName(rootObject));
         }
      }.start();
	}
}
