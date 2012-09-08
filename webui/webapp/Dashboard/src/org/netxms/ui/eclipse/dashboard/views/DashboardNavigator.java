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
package org.netxms.ui.eclipse.dashboard.views;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectTree;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.IActionConstants;

/**
 * Dashboard navigator
 */
public class DashboardNavigator extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.dashboard.views.DashboardNavigator"; //$NON-NLS-1$
	
	private NXCSession session = null;
	private SessionListener sessionListener = null;
	private ObjectTree objectTree;
	private Action actionRefresh;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		session = (NXCSession)ConsoleSharedData.getSession();
		
		final Set<Integer> classFilter = new HashSet<Integer>(2);
		classFilter.add(GenericObject.OBJECT_DASHBOARD);
		objectTree = new ObjectTree(parent, SWT.NONE, ObjectTree.NONE, getRootObjects(classFilter), classFilter);
		objectTree.enableFilter(false);
		objectTree.getTreeViewer().expandToLevel(2);
		
		createActions();
		contributeToActionBars();
		createPopupMenu();

		getSite().setSelectionProvider(objectTree.getTreeViewer());
		
		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if ((n.getCode() == NXCNotification.OBJECT_CHANGED) && (n.getObject() instanceof DashboardRoot))
				{
					objectTree.getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							refresh();
						}
					});
				}
			}
		};
		session.addListener(sessionListener);
	}
	
	/**
	 * @param classFilter
	 * @return
	 */
	private long[] getRootObjects(Set<Integer> classFilter)
	{
		GenericObject[] objects = session.getTopLevelObjects(classFilter);
		long[] ids = new long[objects.length];
		for(int i = 0; i < objects.length; i++)
			ids[i] = objects[i].getObjectId();
		return ids;
	}
	
	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				refresh();
			}
		};
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
		manager.add(actionRefresh);
	}

	/**
	 * Create popup menu for object browser
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager manager = new MenuManager();
		manager.setRemoveAllWhenShown(true);
		manager.addMenuListener(new IMenuListener() {
      	private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		// Create menu.
		Menu menu = manager.createContextMenu(objectTree.getTreeControl());
		objectTree.getTreeControl().setMenu(menu);

		// Register menu for extension.
		getSite().registerContextMenu(manager, objectTree.getTreeViewer());
	}

	/**
	 * Fill context menu
	 * @param manager Menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
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
		manager.add(new Separator());
		manager.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		manager.add(new PropertyDialogAction(getSite(), objectTree.getTreeViewer()));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		objectTree.setFocus();
	}
	
	/**
	 * Refresh dashboard tree
	 */
	private void refresh()
	{
		final Set<Integer> classFilter = new HashSet<Integer>(2);
		classFilter.add(GenericObject.OBJECT_DASHBOARD);
		objectTree.setRootObjects(getRootObjects(classFilter));
		objectTree.getTreeViewer().expandToLevel(2);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		if ((session != null) && (sessionListener != null))
			session.removeListener(sessionListener);
		super.dispose();
	}
}
