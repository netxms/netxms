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
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
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
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.eclipse.ui.progress.UIJob;
import org.eclipse.zest.core.viewers.AbstractZoomableViewer;
import org.eclipse.zest.core.viewers.IZoomableWorkbenchPart;
import org.eclipse.zest.core.widgets.Graph;
import org.eclipse.zest.core.widgets.GraphNode;
import org.eclipse.zest.core.widgets.ZestStyles;
import org.eclipse.zest.layouts.LayoutAlgorithm;
import org.eclipse.zest.layouts.LayoutStyles;
import org.eclipse.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.GridLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.HorizontalTreeLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.algorithms.ManualLayout;
import org.netxms.ui.eclipse.networkmaps.algorithms.SparseTree;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.GraphLayoutFilter;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ObjectFigureType;
import org.netxms.ui.eclipse.shared.IActionConstants;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Base class for network map views
 *
 */
public abstract class NetworkMap extends ViewPart implements ISelectionProvider, IZoomableWorkbenchPart
{
	protected static final int LAYOUT_SPRING = 0;
	protected static final int LAYOUT_RADIAL = 1;
	protected static final int LAYOUT_HTREE = 2;
	protected static final int LAYOUT_VTREE = 3;
	protected static final int LAYOUT_SPARSE_VTREE = 4;
	
	private static final String[] layoutAlgorithmNames = 
		{ "&Spring", "&Radial", "&Horizontal tree", "&Vertical tree", "S&parse vertical tree" };
	
	private static final int SELECTION_EMPTY = 0;
	private static final int SELECTION_MIXED = 1;
	private static final int SELECTION_OBJECTS = 2;
	private static final int SELECTION_ELEMENTS = 3;
	private static final int SELECTION_LINKS = 4;
	
	protected NXCSession session;
	protected GenericObject rootObject;
	protected NetworkMapPage mapPage;
	protected ExtendedGraphViewer viewer;
	protected SessionListener sessionListener;
	protected MapLabelProvider labelProvider;
	protected int layoutAlgorithm = LAYOUT_SPRING;
	protected boolean allowManualLayout = false;     // True if manual layout can be switched on
	protected boolean automaticLayoutEnabled = true; // Current layout mode - automatic or manual
	
	private RefreshAction actionRefresh;
	private Action actionShowStatusIcon;
	private Action actionShowStatusBackground;
	private Action actionShowStatusFrame;
	private Action actionZoomIn;
	private Action actionZoomOut;
	private Action[] actionZoomTo;
	private Action[] actionSetAlgorithm;
	private Action actionEnableAutomaticLayout;
	private Action actionSaveLayout;
	private Action actionOpenSubmap;
	private Action actionFiguresIcons;
	private Action actionFiguresSmallLabels;
	private Action actionFiguresLargeLabels;
	
	private String viewId;
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
		String[] parts = site.getSecondaryId().split("&");
		rootObject = session.findObjectById(Long.parseLong((parts.length > 0) ? parts[0] : site.getSecondaryId()));
		if (rootObject == null)
			throw new PartInitException("Root object for this map is no longer exist or is not accessible");
		
		viewId = site.getId() + ".";
		viewId += (parts.length > 0) ? parts[0] : site.getSecondaryId();
	
