/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.views.helpers.ConnectionPointComparator;
import org.netxms.ui.eclipse.topology.views.helpers.ConnectionPointLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Search results for host connection searches
 *
 */
public class HostSearchResults extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.HostSearchResults";
	
	public static final int COLUMN_SEQUENCE = 0;
	public static final int COLUMN_NODE = 1;
	public static final int COLUMN_INTERFACE = 2;
	public static final int COLUMN_MAC_ADDRESS = 3;
	public static final int COLUMN_IP_ADDRESS = 4;
	public static final int COLUMN_SWITCH = 5;
	public static final int COLUMN_PORT = 6;
	public static final int COLUMN_TYPE = 7;
	
	private static final String TABLE_CONFIG_PREFIX = "HostSearchResults";
	
	private SortableTableViewer viewer;
	private List<ConnectionPoint> results = new ArrayList<ConnectionPoint>();
	private Action actionClearLog;
	private Action actionCopyMAC;
	private Action actionCopyIP;
	private Action actionCopyRecord;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { "Seq.", "Node", "Interface", "MAC", "IP", "Switch", "Port", "Type" };
		final int[] widths = { 70, 120, 120, 90, 90, 120, 120, 60 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_SEQUENCE, SWT.UP, SWT.MULTI | SWT.FULL_SELECTION);
		viewer.setContentProvider(new ArrayContentProvider());
		ConnectionPointLabelProvider labelProvider = new ConnectionPointLabelProvider();
		viewer.setLabelProvider(labelProvider);
		viewer.setComparator(new ConnectionPointComparator(labelProvider));
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
		
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), TABLE_CONFIG_PREFIX);
			}
		});
		
		createActions();
		contributeToActionBars();
		createPopupMenu();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionClearLog = new Action("Clear search log") {
			@Override
			public void run()
			{
				results.clear();
				viewer.setInput(results.toArray());
			}
		};
		actionClearLog.setImageDescriptor(SharedIcons.CLEAR_LOG);
		
		actionCopyIP = new Action("Copy IP address to clipboard") {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_IP_ADDRESS);
			}
		};
		
		actionCopyMAC = new Action("Copy MAC address to clipboard") {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_MAC_ADDRESS);
			}
		};
		
		actionCopyRecord = new Action("&Copy to clipboard", SharedIcons.COPY) {
			@Override
			public void run()
			{
				copyToClipboard(-1);
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
		manager.add(actionCopyRecord);
		manager.add(actionCopyIP);
		manager.add(actionCopyMAC);
		manager.add(new Separator());
		manager.add(actionClearLog);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionCopyRecord);
		manager.add(new Separator());
		manager.add(actionClearLog);
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
		manager.add(actionCopyRecord);
		manager.add(actionCopyIP);
		manager.add(actionCopyMAC);
		manager.add(new Separator());
		manager.add(actionClearLog);
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
	 * Add search result to view
	 * 
	 * @param cp connection point information
	 */
	private void addResult(ConnectionPoint cp)
	{
		cp.setData(results.size());
		results.add(cp);
		viewer.setInput(results.toArray());
	}

	/**
	 * Show connection point information
	 * @param cp connection point information
	 */
	public static void showConnection(ConnectionPoint cp)
	{
		final Shell shell = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getShell();
		
		if (cp == null)
		{
			MessageDialog.openWarning(shell, "Warning", "Connection point information cannot be found");
			return;
		}
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		try
		{
			Node host = (Node)session.findObjectById(cp.getLocalNodeId());
			Node bridge = (Node)session.findObjectById(cp.getNodeId());
			Interface iface = (Interface)session.findObjectById(cp.getInterfaceId());
			
			if ((bridge != null) && (iface != null))
			{
				if (host != null)
				{
					MessageDialog.openInformation(shell, "Connection Point", "Node " + host.getObjectName() + " is " + (cp.isDirectlyConnected() ? "directly" : "indirectly") + " connected to network switch " + bridge.getObjectName() + " port " + iface.getObjectName());
				}
				else
				{
					String ipAddress = (cp.getLocalIpAddress() != null) ? "IP address " + cp.getLocalIpAddress().getHostAddress() + " and " : "";
					MessageDialog.openInformation(shell, "Connection Point", "Node with " + ipAddress + "MAC address " + cp.getLocalMacAddress() + " is " + (cp.isDirectlyConnected() ? "directly" : "indirectly") + " connected to network switch " + bridge.getObjectName() + " port " + iface.getObjectName());
				}
				
				IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
				IViewPart part = page.showView(ID);
				((HostSearchResults)part).addResult(cp);
			}
			else
			{
				MessageDialog.openWarning(shell, "Warning", "Connection point information cannot be found");
			}
		}
		catch(Exception e)
		{
			MessageDialog.openWarning(shell, "Warning", "Connection point information cannot be shown: " + e.getMessage());
		}
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		super.dispose();
	}
	
	/**
	 * Copy content to clipboard
	 * 
	 * @param column column number or -1 to copy all columns
	 */
	private void copyToClipboard(int column)
	{
		final TableItem[] selection = viewer.getTable().getSelection();
		if (selection.length > 0)
		{
			final String newLine = Platform.getOS().equals(Platform.OS_WIN32) ? "\r\n" : "\n"; //$NON-NLS-1$ //$NON-NLS-2$
			final StringBuilder sb = new StringBuilder();
			for(int i = 0; i < selection.length; i++)
			{
				if (i > 0)
					sb.append(newLine);
				if (column == -1)
				{
					for(int j = 0; j < viewer.getTable().getColumnCount(); j++)
					{
						if (j > 0)
							sb.append('\t');
						sb.append(selection[i].getText(j));
					}
				}
				else
				{
					sb.append(selection[i].getText(column));
				}
			}
			WidgetHelper.copyToClipboard(sb.toString());
		}
	}
}
