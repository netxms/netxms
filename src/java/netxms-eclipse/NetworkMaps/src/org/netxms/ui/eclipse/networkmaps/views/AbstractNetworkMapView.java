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
package org.netxms.ui.eclipse.networkmaps.views;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.draw2d.ManhattanConnectionRouter;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.gef4.zest.core.viewers.AbstractZoomableViewer;
import org.eclipse.gef4.zest.core.viewers.IZoomableWorkbenchPart;
import org.eclipse.gef4.zest.core.widgets.Graph;
import org.eclipse.gef4.zest.core.widgets.GraphConnection;
import org.eclipse.gef4.zest.core.widgets.GraphNode;
import org.eclipse.gef4.zest.layouts.LayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.CompositeLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.GridLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.GroupMarker;
import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.Clipboard;
import org.eclipse.swt.dnd.ImageTransfer;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.dialogs.PropertyDialogAction;
import org.eclipse.ui.part.ViewPart;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.configs.SingleDciConfig;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.networkmaps.algorithms.ExpansionAlgorithm;
import org.netxms.ui.eclipse.networkmaps.algorithms.ManualLayout;
import org.netxms.ui.eclipse.networkmaps.api.ObjectDoubleClickHandler;
import org.netxms.ui.eclipse.networkmaps.views.helpers.BendpointEditor;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.LinkDciValueProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ObjectFigureType;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.FilteringMenuManager;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Base class for network map views
 */
public abstract class AbstractNetworkMapView extends ViewPart implements ISelectionProvider, IZoomableWorkbenchPart
{
	protected static final int LAYOUT_SPRING = 0;
	protected static final int LAYOUT_RADIAL = 1;
	protected static final int LAYOUT_HTREE = 2;
	protected static final int LAYOUT_VTREE = 3;
	protected static final int LAYOUT_SPARSE_VTREE = 4;

	private static final String[] layoutAlgorithmNames = { 
	   Messages.get().AbstractNetworkMapView_LayoutSpring,
	   Messages.get().AbstractNetworkMapView_LayoutRadial,
	   Messages.get().AbstractNetworkMapView_LayoutHorzTree, 
	   Messages.get().AbstractNetworkMapView_LayoutVertTree,
	   Messages.get().AbstractNetworkMapView_LayoutSparseVertTree 
	};
	private static final String[] connectionRouterNames = { Messages.get().AbstractNetworkMapView_RouterDirect, Messages.get().AbstractNetworkMapView_RouterManhattan };

	private static final int SELECTION_EMPTY = 0;
	private static final int SELECTION_MIXED = 1;
	private static final int SELECTION_OBJECTS = 2;
	private static final int SELECTION_ELEMENTS = 3;
	private static final int SELECTION_LINKS = 4;

	protected NXCSession session;
	protected AbstractObject rootObject;
	protected NetworkMapPage mapPage;
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;
	protected MapLayoutAlgorithm layoutAlgorithm = MapLayoutAlgorithm.SPRING;
	protected int routingAlgorithm = NetworkMapLink.ROUTING_DIRECT;
	protected boolean allowManualLayout = false; // True if manual layout can be switched on
	protected boolean automaticLayoutEnabled = true; // Current layout mode - automatic or manual
   protected boolean disableGeolocationBackground = false;

	protected Action actionRefresh;
	protected Action actionShowStatusIcon;
	protected Action actionShowStatusBackground;
	protected Action actionShowStatusFrame;
	protected Action actionZoomIn;
	protected Action actionZoomOut;
	protected Action[] actionZoomTo;
	protected Action[] actionSetAlgorithm;
	protected Action[] actionSetRouter;
	protected Action actionEnableAutomaticLayout;
	protected Action actionSaveLayout;
	protected Action actionOpenSubmap;
	protected Action actionFiguresIcons;
	protected Action actionFiguresSmallLabels;
	protected Action actionFiguresLargeLabels;
	protected Action actionShowGrid;
	protected Action actionAlignToGrid;
	protected Action actionSnapToGrid;
	protected Action actionShowObjectDetails;
	protected Action actionCopyImage;
   protected Action actionHideLinkLabels;

