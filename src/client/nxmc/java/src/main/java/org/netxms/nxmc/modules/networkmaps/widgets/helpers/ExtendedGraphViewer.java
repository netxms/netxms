/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets.helpers;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.draw2d.AbstractBorder;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.FreeformLayer;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.IFigure;
import org.eclipse.draw2d.Layer;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.SWTEventDispatcher;
import org.eclipse.draw2d.SWTGraphics;
import org.eclipse.draw2d.ScalableFigure;
import org.eclipse.draw2d.ToolTipHelper;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.gef4.zest.core.viewers.GraphViewer;
import org.eclipse.gef4.zest.core.viewers.internal.GraphModelEntityRelationshipFactory;
import org.eclipse.gef4.zest.core.viewers.internal.IStylingGraphModelFactory;
import org.eclipse.gef4.zest.core.widgets.GraphConnection;
import org.eclipse.gef4.zest.core.widgets.IDecorationFigure;
import org.eclipse.gef4.zest.core.widgets.IDecorationLayer;
import org.eclipse.gef4.zest.core.widgets.ZestStyles;
import org.eclipse.gef4.zest.core.widgets.custom.CGraphNode;
import org.eclipse.gef4.zest.core.widgets.zooming.ZoomListener;
import org.eclipse.gef4.zest.core.widgets.zooming.ZoomManager;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.util.IPropertyChangeListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.netxms.base.GeoLocation;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.networkmaps.algorithms.ManualLayout;
import org.netxms.nxmc.modules.worldmap.tools.MapLoader;
import org.netxms.nxmc.modules.worldmap.tools.Tile;
import org.netxms.nxmc.modules.worldmap.tools.TileSet;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.MapInputListener;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Workaround for bug #244496
 * (https://bugs.eclipse.org/bugs/show_bug.cgi?id=244496)
 */
public class ExtendedGraphViewer extends GraphViewer
{
   public static final double[] zoomLevels = { 0.10, 0.25, 0.50, 0.75, 1.00, 1.25, 1.50, 1.75, 2.00 };

   /** Preference: plain mouse wheel (without Ctrl/Cmd) zooms the map instead of scrolling it */
   public static final String PREF_PLAIN_WHEEL_ZOOM = "NetMap.PlainWheelZoom";

   private I18n i18n = LocalizationHelper.getI18n(ExtendedGraphViewer.class);
   private View view;
   private FigureChangeCallback moveCallback;
   private ExtendedSWTEventDispatcher eventDispatcher;
	private BackgroundFigure backgroundFigure;
	private Image backgroundImage = null;
   private TileSet backgroundTiles = null;
	private GeoLocation backgroundLocation = null;
	private int backgroundZoom;
	private MapLoader mapLoader;
	private Layer backgroundLayer;
	private Layer decorationLayer;
	private Layer indicatorLayer;
   private Layer controlLayer;
	private int crosshairX;
	private int crosshairY;
	private Crosshair crosshairFigure;
	private GridFigure gridFigure;
	private int gridSize = 96;
	private boolean snapToGrid = false;
	private MouseListener snapToGridListener;
	private List<NetworkMapElement> mapDecorations;
	private Set<NetworkMapElement> selectedDecorations = new HashSet<NetworkMapElement>();
	private Map<Long, DecorationLayerAbstractFigure> decorationFigures = new HashMap<Long, DecorationLayerAbstractFigure>();
	private ColorCache colors;
	private Image iconBack;
	private OverlayButton backButton = null;
	private boolean draggingEnabled = true;
   private boolean centeredBackground = false;
   private boolean fitBackground = false;
   private boolean dragStarted = false;
   private boolean objectDragCaptured = false; // left button pressed on an object with movement unlocked, until its release
   private boolean showMapSize = false;
   private volatile int blockRefresh;
   private Point rightClickLocation = null;
   private boolean backgroundDragActive = false;
   private boolean backgroundDragged = false;
   private Point backgroundDragStart = null; // in absolute screen coordinates
   private org.eclipse.draw2d.geometry.Point backgroundDragStartViewLocation = null;
   private org.eclipse.draw2d.geometry.Point objectDragStart = null; // press location of an object drag, object layer coordinates
   private Map<CGraphNode, org.eclipse.draw2d.geometry.Point> objectDragOrigins = new HashMap<>(); // dragged nodes, locations before the drag
   private int dragPanGesture = 0; // mouse drag the ownership below was decided for
   private boolean dragPanOnObject = false; // that drag started on something that handles the drag itself (see isPanStart())
   private Point lastPress = null; // location of the last left button press, until a mouse drag pan decides from it
   private boolean lastPressPans = false; // a drag starting at lastPress pans the view
   private boolean embedded = false; // part of a larger scrollable page, such as a dashboard
   private int figureZoomIndex = -1; // zoom level bucket the figures were last rebuilt for on a zoom change
   private boolean viewStateUpdateScheduled = false;
   private boolean boxSelectionActive = false;
   private boolean boxSelectionAdditive = false; // modifier state latched at press time (RWT drag events carry no modifiers)
   private org.eclipse.draw2d.geometry.Point boxSelectionStart = null; // in map coordinates
   private BoxSelectionFigure boxSelectionFigure = null;

	/**
	 * @param composite
	 * @param style
	 */
   public ExtendedGraphViewer(Composite composite, int style, View view, FigureChangeCallback moveCallback)
	{
      super(composite, style | ZestStyles.GESTURES_DISABLED);

      this.view = view;
      this.moveCallback = moveCallback;

      eventDispatcher = new ExtendedSWTEventDispatcher();
      graph.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_ARROW));

		colors = new ColorCache(graph);

		final ScalableFigure rootLayer = graph.getRootLayer();
      rootLayer.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_ARROW));

      iconBack = ResourceManager.getImage("icons/netmap/back.png");

		mapLoader = new MapLoader(composite.getDisplay());
		graph.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				mapLoader.dispose();
				iconBack.dispose();
            if (backgroundTiles != null)
               backgroundTiles.dispose();
            if ((backgroundLocation != null) && (backgroundImage != null))
               backgroundImage.dispose();
			}
		});

      backgroundLayer = new FreeformLayer();
      rootLayer.add(backgroundLayer, null, 0);
      backgroundFigure = new BackgroundFigure();
      backgroundFigure.setSize(-1, -1);
      backgroundLayer.add(backgroundFigure);
      graph.getViewport().setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_ARROW));
      backgroundLayer.addMouseMotionListener(new MouseMotionListener() {
         @Override
         public void mouseMoved(MouseEvent me)
         {
         }

         @Override
         public void mouseHover(MouseEvent me)
         {
         }

         @Override
         public void mouseExited(MouseEvent me)
         {
         }

         @Override
         public void mouseEntered(MouseEvent me)
         {
         }

         @Override
         public void mouseDragged(MouseEvent me)
         {
            if (boxSelectionActive)
            {
               updateBoxSelection(me.x, me.y);
               return;
            }

            if (!backgroundDragActive)
               return;

            Point currentLocation = graph.getDisplay().getCursorLocation();
            int dx = backgroundDragStart.x - currentLocation.x; // delta in screen coordinates
            int dy = backgroundDragStart.y - currentLocation.y;
            if ((dx == 0) && (dy == 0) && !backgroundDragged)
               return;

            backgroundDragged = true;
            setViewLocationClamped(backgroundDragStartViewLocation.getCopy().translate(dx, dy));
         }
      });
      backgroundLayer.addMouseListener(new MouseListener() {
         @Override
         public void mouseReleased(MouseEvent me)
         {
            if (me.button != 1)
               return;

            if (boxSelectionActive)
            {
               finishBoxSelection(me.x, me.y);
               return;
            }

            if (!backgroundDragged)
            {
               setSelection(null);
            }
            backgroundDragActive = false;
            backgroundDragStart = null;
            backgroundDragStartViewLocation = null;
            graph.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_ARROW));
         }

         @Override
         public void mousePressed(MouseEvent me)
         {
            if (me.button != 1)
               return;

            // Shift+drag on background selects objects within rectangle instead of panning
            if ((me.getState() & SWT.SHIFT) != 0)
            {
               boxSelectionActive = true;
               boxSelectionAdditive = (me.getState() & SWT.MOD1) != 0;
               boxSelectionStart = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
               graph.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_CROSS));
               // Consuming press event makes event dispatcher capture mouse on this layer, so drag events are
               // delivered here even when cursor passes over nodes or links (otherwise object move listener on
               // node layer would take over the drag when object move is unlocked)
               me.consume();
               return;
            }

            backgroundDragActive = true;
            backgroundDragged = false;
            backgroundDragStart = graph.getDisplay().getCursorLocation();
            backgroundDragStartViewLocation = graph.getViewport().getViewLocation().getCopy();
            graph.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_SIZEALL));
         }

         @Override
         public void mouseDoubleClicked(MouseEvent me)
         {
         }
      });

		decorationLayer = new FreeformLayer();
		decorationLayer.setOpaque(false);
		rootLayer.add(decorationLayer, null, 1);

		indicatorLayer = new FreeformLayer();
		rootLayer.add(indicatorLayer, null, 2);

      controlLayer = new FreeformLayer();
      rootLayer.add(controlLayer, null);

		getZoomManager().setZoomLevels(zoomLevels);
		getZoomManager().addZoomListener(new ZoomListener() {
         @Override
         public void zoomChanged(double zoom)
         {
            scheduleViewStateUpdate();

            // The zoom manager scales the figures itself. They depend on the zoom only through the label fonts, which change per
            // zoom level bucket, so the graph is rebuilt only when the zoom moves into another bucket - not on every step of a
            // continuous zoom (pinch, mouse wheel), which would rebuild every figure once per animation frame.
            int zoomIndex = getCurrentZoomIndex();
            if (zoomIndex == figureZoomIndex)
               return;
            figureZoomIndex = zoomIndex;
            ExtendedGraphViewer.this.refresh(true);
         }
      });

      // Ctrl/Cmd + mouse wheel zooms to the cursor, and so does the plain wheel if the user has chosen that (see
      // isPlainWheelZoom()); otherwise the plain wheel keeps the canvas's default scroll.
      // Registered via untyped addListener so it compiles under RWT (which has no MouseWheelListener);
      // RWT does not generate wheel events, so this handler is inert on the web client.
      graph.addListener(SWT.MouseWheel, new Listener() {
         @Override
         public void handleEvent(Event event)
         {
            if (((event.stateMask & SWT.MOD1) == 0) && !isPlainWheelZoom())
               return; // plain wheel scrolls the map
            event.doit = false; // suppress default scroll while zooming
            if (event.count != 0)
               zoomToCursor(event.count > 0, event.x, event.y);
         }
      });

      // Viewport position, extent and size changes all show up in its range models
      graph.getViewport().getHorizontalRangeModel().addPropertyChangeListener((e) -> scheduleViewStateUpdate());
      graph.getViewport().getVerticalRangeModel().addPropertyChangeListener((e) -> scheduleViewStateUpdate());

      // Continuous pointer input (pinch, touch drag, mouse wheel, mouse drag). On the desktop client this is a no-op, because SWT
      // delivers mouse move and mouse wheel to the canvas directly. In the web client RAP delivers neither, so gestures are
      // classified and coalesced in the browser, which shows a touch gesture by transforming the last frame it was sent and sends
      // it here only as often as the server's redraws can keep up (see mapinput.js).
      WidgetHelper.attachMapInputListener(graph, new MapInputListener() {
         @Override
         public void onZoom(double factor, int x, int y, int dx, int dy)
         {
            double currentZoom = getZoom();
            org.eclipse.draw2d.geometry.Point viewLocation = graph.getViewport().getViewLocation().getCopy();

            zoomTo(currentZoom * factor); // ZoomManager clamps to the range derived from zoomLevels

            // Keep the focal point of the gesture stationary, then apply the pan, and clamp only the result. The applied factor
            // is recomputed from the zoom that was actually accepted, so this stays correct when the zoom was clamped.
            double appliedFactor = getZoom() / currentZoom;
            int nx = (int)Math.round((viewLocation.x + x) * appliedFactor) - x - dx;
            int ny = (int)Math.round((viewLocation.y + y) * appliedFactor) - y - dy;
            setViewLocationClamped(new org.eclipse.draw2d.geometry.Point(nx, ny));
         }

         @Override
         public void onPan(int dx, int dy)
         {
            // Moving the pointer right moves the content right, so the view location moves the opposite way.
            setViewLocationClamped(graph.getViewport().getViewLocation().getCopy().translate(-dx, -dy));

            // The Draw2D drag tracker may also be running for this same mouse gesture; it computes its delta independently from
            // the cursor location and would double the movement. This path wins.
            backgroundDragActive = false;
         }

         @Override
         public void onDragPan(int dx, int dy, int gesture, int startX, int startY)
         {
            // Decided once per drag, from the point where it started: as seen when the press arrived, because by the first pan the
            // drag may have moved what was there (a resize handle moves with it). If the press has not arrived - RAP drops a press
            // that comes while a response is being applied, and a press can also reach the server after the drag's first pan -
            // nothing has moved yet, and the point is checked now.
            if (gesture != dragPanGesture)
            {
               dragPanGesture = gesture;
               boolean pans = ((lastPress != null) && (Math.abs(lastPress.x - startX) <= 2) && (Math.abs(lastPress.y - startY) <= 2)) ?
                     lastPressPans : isPanStart(startX, startY);
               lastPress = null;
               dragPanOnObject = !pans;
            }
            if (!dragPanOnObject)
               onPan(dx, dy);
         }
      });

      // Draw2D has handled the press by the time this runs, but nothing has moved yet
      graph.addListener(SWT.MouseDown, (e) -> {
         if (e.button == 1)
         {
            lastPress = new Point(e.x, e.y);
            lastPressPans = isPanStart(e.x, e.y);
         }
      });

      updateMapInputOptions();
      final PreferenceStore preferenceStore = PreferenceStore.getInstance();
      final IPropertyChangeListener preferenceListener = (e) -> {
         if (e.getProperty().equals(PREF_PLAIN_WHEEL_ZOOM) && !graph.isDisposed())
            updateMapInputOptions();
      };
      preferenceStore.addPropertyChangeListener(preferenceListener);
      graph.addDisposeListener((e) -> preferenceStore.removePropertyChangeListener(preferenceListener));

      graph.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				clearDecorationSelection(false);
			}
		});

		MouseListener backgroundMouseListener = new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
			}

			@Override
			public void mousePressed(MouseEvent me)
			{
			   if (me.button == 3)
            {
               Point screenLocation = graph.getDisplay().getCursorLocation();
               Point controlLocation = graph.toControl(screenLocation);
               int mapX = (int)((controlLocation.x + graph.getHorizontalBar().getSelection()) / getZoom());
               int mapY = (int)((controlLocation.y + graph.getVerticalBar().getSelection()) / getZoom());
               rightClickLocation = new Point(mapX, mapY);
            }
				org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
				graph.getRootLayer().translateToRelative(mousePoint);
				IFigure figureUnderMouse = graph.getFigureAt(mousePoint.x, mousePoint.y);
				if ((figureUnderMouse == null) ||
				    !(figureUnderMouse instanceof DecorationLayerAbstractFigure))
				{
					if ((me.getState() & SWT.MOD1) == 0)
					{
						clearDecorationSelection(true);
					}
					return;
				}
			}

			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
			}
		};
      graph.getLightweightSystem().getRootFigure().addMouseListener(backgroundMouseListener);
      graph.getZestRootLayer().addMouseListener(backgroundMouseListener);

		snapToGridListener = new MouseListener() {
			@Override
			public void mouseReleased(MouseEvent me)
			{
            org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
            graph.getRootLayer().translateToParent(mousePoint);  //Scales mouse coordinates to zoomed map coordinates
				if (snapToGrid && (graph.getRootLayer().findFigureAt(mousePoint.x, mousePoint.y) != null))
					alignToGrid(true);
			}

			@Override
			public void mousePressed(MouseEvent me)
			{
				clearMovedState();
			}

			@Override
			public void mouseDoubleClicked(MouseEvent me)
			{
			}
		};

		if (moveCallback != null )
		{
		   // An object drag captures the mouse on the object layer from the press to the release. Otherwise Draw2D delivers each drag
		   // event and the release to whatever figure is under the cursor, and in the web client, which sees the cursor only every
		   // 200 ms during a drag (RAPDragTracker), the cursor is often past the object: a release off the object would not save the
		   // move, and on a map without a fixed size would not finish the drag at all.
		   // The listeners below also move the objects, instead of Zest: Zest adds the grab offset scaled by the zoom to an unscaled
		   // cursor location, which shifts a grabbed object by (zoom - 1) times the offset and spreads multiple selected objects apart.
		   graph.setDraggingEnabled(false);
		   MouseListener moveMouseListener = new MouseListener() {
            @Override
            public void mouseReleased(MouseEvent me)
            {
               if (!objectDragCaptured)
                  return;
               objectDragCaptured = false;
               if (dragStarted)
               {
                  moveDraggedObjects(me.x, me.y); // where the button was released, not where the last drag event saw the cursor
                  finishObjectDrag();
               }
            }

            @Override
            public void mousePressed(MouseEvent me)
            {
               // The previous drag's release never arrived (the mouse is still captured, so this press came here wherever it is)
               if (dragStarted)
                  finishObjectDrag();
               objectDragCaptured = false;

               if ((me.button != 1) || !draggingEnabled)
                  return;
               org.eclipse.draw2d.geometry.Point p = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
               graph.getZestRootLayer().translateToAbsolute(p);
               if (!isObjectAt(p.x, p.y))
                  return;
               objectDragCaptured = true;
               objectDragStart = new org.eclipse.draw2d.geometry.Point(me.x, me.y);
               if (snapToGrid)
                  clearMovedState(); // snap to grid listener does not see this press
               me.consume(); // makes the event dispatcher capture the mouse on this layer until the release
            }

            @Override
            public void mouseDoubleClicked(MouseEvent me)
            {
            }
         };
         MouseMotionListener moveMouseMoutionListener = new MouseMotionListener() {

            @Override
            public void mouseMoved(MouseEvent me)
            {
            }

            @Override
            public void mouseHover(MouseEvent me)
            {
            }

            @Override
            public void mouseExited(MouseEvent me)
            {
            }

            @Override
            public void mouseEntered(MouseEvent me)
            {
            }

            @Override
            public void mouseDragged(MouseEvent me)
            {
               if (!objectDragCaptured)
                  return;

               if (!dragStarted)
               {
                  dragStarted = true;
                  blockRefresh();
                  // Taken here, not at the press: a map update in between replaces the nodes (and selects the new ones)
                  for(Object o : graph.getSelection())
                  {
                     if (o instanceof CGraphNode)
                        objectDragOrigins.put((CGraphNode)o, ((CGraphNode)o).getLocation().getCopy());
                  }
               }
               moveDraggedObjects(me.x, me.y);
            }
         };
         graph.getZestRootLayer().addMouseListener(moveMouseListener);
         graph.getZestRootLayer().addMouseMotionListener(moveMouseMoutionListener);
		}
	}

   /**
    * Get box selection area spanning from drag start point to given point (both in map coordinates).
    *
    * @param x current X coordinate
    * @param y current Y coordinate
    * @return normalized selection rectangle
    */
   private org.eclipse.draw2d.geometry.Rectangle getBoxSelectionArea(int x, int y)
   {
      return new org.eclipse.draw2d.geometry.Rectangle(boxSelectionStart, new org.eclipse.draw2d.geometry.Point(x, y));
   }

   /**
    * Update box selection rectangle figure during drag.
    *
    * @param x current X coordinate in map coordinates
    * @param y current Y coordinate in map coordinates
    */
   private void updateBoxSelection(int x, int y)
   {
      if (boxSelectionFigure == null)
      {
         boxSelectionFigure = new BoxSelectionFigure();
         controlLayer.add(boxSelectionFigure);
      }
      boxSelectionFigure.setBounds(getBoxSelectionArea(x, y));
   }

   /**
    * Finish box selection: remove rectangle figure and select all map elements fully enclosed by it.
    *
    * @param x release X coordinate in map coordinates
    * @param y release Y coordinate in map coordinates
    */
   private void finishBoxSelection(int x, int y)
   {
      org.eclipse.draw2d.geometry.Rectangle area = getBoxSelectionArea(x, y);
      if (boxSelectionFigure != null)
      {
         controlLayer.remove(boxSelectionFigure);
         boxSelectionFigure = null;
      }
      boxSelectionActive = false;
      boxSelectionStart = null;
      graph.setCursor(graph.getDisplay().getSystemCursor(SWT.CURSOR_ARROW));

      List<Object> selection = new ArrayList<Object>();
      if (boxSelectionAdditive)
         selection.addAll(((IStructuredSelection)getSelection()).toList());

      // Links are intentionally not selectable by box: zest viewer maps selection into GraphNode[] and
      // would fail on connection items
      for(Object o : graph.getNodes())
      {
         if (!(o instanceof CGraphNode))
            continue;
         CGraphNode n = (CGraphNode)o;
         Object element = n.getData();
         if ((element != null) && !selection.contains(element) &&
             area.contains(new org.eclipse.draw2d.geometry.Rectangle(n.getLocation(), n.getSize())))
            selection.add(element);
      }
      for(DecorationLayerAbstractFigure f : decorationFigures.values())
      {
         NetworkMapElement element = f.getMapElement();
         if (!selection.contains(element) && area.contains(f.getBounds()))
            selection.add(element);
      }

      setSelection(new StructuredSelection(selection));
   }

   /**
    * Block map from refresh
    */
   public void blockRefresh()
   {
     blockRefresh++;
   }

	/**
	 * Unblock map from refresh
	 */
	public void unblockRefresh()
   {
	   if (blockRefresh > 0) // resetRefreshBlock() may have run since the matching blockRefresh()
	      blockRefresh--;
   }


	/**
	 * Reset refresh bloack
	 */
   public void resetRefreshBlock()
   {
      blockRefresh = 0;
   }

   /**
	 * Update decoration figure
	 *
	 * @param d map decoration element
	 */
	public void updateDecorationFigure(NetworkMapElement d)
	{
		DecorationLayerAbstractFigure figure = decorationFigures.get(d.getId());
		if (figure != null)
			figure.refresh();
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.GraphViewer#inputChanged(java.lang.Object, java.lang.Object)
    */
	@Override
	protected void inputChanged(Object input, Object oldInput)
	{
		boolean dynamicLayoutEnabled = graph.isDynamicLayoutEnabled();
		graph.setDynamicLayout(false);
		super.inputChanged(input, oldInput);
		graph.setDynamicLayout(dynamicLayoutEnabled);

		decorationLayer.removeAll();
		decorationFigures.clear();
		if ((getContentProvider() instanceof MapContentProvider) && (getLabelProvider() instanceof MapLabelProvider))
		{
			mapDecorations = ((MapContentProvider)getContentProvider()).getDecorations(input);
			if (mapDecorations != null)
			{
				MapLabelProvider lp = (MapLabelProvider)getLabelProvider();
				for(NetworkMapElement d : mapDecorations)
				{
			      DecorationLayerAbstractFigure figure = (DecorationLayerAbstractFigure)lp.getFigure(d);
					figure.setLocation(new org.eclipse.draw2d.geometry.Point(d.getX(), d.getY()));
					decorationLayer.add(figure);
					decorationFigures.put(d.getId(), figure);
				}
			}
		}

      // Graph.applyLayout() places the new nodes only in a later pass of the event loop, and in the web client the mouse events
      // of the next request are dispatched first: a drag that starts right after a map update would take every node at its
      // initial (0,0) location as the drag origin, and move the object to the corner of the map. With a manual layout the
      // locations are known now.
      if (getLayoutAlgorithm() instanceof ManualLayout)
         graph.applyLayoutNow();
	}

	/**
	 * Set selection on decoration layer. Intended to be called from decoration figure.
	 *
	 * @param d decoartion object
	 * @param addToExisting if true, add to existing selection
	 */
   protected void setDecorationSelection(NetworkMapElement d, boolean addToExisting)
	{
		if (!addToExisting)
		{
			clearDecorationSelection(false);
			graph.setSelection(null);
		}
		selectedDecorations.add(d);

		SelectionChangedEvent event = new SelectionChangedEvent(this, getSelection());
		fireSelectionChanged(event);
		firePostSelectionChanged(event);
	}

	/**
	 * Clear selection on decoration layer
	 */
	private void clearDecorationSelection(boolean sendEvent)
	{
		if (selectedDecorations.size() == 0)
			return;

		for(NetworkMapElement d : selectedDecorations)
		{
		   DecorationLayerAbstractFigure f = decorationFigures.get(d.getId());
			if (f != null)
				f.setSelected(false);
		}
		selectedDecorations.clear();

		if (sendEvent)
		{
			SelectionChangedEvent event = new SelectionChangedEvent(this, getSelection());
			fireSelectionChanged(event);
			firePostSelectionChanged(event);
		}
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.GraphViewer#setSelectionToWidget(java.util.List, boolean)
    */
	@SuppressWarnings("rawtypes")
	@Override
	protected void setSelectionToWidget(List l, boolean reveal)
	{
		selectedDecorations.clear();
		if (l != null)
		{
			clearDecorationSelection(false);
			for(Object o : l)
			{
            if ((o instanceof NetworkMapDecoration) || (o instanceof NetworkMapDCIContainer) ||
                (o instanceof NetworkMapDCIImage) || (o instanceof NetworkMapTextBox))
				{
					selectedDecorations.add((NetworkMapElement)o);
					DecorationLayerAbstractFigure f = decorationFigures.get(((NetworkMapElement)o).getId());
					if (f != null)
						f.setSelected(true);
				}
			}
		}
		super.setSelectionToWidget(l, reveal);
	}

   /**
    * @see org.eclipse.gef4.zest.core.viewers.AbstractStructuredGraphViewer#getSelectionFromWidget()
    */
	@SuppressWarnings({ "rawtypes", "unchecked" })
	@Override
	protected List getSelectionFromWidget()
	{
		List selection = super.getSelectionFromWidget();
		for(NetworkMapElement d : selectedDecorations)
			selection.add(d);
		return selection;
	}

	/**
	 * Set background color for graph
	 *
	 * @param backgroundColor new background color
	 */
	public void setBackgroundColor(RGB color)
	{
		Color c = colors.create(color);
		graph.setBackground(c);
		graph.getLightweightSystem().getRootFigure().setBackgroundColor(c);
	}

   /**
    * Set background image for graph
    *
    * @param image new image or null to clear background
    * @param centered true to center background image
    * @param fit
    */
   public void setBackgroundImage(Image image, boolean centered, boolean fit)
   {
      if (backgroundImage == image && centeredBackground == centered && fitBackground == fit) //nothing to update
         return;

      backgroundImage = image;
      centeredBackground = centered;
      fitBackground = fit;
      backgroundLocation = null;
      backgroundTiles = null;
      graph.redraw();
   }

	/**
	 * @param location
	 * @param zoom
	 */
	public void setBackgroundImage(final GeoLocation location, final int zoom)
	{
	   if ((backgroundLocation != null) && backgroundLocation.equals(location) && (zoom == backgroundZoom)) //nothing to update
	      return;

      if ((backgroundLocation != null) && (backgroundImage != null))
			backgroundImage.dispose();

		backgroundImage = null;
      backgroundTiles = null;
      centeredBackground = false;
      fitBackground = false;
		backgroundLocation = location;
		backgroundZoom = zoom;
		graph.redraw();
		reloadMapBackground();
	}

	/**
	 * Reload map background
	 */
	private void reloadMapBackground()
	{
      Dimension controlSize = backgroundFigure.getSize();
		final Point mapSize = new Point(controlSize.width, controlSize.height);
      Job job = new Job(i18n.tr("Download map tiles"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				final TileSet tiles = mapLoader.getAllTiles(mapSize, backgroundLocation, MapLoader.TOP_LEFT, backgroundZoom, false);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if ((backgroundLocation == null) || graph.isDisposed())
							return;

						if (backgroundImage != null)
                  {
                     backgroundImage.dispose();
                     backgroundImage = null;
                  }
                  if (backgroundTiles != null)
                  {
                     backgroundTiles.dispose();
                  }
                  backgroundTiles = tiles;
						backgroundFigure.repaint();
						//drawTiles(tiles);
						//graph.redraw();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot download map tiles");
			}
		};
		job.setUser(false);
		job.start();
	}

   /**
    * @see org.eclipse.jface.viewers.StructuredViewer#internalRefresh(java.lang.Object, boolean)
    */
	@Override
	protected void internalRefresh(Object element, boolean updateLabels)
	{
		if (getInput() == null)
		{
			return;
		}
		if (element == getInput())
		{
			getFactory().refreshGraph(graph);
		}
		else
		{
			getFactory().refresh(graph, element, updateLabels);
		}
	}

   /**
    * Check if a map object (node or link) is drawn at the given point of the control. Objects are in the Zest root layer; where
    * there is none, the point belongs to the background.
    *
    * @param x X in control coordinates
    * @param y Y in control coordinates
    * @return true if there is a map object at that point
    */
   private boolean isObjectAt(int x, int y)
   {
      IFigure objectLayer = graph.getZestRootLayer();
      for(IFigure f = graph.getLightweightSystem().getRootFigure().findFigureAt(x, y); f != null; f = f.getParent())
      {
         if (f.getParent() == objectLayer)
            return true;
      }
      return false;
   }

   /**
    * Check if a mouse drag starting at the given point of the control pans the view, as a drag on the map background does. A drag
    * on a map object or a decoration moves it instead while object movement is enabled (and pans while it is locked), and a drag
    * on a resize handle or on an overlay control such as the back button never pans.
    *
    * @param x X in control coordinates
    * @param y Y in control coordinates
    * @return true if a drag starting at that point pans the view
    */
   private boolean isPanStart(int x, int y)
   {
      IFigure objectLayer = graph.getZestRootLayer();
      for(IFigure f = graph.getLightweightSystem().getRootFigure().findFigureAt(x, y); f != null; f = f.getParent())
      {
         if ((f instanceof ResizeHandle) || (f.getParent() == controlLayer))
            return false;
         if ((f.getParent() == objectLayer) || (f.getParent() == decorationLayer))
            return !draggingEnabled;
      }
      return true;
   }

   /**
    * Mark this map as part of a larger scrollable page, such as a dashboard, instead of filling a view of its own. Such a map never
    * zooms with the plain mouse wheel, whatever the user preference says, and in the web client leaves the plain wheel and
    * one-finger touch drag to the page; Ctrl/Cmd + mouse wheel, mouse drag and two-finger touch gestures still pan and zoom it.
    *
    * @param embedded true if the map is embedded in a scrollable page
    */
   public void setEmbedded(boolean embedded)
   {
      this.embedded = embedded;
      updateMapInputOptions();
   }

   /**
    * Check if the plain mouse wheel (without Ctrl/Cmd) zooms this map. It does if the user has chosen that in the network map
    * preferences, except on a map embedded in a scrollable page.
    *
    * @return true if the plain mouse wheel zooms
    */
   private boolean isPlainWheelZoom()
   {
      return !embedded && PreferenceStore.getInstance().getAsBoolean(PREF_PLAIN_WHEEL_ZOOM, false);
   }

   /**
    * Send the mouse wheel and touch options to the web client's map input handler.
    */
   private void updateMapInputOptions()
   {
      WidgetHelper.setMapInputOptions(graph, isPlainWheelZoom(), embedded);
   }

   /**
    * Send the current view state to the web client's map input handler, at most once per pass of the event loop however many
    * changes the pass makes.
    */
   private void scheduleViewStateUpdate()
   {
      if (viewStateUpdateScheduled)
         return;
      viewStateUpdateScheduled = true;
      graph.getDisplay().asyncExec(() -> {
         viewStateUpdateScheduled = false;
         if (graph.isDisposed())
            return;
         // Same quantities setViewLocationClamped() clamps with
         org.eclipse.draw2d.geometry.Point location = graph.getViewport().getViewLocation();
         org.eclipse.draw2d.geometry.Rectangle viewportArea = graph.getViewport().getClientArea();
         Dimension mapSize = backgroundLayer.getSize();
         WidgetHelper.updateMapInputViewState(graph, location.x, location.y, mapSize.width, mapSize.height, viewportArea.width,
               viewportArea.height, getZoom(), zoomLevels[0], zoomLevels[zoomLevels.length - 1]);
      });
   }

   /**
    * Set viewport location, clamped so that the view cannot be scrolled outside the map.
    *
    * @param newViewLocation requested view location
    */
   private void setViewLocationClamped(org.eclipse.draw2d.geometry.Point newViewLocation)
   {
      if (newViewLocation.x < 0)
         newViewLocation.x = 0;
      if (newViewLocation.y < 0)
         newViewLocation.y = 0;

      Dimension scaledSize = backgroundLayer.getSize();
      scaledSize.performScale(getZoom());
      org.eclipse.draw2d.geometry.Rectangle viewportArea = graph.getViewport().getClientArea();
      if (newViewLocation.x > scaledSize.width - viewportArea.width)
         newViewLocation.x = scaledSize.width - viewportArea.width;
      if (newViewLocation.y > scaledSize.height - viewportArea.height)
         newViewLocation.y = scaledSize.height - viewportArea.height;
      if (newViewLocation.x < 0)
         newViewLocation.x = 0;
      if (newViewLocation.y < 0)
         newViewLocation.y = 0;

      graph.getViewport().setViewLocation(newViewLocation);
   }

   /**
    * Center the viewport on the map's content.
    *
    * A map's canvas is normally much larger than the area its elements actually occupy, and the
    * viewport opens at the canvas origin, so the first screen is frequently empty while the content
    * sits somewhere else entirely. This scrolls the viewport as though the user had dragged the
    * content into the middle of it. It changes the scroll position only - the zoom level is left
    * exactly as it was.
    *
    * Centering is computed from the bounding box of the elements themselves, not from the canvas or
    * the background layer, because it is the elements the user is looking for.
    *
    * Does nothing if the map has no elements yet, if the graph layout has not placed them yet, or if
    * the control has not been laid out and the viewport therefore has no size yet. Map content and
    * layout arrive asynchronously, so all of these are normal transient states rather than errors.
    *
    * @return true if the viewport was actually centered, false if there was nothing to center on yet
    */
   public boolean centerOnContent()
   {
      // The viewport client area is the area visible inside the scrollbars, excluding the space they
      // occupy, which is what the content has to be centered within.
      org.eclipse.draw2d.geometry.Rectangle viewportArea = graph.getViewport().getClientArea();
      if ((viewportArea.width <= 0) || (viewportArea.height <= 0))
         return false; // not laid out yet - nothing sensible to center within

      org.eclipse.draw2d.geometry.Rectangle contentArea = null;
      org.eclipse.draw2d.geometry.Point firstLocation = null;
      boolean allAtOneLocation = true;
      int nodeCount = 0;
      for(Object o : graph.getNodes())
      {
         if (!(o instanceof CGraphNode))
            continue;
         CGraphNode n = (CGraphNode)o;
         org.eclipse.draw2d.geometry.Point location = n.getLocation();
         if (firstLocation == null)
            firstLocation = location;
         else if (!location.equals(firstLocation))
            allAtOneLocation = false;
         nodeCount++;
         org.eclipse.draw2d.geometry.Rectangle nodeArea = new org.eclipse.draw2d.geometry.Rectangle(location, n.getSize());
         contentArea = (contentArea == null) ? nodeArea : contentArea.union(nodeArea);
      }
      if (contentArea == null)
         return false; // no content yet - it arrives asynchronously

      // Nodes exist before the layout has placed them: Graph.applyLayout() defers to the first paint, and until it runs every
      // node is still at its initial location. Centering on that would center on a phantom box and end the caller's retries.
      // Several nodes sharing one location is that transient state, never a laid out map.
      if ((nodeCount > 1) && allAtOneLocation)
         return false;

      // Element coordinates are unscaled, the viewport works in scaled ones - same conversion the
      // search-and-reveal path uses.
      org.eclipse.draw2d.geometry.Point center = contentArea.getCenter();
      center.performScale(getZoom());
      org.eclipse.draw2d.geometry.Point target = center.getTranslated(-viewportArea.width / 2, -viewportArea.height / 2);
      setViewLocationClamped(target);
      return true;
   }

	/**
	 * Zoom to next level
	 */
	public void zoomIn()
	{
      getZoomManager().zoomIn();
	}

	/**
	 * Zoom to previous level
	 */
	public void zoomOut()
	{
		getZoomManager().zoomOut();
	}

   /**
    * Zoom one level in or out while keeping the map point under the cursor fixed on screen.
    *
    * @param zoomIn true to zoom in, false to zoom out
    * @param cursorX cursor X position in control coordinates
    * @param cursorY cursor Y position in control coordinates
    */
   private void zoomToCursor(boolean zoomIn, int cursorX, int cursorY)
   {
      double oldZoom = getZoom();
      org.eclipse.draw2d.geometry.Point viewLocation = graph.getViewport().getViewLocation();

      // Map point (unscaled) currently under the cursor
      double mapX = (cursorX + viewLocation.x) / oldZoom;
      double mapY = (cursorY + viewLocation.y) / oldZoom;

      if (zoomIn)
         getZoomManager().zoomIn();
      else
         getZoomManager().zoomOut();

      double newZoom = getZoom();
      if (newZoom == oldZoom)
         return; // already at minimum or maximum zoom level

      // New scroll offset so the same map point stays under the cursor
      int nx = (int)Math.round(mapX * newZoom - cursorX);
      int ny = (int)Math.round(mapY * newZoom - cursorY);
      setViewLocationClamped(new org.eclipse.draw2d.geometry.Point(nx, ny));
   }

   /**
    * Zoom to fit available screen area
    */
   public void zoomFit()
   {
      getZoomManager().setZoom(1.0);
      Rectangle visibleArea = getGraphControl().getClientArea();
      org.eclipse.draw2d.geometry.Rectangle mapArea = getGraphControl().getRootLayer().getBounds();
      double dx = (double)visibleArea.width / (double)mapArea.width;
      double dy = (double)visibleArea.height / (double)mapArea.height;
      getZoomManager().setZoom(Math.min(dx, dy));
   }

	/**
	 * Zoom to specific level
	 *
	 * @param zoomLevel
	 */
	public void zoomTo(double zoomLevel)
	{
		getZoomManager().setZoom(zoomLevel);
	}

	/**
	 * Get current zoom level
	 *
	 * @return
	 */
	public double getZoom()
	{
	   return getZoomManager().getZoom();
	}

	/**
	 * Create zoom actions
	 * @return
	 */
   public Action[] createZoomActions()
	{
		final ZoomManager zoomManager = getZoomManager();
		final Action[] actions = new Action[zoomLevels.length];
		for(int i = 0; i < zoomLevels.length; i++)
		{
			actions[i] = new ZoomAction(zoomLevels[i], zoomManager);
			if (zoomLevels[i] == getZoom())
			{
				actions[i].setChecked(true);
			}
         if (zoomLevels[i] == 1.00)
         {
            if (view != null)
               view.addKeyBinding("M1+0", actions[i]);
         }
		}
		return actions;
	}

	/**
    * Show "back" button
    *
	 * @param action
	 */
	public void showBackButton(final Runnable action)
	{
	   if (backButton != null)
	   {
	      backButton.setAction(action);
	   }
	   else
	   {
	      backButton = new OverlayButton(iconBack, action);
         controlLayer.add(backButton);
         backButton.setLocation(new org.eclipse.draw2d.geometry.Point(10, 10));
	   }
	}

	/**
	 * Hide "back" button
	 */
	public void hideBackButton()
	{
      if (backButton != null)
      {
         controlLayer.remove(backButton);
         backButton = null;
      }
	}

	/**
	 * Show crosshair at given location
	 *
	 * @param x
	 * @param y
	 */
	public void showCrosshair(int x, int y)
	{
		if (crosshairFigure == null)
		{
			crosshairFigure = new Crosshair();
			indicatorLayer.add(crosshairFigure);
			crosshairFigure.setSize(graph.getRootLayer().getSize());
		}
		crosshairX = x;
		crosshairY = y;
		crosshairFigure.repaint();
	}

	/**
	 * Hide crosshair
	 */
	public void hideCrosshair()
	{
		if (crosshairFigure != null)
		{
			indicatorLayer.remove(crosshairFigure);
			crosshairFigure = null;
		}
	}

	/**
	 * If map is fit to screen
	 *
	 * @return true if map is fit to screen map
	 */
	boolean isFitToScreen()
	{
	   return backgroundFigure.getSize().equals(new Dimension(-1, -1));
	}

	/**
	 * Show/hide grid
	 *
	 * @param show
	 */
	public void showGrid(boolean show)
	{
		if (show)
		{
			if (gridFigure == null)
			{
				gridFigure = new GridFigure();
				backgroundLayer.add(gridFigure, null, 1);
				if (isFitToScreen())
				   gridFigure.setSize(backgroundLayer.getSize());
				else
               gridFigure.setSize(backgroundFigure.getSize());
			}
		}
		else
		{
			if (gridFigure != null)
			{
				backgroundLayer.remove(gridFigure);
				gridFigure = null;
			}
		}
	}

	/**
	 * @return
	 */
	public boolean isGridVisible()
	{
		return gridFigure != null;
	}

   /**
    * Show/hide map size border
    *
    * @param show
    */
   public void showSize(boolean show)
   {
      showMapSize = show;
      if (show)
      {
         backgroundFigure.setBorder(new AbstractBorder() {
            @Override
            public void paint(IFigure figure, Graphics graphics, Insets insets)
            {
               tempRect.setBounds(getPaintRectangle(figure, insets));
               tempRect.shrink(1, 1);
               graphics.setLineWidth(1);
               graphics.setLineStyle(SWT.LINE_DASH);
               graphics.setLineDash(new int[] { 4, 8 });
               graphics.setForegroundColor(ThemeEngine.getForegroundColor("Map.Border"));
               graphics.drawRectangle(tempRect);
            }

            @Override
            public Insets getInsets(IFigure figure)
            {
               return new Insets(1, 1, 1, 1);
            }
         });
      }
      else
      {
         backgroundFigure.setBorder(null);
      }
   }

   /**
    * If map size border is visible
    *
    * @return true if map size border is visible
    */
   public boolean isSizeVisible()
   {
      return showMapSize;
   }


   /**
    * Finish an object drag: save the moved objects (manual layout only) and allow map refresh again. Nodes disposed since the drag
    * started (the view was switched to another map, which also resets the refresh block) are not saved.
    */
   private void finishObjectDrag()
   {
      dragStarted = false;
      if (getLayoutAlgorithm() instanceof ManualLayout)
      {
         for(CGraphNode n : objectDragOrigins.keySet())
         {
            if (!n.isDisposed() && (n.getFigure() instanceof ObjectFigure))
               moveCallback.onMove(((ObjectFigure)n.getFigure()).getMapElement());
         }
      }
      objectDragOrigins.clear();
      unblockRefresh();
   }

   /**
    * Move the dragged objects by the cursor movement since the press (object layer coordinates, unscaled like node locations).
    *
    * @param x current cursor X
    * @param y current cursor Y
    */
   private void moveDraggedObjects(int x, int y)
   {
      int dx = x - objectDragStart.x;
      int dy = y - objectDragStart.y;
      for(Map.Entry<CGraphNode, org.eclipse.draw2d.geometry.Point> e : objectDragOrigins.entrySet())
         e.getKey().setLocation(e.getValue().x + dx, e.getValue().y + dy);
   }

	/**
	 * Clear the moved state of all objects, so that snap to grid aligns only the objects moved after this call.
	 */
	private void clearMovedState()
	{
		for(Object o : graph.getNodes())
		{
			if (o instanceof CGraphNode)
				((ObjectFigure)((CGraphNode)o).getFigure()).readMovedState();
		}
	}

	/**
	 * Align objects to grid
	 */
	public void alignToGrid(boolean movedOnly)
	{
		for(Object o : graph.getNodes())
		{
			if (!(o instanceof CGraphNode))
				continue;
			CGraphNode n = (CGraphNode)o;
			if (movedOnly)
			{
				ObjectFigure f = (ObjectFigure)n.getFigure();
				if (!f.readMovedState() || !f.isElementSelected())
					continue;
			}
			org.eclipse.draw2d.geometry.Point p = n.getLocation();
			Dimension size = n.getSize();

			int dx = p.x % gridSize;
			if (dx < gridSize / 2)
				dx = - dx;
			else
				dx = gridSize - dx;
			dx += (gridSize - size.width) / 2;

			int dy = p.y % gridSize;
			if (dy < gridSize / 2)
				dy = - dy;
			else
				dy = gridSize - dy;
			dy += (gridSize - size.height) / 2;

			n.setLocation(p.x + dx, p.y + dy);
		}
	}

	/**
	 * Set snap-to-grid flag
	 *
	 * @param snap
	 */
	public void setSnapToGrid(boolean snap)
	{
		if (snap == snapToGrid)
			return;

		snapToGrid = snap;
		if (snap)
		{
			graph.getZestRootLayer().addMouseListener(snapToGridListener);
		}
		else
		{
		   graph.getZestRootLayer().removeMouseListener(snapToGridListener);
		}
	}

	/**
	 * Get snap-to-grid flag
	 *
	 * @return
	 */
	public boolean isSnapToGrid()
	{
		return snapToGrid;
	}

	/**
    * @return the draggingEnabled
    */
   public boolean isDraggingEnabled()
   {
      return draggingEnabled;
   }

   /**
    * @return if refresh is blocked by any action
    */
   public boolean isRefreshBlocked()
   {
      return blockRefresh > 0;
   }

   /**
    * @param draggingEnabled the draggingEnabled to set
    */
   public void setDraggingEnabled(boolean draggingEnabled)
   {
      this.draggingEnabled = draggingEnabled;
      if (moveCallback == null)
         graph.setDraggingEnabled(draggingEnabled); // otherwise objects are moved by this viewer, not by Zest
   }

   /**
    * Resize tooltip's shell. Usually called by tooltip figure after re-layout.
    */
   void resizeToolTipShell()
   {
      eventDispatcher.resizeToolTipShell();
   }

   /**
    * @see org.eclipse.gef4.zest.core.viewers.GraphViewer#getFactory()
    */
	@Override
	protected IStylingGraphModelFactory getFactory()
	{
		return new GraphModelEntityRelationshipFactory(this) {
			@Override
			public void styleConnection(GraphConnection conn)
			{
				// do nothing here - this will override default behavior
				// to make connection curved if there are multiple connections
				// between nodes
			}
		};
	}

	/**
	 * Take snapshot of network map
	 *
	 * @return
	 */
	public Image takeSnapshot()
	{
	   IFigure rootLayer = graph.getRootLayer();
	   Dimension d = rootLayer.getSize();
      Image image = new Image(graph.getDisplay(), d.width, d.height);
	   GC gc = new GC(image);
	   Graphics g = new SWTGraphics(gc);
	   rootLayer.paint(g);
	   g.dispose();
      gc.dispose();
      return image;
	}

	/**
	 * Additional figure used to display custom background for graph.
	 */
   private class BackgroundFigure extends Figure implements IDecorationLayer
	{
      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
		@Override
		protected void paintFigure(Graphics gc)
		{
         if (backgroundTiles != null)
         {
            drawTiles(gc, backgroundTiles);
         }
         else if (backgroundImage != null)
         {
            if (centeredBackground)
            {
               int backgroundLayerWidth = backgroundFigure.getSize().width();
               int backgroundLayerHeight = backgroundFigure.getSize().height();
               int x = (backgroundLayerWidth / 2) - (backgroundImage.getBounds().width / 2);
               int y = (backgroundLayerHeight / 2) - (backgroundImage.getBounds().height / 2);
               gc.drawImage(backgroundImage, new org.eclipse.draw2d.geometry.Point(x, y));
            }
            else if (fitBackground)
            {
               int backgroundLayerWidth = backgroundFigure.getSize().width();
               int backgroundLayerHeight = backgroundFigure.getSize().height();
               gc.drawImage(backgroundImage, 0, 0, backgroundImage.getBounds().width, backgroundImage.getBounds().height, 0, 0,
                     backgroundLayerWidth, backgroundLayerHeight);
            }
            else
            {
               gc.drawImage(backgroundImage, getLocation());
            }
         }
		}

	   /**
	    * @param tileSet
	    */
	   private void drawTiles(Graphics gc, TileSet tileSet)
	   {
	      if ((backgroundLocation != null) && (backgroundImage != null))
	         backgroundImage.dispose();

	      if ((tileSet == null) || (tileSet.tiles == null) || (tileSet.tiles.length == 0))
	      {
	         backgroundImage = null;
	         return;
	      }

	      final Tile[][] tiles = tileSet.tiles;

	      Dimension size = backgroundFigure.getSize();
	      backgroundImage = new Image(getGraphControl().getDisplay(), size.width, size.height);

	      int x = tileSet.xOffset;
	      int y = tileSet.yOffset;
	      for(int i = 0; i < tiles.length; i++)
	      {
	         for(int j = 0; j < tiles[i].length; j++)
	         {
	            gc.drawImage(tiles[i][j].getImage(), x, y);
	            x += 256;
	            if (x >= size.width)
	            {
	               x = tileSet.xOffset;
	               y += 256;
	            }
	         }
	      }
	   }
	}

	/**
	 * Crosshair
	 */
	private class Crosshair extends Figure
	{
		/**
       * Create crosshair figure
       */
		public Crosshair()
		{
			setOpaque(false);
		}

      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
		@Override
		protected void paintFigure(Graphics gc)
		{
			gc.setLineStyle(SWT.LINE_DOT);
			Dimension size = getSize();
			gc.drawLine(0, crosshairY, size.width, crosshairY);
			gc.drawLine(crosshairX, 0, crosshairX, size.height);
		}
	}

	/**
	 * Grid
	 */
   /**
    * Rubber band rectangle shown during box selection
    */
   private class BoxSelectionFigure extends Figure
   {
      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
      @Override
      protected void paintFigure(Graphics gc)
      {
         org.eclipse.draw2d.geometry.Rectangle r = getBounds();
         Color color = graph.getDisplay().getSystemColor(SWT.COLOR_LIST_SELECTION);
         gc.setBackgroundColor(color);
         gc.setForegroundColor(color);
         gc.setAlpha(48);
         gc.fillRectangle(r);
         gc.setAlpha(255);
         gc.setLineWidth(1);
         gc.drawRectangle(r.x, r.y, r.width - 1, r.height - 1);
      }
   }

	private class GridFigure extends Figure implements IDecorationLayer, IDecorationFigure
	{
		/**
		 * Create new grid figure
		 */
		public GridFigure()
		{
			setOpaque(false);
		}

      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
		@Override
		protected void paintFigure(Graphics gc)
		{
			gc.setLineStyle(SWT.LINE_DOT);
			Dimension size = getSize();
			for(int x = gridSize; x < size.width; x += gridSize)
				gc.drawLine(x, 0, x, size.height);
			for(int y = gridSize; y < size.height; y += gridSize)
				gc.drawLine(0, y, size.width, y);
		}
	}

	/**
	 * Overlay button
	 */
   private class OverlayButton extends Figure
	{
	   private Image icon;
	   private Runnable action;

	   /**
	    * @param icon
	    * @param action
	    */
	   public OverlayButton(Image icon, Runnable action)
	   {
	      this.icon = icon;
	      this.action = action;
	      addMouseListener(new MouseListener() {
            @Override
            public void mousePressed(MouseEvent me)
            {
               OverlayButton.this.action.run();
            }

            @Override
            public void mouseReleased(MouseEvent me)
            {
            }

            @Override
            public void mouseDoubleClicked(MouseEvent me)
            {
            }
	      });
	      setSize(getPreferredSize());
	   }

      /**
       * @see org.eclipse.draw2d.Figure#getPreferredSize(int, int)
       */
      @Override
      public Dimension getPreferredSize(int wHint, int hHint)
      {
         Rectangle iconBounds = icon.getBounds();
         return new Dimension(iconBounds.width, iconBounds.height).expand(6, 6);
      }

      /**
       * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
       */
      @Override
      protected void paintFigure(Graphics gc)
      {
         org.eclipse.draw2d.geometry.Point pos = getLocation();
         Dimension size = getSize();
         gc.setLineWidth(1);
         gc.setForegroundColor(colors.create(86, 20, 152));
         gc.setBackgroundColor(gc.getForegroundColor());
         gc.setAlpha(64);
         gc.fillRectangle(pos.x, pos.y, size.width - 1, size.height - 1);
         gc.setAlpha(255);
         gc.drawRectangle(pos.x, pos.y, size.width - 1, size.height - 1);
         gc.drawImage(icon, pos.x + 3, pos.y + 3);
      }

      /**
       * @param action
       */
      public void setAction(Runnable action)
	   {
	      this.action = action;
	   }
	}

   /**
    * Extended SWT event dispatcher - provides access to tooltip helper's shell
    */
   private static class ExtendedSWTEventDispatcher extends SWTEventDispatcher
   {
      private ExtendedToolTipHelper toolTipHelper;

      /**
       * @see org.eclipse.draw2d.SWTEventDispatcher#getToolTipHelper()
       */
      @Override
      protected ToolTipHelper getToolTipHelper()
      {
         if (toolTipHelper == null)
            toolTipHelper = new ExtendedToolTipHelper(control);
         return toolTipHelper;
      }

      /**
       * Resize tooltip shell
       */
      public void resizeToolTipShell()
      {
         if ((toolTipHelper != null) && toolTipHelper.isShowing())
            toolTipHelper.resizeShell();
      }
   }

   /**
    * Extended tooltip helper - provides access to tooltip helper's shell
    */
   private static class ExtendedToolTipHelper extends ToolTipHelper
   {
      public ExtendedToolTipHelper(Control c)
      {
         super(c);
      }

      /**
       * Resize tooltip shell
       */
      public void resizeShell()
      {
         Dimension shellSize = getLightweightSystem().getRootFigure().getPreferredSize().getExpanded(getShellTrimSize());
         getShell().setSize(shellSize.width, shellSize.height);
      }
   }

   /**
    * On link change
    */
   public void onLinkChange(NetworkMapLink element)
   {
      if (moveCallback != null)
      {
         moveCallback.onLinkChange(element);
      }
   }

   /**
    * Function called on decoration move
    *
    * @param element moved decoration element
    */
   public void onDecorationMove(NetworkMapElement element)
   {
      if (moveCallback != null )
      {
         moveCallback.onMove(element);
      }
   }

   /**
    * Set map size
    *
    * @param width map width
    * @param height map height
    */
   public void setMapSize(int width, int height)
   {
      graph.setPreferredSize(width, height);
      backgroundFigure.setSize(width, height);
      // Map without fixed size has no area outside its borders
      graph.getViewport().setCursor(((width < 0) || (height < 0)) ? graph.getDisplay().getSystemCursor(SWT.CURSOR_ARROW) : null);
      if (showMapSize)
      {
         backgroundFigure.setBorder(new AbstractBorder() {
            @Override
            public void paint(IFigure figure, Graphics graphics, Insets insets)
            {
               tempRect.setBounds(getPaintRectangle(figure, insets));
               tempRect.shrink(1, 1);
               graphics.setLineWidth(1);
               graphics.setLineStyle(SWT.LINE_DASH);
               graphics.setLineDash(new int[] { 4, 8 });
               graphics.setForegroundColor(ThemeEngine.getForegroundColor("Map.Border"));
               graphics.drawRectangle(tempRect);
            }

            @Override
            public Insets getInsets(IFigure figure)
            {
               return new Insets(1, 1, 1, 1);
            }
         });
      }
   }

   /**
    * Get current map size
    *
    * @return map size
    */
   public Dimension getMapSize()
   {
      if (graph.getPreferredSize().height == -1)
      {
         Rectangle visibleArea = getGraphControl().getClientArea();
         return new Dimension(visibleArea.width, visibleArea.height);
      }
      else
         return graph.getPreferredSize();
   }

   /**
    * Get zoom index for font size calculation
    *
    * @return zoom index in array
    */
   public int getCurrentZoomIndex()
   {
      for (int i = 0; i < zoomLevels.length; i++)
      {
         if (getZoom() <= zoomLevels[i])
         {
            return i;
         }
      }
      return 0;
   }

   /**
    * Get last right click location
    */
   public Point getRightClickLocation()
   {
      return rightClickLocation ;
   }

   /**
    * @param location
    * @return
    */
   public Point translateToRelative(Point location)
   {
      org.eclipse.draw2d.geometry.Point mousePoint = new org.eclipse.draw2d.geometry.Point(location.x, location.y);
      graph.getRootLayer().translateToRelative(mousePoint);
      return new Point(mousePoint.x, mousePoint.y);
   }

   /**
    * Get horizontal bar current position
    *
    * @return current horizontal bar position
    */
   public int getHorizontalBarSelection()
   {
      return graph.getHorizontalBar().getSelection();
   }

   /**
    * Get vertical bar current position
    *
    * @return current vertical bar position
    */
   public int getVerticalBarSelection()
   {
      return graph.getVerticalBar().getSelection();
   }

   /**
    * Find and select node by text (search forward)
    *
    * @param text text to search for
    */
   public void findNodeByText(String text)
   {
      findNodeByText(text, true);
   }

   /**
    * Find and select node by text
    *
    * @param text text to search for
    * @param forward true to search forward, false to search backward
    */
   public void findNodeByText(String text, boolean forward)
   {
      List<?> nodes = graph.getNodes();
      if (nodes.isEmpty())
      {
         view.addMessage(MessageArea.INFORMATION, i18n.tr("No matching objects found on the map."));
         return;
      }

      int startIndex;
      boolean wrapped = false;
      if (graph.getSelection().size() == 1)
      {
         Object selected = graph.getSelection().get(0);
         int selectedIndex = nodes.indexOf(selected);
         if (forward)
         {
            startIndex = selectedIndex + 1;
            if (startIndex >= nodes.size())
            {
               startIndex = 0;
               wrapped = true;
            }
         }
         else
         {
            startIndex = selectedIndex - 1;
            if (startIndex < 0)
            {
               startIndex = nodes.size() - 1;
               wrapped = true;
            }
         }
      }
      else
      {
         startIndex = forward ? 0 : nodes.size() - 1;
      }

      boolean found = false;
      int i = startIndex;
      do
      {
         Object o = nodes.get(i);
         if (o instanceof CGraphNode)
         {
            CGraphNode n = (CGraphNode)o;
            MapLabelProvider lp = (MapLabelProvider)getLabelProvider();
            String label = lp.getText(((ObjectFigure)n.getFigure()).getMapElement());
            if (label != null && label.toUpperCase().contains(text.toUpperCase()))
            {
               found = true;
               setSelection(new StructuredSelection(((ObjectFigure)n.getFigure()).getMapElement()), true);

               org.eclipse.draw2d.geometry.Point center = new org.eclipse.draw2d.geometry.Rectangle(n.getLocation(), n.getSize()).getCenter();
               center.performScale(getZoom());
               org.eclipse.draw2d.geometry.Point newViewLocation = center.getTranslated(-graph.getViewport().getClientArea().width / 2, -graph.getViewport().getClientArea().height / 2);
               graph.getViewport().setViewLocation(newViewLocation.x, newViewLocation.y);

               if (wrapped)
               {
                  if (forward)
                     view.addMessage(MessageArea.INFORMATION, i18n.tr("Search continued from the beginning of the map."));
                  else
                     view.addMessage(MessageArea.INFORMATION, i18n.tr("Search continued from the end of the map."));
               }

               break;
            }
         }
         if (forward)
         {
            i = (i + 1) % nodes.size();
            if (i == 0)
               wrapped = true;
         }
         else
         {
            i = (i - 1 + nodes.size()) % nodes.size();
            if (i == nodes.size() - 1)
               wrapped = true;
         }
      } while(i != startIndex);

      if (!found)
      {
         setSelection(null);
         view.addMessage(MessageArea.INFORMATION, i18n.tr("No matching objects found on the map."));
      }
   }
}
