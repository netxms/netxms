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
package org.netxms.ui.eclipse.objectview.objecttabs;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.InterfaceListLabelProvider;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Interfaces" tab
 */
public class InterfacesTab extends ObjectTab
{
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_TYPE = 2;
	public static final int COLUMN_INDEX = 3;
	public static final int COLUMN_SLOT = 4;
	public static final int COLUMN_PORT = 5;
	public static final int COLUMN_DESCRIPTION = 6;
	public static final int COLUMN_MAC_ADDRESS = 7;
	public static final int COLUMN_IP_ADDRESS = 8;
	public static final int COLUMN_PEER_NAME = 9;
	public static final int COLUMN_PEER_MAC_ADDRESS = 10;
	public static final int COLUMN_PEER_IP_ADDRESS = 11;
	public static final int COLUMN_ADMIN_STATE = 12;
	public static final int COLUMN_OPER_STATE = 13;
	public static final int COLUMN_EXPECTED_STATE = 14;
	public static final int COLUMN_STATUS = 15;
	public static final int COLUMN_8021X_PAE_STATE = 16;
	public static final int COLUMN_8021X_BACKEND_STATE = 17;
	
	private SortableTableViewer viewer;
	private InterfaceListLabelProvider labelProvider;
	private Action actionExportToCsv;

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		final String[] names = { "ID", "Name", "ifType", "ifIndex", "Slot", "Port", "Description", "MAC Address", 
		                         "IP Address", "Peer Node", "Peer MAC", "Peer IP", 
				                   "Admin State", "Oper State", "Exp. State", "Status", "802.1x PAE", "802.1x Backend" };
		final int[] widths = { 60, 150, 90, 70, 70, 70, 150, 100, 90, 150, 100, 90, 80, 80, 80, 80, 80, 80 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		labelProvider = new InterfaceListLabelProvider();
		viewer.setLabelProvider(labelProvider);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new InterfaceListComparator());
		viewer.getTable().setHeaderVisible(true);
		viewer.getTable().setLinesVisible(true);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "InterfaceTable");
		viewer.getTable().addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "InterfaceTable");
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
			private static final long serialVersionUID = 1L;

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
		manager.add(actionExportToCsv);
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_CREATION));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_OBJECT_BINDING));
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_TOPOLOGY));
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_DATA_COLLECTION));
		
		if (((IStructuredSelection)viewer.getSelection()).size() == 1)
		{
			manager.add(new Separator());
			manager.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
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
		labelProvider.setNode((Node)object);
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof Node);
	}
}
