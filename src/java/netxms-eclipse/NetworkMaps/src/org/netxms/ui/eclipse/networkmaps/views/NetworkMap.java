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
import java.util.Comparator;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IContributionItem;
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
import org.eclipse.ui.progress.UIJob;
import org.eclipse.zest.core.viewers.AbstractZoomableViewer;
import org.eclipse.zest.core.viewers.IZoomableWorkbenchPart;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.HorizontalTreeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.networkmaps.algorithms.SparseTree;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Base class for network map views
 *
 */
public abstract class NetworkMap extends ViewPart implements ISelectionProvider, IZoomableWorkbenchPart
{
	protected static final int LAYOUT_MANUAL = -1;
	protected static final int LAYOUT_SPRING = 0;
	protected static final int LAYOUT_RADIAL = 1;
	protected static final int LAYOUT_HTREE = 2;
	protected static final int LAYOUT_VTREE = 3;
	protected static final int LAYOUT_SPARSE_VTREE = 4;
	
	private static final String[] layoutAlgorithmNames = 
		{ "&Spring", "&Radial", "&Horizontal tree", "&Vertical tree", "S&parse vertical tree" };
	
	protected NXCSession session;
	protected GenericObject rootObject;
	protected NetworkMapPage mapPage;
	protected ExtendedGraphViewer viewer;
	protected SessionListener sessionListener;
	protected MapLabelProvider labelProvider;
	protected int layoutAlgorithm = LAYOUT_SPRING;
	
	private RefreshAction actionRefresh;
	private Action actionShowStatusIcon;
	private Action actionShowStatusBackground;
	private Action actionShowStatusFrame;
	private Action actionZoomIn;
	private Action actionZoomOut;
	private Action[] actionZoomTo;
	private Action[] actionSetAlgorithm;
	
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
		
		viewer = new ExtendedGraphViewer(parent, SWT.NONE);
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
	 * Do full map refresh
	 */
	private void refreshMap()
	{
		buildMapPage();
		viewer.setInput(mapPage);
	}
	
	/**
	 * Replace current map page with new one
	 * 
	 * @param page new map page
	 */
	protected void replaceMapPage(final NetworkMapPage page)
	{
		new UIJob("Replace map page") {
			@Override
			public IStatus runInUIThread(IProgressMonitor monitor)
			{
				mapPage = page;
				viewer.setInput(mapPage);
				return Status.OK_STATUS;
			}
		}.schedule();
	}
	
	/**
	 * Set layout algorithm for map
	 * @param alg
	 */
	@SuppressWarnings("rawtypes")
	protected void setLayoutAlgorithm(int alg)
	{
		switch(alg)
		{
			case LAYOUT_SPRING:
				viewer.setLayoutAlgorithm(new SpringLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING), true);
				break;
			case LAYOUT_RADIAL:
				viewer.setLayoutAlgorithm(new RadialLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING), true);
				break;
			case LAYOUT_HTREE:
				viewer.setLayoutAlgorithm(new HorizontalTreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING), true);
				break;
			case LAYOUT_VTREE:
				viewer.setLayoutAlgorithm(new TreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING), true);
				break;
			case LAYOUT_SPARSE_VTREE:
				TreeLayoutAlgorithm mainLayoutAlgorithm = new TreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				mainLayoutAlgorithm.setComparator(new Comparator() {
					@Override
					public int compare(Object arg0, Object arg1)
					{
						return arg0.toString().compareToIgnoreCase(arg1.toString());
					}
				});
				viewer.setLayoutAlgorithm(new CompositeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING, 
						new LayoutAlgorithm[] { mainLayoutAlgorithm,
						                        new SparseTree(LayoutStyles.NO_LAYOUT_NODE_RESIZING) }));
				break;
		}

		actionSetAlgorithm[layoutAlgorithm].setChecked(false);
		layoutAlgorithm = alg;
		actionSetAlgorithm[layoutAlgorithm].setChecked(true);
	}

	/**
	 * Create actions
	 */
	protected void createActions()
	{
		actionRefresh = new RefreshAction() {
			@Override
			public void run()
			{
				refreshMap();
			}
		};
		
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
		
		actionShowStatusFrame = new Action("Show status &frame", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowStatusFrame(!labelProvider.isShowStatusFrame());
				setChecked(labelProvider.isShowStatusFrame());
				viewer.refresh();
			}
		};
		actionShowStatusFrame.setChecked(labelProvider.isShowStatusFrame());
	
		actionZoomIn = new Action("Zoom &in") {
			@Override
			public void run()
			{
				viewer.zoomIn();
			}
		};
		actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);

		actionZoomOut = new Action("Zoom &out") {
			@Override
			public void run()
			{
				viewer.zoomOut();
			}
		};
		actionZoomOut.setImageDescriptor(SharedIcons.ZOOM_OUT);
		
		actionZoomTo = viewer.createZoomActions();

		actionSetAlgorithm = new Action[layoutAlgorithmNames.length];
		for(int i = 0; i < layoutAlgorithmNames.length; i++)
		{
			final int alg = i;
			actionSetAlgorithm[i] = new Action(layoutAlgorithmNames[i], Action.AS_RADIO_BUTTON) {
				@Override
				public void run()
				{
					setLayoutAlgorithm(alg);
				}
			};
			actionSetAlgorithm[i].setChecked(layoutAlgorithm == i);
		}
	}
	
	/**
	 * Create "Layout" submenu
	 * @return
	 */
	protected IContributionItem createLayoutSubmenu()
	{
		MenuManager layout = new MenuManager("&Layout");
		for(int i = 0; i < actionSetAlgorithm.length; i++)
			layout.add(actionSetAlgorithm[i]);
		return layout;
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
		MenuManager zoom = new MenuManager("&Zoom");
		for(int i = 0; i < actionZoomTo.length; i++)
			zoom.add(actionZoomTo[i]);
		
		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(zoom);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * @param manager
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{	
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
		manager.add(new Separator());
		manager.add(actionRefresh);
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
				if (currentSelection.isEmpty())
					fillMapContextMenu(mgr);
				else
					fillObjectContextMenu(mgr);
			}
		});
	
		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	
		// Register menu for extension.
		getSite().registerContextMenu(menuMgr, this);
	}

	/**
	 * Fill context menu for map object
	 * @param mgr Menu manager
	 */
	protected void fillObjectContextMenu(IMenuManager mgr)
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
	 * Fill context menu for map view
	 * @param mgr Menu manager
	 */
	protected void fillMapContextMenu(IMenuManager manager)
	{
		MenuManager zoom = new MenuManager("&Zoom");
		for(int i = 0; i < actionZoomTo.length; i++)
			zoom.add(actionZoomTo[i]);
		
		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(zoom);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Called by session listener when NetXMS object was changed.
	 * 
	 * @param object changed NetXMS object
	 */
	protected void onObjectChange(final GenericObject object)
	{
		new UIJob("Refresh map") {
			@Override
			public IStatus runInUIThread(IProgressMonitor monitor)
			{
				NetworkMapObject element = mapPage.findObjectElement(object.getObjectId());
				if (element != null)
					viewer.refresh(element, true);
				if (object.getObjectId() == rootObject.getObjectId())
					rootObject = object;
				return Status.OK_STATUS;
			}
		}.schedule();
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

	/* (non-Javadoc)
	 * @see org.eclipse.zest.core.viewers.IZoomableWorkbenchPart#getZoomableViewer()
	 */
	@Override
	public AbstractZoomableViewer getZoomableViewer()
	{
		return viewer;
	}
}
