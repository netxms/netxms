/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.util.ArrayList;
import java.util.List;
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
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.client.objects.Subnet;
import org.netxms.ui.eclipse.actions.ExportToCsvAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.objectview.Activator;
import org.netxms.ui.eclipse.objectview.Messages;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.NodeListComparator;
import org.netxms.ui.eclipse.objectview.objecttabs.helpers.NodeListLabelProvider;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * "Nodes" tab
 */
public class NodesTab extends ObjectTab
{
	public static final int COLUMN_ID = 0;
	public static final int COLUMN_NAME = 1;
	public static final int COLUMN_IP_ADDRESS = 2;
   public static final int COLUMN_PLATFORM = 3;
   public static final int COLUMN_AGENT_VERSION = 4;
   public static final int COLUMN_SYS_DESCRIPTION = 5;
   public static final int COLUMN_STATUS = 6;
	
	private SortableTableViewer viewer;
	private Action actionExportToCsv;

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		final String[] names = {
		      Messages.get().NodesTab_ColId,
		      Messages.get().NodesTab_ColName,
		      Messages.get().NodesTab_ColPrimaryIP,
		      Messages.get().NodesTab_ColPlatform,
		      Messages.get().NodesTab_ColAgentVersion,
		      Messages.get().NodesTab_ColSysDescr,
		      Messages.get().NodesTab_ColStatus
		};
		final int[] widths = { 60, 150, 100, 150, 100, 300, 100 };
		viewer = new SortableTableViewer(parent, names, widths, COLUMN_NAME, SWT.UP, SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setLabelProvider(new NodeListLabelProvider());
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setComparator(new NodeListComparator());
		viewer.getTable().setHeaderVisible(true);
		viewer.getTable().setLinesVisible(true);
		WidgetHelper.restoreTableViewerSettings(viewer, Activator.getDefault().getDialogSettings(), "NodeTable"); //$NON-NLS-1$
		viewer.getTable().addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				WidgetHelper.saveColumnSettings(viewer.getTable(), Activator.getDefault().getDialogSettings(), "NodeTable"); //$NON-NLS-1$
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
		{
		   if (getObject() instanceof Subnet)
		   {
	         viewer.setInput(getObject().getChildsAsArray());		      
		   }
		   else
		   {
		      List<AbstractObject> list = new ArrayList<AbstractObject>();
		      for(AbstractObject o : getObject().getChildsAsArray())
		      {
		         if (o instanceof AbstractNode)
		            list.add(o);
		      }
	         viewer.setInput(list.toArray());
		   }
		}
		else
		{
			viewer.setInput(new AbstractNode[0]);
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void objectChanged(final AbstractObject object)
	{
		refresh();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#showForObject(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public boolean showForObject(AbstractObject object)
	{
		return (object instanceof Subnet) || (object instanceof Cluster) || (object instanceof Container) || (object instanceof ServiceRoot);
	}
}
