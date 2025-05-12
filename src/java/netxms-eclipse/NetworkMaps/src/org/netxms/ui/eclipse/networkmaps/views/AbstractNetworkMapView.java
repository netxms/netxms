/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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

import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
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
import org.eclipse.jface.commands.ActionHandler;
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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.IWorkbenchActionConstants;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.PlatformUI;
import org.eclipse.ui.contexts.IContextService;
import org.eclipse.ui.handlers.IHandlerService;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.SimpleDciValue;
import org.netxms.client.maps.MapLayoutAlgorithm;
import org.netxms.client.maps.MapObjectDisplayMode;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.configs.MapDataSource;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.NetworkMap;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.GroupMarkers;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.networkmaps.Activator;
import org.netxms.ui.eclipse.networkmaps.Messages;
import org.netxms.ui.eclipse.networkmaps.algorithms.ExpansionAlgorithm;
import org.netxms.ui.eclipse.networkmaps.algorithms.ManualLayout;
import org.netxms.ui.eclipse.networkmaps.api.ObjectDoubleClickHandlerRegistry;
import org.netxms.ui.eclipse.networkmaps.views.helpers.BendpointEditor;
import org.netxms.ui.eclipse.networkmaps.views.helpers.ExtendedGraphViewer;
import org.netxms.ui.eclipse.networkmaps.views.helpers.LinkDciValueProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapContentProvider;
import org.netxms.ui.eclipse.networkmaps.views.helpers.MapLabelProvider;
import org.netxms.ui.eclipse.objectbrowser.api.ObjectContextMenu;
import org.netxms.ui.eclipse.perfview.views.HistoricalGraphView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.Command;
import org.netxms.ui.eclipse.tools.CommandBridge;
import org.netxms.ui.eclipse.tools.FilteringMenuManager;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;

/**
 * Base class for network map views
 */
