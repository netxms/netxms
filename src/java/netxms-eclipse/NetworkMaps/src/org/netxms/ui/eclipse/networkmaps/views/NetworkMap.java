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

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
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
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

public abstract class NetworkMap extends ViewPart implements ISelectionProvider
{
	protected NXCSession session;
	protected GenericObject rootObject;
	protected NetworkMapPage mapPage;
	protected GraphViewer viewer;
	protected SessionListener sessionListener;
	protected MapLabelProvider labelProvider;
	
	protected Action actionShowStatusIcon;
	protected Action actionShowStatusBackground;
	
	private IStructuredSelection currentSelection = new StructuredSelection(new Object[0]);
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();

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
		labelProvider = new MapLabelProvider(viewer);
		viewer.setLabelProvider(labelProvider);
		viewer.setLayoutAlgorithm(new SpringLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING));
		viewer.setInput(mapPage);
		
		getSite().setSelectionProvider(this);
		viewer.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent e)
			{
				currentSelection = transformSelection(e.getSelection());
				if (selectionListeners.isEmpty())
					return;
				
				SelectionChangedEvent event = new SelectionChangedEvent(NetworkMap.this, currentSelection);
				for(ISelectionChangedListener l : selectionListeners)
				{
					l.selectionChanged(event);
				}
			}
		});
		
		createActions();
		contributeToActionBars();
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
	 * Create actions
	 */
	protected void createActions()
	{
		actionShowStatusBackground = new Action("Show status &background", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowStatusBackground(!labelProvider.isShowStatusBackground());
				setChecked(labelProvider.isShowStatusBackground());
				viewer.refresh();
			}
		};
		actionShowStatusBackground.setChecked(labelProvider.isShowStatusBackground());
	
		actionShowStatusIcon = new Action("Show status &icon", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowStatusIcons(!labelProvider.isShowStatusIcons());
				setChecked(labelProvider.isShowStatusIcons());
				viewer.refresh();
			}
		};
		actionShowStatusIcon.setChecked(labelProvider.isShowStatusIcons());
	}
	
	/**
	 * Fill action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * @param manager
	 */
	protected void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
	}

	/**
	 * Fill local tool bar
	 * @param manager
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{	
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
				if (!currentSelection.isEmpty())
					fillContextMenu(mgr);
			}
		});
	
		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	
		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, this);
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
		mgr.add(new PropertyDialogAction(getSite(), this));
	}
	
	/**
	 * Returns true if given object is on current map
	 * 
	 * @param object
	 * @return
	 */
	protected boolean isObjectOnMap(long objectId)
	{
		return mapPage.findObjectElement(objectId) != null;
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

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}
	
	/**
	 * Transform viewer's selection to form usable by another plugins by extracting
	 * NetXMS objects from map elements.
	 *  
	 * @param viewerSelection viewer's selection
	 * @return selection containing only NetXMS objects
	 */
	@SuppressWarnings("rawtypes")
	private IStructuredSelection transformSelection(ISelection viewerSelection)
	{
		IStructuredSelection selection = (IStructuredSelection)viewerSelection;
		if (selection.isEmpty())
			return selection;
				
		List<GenericObject> objects = new ArrayList<GenericObject>();
		Iterator it = selection.iterator();
		while(it.hasNext())
		{
			Object element = it.next();
			if (element instanceof NetworkMapObject)
			{
				GenericObject object = session.findObjectById(((NetworkMapObject)element).getObjectId());
				if (object != null)
					objects.add(object);
			}
		}

		return new StructuredSelection(objects.toArray());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
	 */
	@Override
	public ISelection getSelection()
	{
		//return transformSelection((IStructuredSelection)viewer.getSelection());
		return currentSelection;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void setSelection(ISelection selection)
	{
	}
}
