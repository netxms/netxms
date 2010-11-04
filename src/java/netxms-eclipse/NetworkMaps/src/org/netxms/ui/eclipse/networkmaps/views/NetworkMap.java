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
package org.netxms.ui.eclipse.networkmaps.views;

import java.util.Set;

import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.zest.core.viewers.GraphViewer;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapObjectData;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public abstract class NetworkMap extends ViewPart
{
	protected NXCSession session;
	protected GenericObject rootObject;
	protected NetworkMapPage mapPage;
	protected GraphViewer viewer;
	protected SessionListener sessionListener;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

		session = (NXCSession)ConsoleSharedData.getSession();
		rootObject = session.findObjectById(Long.parseLong(site.getSecondaryId()));
	
		buildMapPage();
	}
	
	/**
	 * Build map page containing data to display. Should be implemented
	 * in derived classes.
	 */
	protected abstract void buildMapPage();

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		FillLayout layout = new FillLayout();
		parent.setLayout(layout);
		
		viewer = new GraphViewer(parent, SWT.NONE);
		viewer.setContentProvider(new MapContentProvider());
		viewer.setLabelProvider(new MapLabelProvider(viewer));
		viewer.setLayoutAlgorithm(new SpringLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING));
		viewer.setInput(mapPage);
		
		createPopupMenu();
		
		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
					onObjectChange((GenericObject)n.getObject());
			}
		};
		session.addListener(sessionListener);
	}

	/**
	 * Create popup menu for map
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
	 * @param mgr Menu manager
	 */
	protected void fillContextMenu(IMenuManager mgr)
	{
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_CREATION));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_MANAGEMENT));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_OBJECT_BINDING));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_DATA_COLLECTION));
		mgr.add(new Separator());
		mgr.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
		mgr.add(new PropertyDialogAction(getSite(), viewer));
	}
	
	/**
	 * Returns true if given object is on current map
	 * 
	 * @param object
	 * @return
	 */
	protected boolean isObjectOnMap(long objectId)
	{
		Set<NetworkMapObjectData> objects = mapPage.getObjects();
		for(NetworkMapObjectData d : objects)
		{
			if (d.getObjectId() == objectId)
				return true;
		}
		return false;
	}
	
	/**
	 * Called by session listener when NetXMS object was changed.
	 * 
	 * @param object cnaged NetXMS object
	 */
	protected void onObjectChange(GenericObject object)
	{
		if (isObjectOnMap(object.getObjectId()))
			viewer.refresh(object, true);
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
		if (sessionListener != null)
			session.removeListener(sessionListener);
		super.dispose();
	}
}