public abstract class AbstractNetworkMapView extends ViewPart implements ISelectionProvider, IZoomableWorkbenchPart, Command
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
   protected CompositeWithMessageBar viewerContainer;
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;
	protected MapLayoutAlgorithm layoutAlgorithm = MapLayoutAlgorithm.SPRING;
	protected int routingAlgorithm = NetworkMapLink.ROUTING_DIRECT;
	protected boolean allowManualLayout = false; // True if manual layout can be switched on
	protected boolean automaticLayoutEnabled = true; // Current layout mode - automatic or manual
	protected boolean alwaysFitLayout = false;
   protected boolean disableGeolocationBackground = false;

	protected Action actionRefresh;
	protected Action actionShowStatusIcon;
	protected Action actionShowStatusBackground;
	protected Action actionShowStatusFrame;
	protected Action actionShowLinkDirection;
   protected Action actionTranslucentLabelBkgnd;
	protected Action actionZoomIn;
	protected Action actionZoomOut;
	protected Action actionZoomFit;
	protected Action[] actionZoomTo;
	protected Action[] actionSetAlgorithm;
	protected Action[] actionSetRouter;
	protected Action actionAlwaysFitLayout;
	protected Action actionEnableAutomaticLayout;
	protected Action actionSaveLayout;
	protected Action actionOpenDrillDownObject;
	protected Action actionFiguresIcons;
	protected Action actionFiguresSmallLabels;
	protected Action actionFiguresLargeLabels;
	protected Action actionFiguresStatusIcons;
	protected Action actionFiguresFloorPlan;
	protected Action actionShowGrid;
	protected Action actionAlignToGrid;
	protected Action actionSnapToGrid;
	protected Action actionShowObjectDetails;
	protected Action actionCopyImage;
   protected Action actionSaveImage;
   protected Action actionHideLinkLabels;
   protected Action actionHideLinks;
   protected Action actionSelectAllObjects;

	private String viewId;
	private IStructuredSelection currentSelection = new StructuredSelection(new Object[0]);
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
	private BendpointEditor bendpointEditor = null;
	private SessionListener sessionListener;
	private ObjectDoubleClickHandlerRegistry doubleClickHandlers;
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
      disableGeolocationBackground = ps.getBoolean("DISABLE_GEOLOCATION_BACKGROUND"); //$NON-NLS-1$

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
	public final void createPartControl(Composite parent)
	{
		parent.setLayout(new FillLayout());

		viewerContainer = new CompositeWithMessageBar(parent, SWT.NONE);
		
		viewer = new ExtendedGraphViewer(viewerContainer.getContent(), SWT.NONE);
      labelProvider = new MapLabelProvider(viewer);
		viewer.setContentProvider(new MapContentProvider(viewer, labelProvider));
		viewer.setLabelProvider(labelProvider);
      viewer.setBackgroundColor(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND).getRGB());

      IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			alwaysFitLayout = settings.getBoolean(viewId + ".alwaysFitLayout"); //$NON-NLS-1$
		}
		catch(Exception e)
		{
		}
		
		// Zoom level restore and save
		try
		{
		   viewer.zoomTo(settings.getDouble(viewId + ".zoom")); //$NON-NLS-1$
		}
		catch(NumberFormatException e)
		{
		}
		viewer.getGraphControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            IDialogSettings settings = Activator.getDefault().getDialogSettings();
            settings.put(viewId + ".zoom", viewer.getZoom()); //$NON-NLS-1$
         }
      });

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
						actionOpenDrillDownObject.setEnabled(object.getDrillDownObjectId() != 0);
					}
					else
					{
						actionOpenDrillDownObject.setEnabled(false);
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
					actionOpenDrillDownObject.setEnabled(false);
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
				if (selectionType == SELECTION_EMPTY)
				   return;
				
				if (selectionType == SELECTION_OBJECTS)
				{
					doubleClickHandlers.handleDoubleClick((AbstractObject)currentSelection.getFirstElement());
				}
				else if (selectionType == SELECTION_LINKS)
				{
				   NetworkMapLink link = (NetworkMapLink)currentSelection.getFirstElement();
				   if (link.getRouting() != NetworkMapLink.ROUTING_BENDPOINTS)
				      openLinkDci();
				}
			}
		});

		sessionListener = new SessionListener() {
			@Override
			public void notificationHandler(final SessionNotification n)
			{
				if (n.getCode() == SessionNotification.OBJECT_CHANGED)
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

		activateContext();
		doubleClickHandlers = new ObjectDoubleClickHandlerRegistry(this);
		setupMapControl();
		refreshMap();
	}

	/**
	 * Called from createPartControl to allow subclasses to do additional map setup. Subclasses
	 * should override this method instead of createPartControl. Default implementation do nothing.
	 */
	protected void setupMapControl()
	{
	}

   /**
    * Activate context
    */
   private void activateContext()
   {
      IContextService contextService = (IContextService)getSite().getService(IContextService.class);
      if (contextService != null)
      {
         contextService.activateContext("org.netxms.ui.eclipse.networkmaps.context.NetworkMaps"); //$NON-NLS-1$
      }
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

		viewer.setLayoutAlgorithm(alwaysFitLayout ? algorithm : new CompositeLayoutAlgorithm(new LayoutAlgorithm[] { algorithm, new ExpansionAlgorithm() }));

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
      final IHandlerService handlerService = (IHandlerService)getSite().getService(IHandlerService.class);
      
		actionRefresh = new RefreshAction(this) {
			@Override
			public void run()
			{
				refreshMap();
			}
		};

		actionShowLinkDirection = new Action("Show link direction", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setShowLinkDirection(!labelProvider.isShowLinkDirection());
            setChecked(labelProvider.isShowLinkDirection());
            updateObjectPositions();
            saveLayout();
            viewer.refresh();
         }
      };
      actionShowLinkDirection.setChecked(labelProvider.isShowLinkDirection());
		
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
		actionShowStatusBackground.setEnabled(labelProvider.getObjectFigureType() == MapObjectDisplayMode.ICON);

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
		actionShowStatusIcon.setEnabled(labelProvider.getObjectFigureType() == MapObjectDisplayMode.ICON);

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
		actionShowStatusFrame.setEnabled(labelProvider.getObjectFigureType() == MapObjectDisplayMode.ICON);

      actionTranslucentLabelBkgnd = new Action(Messages.get().AbstractNetworkMapView_TranslucentLabelBkgnd, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setTranslucentLabelBackground(actionTranslucentLabelBkgnd.isChecked());
            updateObjectPositions();
            saveLayout();
            viewer.refresh();
         }
      };
      actionTranslucentLabelBkgnd.setChecked(labelProvider.isTranslucentLabelBackground());

		actionZoomIn = new Action(Messages.get().AbstractNetworkMapView_ZoomIn, SharedIcons.ZOOM_IN) {
			@Override
			public void run()
			{
				viewer.zoomIn();
			}
		};
		actionZoomIn.setId("org.netxms.ui.eclipse.networkmaps.localActions.AbstractMap.ZoomIn"); //$NON-NLS-1$
		actionZoomIn.setActionDefinitionId("org.netxms.ui.eclipse.networkmaps.localCommands.AbstractMap.ZoomIn"); //$NON-NLS-1$
      handlerService.activateHandler(actionZoomIn.getActionDefinitionId(), new ActionHandler(actionZoomIn));

		actionZoomOut = new Action(Messages.get().AbstractNetworkMapView_ZoomOut, SharedIcons.ZOOM_OUT) {
			@Override
			public void run()
			{
				viewer.zoomOut();
			}
		};
		actionZoomOut.setId("org.netxms.ui.eclipse.networkmaps.localActions.AbstractMap.ZoomOut"); //$NON-NLS-1$
		actionZoomOut.setActionDefinitionId("org.netxms.ui.eclipse.networkmaps.localCommands.AbstractMap.ZoomOut"); //$NON-NLS-1$
      handlerService.activateHandler(actionZoomOut.getActionDefinitionId(), new ActionHandler(actionZoomOut));

      actionZoomFit = new Action(Messages.get().AbstractNetworkMapView_ZoomFit, Activator.getImageDescriptor("icons/fit.png")) { //$NON-NLS-1$
         @Override
         public void run()
         {
            viewer.zoomFit();
         }
      };
      actionZoomFit.setId("org.netxms.ui.eclipse.networkmaps.localActions.AbstractMap.ZoomToFit"); //$NON-NLS-1$
      actionZoomFit.setActionDefinitionId("org.netxms.ui.eclipse.networkmaps.localCommands.AbstractMap.ZoomToFit"); //$NON-NLS-1$
      handlerService.activateHandler(actionZoomFit.getActionDefinitionId(), new ActionHandler(actionZoomFit));

		actionZoomTo = viewer.createZoomActions(handlerService);

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
		
		actionAlwaysFitLayout = new Action(Messages.get().AbstractNetworkMapView_AlwaysFitLayout, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            alwaysFitLayout = actionAlwaysFitLayout.isChecked();
            setLayoutAlgorithm(layoutAlgorithm, true);
            IDialogSettings settings = Activator.getDefault().getDialogSettings();
            settings.put(viewId + ".alwaysFitLayout", alwaysFitLayout); //$NON-NLS-1$
         }
      };
      actionAlwaysFitLayout.setChecked(alwaysFitLayout);

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
				saveLayout();
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

		actionOpenDrillDownObject = new Action("Open drill-down object") {
			@Override
			public void run()
			{
				openDrillDownObject();
			}
		};
		actionOpenDrillDownObject.setEnabled(false);

		actionFiguresIcons = new Action(Messages.get().AbstractNetworkMapView_Icons, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
			   setObjectDisplayMode(MapObjectDisplayMode.ICON, true);
			}
		};

		actionFiguresSmallLabels = new Action(Messages.get().AbstractNetworkMapView_SmallLabels, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setObjectDisplayMode(MapObjectDisplayMode.SMALL_LABEL, true);
			}
		};

		actionFiguresLargeLabels = new Action(Messages.get().AbstractNetworkMapView_LargeLabels, Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setObjectDisplayMode(MapObjectDisplayMode.LARGE_LABEL, true);
			}
		};
		
		actionFiguresStatusIcons = new Action(Messages.get().AbstractNetworkMapView_StatusIcons, Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            setObjectDisplayMode(MapObjectDisplayMode.STATUS, true);
         }
      };
      
      actionFiguresFloorPlan = new Action("Floor plan", Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            setObjectDisplayMode(MapObjectDisplayMode.FLOOR_PLAN, true);
         }
      };


		actionShowGrid = new Action(Messages.get().AbstractNetworkMapView_ShowGrid, Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				viewer.showGrid(actionShowGrid.isChecked());
			}
		};
		actionShowGrid.setImageDescriptor(Activator.getImageDescriptor("icons/grid.png")); //$NON-NLS-1$
		actionShowGrid.setChecked(viewer.isGridVisible());
		actionShowGrid.setId("org.netxms.ui.eclipse.networkmaps.localActions.AbstractMap.ShowGrid"); //$NON-NLS-1$
		actionShowGrid.setActionDefinitionId("org.netxms.ui.eclipse.networkmaps.localCommands.AbstractMap.ShowGrid"); //$NON-NLS-1$
      handlerService.activateHandler(actionShowGrid.getActionDefinitionId(), new ActionHandler(actionShowGrid));

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
		actionAlignToGrid.setId("org.netxms.ui.eclipse.networkmaps.localActions.AbstractMap.AlignToGrid"); //$NON-NLS-1$
		actionAlignToGrid.setActionDefinitionId("org.netxms.ui.eclipse.networkmaps.localCommands.AbstractMap.AlignToGrid"); //$NON-NLS-1$
      handlerService.activateHandler(actionAlignToGrid.getActionDefinitionId(), new ActionHandler(actionAlignToGrid));

		actionShowObjectDetails = new Action(Messages.get().AbstractNetworkMapView_ShowObjDetails) {
			@Override
			public void run()
			{
				showObjectDetails();
			}
		};
		
		actionCopyImage = new Action(Messages.get().AbstractNetworkMapView_CopyToClipboard, SharedIcons.COPY) {
         @Override
         public void run()
         {
            Image image = viewer.takeSnapshot();
            ImageTransfer imageTransfer = ImageTransfer.getInstance();
            final Clipboard clipboard = new Clipboard(viewer.getControl().getDisplay());
            clipboard.setContents(new Object[] { image.getImageData() }, new Transfer[] { imageTransfer });
         }
		};
		
      actionSaveImage = new Action(Messages.get().AbstractNetworkMapView_SaveToFile) {
         @Override
         public void run()
         {
            saveMapImageToFile(null);
         }
      };
      
		actionHideLinkLabels = new Action(Messages.get().AbstractNetworkMapView_HideLinkLabels, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {         
            labelProvider.setConnectionLabelsVisible(!actionHideLinkLabels.isChecked());
            viewer.refresh(true);
         }
      };
      actionHideLinkLabels.setImageDescriptor(Activator.getImageDescriptor("icons/hide_link.png")); //$NON-NLS-1$
      
      actionHideLinks = new Action(Messages.get().AbstractNetworkMapView_HideLinks, Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setConnectionsVisible(!actionHideLinks.isChecked());
            viewer.refresh(true);
         }
      };
      actionHideLinks.setImageDescriptor(Activator.getImageDescriptor("icons/hide_net_link.png")); //$NON-NLS-1$

      actionSelectAllObjects = new Action(Messages.get().AbstractNetworkMapView_SelectAllObjects) {
         @Override
         public void run()
         {
            viewer.setSelection(new StructuredSelection(mapPage.getObjectElements()));
         }
      };
      actionSelectAllObjects.setId("org.netxms.ui.eclipse.networkmaps.localActions.AbstractMap.SelectAllObjects"); //$NON-NLS-1$
      actionSelectAllObjects.setActionDefinitionId("org.netxms.ui.eclipse.networkmaps.localCommands.AbstractMap.SelectAllObjects"); //$NON-NLS-1$
      handlerService.activateHandler(actionSelectAllObjects.getActionDefinitionId(), new ActionHandler(actionSelectAllObjects));
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
		}
      layout.add(actionAlwaysFitLayout);
		layout.add(new Separator());
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
		figureType.add(actionFiguresStatusIcons);
		figureType.add(actionFiguresFloorPlan);

		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(actionShowLinkDirection);
      manager.add(actionTranslucentLabelBkgnd);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(createRoutingSubmenu());
      manager.add(figureType);
      manager.add(new Separator());
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(actionZoomFit);
		manager.add(zoom);
		manager.add(new Separator());
		manager.add(actionAlignToGrid);
		manager.add(actionSnapToGrid);
		manager.add(actionShowGrid);
      manager.add(new Separator()); 
      manager.add(actionHideLinkLabels); 
      manager.add(actionHideLinks);
      manager.add(new Separator());      
      manager.add(actionCopyImage);
      manager.add(actionSaveImage);
		manager.add(new Separator());
      manager.add(actionSelectAllObjects);
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
      manager.add(actionZoomFit);
		manager.add(new Separator());
		manager.add(actionAlignToGrid);
		manager.add(actionSnapToGrid);
		manager.add(actionShowGrid);
      manager.add(new Separator()); 
      manager.add(actionHideLinkLabels);  
      manager.add(actionHideLinks);
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
		manager.add(actionOpenDrillDownObject);
		manager.add(new Separator());
		ObjectContextMenu.fill(manager, getSite(), this);
		if (currentSelection.size() == 1)
		{
			manager.insertAfter(GroupMarkers.MB_PROPERTIES, actionShowObjectDetails);
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
      figureType.add(actionFiguresStatusIcons);
      figureType.add(actionFiguresFloorPlan);

		manager.add(actionShowStatusBackground);
		manager.add(actionShowStatusIcon);
		manager.add(actionShowStatusFrame);
		manager.add(actionShowLinkDirection);
      manager.add(actionTranslucentLabelBkgnd);
		manager.add(new Separator());
		manager.add(createLayoutSubmenu());
		manager.add(createRoutingSubmenu());
      manager.add(figureType);
      manager.add(new Separator());
		manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(actionZoomFit);
		manager.add(zoom);
		manager.add(new Separator());
		manager.add(actionAlignToGrid);
		manager.add(actionSnapToGrid);
		manager.add(actionShowGrid);
      manager.add(new Separator()); 
      manager.add(actionHideLinkLabels);
      manager.add(actionHideLinks);
		manager.add(new Separator());
		manager.add(new GroupMarker(IWorkbenchActionConstants.MB_ADDITIONS));
      manager.add(new Separator()); 
      manager.add(actionSelectAllObjects);
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
	 * Open drill-down object for currently selected object
	 */
	private void openDrillDownObject()
	{
		if (currentSelection == null)
			return;

		long objectId = 0;
		Object object = currentSelection.getFirstElement();
		if (object instanceof AbstractObject)
			objectId = (object instanceof NetworkMap) ? ((AbstractObject)object).getObjectId() : ((AbstractObject)object).getDrillDownObjectId();
		else if (object instanceof NetworkMapTextBox)
		   objectId = ((NetworkMapTextBox)object).getDrillDownObjectId();
		
		if (objectId != 0)
      {
         Object test = session.findObjectById(objectId);
         if (test instanceof NetworkMap)
         {
            try
            {
               getSite().getPage().showView(PredefinedMap.ID, Long.toString(objectId), IWorkbenchPage.VIEW_ACTIVATE);
            }
            catch(PartInitException e)
            {
               MessageDialogHelper.openError(getSite().getShell(), Messages.get().AbstractNetworkMapView_Error, String.format("Cannot open drill-down object view: %s", e.getMessage()));
            }
         }
         if (test instanceof Dashboard)
         {
            try
            {
               getSite().getPage().showView("org.netxms.ui.eclipse.dashboard.views.DashboardView", Long.toString(objectId), IWorkbenchPage.VIEW_ACTIVATE);
            }
            catch(PartInitException e)
            {
               MessageDialogHelper.openError(getSite().getShell(), Messages.get().AbstractNetworkMapView_Error, String.format("Cannot open drill-down object view: %s", e.getMessage()));
            }
         }
      }
	}
	
	/**
	 * Handler for opening network map dci on double click
	 */
	private void openLinkDci()
	{
	   final NetworkMapLink link = (NetworkMapLink)currentSelection.getFirstElement();	   
	   if (!link.hasDciData())
	      return;
	   new ConsoleJob("Open link dci job", this, Activator.PLUGIN_ID) {         
         /* (non-Javadoc)
          * @see org.netxms.ui.eclipse.jobs.ConsoleJob#runInternal(org.eclipse.core.runtime.IProgressMonitor)
          */
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            DciValue[] values = session.getLastValues(link.getDciAsList());
            final StringBuilder sb = new StringBuilder();
            for(DciValue v : values)
            {
               sb.append("&");
               sb.append(Integer.toString(v instanceof SimpleDciValue ? ChartDciConfig.ITEM : ChartDciConfig.TABLE));
               sb.append("@");
               sb.append(Long.toString(v.getNodeId()));
               sb.append("@");
               sb.append(Long.toString(v.getId()));
               sb.append("@");
               sb.append(URLEncoder.encode(v.getDescription(), "UTF-8"));
               sb.append("@");
               sb.append(URLEncoder.encode(v.getName(), "UTF-8"));
            }
            
            runInUIThread(new Runnable() {               
               /* (non-Javadoc)
                * @see java.lang.Runnable#run()
                */
               @Override
               public void run()
               {
                  try
                  {
                     getSite().getPage().showView(HistoricalGraphView.ID, sb.toString(), IWorkbenchPage.VIEW_ACTIVATE);
                  }
                  catch(PartInitException e)
                  {
                     MessageDialogHelper.openError(getSite().getShell(), "Error", String.format("Error opening view: %s", e.getLocalizedMessage()));
                  }
               }
            });
         }         
         /* (non-Javadoc)
          * @see org.netxms.ui.eclipse.jobs.ConsoleJob#getErrorMessage()
          */
         @Override
         protected String getErrorMessage()
         {
            return "Cannot open dci link historical graph view";
         }
      }.start();
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
		try
		{
			PlatformUI.getWorkbench().getActiveWorkbenchWindow().getActivePage()
					.showView("org.netxms.ui.eclipse.objectview.view.tabbed_object_view"); //$NON-NLS-1$
         CommandBridge.getInstance().execute("TabbedObjectView/changeObject", object.getObjectId());
		}
		catch(PartInitException e)
		{
			MessageDialogHelper.openError(getSite().getShell(), Messages.get().AbstractNetworkMapView_Error,
					String.format(Messages.get().AbstractNetworkMapView_OpenObjDetailsError, e.getLocalizedMessage()));
		}
	}

	/**
	 * Goes thought all links and trys to add to request list required DCIs.
	 */
	protected void addDciToRequestList()
   {
      Collection<NetworkMapLink> linkList = mapPage.getLinks();
      for(NetworkMapLink item : linkList)
      {
         if(item.hasDciData())
         {
            for (MapDataSource value :item.getDciAsList())
            {
               if(value.getType() == MapDataSource.ITEM)
               {
                  dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, 1);
               }
               else
               {
                  dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, 1);
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
               for (MapDataSource value : item.getObjectDCIArray())
               {
                  if(value.getType() == MapDataSource.ITEM)
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, 1);
                  }
                  else
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, 1);
                  }
               }
            }
         }

         if (element instanceof NetworkMapDCIImage)
         {
            NetworkMapDCIImage item = (NetworkMapDCIImage)element;
            DCIImageConfiguration config = item.getImageOptions();
            MapDataSource value = config.getDci();
            if (value.getType() == MapDataSource.ITEM)
            {
               dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, 1);
            }
            else
            {
               dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, 1);
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
   
   /**
    * @param mode
    * @param saveLayout
    */
   protected void setObjectDisplayMode(MapObjectDisplayMode mode, boolean saveLayout)
   {
      labelProvider.setObjectFigureType(mode);
      if (saveLayout)
      {
         updateObjectPositions();
         saveLayout();
      }
      viewer.refresh(true);
      actionShowStatusBackground.setEnabled(mode == MapObjectDisplayMode.ICON);
      actionShowStatusFrame.setEnabled(mode == MapObjectDisplayMode.ICON);
      actionShowStatusIcon.setEnabled(mode == MapObjectDisplayMode.ICON);
      actionFiguresIcons.setChecked(labelProvider.getObjectFigureType() == MapObjectDisplayMode.ICON);
      actionFiguresSmallLabels.setChecked(labelProvider.getObjectFigureType() == MapObjectDisplayMode.SMALL_LABEL);
      actionFiguresLargeLabels.setChecked(labelProvider.getObjectFigureType() == MapObjectDisplayMode.LARGE_LABEL);
      actionFiguresStatusIcons.setChecked(labelProvider.getObjectFigureType() == MapObjectDisplayMode.STATUS);
      actionFiguresFloorPlan.setChecked(labelProvider.getObjectFigureType() == MapObjectDisplayMode.FLOOR_PLAN);
   }

   /**
    * Save map image to file
    */
   public boolean saveMapImageToFile(String fileName)
   {
      if (fileName == null)
      {
         FileDialog dlg = new FileDialog(getSite().getShell(), SWT.SAVE);
         dlg.setFilterExtensions(new String[] { ".png" });
         dlg.setOverwrite(true);
         fileName = dlg.open();
         if (fileName == null)
            return false;
      }
      
      Image image = viewer.takeSnapshot();
      try
      {
         ImageLoader loader = new ImageLoader();
         loader.data = new ImageData[] { image.getImageData() };
         loader.save(fileName, SWT.IMAGE_PNG);
         return true;
      }
      catch(Exception e)
      {
         Activator.logError("Exception in saveMapImageToFile", e);
         return false;
      }
      finally
      {
         image.dispose();
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.tools.Command#execute(java.lang.String, java.lang.Object)
    */
   @Override
   public Object execute(String name, Object arg)
   {
      if ("AbstractNetworkMap/SaveToFile".equals(name))
      {
         return Boolean.valueOf(saveMapImageToFile((String)arg));
      }
      return null;
   }
}
