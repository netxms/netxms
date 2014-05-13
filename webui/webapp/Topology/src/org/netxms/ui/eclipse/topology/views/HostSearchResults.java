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

import java.util.ArrayList;
import java.util.List;
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
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ConnectionPointType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.views.helpers.ConnectionPointComparator;
import org.netxms.ui.eclipse.topology.views.helpers.ConnectionPointLabelProvider;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Search results for host connection searches
 *
 */
public class HostSearchResults extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.HostSearchResults"; //$NON-NLS-1$
	
	public static final int COLUMN_SEQUENCE = 0;
	public static final int COLUMN_NODE = 1;
	public static final int COLUMN_INTERFACE = 2;
	public static final int COLUMN_MAC_ADDRESS = 3;
	public static final int COLUMN_IP_ADDRESS = 4;
	public static final int COLUMN_SWITCH = 5;
	public static final int COLUMN_PORT = 6;
	public static final int COLUMN_TYPE = 7;
	
	private static final String TABLE_CONFIG_PREFIX = "HostSearchResults"; //$NON-NLS-1$
	
	private SortableTableViewer viewer;
	private List<ConnectionPoint> results = new ArrayList<ConnectionPoint>();
	private Action actionClearLog;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		final String[] names = { Messages.get().HostSearchResults_ColSeq, Messages.get().HostSearchResults_ColNode, Messages.get().HostSearchResults_ColIface, Messages.get().HostSearchResults_ColMac, Messages.get().HostSearchResults_ColIp, Messages.get().HostSearchResults_ColSwitch, Messages.get().HostSearchResults_ColPort, Messages.get().HostSearchResults_ColType };
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
		actionClearLog = new Action(Messages.get().HostSearchResults_ClearLog) {
			@Override
			public void run()
			{
				results.clear();
				viewer.setInput(results.toArray());
			}
		};
		actionClearLog.setImageDescriptor(SharedIcons.CLEAR_LOG);
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
		/*
		manager.add(actionCopyRecord);
		manager.add(actionCopyIP);
		manager.add(actionCopyMAC);
		manager.add(new Separator());
		*/
		manager.add(actionClearLog);
	}

	/**
	 * Fill local toolbar
	 * @param manager menu manager
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		/*
		manager.add(actionCopyRecord);
		manager.add(new Separator());
		*/
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
		/*
		manager.add(actionCopyRecord);
		manager.add(actionCopyIP);
		manager.add(actionCopyMAC);
		manager.add(new Separator());
		*/
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
			MessageDialogHelper.openWarning(shell, Messages.get().HostSearchResults_Warning, Messages.get().HostSearchResults_NotFound);
			return;
		}
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		try
		{
			Node host = (Node)session.findObjectById(cp.getLocalNodeId());
			Node bridge = (Node)session.findObjectById(cp.getNodeId());
			AbstractObject iface = session.findObjectById(cp.getInterfaceId());
			
			if ((bridge != null) && (iface != null))
			{
			   if (cp.getType() == ConnectionPointType.WIRELESS)
			   {
               if (host != null)
               {
                  MessageDialogHelper.openInformation(shell, Messages.get().HostSearchResults_ConnectionPoint, 
                        String.format("Node %1$s is connected to wireless access point %2$s/%3$s",
                              host.getObjectName(), bridge.getObjectName(), iface.getObjectName()));
               }
               else
               {
                  if (cp.getLocalIpAddress() != null)
                  {
                     MessageDialogHelper.openInformation(shell, Messages.get().HostSearchResults_ConnectionPoint, 
                           String.format("Node with IP address %1$s and MAC address %2$s is connected to wireless access point %3$s/%4$s",
                                 cp.getLocalIpAddress().getHostAddress(), cp.getLocalMacAddress(),
                                 bridge.getObjectName(), iface.getObjectName()));
                  }
                  else
                  {
                     MessageDialogHelper.openInformation(shell, Messages.get().HostSearchResults_ConnectionPoint, 
                           String.format("Node with MAC address %1$s is connected to wireless access point %2$s/%3$s",
                                 cp.getLocalMacAddress(), bridge.getObjectName(), iface.getObjectName()));
                  }
               }
			   }
			   else
			   {
   				if (host != null)
   				{
   					MessageDialogHelper.openInformation(shell, Messages.get().HostSearchResults_ConnectionPoint, 
   					      String.format(Messages.get().HostSearchResults_NodeConnected,
   					            host.getObjectName(), cp.getType() == ConnectionPointType.DIRECT ? Messages.get().HostSearchResults_ModeDirectly : Messages.get().HostSearchResults_ModeIndirectly,
   					            bridge.getObjectName(), iface.getObjectName()));
   				}
   				else
   				{
   				   if (cp.getLocalIpAddress() != null)
   				   {
   	               MessageDialogHelper.openInformation(shell, Messages.get().HostSearchResults_ConnectionPoint, 
   	                     String.format(Messages.get().HostSearchResults_NodeIpMacConnected,
   	                           cp.getLocalIpAddress().getHostAddress(), cp.getLocalMacAddress(),
   	                           cp.getType() == ConnectionPointType.DIRECT ? Messages.get().HostSearchResults_ModeDirectly : Messages.get().HostSearchResults_ModeIndirectly,
   	                           bridge.getObjectName(), iface.getObjectName()));
   				   }
   				   else
   				   {
                     MessageDialogHelper.openInformation(shell, Messages.get().HostSearchResults_ConnectionPoint, 
                           String.format(Messages.get().HostSearchResults_NodeMacConnected,
                                 cp.getLocalMacAddress(),
                                 cp.getType() == ConnectionPointType.DIRECT ? Messages.get().HostSearchResults_ModeDirectly : Messages.get().HostSearchResults_ModeIndirectly,
                                 bridge.getObjectName(), iface.getObjectName()));
   				   }
   				}
			   }
			   
				IWorkbenchPage page = PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage();
				IViewPart part = page.showView(ID);
				((HostSearchResults)part).addResult(cp);
			}
			else
			{
				MessageDialogHelper.openWarning(shell, Messages.get().HostSearchResults_Warning, Messages.get().HostSearchResults_NotFound);
			}
		}
		catch(Exception e)
		{
			MessageDialogHelper.openWarning(shell, Messages.get().HostSearchResults_Warning, String.format(Messages.get().HostSearchResults_ShowError, e.getLocalizedMessage()));
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
}