	private String viewId;
	private IStructuredSelection currentSelection = new StructuredSelection(new Object[0]);
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
	private BendpointEditor bendpointEditor = null;
	private SessionListener sessionListener;
	private List<DoubleClickHandlerData> doubleClickHandlers = new ArrayList<DoubleClickHandlerData>(0);
   private LinkDciValueProvider dciValueProvider;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);

      dciValueProvider = LinkDciValueProvider.getInstance();

		final IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
      disableGeolocationBackground = ps.getBoolean("DISABLE_GEOLOCATION_BACKGROUND");

		session = (NXCSession)ConsoleSharedData.getSession();
		String[] parts = site.getSecondaryId().split("&"); //$NON-NLS-1$
		rootObject = session.findObjectById(Long.parseLong((parts.length > 0) ? parts[0] : site.getSecondaryId()));
		if (rootObject == null)
			throw new PartInitException(Messages.get().AbstractNetworkMapView_RootObjectNotFound);

		viewId = site.getId() + "."; //$NON-NLS-1$
		viewId += (parts.length > 0) ? parts[0] : site.getSecondaryId();

		parseSecondaryId(parts);
	}

	/**
	 * Parse secondary ID (already splitted by & characters). Intended to be overrided in subclasses.
	 * 
	 * @param parts secondary ID divided at & characters
	 * @throws PartInitException
	 */
	protected void parseSecondaryId(String[] parts) throws PartInitException
	{
	}

	/**
	 * Build map page containing data to display. Should be implemented in derived classes.
	 */
	protected abstract void buildMapPage();

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.ViewPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		FillLayout layout = new FillLayout();
		parent.setLayout(layout);

		viewer = new ExtendedGraphViewer(parent, SWT.NONE);
		viewer.setContentProvider(new MapContentProvider(viewer));
		labelProvider = new MapLabelProvider(viewer);
		viewer.setLabelProvider(labelProvider);
      viewer.setBackgroundColor(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND).getRGB());

		try
		{
			IDialogSettings settings = Activator.getDefault().getDialogSettings();
			labelProvider.setObjectFigureType(ObjectFigureType.values()[settings.getInt(viewId + ".objectFigureType")]); //$NON-NLS-1$
		}
		catch(Exception e)
		{
		}

		getSite().setSelectionProvider(this);

		ISelectionChangedListener listener = new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent e)
			{
				if (bendpointEditor != null)
				{
					bendpointEditor.stop();
					bendpointEditor = null;
				}

				currentSelection = transformSelection(e.getSelection());

				if (currentSelection.size() == 1)
				{
					int selectionType = analyzeSelection(currentSelection);
					if (selectionType == SELECTION_OBJECTS)
					{
						AbstractObject object = (AbstractObject)currentSelection.getFirstElement();
						actionOpenSubmap.setEnabled(object.getSubmapId() != 0);
					}
					else
					{
						actionOpenSubmap.setEnabled(false);
						if (selectionType == SELECTION_LINKS)
						{
							NetworkMapLink link = (NetworkMapLink)currentSelection.getFirstElement();
							if (link.getRouting() == NetworkMapLink.ROUTING_BENDPOINTS)
							{
								bendpointEditor = new BendpointEditor(link,
										(GraphConnection)viewer.getGraphControl().getSelection().get(0), viewer);
							}
						}
					}
				}
				else
				{
					actionOpenSubmap.setEnabled(false);
				}

				if (selectionListeners.isEmpty())
					return;

				SelectionChangedEvent event = new SelectionChangedEvent(AbstractNetworkMapView.this, currentSelection);
				for(ISelectionChangedListener l : selectionListeners)
				{
					l.selectionChanged(event);
				}
			}
		};
		viewer.addPostSelectionChangedListener(listener);

		viewer.addDoubleClickListener(new IDoubleClickListener() {
			@Override
			public void doubleClick(DoubleClickEvent event)
			{
				int selectionType = analyzeSelection(currentSelection);
				if (selectionType == SELECTION_OBJECTS)
				{
					AbstractObject object = (AbstractObject)currentSelection.getFirstElement();

					for(DoubleClickHandlerData h : doubleClickHandlers)
					{
						if ((h.enabledFor == null) || (h.enabledFor.isInstance(object)))
						{
							if (h.handler.onDoubleClick(object))
							{
								return;
							}
						}
					}
				}

				// Default behaviour
				actionOpenSubmap.run();
			}
		});

		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(final SessionNotification n)
			{
				if (n.getCode() == NXCNotification.OBJECT_CHANGED)
				{
					viewer.getControl().getDisplay().asyncExec(new Runnable() {
						@Override
						public void run()
						{
							onObjectChange((AbstractObject)n.getObject());
						}
					});
				}
			}
		};
		session.addListener(sessionListener);

		createActions();
		contributeToActionBars();
		createPopupMenu();

		if (automaticLayoutEnabled)
		{
			setLayoutAlgorithm(layoutAlgorithm, true);
		}
		else
		{
			viewer.setLayoutAlgorithm(new ManualLayout());
		}

		registerDoubleClickHandlers();
		refreshMap();
	}

	/**
	 * Register double click handlers
	 */
	private void registerDoubleClickHandlers()
	{
		// Read all registered extensions and create handlers
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg
				.getConfigurationElementsFor("org.netxms.ui.eclipse.networkmaps.objectDoubleClickHandlers"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final DoubleClickHandlerData h = new DoubleClickHandlerData();
				h.handler = (ObjectDoubleClickHandler)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				h.priority = safeParseInt(elements[i].getAttribute("priority")); //$NON-NLS-1$
				final String className = elements[i].getAttribute("enabledFor"); //$NON-NLS-1$
				try
				{
					h.enabledFor = (className != null) ? Class.forName(className) : null;
				}
				catch(Exception e)
				{
					h.enabledFor = null;
				}
				doubleClickHandlers.add(h);
			}
			catch(CoreException e)
			{
				e.printStackTrace();
			}
		}

		// Sort handlers by priority
		Collections.sort(doubleClickHandlers, new Comparator<DoubleClickHandlerData>() {
			@Override
			public int compare(DoubleClickHandlerData arg0, DoubleClickHandlerData arg1)
			{
				return arg0.priority - arg1.priority;
			}
		});
	}

	/**
	 * Do full map refresh
	 */
	protected void refreshMap()
	{
		buildMapPage();
		viewer.setInput(mapPage);
		viewer.setSelection(StructuredSelection.EMPTY);
	}

	/**
	 * Replace current map page with new one
	 * 
	 * @param page new map page
	 */
	protected void replaceMapPage(final NetworkMapPage page, Display display)
	{
		display.asyncExec(new Runnable() {
			@Override
			public void run()
			{
				mapPage = page;
				addDciToRequestList();
				viewer.setInput(mapPage);
			}
		});
	}

	/**
	 * Set layout algorithm for map
	 * 
	 * @param alg Layout algorithm
	 * @param forceChange
	 */
	protected void setLayoutAlgorithm(MapLayoutAlgorithm alg, boolean forceChange)
	{
		if (alg == MapLayoutAlgorithm.MANUAL)
		{
			if (!automaticLayoutEnabled)
				return; // manual layout already

			automaticLayoutEnabled = false;
			// TODO: rewrite, enum value should not be used as index
			actionSetAlgorithm[layoutAlgorithm.getValue()].setChecked(false);
			actionEnableAutomaticLayout.setChecked(false);
			return;
		}

		if (automaticLayoutEnabled && (alg == layoutAlgorithm) && !forceChange)
			return; // nothing to change

		if (!automaticLayoutEnabled)
		{
			actionEnableAutomaticLayout.setChecked(true);
			automaticLayoutEnabled = true;
		}

		LayoutAlgorithm algorithm;

		switch(alg)
		{
			case SPRING:
				algorithm = new SpringLayoutAlgorithm();
				break;
			case RADIAL:
				algorithm = new RadialLayoutAlgorithm();
				break;
			case HTREE:
				algorithm = new TreeLayoutAlgorithm(TreeLayoutAlgorithm.LEFT_RIGHT);
				break;
			case VTREE:
				algorithm = new TreeLayoutAlgorithm(TreeLayoutAlgorithm.TOP_DOWN);
				break;
			case SPARSE_VTREE:
				algorithm = new TreeLayoutAlgorithm(TreeLayoutAlgorithm.TOP_DOWN);
				((TreeLayoutAlgorithm)algorithm).setNodeSpace(new Dimension(100, 100));
				break;
			default:
				algorithm = new GridLayoutAlgorithm();
				break;
		}
		// viewer.setLayoutAlgorithm(algorithm);

		viewer.setLayoutAlgorithm(new CompositeLayoutAlgorithm(new LayoutAlgorithm[] { algorithm, new ExpansionAlgorithm() }));

		actionSetAlgorithm[layoutAlgorithm.getValue()].setChecked(false);
		layoutAlgorithm = alg;
		actionSetAlgorithm[layoutAlgorithm.getValue()].setChecked(true);
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
					Dimension size = ((GraphNode)o).getSize();
					((NetworkMapElement)data).setLocation(loc.x + (size.width + 1) / 2, loc.y + (size.height + 1) / 2);
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
		viewer.setLayoutAlgorithm(new ManualLayout(), true);

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
		setLayoutAlgorithm(layoutAlgorithm, true);

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

		actionShowStatusBackground = new Action(Messages.get().AbstractNetworkMapView_ShowStatusBkgnd, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowStatusBackground(!labelProvider.isShowStatusBackground());
				setChecked(labelProvider.isShowStatusBackground());
				updateObjectPositions();
				saveLayout();
				viewer.refresh();
			}
		};
		actionShowStatusBackground.setChecked(labelProvider.isShowStatusBackground());
		actionShowStatusBackground.setEnabled(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);

		actionShowStatusIcon = new Action(Messages.get().AbstractNetworkMapView_ShowStatusIcon, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowStatusIcons(!labelProvider.isShowStatusIcons());
				setChecked(labelProvider.isShowStatusIcons());
				updateObjectPositions();
				saveLayout();
				viewer.refresh();
			}
		};
		actionShowStatusIcon.setChecked(labelProvider.isShowStatusIcons());
		actionShowStatusIcon.setEnabled(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);

		actionShowStatusFrame = new Action(Messages.get().AbstractNetworkMapView_ShowStatusFrame, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				labelProvider.setShowStatusFrame(!labelProvider.isShowStatusFrame());
				setChecked(labelProvider.isShowStatusFrame());
				updateObjectPositions();
				saveLayout();
				viewer.refresh();
			}
		};
		actionShowStatusFrame.setChecked(labelProvider.isShowStatusFrame());
		actionShowStatusFrame.setEnabled(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);

		actionZoomIn = new Action(Messages.get().AbstractNetworkMapView_ZoomIn) {
			@Override
			public void run()
			{
				viewer.zoomIn();
			}
		};
		actionZoomIn.setImageDescriptor(SharedIcons.ZOOM_IN);

		actionZoomOut = new Action(Messages.get().AbstractNetworkMapView_ZoomOut) {
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
			final MapLayoutAlgorithm alg = MapLayoutAlgorithm.getByValue(i);
			actionSetAlgorithm[i] = new Action(layoutAlgorithmNames[i], Action.AS_RADIO_BUTTON) {
				@Override
				public void run()
				{
					setLayoutAlgorithm(alg, true);
					viewer.setInput(mapPage);
				}
			};
			actionSetAlgorithm[i].setChecked(layoutAlgorithm.getValue() == i);
			actionSetAlgorithm[i].setEnabled(automaticLayoutEnabled);
		}

		actionSetRouter = new Action[connectionRouterNames.length];
		for(int i = 0; i < connectionRouterNames.length; i++)
		{
			final int alg = i + 1;
			actionSetRouter[i] = new Action(connectionRouterNames[i], Action.AS_RADIO_BUTTON) {
				@Override
				public void run()
				{
					setConnectionRouter(alg, true);
				}
			};
			actionSetRouter[i].setChecked(routingAlgorithm == alg);
		}

		actionEnableAutomaticLayout = new Action(Messages.get().AbstractNetworkMapView_EnableAutoLayout, Action.AS_CHECK_BOX) {
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

		actionSaveLayout = new Action(Messages.get().AbstractNetworkMapView_SaveLayout) {
			@Override
			public void run()
			{
				updateObjectPositions();
				saveLayout();
			}
		};
		actionSaveLayout.setImageDescriptor(SharedIcons.SAVE);
		actionSaveLayout.setEnabled(!automaticLayoutEnabled);

		actionOpenSubmap = new Action(Messages.get().AbstractNetworkMapView_OpenSubmap) {
			@Override
			public void run()
			{
				openSubmap();
			}
		};
		actionOpenSubmap.setEnabled(false);

		actionFiguresIcons = new Action(Messages.get().AbstractNetworkMapView_Icons, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				labelProvider.setObjectFigureType(ObjectFigureType.ICON);
				updateObjectPositions();
				saveLayout();
				viewer.refresh(true);
				actionShowStatusBackground.setEnabled(true);
				actionShowStatusFrame.setEnabled(true);
				actionShowStatusIcon.setEnabled(true);
			}
		};
		actionFiguresIcons.setChecked(labelProvider.getObjectFigureType() == ObjectFigureType.ICON);

		actionFiguresSmallLabels = new Action(Messages.get().AbstractNetworkMapView_SmallLabels, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				labelProvider.setObjectFigureType(ObjectFigureType.SMALL_LABEL);
				updateObjectPositions();
				saveLayout();
				viewer.refresh(true);
				actionShowStatusBackground.setEnabled(false);
				actionShowStatusFrame.setEnabled(false);
				actionShowStatusIcon.setEnabled(false);
			}
		};
		actionFiguresSmallLabels.setChecked(labelProvider.getObjectFigureType() == ObjectFigureType.SMALL_LABEL);

		actionFiguresLargeLabels = new Action(Messages.get().AbstractNetworkMapView_LargeLabels, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
				labelProvider.setObjectFigureType(ObjectFigureType.LARGE_LABEL);
				updateObjectPositions();
				saveLayout();
				viewer.refresh(true);
				actionShowStatusBackground.setEnabled(false);
				actionShowStatusFrame.setEnabled(false);
				actionShowStatusIcon.setEnabled(false);
			}
		};
		actionFiguresLargeLabels.setChecked(labelProvider.getObjectFigureType() == ObjectFigureType.LARGE_LABEL);

		actionShowGrid = new Action(Messages.get().AbstractNetworkMapView_ShowGrid, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				viewer.showGrid(actionShowGrid.isChecked());
			}
		};
		actionShowGrid.setImageDescriptor(Activator.getImageDescriptor("icons/grid.png")); //$NON-NLS-1$
		actionShowGrid.setChecked(viewer.isGridVisible());

		actionSnapToGrid = new Action(Messages.get().AbstractNetworkMapView_SnapToGrid, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				viewer.setSnapToGrid(actionSnapToGrid.isChecked());
			}
		};
		actionSnapToGrid.setImageDescriptor(Activator.getImageDescriptor("icons/snap_to_grid.png")); //$NON-NLS-1$
		actionSnapToGrid.setChecked(viewer.isSnapToGrid());

		actionAlignToGrid = new Action(Messages.get().AbstractNetworkMapView_AlignToGrid, Activator.getImageDescriptor("icons/align_to_grid.gif")) { //$NON-NLS-1$
			@Override
			public void run()
			{
				viewer.alignToGrid(false);
				updateObjectPositions();
			}
		};

		actionShowObjectDetails = new Action(Messages.get().AbstractNetworkMapView_ShowObjDetails) {
			@Override
			public void run()
			{
				showObjectDetails();
			}
		};
		
		actionCopyImage = new Action("Copy map image to clipboard", SharedIcons.COPY) {
         @Override
         public void run()
         {
            Image image = viewer.takeSnapshot();
            ImageTransfer imageTransfer = ImageTransfer.getInstance();
            final Clipboard clipboard = new Clipboard(viewer.getControl().getDisplay());
            clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
         }
		};
		
		actionHideLinkLabels = new Action("Hide link labels", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {         
            labelProvider.setLabelHideStatus(actionHideLinkLabels.isChecked());
            viewer.refresh(true);
         }
      };
      actionHideLinkLabels.setImageDescriptor(Activator.getImageDescriptor("icons/hide_link.png")); //$NON-NLS-1$
	}

	/**
	 * Create "Layout" submenu
	 * 
	 * @return
	 */
	protected IContributionItem createLayoutSubmenu()
	{
		MenuManager layout = new MenuManager(Messages.get().AbstractNetworkMapView_Layout);
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
	 * Create "Routing" submenu
	 * 
	 * @return
	 */
	protected IContributionItem createRoutingSubmenu()
	{
		MenuManager submenu = new MenuManager(Messages.get().AbstractNetworkMapView_Routing);
		for(int i = 0; i < actionSetRouter.length; i++)
			submenu.add(actionSetRouter[i]);
		return submenu;
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
	 * 
	 * @param manager
	 */
	protected void fillLocalPullDown(IMenuManager manager)
	{
		MenuManager zoom = new MenuManager(Messages.get().AbstractNetworkMapView_Zoom);
		for(int i = 0; i < actionZoomTo.length; i++)
			zoom.add(actionZoomTo[i]);

		MenuManager figureType = new MenuManager(Messages.get().AbstractNetworkMapView_DisplayObjectAs);
		figureType.add(actionFiguresIcons);
		figureType.add(actionFiguresSmallLabels);
		figureType.add(actionFiguresLargeLabels);

		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(createRoutingSubmenu());
		manager.add(zoom);
		manager.add(figureType);
		manager.add(new Separator());
		manager.add(actionAlignToGrid);
		manager.add(actionSnapToGrid);
		manager.add(actionShowGrid);
      manager.add(new Separator()); 
      manager.add(actionHideLinkLabels);    
      manager.add(new Separator());      
      manager.add(actionCopyImage);
		manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 */
	protected void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionZoomIn);
		manager.add(actionZoomOut);
		manager.add(new Separator());
		manager.add(actionAlignToGrid);
		manager.add(actionSnapToGrid);
		manager.add(actionShowGrid);
      manager.add(new Separator()); 
      manager.add(actionHideLinkLabels);  
		manager.add(new Separator());
		if (allowManualLayout)
		{
			manager.add(actionSaveLayout);
		}
      manager.add(actionCopyImage);
      manager.add(new Separator());
		manager.add(actionRefresh);
	}

	/**
	 * Create popup menu for map
	 */
	private void createPopupMenu()
	{
		// Create menu manager.
		MenuManager menuMgr = new FilteringMenuManager(Activator.PLUGIN_ID);
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
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
	 * 
	 * @param manager Menu manager
	 */
	protected void fillObjectContextMenu(IMenuManager manager)
	{
		manager.add(actionOpenSubmap);
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_OBJECT_CREATION));
		manager.add(new Separator());
		manager.add(new GroupMarker(GroupMarkers.MB_NXVS));
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

		if (currentSelection.size() == 1)
		{
			manager.add(new Separator());
			manager.add(actionShowObjectDetails);
			manager.add(new GroupMarker(GroupMarkers.MB_PROPERTIES));
			manager.add(new PropertyDialogAction(getSite(), this));
		}
	}

	/**
	 * Fill context menu for map element
	 * 
	 * @param manager Menu manager
	 */
	protected void fillElementContextMenu(IMenuManager manager)
	{
	}

	/**
	 * Fill context menu for link between objects
	 * 
	 * @param manager Menu manager
	 */
	protected void fillLinkContextMenu(IMenuManager manager)
	{
	}

	/**
	 * Fill context menu for map view
	 * 
	 * @param manager Menu manager
	 */
	protected void fillMapContextMenu(IMenuManager manager)
	{
		MenuManager zoom = new MenuManager(Messages.get().AbstractNetworkMapView_Zoom);
		for(int i = 0; i < actionZoomTo.length; i++)
			zoom.add(actionZoomTo[i]);

		MenuManager figureType = new MenuManager(Messages.get().AbstractNetworkMapView_DisplayObjectAs);
		figureType.add(actionFiguresIcons);
		figureType.add(actionFiguresSmallLabels);
		figureType.add(actionFiguresLargeLabels);

		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(createRoutingSubmenu());
		manager.add(zoom);
		manager.add(figureType);
		manager.add(new Separator());
		manager.add(actionAlignToGrid);
		manager.add(actionSnapToGrid);
		manager.add(actionShowGrid);
      manager.add(new Separator()); 
      manager.add(actionHideLinkLabels);  
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
		if (first instanceof AbstractObject)
		{
			type = SELECTION_OBJECTS;
			firstClass = AbstractObject.class;
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
	protected void onObjectChange(final AbstractObject object)
	{
		NetworkMapObject element = mapPage.findObjectElement(object.getObjectId());
		if (element != null)
			viewer.refresh(element, true);

		List<NetworkMapLink> links = mapPage.findLinksWithStatusObject(object.getObjectId());
		if (links != null)
		{
			for(NetworkMapLink l : links)
				viewer.refresh(l);
			addDciToRequestList();
		}

		if (object.getObjectId() == rootObject.getObjectId())
			rootObject = object;
	}

	/**
	 * Called when map layout has to be saved. Object positions already updated when this method is called. Default implementation
	 * does nothing.
	 */
	protected void saveLayout()
	{
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		viewer.getControl().setFocus();
	}

	/*
	 * (non-Javadoc)
	 * 
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
			settings.put(viewId + ".objectFigureType", labelProvider.getObjectFigureType().ordinal()); //$NON-NLS-1$
		}
		removeDciFromRequestList();
		super.dispose();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
	 */
	@Override
	public void addSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.add(listener);
	}

	/**
	 * Transform viewer's selection to form usable by another plugins by extracting NetXMS objects from map elements.
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
				AbstractObject object = session.findObjectById(((NetworkMapObject)element).getObjectId());
				if (object != null)
				{
					objects.add(object);
				}
				else
				{
					// Fix for issue NX-24
					// If object not found, add map element to selection
					// This will allow removal of unknown objects from map
					objects.add(element);
				}
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

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
	 */
	@Override
	public ISelection getSelection()
	{
		return currentSelection;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener
	 * )
	 */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void setSelection(ISelection selection)
	{
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.gef4.zest.core.viewers.IZoomableWorkbenchPart#getZoomableViewer()
	 */
	@Override
	public AbstractZoomableViewer getZoomableViewer()
	{
		return viewer;
	}

	/**
	 * Open submap for currently selected object
	 */
	private void openSubmap()
	{
		if (currentSelection == null)
			return;

		Object object = currentSelection.getFirstElement();
		if (object instanceof AbstractObject)
		{
			long submapId = (object instanceof NetworkMap) ? ((AbstractObject)object).getObjectId() : ((AbstractObject)object)
					.getSubmapId();
			if (submapId != 0)
			{
				try
				{
					getSite().getPage().showView(PredefinedMap.ID, Long.toString(submapId), IWorkbenchPage.VIEW_ACTIVATE);
				}
				catch(PartInitException e)
				{
					MessageDialogHelper.openError(getSite().getShell(), Messages.get().AbstractNetworkMapView_Error, String.format(Messages.get().AbstractNetworkMapView_OpenSubmapError, e.getMessage()));
				}
			}
		}
	}

	/**
	 * Set map default connection routing algorithm
	 * 
	 * @param routingAlgorithm
	 */
	public void setConnectionRouter(int routingAlgorithm, boolean doSave)
	{
		switch(routingAlgorithm)
		{
			case NetworkMapLink.ROUTING_MANHATTAN:
				this.routingAlgorithm = NetworkMapLink.ROUTING_MANHATTAN;
				viewer.getGraphControl().setRouter(new ManhattanConnectionRouter());
				break;
			default:
				this.routingAlgorithm = NetworkMapLink.ROUTING_DIRECT;
				viewer.getGraphControl().setRouter(null);
				break;
		}
		for(int i = 0; i < actionSetRouter.length; i++)
			actionSetRouter[i].setChecked(routingAlgorithm == (i + 1));
		if (doSave)
		{
			updateObjectPositions();
			saveLayout();
		}
		viewer.refresh();
	}

	/**
	 * Show details for selected object
	 */
	private void showObjectDetails()
	{
		if ((currentSelection.size() != 1) || !(currentSelection.getFirstElement() instanceof AbstractObject))
			return;

		AbstractObject object = (AbstractObject)currentSelection.getFirstElement();
		if (object != null)
		{
			try
			{
				PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage()
						.showView("org.netxms.ui.eclipse.objectview.view.tabbed_object_view"); //$NON-NLS-1$

				if (!selectionListeners.isEmpty())
				{
					SelectionChangedEvent event = new SelectionChangedEvent(AbstractNetworkMapView.this, currentSelection);
					for(ISelectionChangedListener l : selectionListeners)
					{
						l.selectionChanged(event);
					}
				}
			}
			catch(PartInitException e)
			{
				MessageDialogHelper.openError(getSite().getShell(), Messages.get().AbstractNetworkMapView_Error,
						String.format(Messages.get().AbstractNetworkMapView_OpenObjDetailsError, e.getLocalizedMessage()));
			}
		}
	}

	private static int safeParseInt(String string)
	{
		if (string == null)
			return 65535;
		try
		{
			return Integer.parseInt(string);
		}
		catch(NumberFormatException e)
		{
			return 65535;
		}
	}

	/**
	 * Internal data for object double click handlers
	 */
	private class DoubleClickHandlerData
	{
		ObjectDoubleClickHandler handler;
		int priority;
		Class<?> enabledFor;
	}
	
	/**
	 * Goes thought all links and trys to add to request list required DCIs.
	 */
	protected void addDciToRequestList()
   {
      Collection<NetworkMapLink> linkList = mapPage.getLinks();
      for (NetworkMapLink item : linkList)
      {
         if(item.hasDciData())
         {
            for (SingleDciConfig value :item.getDciAsList())
            {
               if(value.type == SingleDciConfig.ITEM)
               {
                  dciValueProvider.addDci(value.getNodeId(), value.dciId, mapPage);
               }
               else
               {
                  dciValueProvider.addDci(value.getNodeId(), value.dciId, value.column, value.instance, mapPage);
               }
            }
         }
      }
      Collection<NetworkMapElement> mapElements = mapPage.getElements();
      for (NetworkMapElement element : mapElements)
      {
         if(element instanceof NetworkMapDCIContainer)
         {
            NetworkMapDCIContainer item = (NetworkMapDCIContainer)element;
            if(item.hasDciData())
            {
               for (SingleDciConfig value : item.getObjectDCIArray())
               {
                  if(value.type == SingleDciConfig.ITEM)
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.dciId, mapPage);
                  }
                  else
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.dciId, value.column, value.instance, mapPage);
                  }
               }
            }
         }
         
         if(element instanceof NetworkMapDCIImage)
         {
            NetworkMapDCIImage item = (NetworkMapDCIImage)element;
            DCIImageConfiguration config = item.getImageOptions();
            SingleDciConfig value = config.getDci();
            if(value.type == SingleDciConfig.ITEM)
            {
               dciValueProvider.addDci(value.getNodeId(), value.dciId, mapPage);
            }
            else
            {
               dciValueProvider.addDci(value.getNodeId(), value.dciId, value.column, value.instance, mapPage);
            }
         }
      }
   }
   
	/**
    * Removes DCIs from request list 
    */
   protected void removeDciFromRequestList()
   {
      Collection<NetworkMapLink> linkList = mapPage.getLinks();
      for (NetworkMapLink item : linkList)
      {
         if(item.hasDciData())
         {
            dciValueProvider.removeDcis(mapPage);
         }
      }
   }
}
