/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Interfaces" tab
 */
public class InterfacesTab extends ObjectTab
{
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
   public static final int COLUMN_ALIAS = 2;
	public static final int COLUMN_TYPE = 3;
	public static final int COLUMN_INDEX = 4;
	public static final int COLUMN_SLOT = 5;
	public static final int COLUMN_PORT = 6;
   public static final int COLUMN_MTU = 7;
   public static final int COLUMN_SPEED = 8;
	public static final int COLUMN_DESCRIPTION = 9;
	public static final int COLUMN_MAC_ADDRESS = 10;
	public static final int COLUMN_IP_ADDRESS = 11;
	public static final int COLUMN_PEER_NAME = 12;
	public static final int COLUMN_PEER_MAC_ADDRESS = 13;
	public static final int COLUMN_PEER_IP_ADDRESS = 14;
   public static final int COLUMN_PEER_PROTOCOL = 15;
	public static final int COLUMN_ADMIN_STATE = 16;
	public static final int COLUMN_OPER_STATE = 17;
	public static final int COLUMN_EXPECTED_STATE = 18;
	public static final int COLUMN_STATUS = 19;
	public static final int COLUMN_8021X_PAE_STATE = 20;
	public static final int COLUMN_8021X_BACKEND_STATE = 21;
	
	private SortableTableViewer viewer;
	private InterfaceListLabelProvider labelProvider;
	private Action actionCopyToClipboard;
	private Action actionCopyMacAddressToClipboard;
	private Action actionCopyIpAddressToClipboard;
	private Action actionCopyPeerNameToClipboard;
	private Action actionCopyPeerMacToClipboard;
	private Action actionCopyPeerIpToClipboard;
	private Action actionExportToCsv;

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		final String[] names = { 
	      Messages.get().InterfacesTab_ColId, 
	      Messages.get().InterfacesTab_ColName,
	      "Alias",
	      Messages.get().InterfacesTab_ColIfType, 
	      Messages.get().InterfacesTab_ColIfIndex, 
	      Messages.get().InterfacesTab_ColSlot, 
	      Messages.get().InterfacesTab_ColPort,
	      "MTU",
         "Speed",
	      Messages.get().InterfacesTab_ColDescription, 
	      Messages.get().InterfacesTab_ColMacAddr,
	      Messages.get().InterfacesTab_ColIpAddr, 
	      Messages.get().InterfacesTab_ColPeerNode, 
	      Messages.get().InterfacesTab_ColPeerMAC, 
	      Messages.get().InterfacesTab_ColPeerIP,
	      Messages.get().InterfacesTab_PeerDiscoveryProtocol,
	      Messages.get().InterfacesTab_ColAdminState, 
	      Messages.get().InterfacesTab_ColOperState, 
	      Messages.get().InterfacesTab_ColExpState, 
	      Messages.get().InterfacesTab_ColStatus, 
	      Messages.get().InterfacesTab_Col8021xPAE, 
	      Messages.get().InterfacesTab_Col8021xBackend 
		};
		final int[] widths = { 60, 150, 150, 150, 70, 70, 70, 70, 90, 150, 100, 90, 150, 100, 90, 80, 80, 80, 80, 80, 80, 80 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		labelProvider = new InterfaceListLabelProvider();
		viewer.setLabelProvider(labelProvider);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new InterfaceListComparator());
		viewer.getTable().setHeaderVisible(true);
		viewer.getTable().setLinesVisible(true);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "InterfaceTable.V3"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InterfaceTable.V3"); //$NON-NLS-1$
			}
		});
		createActions();
		createPopupMenu();
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionCopyToClipboard = new Action(Messages.get().InterfacesTab_ActionCopy, SharedIcons.COPY) {
			@Override
			public void run()
			{
				copyToClipboard(-1);
			}
		};	

		actionCopyMacAddressToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyMAC) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_MAC_ADDRESS);
			}
		};	

		actionCopyIpAddressToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyIP) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_IP_ADDRESS);
			}
		};	

		actionCopyPeerNameToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerName) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_NAME);
			}
		};	

		actionCopyPeerMacToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerMAC) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_MAC_ADDRESS);
			}
		};	

		actionCopyPeerIpToClipboard = new Action(Messages.get().InterfacesTab_ActionCopyPeerIP) {
			@Override
			public void run()
			{
				copyToClipboard(COLUMN_PEER_IP_ADDRESS);
			}
		};	
		
		actionExportToCsv = new ExportToCsvAction(getViewPart(), viewer, true);
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
			public void menuAboutToShow(IMenuManager manager)
			{
				fillContextMenu(manager);
			}
		});

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);

		// Register menu for extension.
		if (getViewPart() != null)
			getViewPart().getSite().registerContextMenu(menuMgr, viewer);
	}

	/**
	 * Fill context menu
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionCopyToClipboard);
		manager.add(actionCopyMacAddressToClipboard);
		manager.add(actionCopyIpAddressToClipboard);
		manager.add(actionCopyPeerNameToClipboard);
		manager.add(actionCopyPeerMacToClipboard);
		manager.add(actionCopyPeerIpToClipboard);
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_CREATION));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_MANAGEMENT));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_BINDING));
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_TOPOLOGY));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_DATA_COLLECTION));
		
		if (((IStructuredSelection)viewer.getSelection()).size() == 1)
		{
			manager.add(new Separator());
			manager.add(new GroupMarker(GroupMarkers.MB_PROPERTIES));
			manager.add(new PropertyDialogAction(getViewPart().getSite(), viewer));
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void currentObjectUpdated(AbstractObject object)
	{
		objectChanged(object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
	 */
	@Override
	public void refresh()
	{
		if (getObject() != null)
			viewer.setInput(getObject().getAllChilds(AbstractObject.OBJECT_INTERFACE).toArray());
		else
			viewer.setInput(new Interface[0]);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(final AbstractObject object)
	{
		labelProvider.setNode((AbstractNode)object);
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof AbstractNode);
	}

	/* (non-Javadoc)
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#getSelectionProvider()
    */
   @Override
   public ISelectionProvider getSelectionProvider()
   {
      return viewer;
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