		parseSecondaryId(parts);
		buildMapPage();
	}
	
	/**
	 * Parse secondary ID (already splitted by & characters). Intended to be
	 * overrided in subclasses.
	 * 
	 * @param parts secondary ID divided at & characters
	 * @throws PartInitException
	 */
	protected void parseSecondaryId(String[] parts) throws PartInitException
	{
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

		try
		{
			IDialogSettings settings = Activator.getDefault().getDialogSettings();
			labelProvider.setObjectFigureType(ObjectFigureType.values()[settings.getInt(viewId + ".objectFigureType")]);
		}
		catch(Exception e)
		{
			e.printStackTrace();
		}
		
		getSite().setSelectionProvider(this);
		
		/* FIXME: remove two listeners after upgrade to Zest 2.0. For Zest 1.3
		 * it is needed because of crazy internal implementation of selection listeners
		 * in GraphViewer.
		 */
		ISelectionChangedListener listener = new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent e)
			{
				currentSelection = transformSelection(e.getSelection());

				if ((currentSelection.size() == 1) && (analyzeSelection(currentSelection) == SELECTION_OBJECTS))
				{
					GenericObject object = (GenericObject)currentSelection.getFirstElement();
					actionOpenSubmap.setEnabled(object.getSubmapId() != 0);
				}
				else
				{
					actionOpenSubmap.setEnabled(false);
				}
				
				if (selectionListeners.isEmpty())
					return;
				
				SelectionChangedEvent event = new SelectionChangedEvent(NetworkMap.this, currentSelection);
				for(ISelectionChangedListener l : selectionListeners)
				{
					l.selectionChanged(event);
				}
			}
		};
		viewer.addSelectionChangedListener(listener);
		viewer.addPostSelectionChangedListener(listener);
		
		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				actionOpenSubmap.run();
			}
		});

		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
					onObjectChange((GenericObject)n.getObject());
			}
		};
		session.addListener(sessionListener);

		createActions();
		contributeToActionBars();
		createPopupMenu();
		
		if (automaticLayoutEnabled)
		{
			setLayoutAlgorithm(layoutAlgorithm);
		}
		else
		{
			viewer.setLayoutAlgorithm(new ManualLayout(LayoutStyles.NO_LAYOUT_NODE_RESIZING));
		}
		//viewer.setNodeStyle(ZestStyles.NODES_NO_ANIMATION);
		viewer.setInput(mapPage);
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
		new UIJob(viewer.getControl().getDisplay(), "Replace map page") {
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
		LayoutAlgorithm algorithm;
		
		switch(alg)
		{
			case LAYOUT_SPRING:
				algorithm = new SpringLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_RADIAL:
				algorithm = new RadialLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_HTREE:
				algorithm = new HorizontalTreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
			case LAYOUT_VTREE:
				algorithm = new TreeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
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
				algorithm = new CompositeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING, 
						new LayoutAlgorithm[] { mainLayoutAlgorithm,
						                        new SparseTree(LayoutStyles.NO_LAYOUT_NODE_RESIZING) });
				break;
			default:
				algorithm = new GridLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
				break;
		}
		algorithm.setFilter(new GraphLayoutFilter(true));
		ManualLayout decorationLayoutAlgorithm = new ManualLayout(LayoutStyles.NO_LAYOUT_NODE_RESIZING);
		decorationLayoutAlgorithm.setFilter(new GraphLayoutFilter(false));
		viewer.setLayoutAlgorithm(new CompositeLayoutAlgorithm(LayoutStyles.NO_LAYOUT_NODE_RESIZING, 
				new LayoutAlgorithm[] { algorithm, decorationLayoutAlgorithm }), true);

		actionSetAlgorithm[layoutAlgorithm].setChecked(false);
		layoutAlgorithm = alg;
		actionSetAlgorithm[layoutAlgorithm].setChecked(true);
	}
	
	/**
	 * Update stored object positions with actual positions read from graph control
	 */
	protected void updateObjectPositions()
	{
		Graph graph = viewer.getGraphControl();
		List<?> nodes = graph.getNodes();
		for(Object o : nodes)
		{
			if (o instanceof GraphNode)
			{
				Object data = ((GraphNode)o).getData();
				if (data instanceof NetworkMapElement)
				{
					Point loc = ((GraphNode)o).getLocation();
					((NetworkMapElement)data).setLocation(loc.x, loc.y);
				}
			}
		}
	}

	/**
	 * Set manual layout mode
	 */
	protected void setManualLayout()
	{
		updateObjectPositions();
		
		automaticLayoutEnabled = false;
		viewer.setLayoutAlgorithm(new ManualLayout(LayoutStyles.NO_LAYOUT_NODE_RESIZING), true);
		
		for(int i = 0; i < actionSetAlgorithm.length; i++)
			actionSetAlgorithm[i].setEnabled(false);
		actionSaveLayout.setEnabled(true);
	}
	
	/**
	 * Set automatic layout mode
	 */
	protected void setAutomaticLayout()
	{
		automaticLayoutEnabled = true;
		setLayoutAlgorithm(layoutAlgorithm);
		
		for(int i = 0; i < actionSetAlgorithm.length; i++)
			actionSetAlgorithm[i].setEnabled(true);
		actionSaveLayout.setEnabled(false);
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
		actionShowStatusBackground.setEnabled(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);
	
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
		actionShowStatusIcon.setEnabled(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);
		
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
		actionShowStatusFrame.setEnabled(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);
	
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
			actionSetAlgorithm[i].setEnabled(automaticLayoutEnabled);
		}
		
		actionEnableAutomaticLayout = new Action("Enable &automatic layout", Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				if (automaticLayoutEnabled)
				{
					setManualLayout();
				}
				else
				{
					setAutomaticLayout();
				}
				setChecked(automaticLayoutEnabled);
			}
		};
		actionEnableAutomaticLayout.setChecked(automaticLayoutEnabled);
		
		actionSaveLayout = new Action("&Save layout") {
			@Override
			public void run()
			{
				updateObjectPositions();
				saveLayout();
			}
		};
		actionSaveLayout.setImageDescriptor(SharedIcons.SAVE);
		actionSaveLayout.setEnabled(!automaticLayoutEnabled);
		
		actionOpenSubmap = new Action("Open s&ubmap") {
			@Override
			public void run()
			{
				openSubmap();
			}
		};
		actionOpenSubmap.setEnabled(false);
		
		actionFiguresIcons = new Action("&Icons", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				labelProvider.setObjectFigureType(ObjectFigureType.ICON);
				viewer.refresh(true);
				actionShowStatusBackground.setEnabled(true);
				actionShowStatusFrame.setEnabled(true);
				actionShowStatusIcon.setEnabled(true);
			}
		};
		actionFiguresIcons.setChecked(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);
		
		actionFiguresSmallLabels = new Action("&Small labels", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				labelProvider.setObjectFigureType(ObjectFigureType.SMALL_LABEL);
				viewer.refresh(true);
				actionShowStatusBackground.setEnabled(false);
				actionShowStatusFrame.setEnabled(false);
				actionShowStatusIcon.setEnabled(false);
			}
		};
		actionFiguresSmallLabels.setChecked(labelProvider.getObjectFigureType() == ObjectFigureType.SMALL_LABEL);
		
		actionFiguresLargeLabels = new Action("&Large labels", Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				labelProvider.setObjectFigureType(ObjectFigureType.LARGE_LABEL);
				viewer.refresh(true);
				actionShowStatusBackground.setEnabled(false);
				actionShowStatusFrame.setEnabled(false);
				actionShowStatusIcon.setEnabled(false);
			}
		};
		actionFiguresLargeLabels.setChecked(labelProvider.getObjectFigureType() == ObjectFigureType.LARGE_LABEL);
	}
	
	/**
	 * Create "Layout" submenu
	 * @return
	 */
	protected IContributionItem createLayoutSubmenu()
	{
		MenuManager layout = new MenuManager("&Layout");
		if (allowManualLayout)
		{
			layout.add(actionEnableAutomaticLayout);
			layout.add(new Separator());
		}
		for(int i = 0; i < actionSetAlgorithm.length; i++)
			layout.add(actionSetAlgorithm[i]);
		if (allowManualLayout)
		{
			layout.add(new Separator());
			layout.add(actionSaveLayout);
		}
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
		
		MenuManager figureType = new MenuManager("&Display objects as");
		figureType.add(actionFiguresIcons);
		figureType.add(actionFiguresSmallLabels);
		figureType.add(actionFiguresLargeLabels);
		
		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(zoom);
		manager.add(figureType);
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
		if (allowManualLayout)
		{
			manager.add(actionSaveLayout);
		}
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
			public void menuAboutToShow(IMenuManager manager)
			{
				int selType = analyzeSelection(currentSelection);
				switch(selType)
				{
					case SELECTION_EMPTY:
						fillMapContextMenu(manager);
						break;
					case SELECTION_OBJECTS:
						fillObjectContextMenu(manager);
						break;
					case SELECTION_ELEMENTS:
						fillElementContextMenu(manager);
						break;
					case SELECTION_LINKS:
						fillLinkContextMenu(manager);
						break;
				}
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
	 * @param manager Menu manager
	 */
	protected void fillObjectContextMenu(IMenuManager manager)
	{
		manager.add(actionOpenSubmap);
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
		
		if (currentSelection.size() == 1)
		{
			manager.add(new Separator());
			manager.add(new GroupMarker(IActionConstants.MB_PROPERTIES));
			manager.add(new PropertyDialogAction(getSite(), this));
		}
	}
	
	/**
	 * Fill context menu for map element
	 * @param manager Menu manager
	 */
	protected void fillElementContextMenu(IMenuManager manager)
	{
	}
	
	/**
	 * Fill context menu for link between objects
	 * @param manager Menu manager
	 */
	protected void fillLinkContextMenu(IMenuManager manager)
	{
	}
	
	/**
	 * Fill context menu for map view
	 * @param manager Menu manager
	 */
	protected void fillMapContextMenu(IMenuManager manager)
	{
		MenuManager zoom = new MenuManager("&Zoom");
		for(int i = 0; i < actionZoomTo.length; i++)
			zoom.add(actionZoomTo[i]);
		
		MenuManager figureType = new MenuManager("&Display objects as");
		figureType.add(actionFiguresIcons);
		figureType.add(actionFiguresSmallLabels);
		figureType.add(actionFiguresLargeLabels);
		
		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(zoom);
		manager.add(figureType);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
		manager.add(new Separator());
		manager.add(actionRefresh);
	}
	
	/**
	 * Tests if given selection contains only NetXMS objects
	 * 
	 * @param selection
	 * @return
	 */
	@SuppressWarnings("rawtypes")
	private int analyzeSelection(IStructuredSelection selection)
	{
		if (selection.isEmpty())
			return SELECTION_EMPTY;
		
		Iterator it = selection.iterator();
		Object first = it.next();
		int type;
		Class firstClass;
		if (first instanceof GenericObject)
		{
			type = SELECTION_OBJECTS;
			firstClass = GenericObject.class;
		}
		else if (first instanceof NetworkMapElement)
		{
			type = SELECTION_ELEMENTS;
			firstClass = NetworkMapElement.class;
		}
		else if (first instanceof NetworkMapLink)
		{
			type = SELECTION_LINKS;
			firstClass = NetworkMapLink.class;
		}
		else
		{
			return SELECTION_MIXED;
		}
		
		while(it.hasNext())
		{
			final Object o = it.next();
			if (!firstClass.isInstance(o))
				return SELECTION_MIXED;
		}
		return type;
	}
	
	/**
	 * Called by session listener when NetXMS object was changed.
	 * 
	 * @param object changed NetXMS object
	 */
	protected void onObjectChange(final GenericObject object)
	{
		new UIJob(viewer.getControl().getDisplay(), "Refresh map") {
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
	
	/**
	 * Called when map layout has to be saved. Object positions already updated
	 * when this method is called. Default implementation does nothing.
	 */
	protected void saveLayout()
	{
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
		if (labelProvider != null)
		{
			IDialogSettings settings = Activator.getDefault().getDialogSettings();
			settings.put(viewId + ".objectFigureType", labelProvider.getObjectFigureType().ordinal());
		}
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
				
		List<Object> objects = new ArrayList<Object>();
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
			else if (isSelectableElement(element))
			{
				objects.add(element);
			}
		}

		return new StructuredSelection(objects.toArray());
	}
	
	/**
	 * Tests if given map element is selectable. Default implementation always returns false.
	 * 
	 * @param element element to test
	 * @return true if given element is selectable
	 */
	protected boolean isSelectableElement(Object element)
	{
		return false;
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
	
	/**
	 * Enable or disable layout animation
	 * 
	 * @param enabled
	 */
	protected void setAnimationEnabled(boolean enabled)
	{
		int nodeStyle = viewer.getGraphControl().getNodeStyle();
		if (enabled)
			nodeStyle &= ~ZestStyles.NODES_NO_LAYOUT_ANIMATION;
		else
			nodeStyle |= ZestStyles.NODES_NO_LAYOUT_ANIMATION;
		viewer.getGraphControl().setNodeStyle(nodeStyle);
	}
	
	/**
	 * Open submap for currently selected object
	 */
	private void openSubmap()
	{
		if (currentSelection == null)
			return;
		
		Object object = currentSelection.getFirstElement();
		if (object instanceof GenericObject)
		{
			long submapId = ((GenericObject)object).getSubmapId();
			if (submapId != 0)
			{
				try
				{
					getSite().getPage().showView(PredefinedMap.ID, Long.toString(submapId), IWorkbenchPage.VIEW_ACTIVATE);
				}
				catch(PartInitException e)
				{
					MessageDialog.openError(getSite().getShell(), "Error", "Cannot open submap view: " + e.getMessage());
				}
			}
		}
	}
}
