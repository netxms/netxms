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
package org.netxms.nxmc.modules.networkmaps.views;

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
import org.eclipse.gef4.zest.layouts.algorithms.GridLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.RadialLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.SpringLayoutAlgorithm;
import org.eclipse.gef4.zest.layouts.algorithms.TreeLayoutAlgorithm;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IContributionItem;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.MapCanvasType;
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
import org.netxms.client.objects.NetworkMap;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.windows.MainWindow;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.views.HistoricalGraphView;
import org.netxms.nxmc.modules.networkmaps.ObjectDoubleClickHandlerRegistry;
import org.netxms.nxmc.modules.networkmaps.algorithms.ManualLayout;
import org.netxms.nxmc.modules.networkmaps.views.helpers.BendpointEditor;
import org.netxms.nxmc.modules.networkmaps.views.helpers.MapImageManipulationHelper;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.ExtendedGraphViewer;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.FigureChangeCallback;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.LinkDciValueProvider;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.MapContentProvider;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.MapLabelProvider;
import org.netxms.nxmc.modules.objects.ObjectContextMenuManager;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Base class for network map views
 */
public abstract class AbstractNetworkMapView extends ObjectView implements ISelectionProvider, IZoomableWorkbenchPart
{
   private static final Logger logger = LoggerFactory.getLogger(AbstractNetworkMapView.class);
   private final I18n i18n = LocalizationHelper.getI18n(AbstractNetworkMapView.class);

	protected static final int LAYOUT_SPRING = 0;
	protected static final int LAYOUT_RADIAL = 1;
	protected static final int LAYOUT_HTREE = 2;
	protected static final int LAYOUT_VTREE = 3;
	protected static final int LAYOUT_SPARSE_VTREE = 4;

	private static final String[] layoutAlgorithmNames =
   	   {
   	      LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Spring"),
   	      LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Radial"),
   	      LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Horizontal tree"),
   	      LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Vertical tree"),
   	      LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Sparse vertical tree")
   	   };
   private static final String[] connectionRouterNames =
         {
            LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Direct"),
            LocalizationHelper.getI18n(AbstractNetworkMapView.class).tr("Manhattan")
         };

	private static final int SELECTION_EMPTY = 0;
	private static final int SELECTION_MIXED = 1;
	private static final int SELECTION_OBJECTS = 2;
	private static final int SELECTION_ELEMENTS = 3;
	private static final int SELECTION_LINKS = 4;

	private Composite searchBar;
	private Composite mainContent;
   private LabeledText queryEditor;
   private Button previousButton;
   private Button nextButton;
	protected NetworkMapPage mapPage;
	protected ExtendedGraphViewer viewer;
	protected MapLabelProvider labelProvider;
	protected org.netxms.nxmc.modules.networkmaps.widgets.GeoNetworkMapViewer geoViewer;
	private Composite contentParent;
	private Composite stackHolder;
	private Composite graphPanel;
	private org.eclipse.swt.custom.StackLayout stackLayout;
	protected MapLayoutAlgorithm layoutAlgorithm = MapLayoutAlgorithm.SPRING;
	protected int routingAlgorithm = NetworkMapLink.ROUTING_DIRECT;
	protected boolean allowManualLayout = false; // True if manual layout can be switched on
	protected boolean automaticLayoutEnabled = true; // Current layout mode - automatic or manual
	protected boolean objectMoveLocked = true; //default false for adhock maps and true for predefined
	protected boolean readOnly = true;
	protected boolean saveSchedulted = false;

	protected Action actionShowStatusIcon;
	protected Action actionShowStatusBackground;
	protected Action actionShowStatusFrame;
	protected Action actionShowLinkDirection;
   protected Action actionTranslucentLabelBkgnd;
	protected Action actionZoomIn;
	protected Action actionZoomOut;
	protected Action actionZoomFit;
	protected Action[] actionZoomTo;
   protected Action[] actionGeoZoomTo;
	protected Action[] actionSetAlgorithm;
	protected Action[] actionSetRouter;
	protected Action actionEnableAutomaticLayout;
	protected Action actionOpenDrillDownObject;
	protected Action actionFiguresIcons;
	protected Action actionFiguresSmallLabels;
	protected Action actionFiguresLargeLabels;
	protected Action actionFiguresStatusIcons;
	protected Action actionFiguresFloorPlan;
	protected Action actionShowGrid;
   protected Action actionShowSize;
	protected Action actionAlignToGrid;
	protected Action actionSnapToGrid;
	protected Action actionShowObjectDetails;
	protected Action actionCopyImage;
   protected Action actionSaveImage;
   protected Action actionHideLinkLabels;
   protected Action actionHideLinks;
   protected Action actionSelectAllObjects;
   protected Action actionLock;
   protected Action actionToggleCanvasType;
   protected Action actionHSpanIncrease;
   protected Action actionHSpanDecrease;
   protected Action actionHSpanFull;
   protected Action actionVSpanIncrease;
   protected Action actionVSpanDecrease;
   protected Action actionShowLineChart;
   protected Action actionFindObject;

	private IStructuredSelection currentSelection = new StructuredSelection(new Object[0]);
	private Set<ISelectionChangedListener> selectionListeners = new HashSet<ISelectionChangedListener>();
	protected BendpointEditor bendpointEditor = null;
	private SessionListener sessionListener;
	private ObjectDoubleClickHandlerRegistry doubleClickHandlers;
   private LinkDciValueProvider dciValueProvider = LinkDciValueProvider.getInstance();

   /**
    * Create new map view.
    *
    * @param name view name
    * @param image view icon
    * @param id view base ID
    */
   public AbstractNetworkMapView(String name, ImageDescriptor image, String id)
   {
      super(name, image, id, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
   protected void postClone(View view)
   {
      super.postClone(view);
      viewer.zoomTo(((AbstractNetworkMapView)view).viewer.getZoom());
   }

	/**
	 * Build map page containing data to display. Should be implemented in derived classes.
	 */
	protected abstract void buildMapPage(NetworkMapPage oldMapPage);

   /**
    * Message-area entry id for the "N nodes hidden (no location)" notice. Zero
    * when no message is currently shown. Tracked separately from any other
    * messages so we can update or remove it idempotently as the count changes.
    */
   private int unplacedCountMessageId = 0;

   /**
    * Add, update or remove the message-area entry reporting how many map
    * elements are hidden because their object has no geolocation. Called by
    * {@code GeoNetworkMapViewer} via the unplaced-count listener.
    */
   private void updateUnplacedCountMessage(int count)
   {
      if (count <= 0)
      {
         if (unplacedCountMessageId != 0)
         {
            deleteMessage(unplacedCountMessageId);
            unplacedCountMessageId = 0;
         }
         return;
      }
      String text = (count == 1)
            ? i18n.tr("1 node hidden (no location)")
            : i18n.tr("{0} nodes hidden (no location)", count);
      // Replace the previous entry so the count updates in place rather than
      // accumulating duplicate messages as objects gain/lose geolocation.
      if (unplacedCountMessageId != 0)
         deleteMessage(unplacedCountMessageId);
      unplacedCountMessageId = addMessage(org.netxms.nxmc.base.widgets.MessageArea.INFORMATION, text, true);
   }

   /**
    * Per-view session override for the canvas type — set by the toolbar
    * toggle action. {@code null} means "use the map's persisted canvas type".
    * Intentionally NOT persisted: the toggle is a temporary view switch and
    * resets every time the user reopens the map (or navigates to a different
    * one). To make the change permanent the user edits Canvas type on the
    * Map Background property page.
    */
   private Boolean canvasTypeOverride;

   /**
    * True when the currently-bound object's effective canvas type is
    * {@link MapCanvasType#GEOGRAPHICAL}. The session-local toolbar override
    * takes precedence over the map's persisted canvas type when present.
    */
   protected final boolean isGeographicalCanvas()
   {
      // Session-local toolbar override wins regardless of context — ad-hoc
      // topology views (L2, IP, Internal) carry a Node as context, not a
      // NetworkMap, so they have no persisted canvas type to fall through to.
      if (canvasTypeOverride != null)
         return canvasTypeOverride.booleanValue();
      AbstractObject o = getObject();
      if (!(o instanceof NetworkMap))
         return false;
      return ((NetworkMap)o).getCanvasType() == MapCanvasType.GEOGRAPHICAL;
   }

   /**
    * Build content for geographical canvas mode: a tile-backed canvas with
    * object pins, links (with bendpoints / routing), DCI labels, and the
    * unplaced-objects message. Skips all Zest-based widgets and toolbar
    * actions.
    */
   private void createGeoContent(Composite parent)
   {
      // geoViewer lives alongside graphPanel inside a StackLayout on parent
      geoViewer = new org.netxms.nxmc.modules.networkmaps.widgets.GeoNetworkMapViewer(parent, SWT.NONE, this);
      applyGeoLinkDefaults();
      geoViewer.setLinkChangeCallback(new FigureChangeCallback() {
         @Override
         public void onMove(NetworkMapElement element)
         {
            // Object positions on the geo canvas are world-anchored, never
            // moved by user gestures — onMove cannot fire here.
         }

         @Override
         public void onLinkChange(NetworkMapLink link)
         {
            AbstractNetworkMapView.this.onLinkChange(link);
         }
      });
      wireGeoViewerSelection();
      attachGeoContextMenu();
      geoViewer.setUnplacedCountListener(this::updateUnplacedCountMessage);

      requestInitialView();

      AbstractObject o = getObject();
      if (o instanceof NetworkMap)
      {
         NetworkMap m = (NetworkMap)o;
         if (m.getInitialViewMode() != org.netxms.client.maps.MapInitialViewMode.FIT_TO_SCREEN)
         {
            double lat = (m.getBackgroundLocation() != null) ? m.getBackgroundLocation().getLatitude() : 0.0;
            double lon = (m.getBackgroundLocation() != null) ? m.getBackgroundLocation().getLongitude() : 0.0;
            int zoom = (m.getBackgroundZoom() > 0) ? m.getBackgroundZoom() : 3;
            geoViewer.showMap(new org.netxms.nxmc.modules.worldmap.tools.MapAccessor(lat, lon, zoom));
         }
      }
      onGeoViewerCreated();
   }

   /**
    * Hook called once after {@code geoViewer} is created and wired with
    * selection/context menu. Subclasses (e.g. {@code PredefinedMapView})
    * override to install drop support or any other geo-only integration that
    * needs to reach back into subclass state ({@code saveMap}, etc.). Runs at
    * most once per geo viewer instance.
    */
   protected void onGeoViewerCreated()
   {
   }

   /**
    * Ask the geographical viewer to apply the persisted initial view mode
    * once it has a valid viewport size and content. Called when entering
    * geo mode (initial creation or graph→geo switch).
    */
   private void requestInitialView()
   {
      AbstractObject o = getObject();
      if (o instanceof NetworkMap)
      {
         NetworkMap m = (NetworkMap)o;
         org.netxms.client.maps.MapInitialViewMode mode = m.getInitialViewMode();
         if (mode == null)
            mode = org.netxms.client.maps.MapInitialViewMode.FIT_TO_SCREEN;
         geoViewer.setInitialView(mode, m.getBackgroundLocation(), m.getBackgroundZoom());
      }
      else
      {
         // No persisted initial view (ad-hoc topology / VLAN views): fit
         // the placed objects' bbox into the viewport on first paint.
         geoViewer.setInitialView(org.netxms.client.maps.MapInitialViewMode.FIT_TO_SCREEN, null, 0);
      }
   }

   /**
    * Flip the StackLayout to the correct child for the current object's
    * canvas type. Used to handle a live canvas-type change (property-page
    * toggle) on an already-open view — {@code contextChanged} won't fire
    * because the context object didn't change.
    */
   protected void ensureCorrectCanvas()
   {
      if ((stackHolder == null) || stackHolder.isDisposed() || (stackLayout == null))
         return;
      boolean geo = isGeographicalCanvas();
      if (actionToggleCanvasType != null)
         actionToggleCanvasType.setChecked(geo);
      updateActionsForCanvas(geo);
      if (geo)
      {
         if (stackLayout.topControl != geoViewer)
         {
            // Capture the single selected object before swap so the same
            // pin is highlighted on the geo canvas (or selection cleared
            // if the object has no geolocation).
            AbstractObject carryOver = singleSelectedObject();
            switchToGeoContent();
            geoViewer.selectObject(carryOver);
         }
         else
         {
            // Already showing geo — map options may have changed, refresh defaults.
            applyGeoLinkDefaults();
            geoViewer.redraw();
         }
         updateUnplacedCountMessage(geoViewer.getUnplacedObjectCount());
      }
      else
      {
         if (stackLayout.topControl != graphPanel)
         {
            // Capture before the StackLayout swap and the setInput rebuild
            // so we can re-apply the same object selection on the graph
            // side (Zest's setInput drops any prior selection).
            AbstractObject carryOver = singleSelectedObject();
            stackLayout.topControl = graphPanel;
            stackHolder.layout(true, true);
            if (mapPage == null)
               buildMapPage(null);
            viewer.setInput(mapPage);
            applyObjectSelectionToGraph(carryOver);
            graphPanel.redraw();
         }
         // Geo-specific message is meaningless on the graph canvas — drop it.
         updateUnplacedCountMessage(0);
      }
   }

   /**
    * Return the single AbstractObject in the current selection, or null when
    * the selection is empty, holds multiple items, or is something other
    * than an object (link, decoration). Used to carry an object selection
    * across a canvas swap.
    */
   private AbstractObject singleSelectedObject()
   {
      if ((currentSelection == null) || (currentSelection.size() != 1))
         return null;
      Object first = currentSelection.getFirstElement();
      return (first instanceof AbstractObject) ? (AbstractObject)first : null;
   }

   /**
    * Apply an object-selection to the Zest viewer after a geo→graph swap.
    * Looks up the {@link NetworkMapObject} element for {@code object} in
    * {@link #mapPage} and selects it, or clears the selection if the object
    * isn't present on this map.
    */
   private void applyObjectSelectionToGraph(AbstractObject object)
   {
      if (object == null)
      {
         viewer.setSelection(new StructuredSelection());
         return;
      }
      NetworkMapObject element = (mapPage != null) ? mapPage.findObjectElement(object.getObjectId()) : null;
      if (element != null)
         viewer.setSelection(new StructuredSelection(element));
      else
         viewer.setSelection(new StructuredSelection());
   }

   /**
    * Toggle the enabled state of actions that make sense only on the Zest
    * graph canvas. Called from {@link #ensureCorrectCanvas} on every canvas
    * swap so the toolbar and menu state always reflects the active canvas.
    *
    * <p>The canvas-toggle and the actions that genuinely work on both
    * canvases (find, copy/save image, select all, hide links/labels, show
    * link direction, zoom in/out/fit, drill-down, line chart) stay enabled
    * in both modes.</p>
    */
   private void updateActionsForCanvas(boolean geo)
   {
      boolean graph = !geo;

      // Layout/topology: graph-only. Routing applies to both canvases.
      actionEnableAutomaticLayout.setEnabled(graph);
      for(IAction a : actionSetAlgorithm)
         a.setEnabled(graph);
      actionAlignToGrid.setEnabled(graph);
      actionSnapToGrid.setEnabled(graph);
      actionShowGrid.setEnabled(graph);
      actionShowSize.setEnabled(graph);
      actionLock.setEnabled(graph);

      // Object spacing — modifies the graph map's pixel dimensions, which the
      // geo canvas doesn't honour (positions come from lat/lon).
      actionHSpanIncrease.setEnabled(graph);
      actionHSpanDecrease.setEnabled(graph);
      actionHSpanFull.setEnabled(graph);
      actionVSpanIncrease.setEnabled(graph);
      actionVSpanDecrease.setEnabled(graph);

      // Object display style — Zest figure rendering. Geo canvas always
      // renders pins regardless of these toggles.
      actionFiguresIcons.setEnabled(graph);
      actionFiguresSmallLabels.setEnabled(graph);
      actionFiguresLargeLabels.setEnabled(graph);
      actionFiguresStatusIcons.setEnabled(graph);
      actionFiguresFloorPlan.setEnabled(graph);
      actionShowStatusIcon.setEnabled(graph);
      actionShowStatusBackground.setEnabled(graph);
      actionShowStatusFrame.setEnabled(graph);
      actionTranslucentLabelBkgnd.setEnabled(graph);

      // Zoom-to-percent vs zoom-to-tile-level: only one applies at a time,
      // and the menu builders pick the right array. We toggle enabled state in
      // sync so disabled entries don't accidentally surface anywhere else.
      for(IAction a : actionZoomTo)
         a.setEnabled(graph);
      for(IAction a : actionGeoZoomTo)
         a.setEnabled(geo);
   }

   /**
    * Build the array of "zoom to level N" actions for the geographical canvas.
    * Each action sets the tile viewer to that zoom level and recenters on the
    * current map position. Action labels read "Level 1"..."Level N". The
    * actions are AS_RADIO_BUTTON so the menu shows which level is active.
    */
   private Action[] createGeoZoomActions()
   {
      int min = org.netxms.nxmc.modules.worldmap.tools.MapAccessor.MIN_MAP_ZOOM;
      int max = org.netxms.nxmc.modules.worldmap.tools.MapAccessor.MAX_MAP_ZOOM;
      Action[] actions = new Action[max - min + 1];
      for(int i = 0; i < actions.length; i++)
      {
         final int level = min + i;
         actions[i] = new Action(i18n.tr("Level {0}", level), Action.AS_RADIO_BUTTON) {
            @Override
            public void run()
            {
               org.netxms.nxmc.modules.worldmap.tools.MapAccessor a = geoViewer.getMapAccessor();
               if (a == null)
                  return;
               a.setZoom(level);
               geoViewer.showMap(a);
            }
         };
      }
      return actions;
   }

   /**
    * Set the radio-checked state on the geo zoom-level actions to reflect the
    * geo viewer's current zoom. Called whenever the zoom changes. No-op when
    * the actions haven't been built yet.
    */
   private void syncGeoZoomChecked()
   {
      org.netxms.nxmc.modules.worldmap.tools.MapAccessor a = geoViewer.getMapAccessor();
      if (a == null)
         return;
      int level = a.getZoom();
      int min = org.netxms.nxmc.modules.worldmap.tools.MapAccessor.MIN_MAP_ZOOM;
      for(int i = 0; i < actionGeoZoomTo.length; i++)
         actionGeoZoomTo[i].setChecked((min + i) == level);
   }

   /**
    * Show the geographical viewer (creating it on first use) and populate it
    * with the current map's content. Graph-mode widgets remain alive behind
    * the StackLayout so their references ({@code viewer}, listeners, actions)
    * stay valid — operations against the hidden graph viewer are harmless.
    */
   private void switchToGeoContent()
   {
      if (stackHolder == null || stackHolder.isDisposed())
         return;
      if (geoViewer == null)
         createGeoContent(stackHolder);
      if (stackLayout != null)
      {
         stackLayout.topControl = geoViewer;
         stackHolder.layout(true, true);
      }
      applyGeoLinkDefaults();
      buildMapPage(mapPage);
      geoViewer.setContent(mapPage);
      geoViewer.redraw();
      // requestInitialView() runs from createGeoContent on first creation and
      // from contextChanged on navigation; calling it here too would re-fit
      // on every property-page toggle and clobber the user's pan/zoom.
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#contextChanged(java.lang.Object, java.lang.Object)
    */
   @Override
   protected void contextChanged(Object oldContext, Object newContext)
   {
      // Drop any session-local canvas-type override from the previous map —
      // the toggle is per-view and per-map, so navigating to a different map
      // resets to that map's persisted canvas type.
      canvasTypeOverride = null;
      // Flip the StackLayout to the right child for the new context's canvas
      // type (creating the geo viewer on first use). super.contextChanged
      // then runs onObjectChange in the subclass — syncObjects() must run for
      // both modes so interface-status / utilization link styling resolves.
      ensureCorrectCanvas();
      // Re-request the initial view so navigating between maps applies each
      // map's persisted view mode. tryApplyInitialView is gated on having
      // both viewport and content, so it deferred-applies after the async
      // sync → refresh → setContent path completes for the new map.
      if (isGeographicalCanvas())
         requestInitialView();
      super.contextChanged(oldContext, newContext);
   }

   protected void applyGeoLinkDefaults()
   {
      AbstractObject o = getObject();
      NetworkMap m = (o instanceof NetworkMap) ? (NetworkMap)o : null;
      // Ad-hoc topology / VLAN views have no persisted link defaults — use
      // the same fallbacks the Zest-mode MapLabelProvider uses (color -1
      // means "no map-level default", style SOLID, routing DIRECT).
      int color = (m != null) ? m.getDefaultLinkColor() : -1;
      int colorSource = (m != null) ? m.getDefaultLinkColorSource() : NetworkMapLink.COLOR_SOURCE_DEFAULT;
      // Match the Zest-mode fallback (MapLabelProvider.setDefaultLinkWidth):
      // an unset map-level width (0) falls back to the NetMap.DefaultLinkWidth
      // preference, which itself defaults to 5.
      int width = (m != null) ? m.getDefaultLinkWidth() : 0;
      if (width <= 0)
         width = PreferenceStore.getInstance().getAsInteger("NetMap.DefaultLinkWidth", 5);
      int style = (m != null) ? m.getDefaultLinkStyle() : 1;
      int routing = (m != null) ? m.getDefaultLinkRouting() : NetworkMapLink.ROUTING_DIRECT;
      // Pass the encoded color through; geoViewer resolves it via its own
      // colorCache so the SWT resource is owned by the cache (no per-call
      // new Color(...) leak — applyGeoLinkDefaults runs on every object
      // update and would otherwise burn through GDI / X11 handles).
      geoViewer.setLinkDefaults(color, colorSource, width, style);
      geoViewer.setDefaultLinkRouting(routing);
      geoViewer.setShowLinkDirection((m != null) && m.isShowLinkDirection());
      // Sync the in-session UI toggles for hide-links / hide-link-labels.
      if (actionHideLinks != null)
         geoViewer.setLinksVisible(!actionHideLinks.isChecked());
      if (actionHideLinkLabels != null)
         geoViewer.setLinkLabelsVisible(!actionHideLinkLabels.isChecked());
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
	@Override
   public final void createContent(Composite parent)
	{
      this.contentParent = parent;
      parent.setLayout(new FormLayout());

      // Search bar lives above the stack so it's reachable from both canvas
      // modes. Hidden by default; activateFindObjectMode toggles visibility
      // and re-attaches the stack holder's top to it (or to the parent's
      // top edge when hidden).
      searchBar = new Composite(parent, SWT.NONE);
      GridLayout gridLayout = new GridLayout();
      gridLayout.numColumns = 3;
      searchBar.setLayout(gridLayout);

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      searchBar.setLayoutData(fd);

      queryEditor = new LabeledText(searchBar, SWT.NONE);
      queryEditor.setLabel("Search string");
      queryEditor.getTextControl().addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
            if (e.stateMask == 0)
            {
               if (e.keyCode == SWT.ARROW_DOWN || e.keyCode == 13)
               {
                  performFindObject(queryEditor.getText(), true);
               }
               else if (e.keyCode == SWT.ARROW_UP)
               {
                  performFindObject(queryEditor.getText(), false);
               }
            }
         }
      });
      queryEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      previousButton = new Button(searchBar, SWT.PUSH);
      previousButton.setImage(SharedIcons.IMG_UP);
      previousButton.setToolTipText("Find previous (Arrow Up)");
      previousButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            performFindObject(queryEditor.getText(), false);
         }
      });
      previousButton.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));

      nextButton = new Button(searchBar, SWT.PUSH);
      nextButton.setImage(SharedIcons.IMG_DOWN);
      nextButton.setToolTipText("Find next (Arrow Down)");
      nextButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            performFindObject(queryEditor.getText(), true);
         }
      });
      nextButton.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      searchBar.setVisible(false);

      // stackHolder owns the StackLayout that swaps between the Zest graph
      // panel and the geographical viewer.
      stackHolder = new Composite(parent, SWT.NONE);
      this.stackLayout = new org.eclipse.swt.custom.StackLayout();
      stackHolder.setLayout(this.stackLayout);
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      stackHolder.setLayoutData(fd);

      // Always create the graph-mode panel and Zest viewer; createActions(),
      // createContextMenu(), the session listener and double-click registry
      // below all assume graph-mode widgets exist, and ensureCorrectCanvas()
      // needs graphPanel as a swap target when the user toggles back to GRAPH.
      graphPanel = new Composite(stackHolder, SWT.NONE);
      this.stackLayout.topControl = graphPanel;
      graphPanel.setLayout(new FillLayout());

      mainContent = new Composite(graphPanel, SWT.NONE);
      mainContent.setLayout(new FillLayout());
      mainContent.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_NO));

      viewer = new ExtendedGraphViewer(mainContent, SWT.NONE, this, new FigureChangeCallback() {
         @Override
         public void onMove(NetworkMapElement element)
         {
            onElementMove(element);
         }

         @Override
         public void onLinkChange(NetworkMapLink link)
         {
            AbstractNetworkMapView.this.onLinkChange(link);
         }
      });
      labelProvider = new MapLabelProvider(viewer);
		viewer.setContentProvider(new MapContentProvider(viewer, labelProvider));
		viewer.setLabelProvider(labelProvider);
      viewer.setBackgroundColor(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND).getRGB());

		viewer.getGraphControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            savePreferences();
         }
      });

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
							if (link.getRouting() == NetworkMapLink.ROUTING_BENDPOINTS && !objectMoveLocked)
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

      viewer.addDoubleClickListener((e) -> {
         int selectionType = analyzeSelection(currentSelection);
         if (selectionType == SELECTION_EMPTY)
            return;

         if (selectionType == SELECTION_OBJECTS)
         {
            doubleClickHandlers.handleDoubleClick((AbstractObject)currentSelection.getFirstElement());
         }
         else if (selectionType == SELECTION_LINKS && objectMoveLocked)
			{
            showLinkLineChart();
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
                     processObjectUpdateNotification((AbstractObject)n.getObject());
						}
					});
				}
			}
		};
		session.addListener(sessionListener);

		createActions();
		createContextMenu();
      loadPreferences();

		if (automaticLayoutEnabled)
		{
			setLayoutAlgorithm(layoutAlgorithm, true);
		}
		else
		{
			viewer.setLayoutAlgorithm(new ManualLayout());
		}

		doubleClickHandlers = new ObjectDoubleClickHandlerRegistry(this);
		setupMapControl();

      // If the map is opened with the GEOGRAPHICAL canvas type, create the
      // geo viewer now and flip the StackLayout to it. Graph-mode widgets
      // remain alive behind the StackLayout so subsequent toggles back to
      // GRAPH work without recreating anything.
      if (isGeographicalCanvas())
      {
         createGeoContent(stackHolder);
         stackLayout.topControl = geoViewer;
         stackHolder.layout();
      }
	}

   /**
    * Update map size
    *
    * @param width
    * @param height
    */
   protected void updateMapSize(int width, int height)
   {
      viewer.setMapSize(width, height);
      saveLayout();
   }

	/**
	 * Update map size
	 *
	 * @param width
	 * @param height
	 */
	protected void setMapSize(int width, int height)
	{
	   viewer.setMapSize(width, height);
	}

	/**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }

   /**
    * Called from createPartControl to allow subclasses to do additional map setup. Subclasses should override this method instead
    * of createPartControl. Default implementation do nothing.
    */
	protected void setupMapControl()
	{
	}

	/**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      if (isGeographicalCanvas())
      {
         if (geoViewer != null)
         {
            buildMapPage(mapPage);
            geoViewer.setContent(mapPage);
         }
         return;
      }

      IStructuredSelection selection = viewer.getStructuredSelection();
      List<Object> newSelection = new ArrayList<Object>(selection.size());

		buildMapPage(mapPage);
		viewer.setInput(mapPage);
		for (Object s : selection)
		{
		   if (s instanceof NetworkMapLink)
         {
		      newSelection.add(mapPage.findLink((NetworkMapLink)s));
         }
		   else if (s instanceof NetworkMapElement)
		   {
		      NetworkMapElement element = (NetworkMapElement)s;
            newSelection.add(mapPage.getElement(element.getId(), element.getClass()));
		   }
		}
      viewer.setSelection(new StructuredSelection(newSelection));
	}

	/**
	 * Replace current map page with new one
	 *
	 * @param page new map page
	 */
	protected void replaceMapPage(final NetworkMapPage page, Display display)
	{
      display.asyncExec(() -> {
         NetworkMapPage oldMapPage = mapPage;
         mapPage = page;
         refreshDciRequestList(oldMapPage, true);
         if (isGeographicalCanvas() && (geoViewer != null))
            geoViewer.setContent(mapPage);
         else
            viewer.setInput(mapPage);
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

		viewer.setLayoutAlgorithm(algorithm);

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
	}

	/**
	 * Create actions
	 */
	protected void createActions()
	{
      actionShowLinkDirection = new Action("Show link &direction", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setShowLinkDirection(!labelProvider.isShowLinkDirection());
            setChecked(labelProvider.isShowLinkDirection());
            updateObjectPositions();
            saveLayout();
            viewer.refresh();
            if (geoViewer != null)
            {
               geoViewer.setShowLinkDirection(labelProvider.isShowLinkDirection());
               geoViewer.redraw();
            }
         }
      };
      actionShowLinkDirection.setChecked(labelProvider.isShowLinkDirection());

      actionShowStatusBackground = new Action(i18n.tr("Show status &background"), Action.AS_CHECK_BOX) {
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

      actionShowStatusIcon = new Action(i18n.tr("Show status &icon"), Action.AS_CHECK_BOX) {
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

      actionShowStatusFrame = new Action(i18n.tr("Show status &frame"), Action.AS_CHECK_BOX) {
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

      actionTranslucentLabelBkgnd = new Action(i18n.tr("Translucent label background"), Action.AS_CHECK_BOX) {
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

      actionZoomIn = new Action(i18n.tr("Zoom &in"), SharedIcons.ZOOM_IN) {
			@Override
			public void run()
			{
            if (isGeographicalCanvas() && (geoViewer != null))
               geoViewer.zoomIn();
            else
               viewer.zoomIn();
			}
		};
      addKeyBinding("M1+=", actionZoomIn);

      actionZoomOut = new Action(i18n.tr("Zoom &out"), SharedIcons.ZOOM_OUT) {
			@Override
			public void run()
			{
            if (isGeographicalCanvas() && (geoViewer != null))
               geoViewer.zoomOut();
            else
               viewer.zoomOut();
			}
		};
      addKeyBinding("M1+-", actionZoomOut);

      actionZoomFit = new Action(i18n.tr("Zoom to &fit"), ResourceManager.getImageDescriptor("icons/netmap/fit.png")) {
         @Override
         public void run()
         {
            if (isGeographicalCanvas() && (geoViewer != null))
               geoViewer.zoomFit();
            else
               viewer.zoomFit();
         }
      };
      addKeyBinding("M1+F", actionZoomFit);

      actionZoomTo = viewer.createZoomActions();
      actionGeoZoomTo = createGeoZoomActions();

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

      actionEnableAutomaticLayout = new Action(i18n.tr("Enable &automatic layout"), Action.AS_CHECK_BOX) {
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

		actionLock = new Action(i18n.tr("Lock"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            lockObjectMove(actionLock.isChecked());
         }
      };
      actionLock.setImageDescriptor(ResourceManager.getImageDescriptor("icons/netmap/lock.png"));
      actionLock.setEnabled(true);

      actionToggleCanvasType = new Action(i18n.tr("Geographical view"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            toggleCanvasType(actionToggleCanvasType.isChecked());
         }
      };
      actionToggleCanvasType.setImageDescriptor(ResourceManager.getImageDescriptor("icons/worldmap.png"));
      actionToggleCanvasType.setChecked(isGeographicalCanvas());

		actionOpenDrillDownObject = new Action("Open drill-down object") {
			@Override
			public void run()
			{
				openDrillDownObject();
			}
		};
		actionOpenDrillDownObject.setEnabled(false);

      actionFiguresIcons = new Action(i18n.tr("&Icons"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
			   setObjectDisplayMode(MapObjectDisplayMode.ICON, true);
			}
		};

      actionFiguresSmallLabels = new Action(i18n.tr("&Small labels"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setObjectDisplayMode(MapObjectDisplayMode.SMALL_LABEL, true);
			}
		};

      actionFiguresLargeLabels = new Action(i18n.tr("&Large labels"), Action.AS_RADIO_BUTTON) {
			@Override
			public void run()
			{
            setObjectDisplayMode(MapObjectDisplayMode.LARGE_LABEL, true);
			}
		};

      actionFiguresStatusIcons = new Action(i18n.tr("Status icons"), Action.AS_RADIO_BUTTON) {
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

      actionShowGrid = new Action(i18n.tr("Show &grid"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				viewer.showGrid(actionShowGrid.isChecked());
			}
		};
      actionShowGrid.setImageDescriptor(ResourceManager.getImageDescriptor("icons/netmap/grid.png"));
      actionShowGrid.setChecked(viewer.isGridVisible());
      addKeyBinding("M1+G", actionShowGrid);

      actionShowSize = new Action(i18n.tr("Show map &size border"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            viewer.showSize(actionShowSize.isChecked());
         }
      };
      actionShowSize.setChecked(viewer.isSizeVisible());

      actionSnapToGrid = new Action(i18n.tr("S&nap to grid"), Action.AS_CHECK_BOX) {
			@Override
			public void run()
			{
				viewer.setSnapToGrid(actionSnapToGrid.isChecked());
			}
		};
      actionSnapToGrid.setImageDescriptor(ResourceManager.getImageDescriptor("icons/netmap/snap_to_grid.png"));
		actionSnapToGrid.setChecked(viewer.isSnapToGrid());

      actionAlignToGrid = new Action(i18n.tr("&Align to grid"), ResourceManager.getImageDescriptor("icons/netmap/align_to_grid.gif")) {
			@Override
			public void run()
			{
				viewer.alignToGrid(false);
				updateObjectPositions();
				saveLayout();
			}
		};
      addKeyBinding("M1+M3+G", actionAlignToGrid);

      actionShowObjectDetails = new Action(i18n.tr("Go to &object")) {
			@Override
			public void run()
			{
				showObjectDetails();
			}
		};

      actionCopyImage = new Action(i18n.tr("&Copy map image to clipboard"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            if (isGeographicalCanvas() && (geoViewer != null))
               geoViewer.copyToClipboard();
            else
               MapImageManipulationHelper.copyMapImageToClipboard(viewer);
         }
		};

      actionSaveImage = new Action(i18n.tr("Save map image to file")) {
         @Override
         public void run()
         {
            saveMapImageToFile(null);
         }
      };

      actionHideLinkLabels = new Action(i18n.tr("Hide link labels"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setConnectionLabelsVisible(!actionHideLinkLabels.isChecked());
            viewer.refresh(true);
            if (geoViewer != null)
            {
               geoViewer.setLinkLabelsVisible(!actionHideLinkLabels.isChecked());
               geoViewer.redraw();
            }
         }
      };
      actionHideLinkLabels.setImageDescriptor(ResourceManager.getImageDescriptor("icons/netmap/hide_link_labels.png"));

      actionHideLinks = new Action(i18n.tr("Hide links"), Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            labelProvider.setConnectionsVisible(!actionHideLinks.isChecked());
            viewer.refresh(true);
            if (geoViewer != null)
            {
               geoViewer.setLinksVisible(!actionHideLinks.isChecked());
               geoViewer.redraw();
            }
         }
      };
      actionHideLinks.setImageDescriptor(ResourceManager.getImageDescriptor("icons/netmap/hide_links.png"));

      actionSelectAllObjects = new Action(i18n.tr("Select &all objects")) {
         @Override
         public void run()
         {
            if (isGeographicalCanvas() && (geoViewer != null))
               geoViewer.selectAllObjects();
            else
               viewer.setSelection(new StructuredSelection(mapPage.getObjectElements()));
         }
      };
      addKeyBinding("M1+A", actionSelectAllObjects);

      actionHSpanIncrease = new Action(i18n.tr("Increase horizontal size"), ResourceManager.getImageDescriptor("icons/dashboard-control/h-span-increase.png")) {
         @Override
         public void run()
         {
            Dimension size = viewer.getMapSize();
            updateMapSize((int)Math.round(size.width * 1.25), size.height);
         }
      };

      actionHSpanDecrease = new Action(i18n.tr("Decrease horizontal size"), ResourceManager.getImageDescriptor("icons/dashboard-control/h-span-decrease.png")) {
         @Override
         public void run()
         {
            Dimension size = viewer.getMapSize();
            updateMapSize((int)Math.round(size.width * 0.75), size.height);
         }
      };

      actionHSpanFull = new Action(i18n.tr("&Fit to screen"), ResourceManager.getImageDescriptor("icons/dashboard-control/full-width.png")) {
         @Override
         public void run()
         {
            Rectangle visibleArea = viewer.getGraphControl().getClientArea();
            updateMapSize((int)(visibleArea.width * 1 / viewer.getZoom()), (int)(visibleArea.height * 1 / viewer.getZoom()));
         }
      };

      actionVSpanIncrease = new Action(i18n.tr("Increase vertical size"), ResourceManager.getImageDescriptor("icons/dashboard-control/v-span-increase.png")) {
         @Override
         public void run()
         {
            Dimension size = viewer.getMapSize();
            updateMapSize(size.width, (int)Math.round(size.height * 1.25));
         }
      };

      actionVSpanDecrease = new Action(i18n.tr("Decrease vertical size"), ResourceManager.getImageDescriptor("icons/dashboard-control/v-span-decrease.png")) {
         @Override
         public void run()
         {
            Dimension size = viewer.getMapSize();
            updateMapSize(size.width, (int)Math.round(size.height * 0.75));
         }
      };

      actionShowLineChart = new Action(i18n.tr("&Line chart"), ResourceManager.getImageDescriptor("icons/object-views/chart-line.png")) {
         @Override
         public void run()
         {
            showLinkLineChart();
         }
      };
      addKeyBinding("M1+M3+L", actionShowLineChart);


      actionFindObject = new Action("&Find object", Action.AS_CHECK_BOX) {
         @Override
         public void run()
         {
            activateFindObjectMode(actionFindObject.isChecked());
         }
      };
      addKeyBinding("M1+F2", actionFindObject);
      actionFindObject.setImageDescriptor(SharedIcons.FIND);
	}

	/**
	 * Activate find object mode
	 *
    * @param checked true to enable
    */
   protected void activateFindObjectMode(boolean checked)
   {
      if (checked)
      {
         searchBar.setVisible(true);
         previousButton.setEnabled(true);
         nextButton.setEnabled(true);
         queryEditor.setEnabled(true);
         queryEditor.setFocus();

         FormData fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(searchBar);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(100, 0);
         stackHolder.setLayoutData(fd);
      }
      else
      {
         previousButton.setEnabled(false);
         nextButton.setEnabled(false);
         queryEditor.setEnabled(false);
         searchBar.setVisible(false);

         FormData fd = new FormData();
         fd.left = new FormAttachment(0, 0);
         fd.top = new FormAttachment(0, 0);
         fd.right = new FormAttachment(100, 0);
         fd.bottom = new FormAttachment(100, 0);
         stackHolder.setLayoutData(fd);
      }
      contentParent.layout();
   }

   /**
    * Dispatch a find-object request to the active viewer (Zest in graph mode,
    * geo viewer in geographical mode). Handles the case where no match is
    * found by surfacing an information banner via the host's message area.
    */
   private void performFindObject(String text, boolean forward)
   {
      if ((text == null) || text.isEmpty())
         return;
      if (isGeographicalCanvas() && (geoViewer != null))
      {
         if (!geoViewer.findObjectByText(text, forward))
            addMessage(org.netxms.nxmc.base.widgets.MessageArea.INFORMATION, i18n.tr("No matching objects found on the map."));
      }
      else
      {
         viewer.findNodeByText(text, forward);
      }
   }

   /**
	 * Enable edit mode
	 *
	 * @param checked true to enable
	 */
   protected void lockObjectMove(boolean checked)
   {
      objectMoveLocked = checked;
      viewer.setDraggingEnabled(!objectMoveLocked);
   }

   /**
	 * Create "Layout" submenu
	 *
	 * @return
	 */
	protected IContributionItem createLayoutSubmenu()
	{
      MenuManager layout = new MenuManager(i18n.tr("&Layout"));
		if (allowManualLayout)
		{
			layout.add(actionEnableAutomaticLayout);
		}
		layout.add(new Separator());
		for(int i = 0; i < actionSetAlgorithm.length; i++)
			layout.add(actionSetAlgorithm[i]);
		return layout;
	}

	/**
	 * Create "Routing" submenu
	 *
	 * @return
	 */
	protected IContributionItem createRoutingSubmenu()
	{
      MenuManager submenu = new MenuManager(i18n.tr("&Routing"));
		for(int i = 0; i < actionSetRouter.length; i++)
			submenu.add(actionSetRouter[i]);
		return submenu;
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
	{
      MenuManager zoom = new MenuManager(i18n.tr("&Zoom"));
      Action[] zoomEntries = isGeographicalCanvas() ? actionGeoZoomTo : actionZoomTo;
      if (isGeographicalCanvas())
         syncGeoZoomChecked();
      for(int i = 0; i < zoomEntries.length; i++)
         zoom.add(zoomEntries[i]);

      if (!readOnly)
      {
         manager.add(actionLock);
         manager.add(new Separator());
      }

      MenuManager figureType = new MenuManager(i18n.tr("&Display objects as"));
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
      manager.add(actionHSpanIncrease);
      manager.add(actionHSpanDecrease);
      manager.add(actionHSpanFull);
      manager.add(actionVSpanIncrease);
      manager.add(actionVSpanDecrease);
      manager.add(new Separator());
      manager.add(actionAlignToGrid);
      manager.add(actionSnapToGrid);
      manager.add(actionShowGrid);
      manager.add(actionShowSize);
      manager.add(new Separator());
      manager.add(actionHideLinkLabels);
      manager.add(actionHideLinks);
      manager.add(new Separator());
      manager.add(actionCopyImage);
      manager.add(actionSaveImage);
      manager.add(new Separator());
      manager.add(actionSelectAllObjects);
      manager.add(actionFindObject);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
	{
      manager.add(actionFindObject);
      manager.add(new Separator());
      manager.add(actionZoomIn);
      manager.add(actionZoomOut);
      manager.add(actionZoomFit);
      manager.add(new Separator());
      manager.add(actionHSpanIncrease);
      manager.add(actionHSpanDecrease);
      manager.add(actionHSpanFull);
      manager.add(actionVSpanIncrease);
      manager.add(actionVSpanDecrease);
      manager.add(new Separator());
      manager.add(actionAlignToGrid);
      manager.add(actionSnapToGrid);
      manager.add(actionShowGrid);
      manager.add(new Separator());
      manager.add(actionHideLinkLabels);
      manager.add(actionHideLinks);
      manager.add(new Separator());
      if (!readOnly)
         manager.add(actionLock);
      // Canvas toggle is a per-view session switch, not a write operation,
      // so it shows for read-only views (ad-hoc predefined maps and ad-hoc
      // topology / VLAN views) too. The view always knows how to render
      // its placed objects as geo pins; views without a NetworkMap context
      // simply have no persisted canvas type to fall back on and start in
      // GRAPH each time.
      manager.add(actionToggleCanvasType);
      manager.add(actionCopyImage);
	}

	/**
	 * Create popup menu for map
	 */
	private void createContextMenu()
	{
		// Create menu manager.
      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null) {
         @Override
         protected void fillContextMenu()
         {
            int selectionType = analyzeSelection(currentSelection);
            switch(selectionType)
            {
               case SELECTION_EMPTY:
                  fillMapContextMenu(this);
                  break;
               case SELECTION_OBJECTS:
                  AbstractObject currentObject = (AbstractObject)currentSelection.getFirstElement();
                  fillObjectContextMenu(this, currentObject.isPartialObject());
                  if (!currentObject.isPartialObject())
                  {
                     add(new Separator());
                     super.fillContextMenu();
                  }
                  break;
               case SELECTION_ELEMENTS:
                  fillElementContextMenu(this);
                  break;
               case SELECTION_LINKS:
                  fillLinkContextMenu(this);
                  break;
            }
         }
      };

		// Create menu.
		Menu menu = menuMgr.createContextMenu(viewer.getControl());
		viewer.getControl().setMenu(menu);
	}

   /**
    * Forward selection changes from the geographical viewer into this view's
    * {@code currentSelection} (and to view-level selection listeners), so the
    * shared context-menu logic — which keys off {@code currentSelection} —
    * works on the geo canvas the same way it does on the Zest canvas.
    */
   private void wireGeoViewerSelection()
   {
      geoViewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent e)
         {
            currentSelection = transformSelection(e.getSelection());

            NetworkMapLink editLink = null;
            if (currentSelection.size() == 1)
            {
               Object first = currentSelection.getFirstElement();
               if ((first instanceof NetworkMapLink)
                     && (((NetworkMapLink)first).getRouting() == NetworkMapLink.ROUTING_BENDPOINTS))
                  editLink = (NetworkMapLink)first;
            }
            geoViewer.setEditedLink(editLink);

            if (selectionListeners.isEmpty())
               return;
            SelectionChangedEvent forwarded = new SelectionChangedEvent(AbstractNetworkMapView.this, currentSelection);
            for(ISelectionChangedListener l : selectionListeners)
               l.selectionChanged(forwarded);
         }
      });
   }

   /**
    * Attach a context menu to the geographical viewer using the same
    * {@code ObjectContextMenuManager} as the Zest canvas. Right-click on a
    * pin shows the object menu; right-click on free space falls through to
    * {@link #fillMapContextMenu(IMenuManager)}.
    */
   private void attachGeoContextMenu()
   {
      MenuManager menuMgr = new ObjectContextMenuManager(this, this, null) {
         @Override
         protected void fillContextMenu()
         {
            int selectionType = analyzeSelection(currentSelection);
            switch(selectionType)
            {
               case SELECTION_EMPTY:
                  fillMapContextMenu(this);
                  break;
               case SELECTION_OBJECTS:
                  AbstractObject currentObject = (AbstractObject)currentSelection.getFirstElement();
                  fillObjectContextMenu(this, currentObject.isPartialObject());
                  if (!currentObject.isPartialObject())
                  {
                     add(new Separator());
                     super.fillContextMenu();
                  }
                  break;
               case SELECTION_ELEMENTS:
                  fillElementContextMenu(this);
                  break;
               case SELECTION_LINKS:
                  fillLinkContextMenu(this);
                  break;
            }
         }
      };
      Menu menu = menuMgr.createContextMenu(geoViewer);
      geoViewer.setMenu(menu);
   }

   /**
    * Fill context menu for object.
    *
    * @param manager menu manager
    */
   protected void fillObjectContextMenu(IMenuManager manager, boolean isPartial)
   {
      manager.add(actionOpenDrillDownObject);
      if (currentSelection.size() == 1 && !isPartial)
         manager.add(actionShowObjectDetails);
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
      if ((currentSelection.size() == 1) && selectedLinkHasData())
         manager.add(actionShowLineChart);
	}

	/**
	 * Fill context menu for map view
	 *
	 * @param manager Menu manager
	 */
	protected void fillMapContextMenu(IMenuManager manager)
	{
      MenuManager zoom = new MenuManager(i18n.tr("&Zoom"));
      Action[] zoomEntries = isGeographicalCanvas() ? actionGeoZoomTo : actionZoomTo;
      if (isGeographicalCanvas())
         syncGeoZoomChecked();
      for(int i = 0; i < zoomEntries.length; i++)
         zoom.add(zoomEntries[i]);

      MenuManager figureType = new MenuManager(i18n.tr("&Display objects as"));
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
      manager.add(actionShowSize);
      manager.add(new Separator());
      manager.add(actionHideLinkLabels);
      manager.add(actionHideLinks);
      manager.add(new Separator());
      manager.add(actionSelectAllObjects);
      manager.add(actionFindObject);
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
   protected void processObjectUpdateNotification(final AbstractObject object)
	{
      if (mapPage == null) //For object maps - exist but not created till active
         return;

		NetworkMapObject element = mapPage.findObjectElement(object.getObjectId());
		if (element != null)
			viewer.refresh(element, true);

		List<NetworkMapLink> links = mapPage.findLinksWithStatusObject(object.getObjectId());
		if (links != null)
		{
			for(NetworkMapLink l : links)
				viewer.refresh(l);
		}
	}

	/**
	 * Called when map layout has to be saved. Object positions already updated when this method is called. Default implementation
	 * does nothing.
	 */
	protected void saveLayout()
	{
	}

   /**
    * @see org.netxms.nxmc.base.views.View#setFocus()
    */
	@Override
	public void setFocus()
	{
	   if (!viewer.getControl().isDisposed())
	      viewer.getControl().setFocus();
	}

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
	@Override
	public void dispose()
	{
		if (sessionListener != null)
			session.removeListener(sessionListener);

      if (mapPage != null)
         dciValueProvider.removeDcis(mapPage);

		super.dispose();
	}

	void fullDciCacheRefresh()
	{
      if (mapPage != null)
      {
         dciValueProvider.removeDcis(mapPage);
         refreshDciRequestList(mapPage, false);
      }
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#addSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
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
				AbstractObject object = session.findObjectById(((NetworkMapObject)element).getObjectId(), true);
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
			else if (element instanceof AbstractObject)
			{
				// Geographical viewer puts AbstractObject directly into its
				// selection (pin click → object selection), so pass these
				// through here too. Without this, transformSelection would
				// drop the object and the context menu would fall back to
				// the empty/map menu instead of the object menu.
				objects.add(element);
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

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
	@Override
	public ISelection getSelection()
	{
		return currentSelection;
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
	@Override
	public void removeSelectionChangedListener(ISelectionChangedListener listener)
	{
		selectionListeners.remove(listener);
	}

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
	@Override
	public void setSelection(ISelection selection)
	{
	}

   /**
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
      if ((currentSelection == null) || currentSelection.isEmpty())
			return;

      Object currentObject = currentSelection.getFirstElement();

		long drillDownObjectId = 0;
		if (currentObject instanceof AbstractObject)
			drillDownObjectId = (currentObject instanceof NetworkMap) ? ((AbstractObject)currentObject).getObjectId() : ((AbstractObject)currentObject).getDrillDownObjectId();
		else if (currentObject instanceof NetworkMapTextBox)
		   drillDownObjectId = ((NetworkMapTextBox)currentObject).getDrillDownObjectId();

		if (drillDownObjectId != 0)
      {
		   doubleClickHandlers.openDrillDownObjectView(drillDownObjectId, getObject());
      }
	}

   /**
    * Check if currently selected link has DCI data.
    *
    * @return true if currently selected link has DCI data
    */
   protected boolean selectedLinkHasData()
   {
      Object o = currentSelection.getFirstElement();
      return (o != null) && (o instanceof NetworkMapLink) && ((NetworkMapLink)o).hasDciData();
   }

	/**
    * Show line chart for DCIs configured on network map link
    */
	private void showLinkLineChart()
	{
      final NetworkMapLink link = (NetworkMapLink)currentSelection.getFirstElement();
	   if (!link.hasDciData())
	      return;

      new Job(i18n.tr("Get DCI configuration"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            DciValue[] values = session.getLastValues(link.getDciAsList());

            final List<ChartDciConfig> items = new ArrayList<ChartDciConfig>(values.length);
            for(DciValue dci : values)
               items.add(new ChartDciConfig(dci));

            runInUIThread(() -> {
               AbstractObject object = getObject();
               openView(new HistoricalGraphView(object, items, null, getObjectId()));
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get DCI configuration");
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
		int alg = (routingAlgorithm == NetworkMapLink.ROUTING_MANHATTAN)
				? NetworkMapLink.ROUTING_MANHATTAN : NetworkMapLink.ROUTING_DIRECT;
		this.routingAlgorithm = alg;

		// Keep the Zest router in sync; cheap and safe to do even when the
		// graph canvas is currently hidden behind the geo viewer.
		viewer.getGraphControl().setRouter((alg == NetworkMapLink.ROUTING_MANHATTAN) ? new ManhattanConnectionRouter() : null);

		// Geo viewer projects links itself, so push the default routing in
		// and trigger a repaint of the live tile canvas.
		if (geoViewer != null)
		{
			geoViewer.setDefaultLinkRouting(alg);
			if (isGeographicalCanvas())
				geoViewer.redraw();
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
      MainWindow.switchToObject(object.getObjectId(), 0);
	}

   /**
    * Goes thought all links and tries to add to request list required DCIs.
    */
   protected void refreshDciRequestList(NetworkMapPage oldMapPage, boolean removeOldMapDci)
   {
      Collection<NetworkMapLink> linkList = mapPage.getLinks();
      removeOldMapDci = (oldMapPage != null && oldMapPage.getMapObjectId() != 0 && removeOldMapDci);
      for(NetworkMapLink item : linkList)
      {
         if (item.hasDciData())
         {
            for(MapDataSource value : item.getDciAsList())
            {
               if (value.getType() == MapDataSource.ITEM)
               {
                  dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, removeOldMapDci ? 2 : 1);
               }
               else
               {
                  dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, removeOldMapDci ? 2 : 1);
               }
            }
         }
      }
      Collection<NetworkMapElement> mapElements = mapPage.getElements();
      for(NetworkMapElement element : mapElements)
      {
         if (element instanceof NetworkMapDCIContainer)
         {
            NetworkMapDCIContainer item = (NetworkMapDCIContainer)element;
            if (item.hasDciData())
            {
               for(MapDataSource value : item.getObjectDCIArray())
               {
                  if (value.getType() == MapDataSource.ITEM)
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, removeOldMapDci ? 2 : 1);
                  }
                  else
                  {
                     dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, removeOldMapDci ? 2 : 1);
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
               dciValueProvider.addDci(value.getNodeId(), value.getDciId(), mapPage, removeOldMapDci ? 2 : 1);
            }
            else
            {
               dciValueProvider.addDci(value.getNodeId(), value.getDciId(), value.getColumn(), value.getInstance(), mapPage, removeOldMapDci ? 2 : 1);
            }
         }
      }
      if (removeOldMapDci)
         dciValueProvider.removeDcis(oldMapPage);
   }

   /**
    * Switch the active canvas locally. The toolbar toggle is a temporary view
    * switch — it lives only for the current view instance and is never
    * persisted (neither on the server nor in PreferenceStore). Closing and
    * reopening the map (or navigating to a different map and back) resets to
    * the map's saved canvas type. To make the change permanent the user
    * edits Canvas type on the Map Background property page.
    */
   private void toggleCanvasType(boolean geographical)
   {
      // Ad-hoc topology / VLAN views don't bind a NetworkMap — they have no
      // persisted canvas type and default to GRAPH, so the override stays
      // engaged whenever the user wants GEOGRAPHICAL.
      AbstractObject o = getObject();
      boolean mapDefault = (o instanceof NetworkMap) && (((NetworkMap)o).getCanvasType() == MapCanvasType.GEOGRAPHICAL);
      canvasTypeOverride = (geographical == mapDefault) ? null : Boolean.valueOf(geographical);
      ensureCorrectCanvas();
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
    * Save map image to file. Captures the geographical viewer when in geo
    * mode and the Zest viewer otherwise.
    */
   public boolean saveMapImageToFile(String fileName)
   {
      if (isGeographicalCanvas() && (geoViewer != null))
         return geoViewer.saveAsImage(getWindow().getShell(), logger, fileName);
      return MapImageManipulationHelper.saveMapImageToFile(getWindow().getShell(), viewer, logger, fileName);
   }

   /**
    * Method called on element move
    *
    * @param element moved element
    */
   protected void onElementMove(NetworkMapElement element)
   {
   }

   /**
    * Method called on link change
    *
    * @param link changed link
    */
   public void onLinkChange(NetworkMapLink link)
   {
   }

   /**
    * Save preferences like zoom level and find mode
    */
   private void savePreferences()
   {
      final PreferenceStore settings = PreferenceStore.getInstance();
      settings.set(getBaseId() + ".find", actionFindObject.isChecked());
      settings.set(getBaseId() + ".zoom", viewer.getZoom());
   }

   /**
    * Load preferences like zoom level and find mode
    */
   private void loadPreferences()
   {
      final PreferenceStore settings = PreferenceStore.getInstance();
      viewer.zoomTo(settings.getAsDouble(getBaseId() + ".zoom", 1.0));
      boolean findMode = settings.getAsBoolean(getBaseId() + ".find", false);
      actionFindObject.setChecked(findMode);
      activateFindObjectMode(findMode);
   }
}
