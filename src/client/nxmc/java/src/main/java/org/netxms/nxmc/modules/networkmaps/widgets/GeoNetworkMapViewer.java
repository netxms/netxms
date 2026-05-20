/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.networkmaps.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.ISelectionProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DropTarget;
import org.eclipse.swt.dnd.DropTargetAdapter;
import org.eclipse.swt.dnd.DropTargetEvent;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.MapInitialViewMode;
import org.netxms.client.maps.NetworkMapLink;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.GeoLinkRenderer;
import org.netxms.nxmc.modules.networkmaps.widgets.helpers.LinkDciValueProvider;
import org.netxms.nxmc.modules.worldmap.GeoLocationCache;
import org.netxms.nxmc.modules.worldmap.widgets.AbstractGeoMapViewer;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.FontTools;

/**
 * Network map viewer that renders nodes as pins on a geographical tile map.
 * Used for network maps whose canvas type is {@code GEOGRAPHICAL}. Renders
 * pins, links (with bendpoints, color/width/style, direction arrow and
 * connector / DCI labels) and is wired to the same hide-links /
 * hide-link-labels / show-link-direction actions as the Zest-based canvas.
 */
public class GeoNetworkMapViewer extends AbstractGeoMapViewer implements ISelectionProvider
{
   private static final Color INNER_BORDER_COLOR = new Color(Display.getCurrent(), 255, 255, 255);
   private static final Color PIN_SELECTION_COLOR = new Color(Display.getCurrent(), 0, 148, 255);

   private static final int SEGMENT_HIT_TOLERANCE = 5;
   private static final int BENDPOINT_HIT_TOLERANCE = 6;
   private static final int HANDLE_SIZE = 12;
   private static final int HANDLE_HIT_TOLERANCE = HANDLE_SIZE / 2 + 2;
   /**
    * Perpendicular spacing in pixels between adjacent parallel links sharing
    * the same endpoint pair. Mirrors the fan-out behaviour the graph (Zest)
    * canvas gets from {@code MultiConnectionAnchor}, except here we apply it
    * directly in pixel space because there is no draw2d anchor mediating the
    * pin positions.
    */
   private static final int LINK_FAN_SPACING_PX = 16;
   private static final double[] NO_FAN_OFFSET = { 0.0, 0.0 };
   private static final Color HANDLE_FILL = new Color(Display.getCurrent(), 255, 255, 255);
   private static final Color HANDLE_BORDER = new Color(Display.getCurrent(), 0, 0, 0);

   private NetworkMapPage content;
   private final List<AbstractObject> placedObjects = new ArrayList<>();
   private final List<ObjectIcon> objectIcons = new ArrayList<>();
   private final List<LinkGeometry> linkGeometries = new ArrayList<>();
   // Edited link's pixel-space geometry; independent of #linkGeometries so
   // handles stay hit-testable when links are hidden or endpoints off-screen.
   private LinkGeometry editedLinkGeometry;
   // Fan-out offset (pixels) for the edited link; recomputed each paint.
   private double[] editedLinkFanOffset = NO_FAN_OFFSET;
   private java.util.function.IntConsumer unplacedCountListener;
   private int lastReportedUnplacedCount = -1;
   private AbstractObject currentObject;
   private NetworkMapLink currentLink;
   private LinkHitInfo rightClickLinkHit;
   private Font objectLabelFont;
   private boolean showObjectNames = true;
   private ISelection selection = new StructuredSelection();
   private final Set<ISelectionChangedListener> selectionListeners = new HashSet<>();
   private final LinkDciValueProvider dciValueProvider = LinkDciValueProvider.getInstance();
   private Color defaultLinkColor;
   private int defaultLinkColorSource;
   private int defaultLinkWidth;
   private int defaultLinkStyle = 1;
   private int defaultLinkRouting = NetworkMapLink.ROUTING_DIRECT;
   private boolean showLinkDirection;
   private boolean linksVisible = true;
   private boolean linkLabelsVisible = true;

   // Pending initial-view-mode; applied once viewport size + content are ready, then cleared.
   private MapInitialViewMode pendingInitialMode;
   private GeoLocation pendingBgLocation;
   private int pendingBgZoom;

   // Last point at which the user pressed the right mouse button. Used by the
   // context menu's "Place node here" action — captured here because the menu
   // itself runs after the SWT mouseDown handler returns.
   private Point rightClickPoint;
   private DropHandler dropHandler;

   // Bendpoint editing state. editedLink is non-null while a single
   // BENDPOINTS-routed link is selected and the host has enabled editing.
   // draggingBendpointIndex is the index into editedLink.getGeoBendPoints()
   // currently being dragged, or -1 when no drag is in progress.
   private NetworkMapLink editedLink;
   private int draggingBendpointIndex = -1;
   private org.netxms.nxmc.modules.networkmaps.widgets.helpers.FigureChangeCallback linkChangeCallback;

   public GeoNetworkMapViewer(Composite parent, int style, View view)
   {
      super(parent, style, view);

      objectLabelFont = FontTools.createAdjustedFont(JFaceResources.getDefaultFont(), 0, SWT.BOLD);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            objectLabelFont.dispose();
         }
      });

      addControlListener(new ControlAdapter() {
         @Override
         public void controlResized(ControlEvent e)
         {
            tryApplyInitialView();
         }
      });
   }

   /**
    * Suppress the base class's automatic tile load while we're still waiting
    * for the fit-to-screen target to be computed (which requires placed
    * objects from {@link #setContent}). Without this, the controlResized
    * handler in {@code AbstractGeoMapViewer} would call {@code reloadMap}
    * with whatever accessor is in place (default 0,0 or the persisted
    * backgroundLocation), tiles would load for that location, and the first
    * paint would draw pins at the wrong relative position — only to redraw
    * them after {@link #tryApplyInitialView} pans / zooms to the real target.
    * The user perceives that re-shift as the pins "jumping".
    */
   @Override
   public void reloadMap()
   {
      if ((pendingInitialMode == MapInitialViewMode.FIT_TO_SCREEN) && (content == null))
         return;
      super.reloadMap();
   }

   /**
    * Set map content. Elements without a resolvable object or geolocation are
    * kept in the page but not rendered; callers can surface the hidden count
    * via {@link #getUnplacedObjectCount()}.
    *
    * <p>If a link is currently being bendpoint-edited, the saved-and-refreshed
    * map page typically replaces the link instance with a fresh copy. We
    * re-resolve {@link #editedLink} by ID so the handles, drag and double-click
    * gestures keep targeting the same logical link across save round-trips
    * (otherwise the edited bendpoint would appear to vanish after the 1-second
    * save timer fires).</p>
    */
   public void setContent(NetworkMapPage page)
   {
      this.content = page;
      // Re-resolve cached link refs against the new page (server saves replace instances).
      editedLink = relinkInPage(editedLink, page);
      if (editedLink == null)
         draggingBendpointIndex = -1;
      currentLink = relinkInPage(currentLink, page);
      rebuildObjectList();
      tryApplyInitialView();
      redraw();
   }

   /**
    * Look up the link in {@code page} that has the same id as {@code link} and
    * return that instance, or {@code null} if no such link exists in {@code
    * page}. {@code null} input passes through unchanged.
    */
   private static NetworkMapLink relinkInPage(NetworkMapLink link, NetworkMapPage page)
   {
      if ((link == null) || (page == null))
         return null;
      long id = link.getId();
      for(NetworkMapLink l : page.getLinks())
      {
         if (l.getId() == id)
            return l;
      }
      return null;
   }

   /**
    * Request that the viewer position itself according to {@code mode} once
    * its viewport and content are ready. Call this after the map's
    * {@code NetworkMap} object has been received and again whenever the
    * canvas type is freshly switched to GEOGRAPHICAL — the request is
    * one-shot, so it won't fight the user's manual pan / zoom afterwards.
    *
    * <p>Drops the current content and placed-object list so the FIT_TO_SCREEN
    * gate in {@link #tryApplyInitialView} treats the viewer as "not ready
    * yet". Without this, navigating MapA → MapB (or canvas-toggle round-trips)
    * would let the new request's FIT pass compute the bbox from the previous
    * map's placed objects before the async {@code setContent} for MapB lands.</p>
    */
   public void setInitialView(MapInitialViewMode mode, GeoLocation backgroundLocation, int backgroundZoom)
   {
      this.pendingInitialMode = mode;
      this.pendingBgLocation = backgroundLocation;
      this.pendingBgZoom = backgroundZoom;
      this.content = null;
      this.placedObjects.clear();
      tryApplyInitialView();
   }

   private void tryApplyInitialView()
   {
      if (pendingInitialMode == null)
         return;
      org.eclipse.swt.graphics.Rectangle client = getClientArea();
      if ((client.width <= 0) || (client.height <= 0))
         return; // viewport not laid out yet — wait for controlResized
      if ((pendingInitialMode == MapInitialViewMode.FIT_TO_SCREEN) && (content == null))
         return; // need placed-objects list to compute the bbox

      org.netxms.nxmc.modules.worldmap.tools.MapAccessor target;
      if (pendingInitialMode == MapInitialViewMode.FIT_TO_SCREEN)
      {
         target = computeFitToScreen(40);
      }
      else
      {
         double lat = (pendingBgLocation != null) ? pendingBgLocation.getLatitude() : 0.0;
         double lon = (pendingBgLocation != null) ? pendingBgLocation.getLongitude() : 0.0;
         int zoom = (pendingBgZoom > 0) ? pendingBgZoom : 3;
         target = new org.netxms.nxmc.modules.worldmap.tools.MapAccessor(lat, lon, zoom);
      }

      pendingInitialMode = null;
      pendingBgLocation = null;
      if (target != null)
         showMap(target);
   }

   /**
    * Compute the smallest zoom level for which the bounding box of all placed
    * objects fits inside {@code viewportSize} (with some margin). Returns
    * {@link MapAccessor#MAX_MAP_ZOOM} when there is one or no point. Falls
    * back to a sensible default when the viewport size is not known yet.
    */
   public org.netxms.nxmc.modules.worldmap.tools.MapAccessor computeFitToScreen(int marginPx)
   {
      if (placedObjects.isEmpty())
         return null;

      double minLat = Double.POSITIVE_INFINITY;
      double maxLat = Double.NEGATIVE_INFINITY;
      double[] lons = new double[placedObjects.size()];
      int i = 0;
      for(AbstractObject o : placedObjects)
      {
         GeoLocation g = o.getGeolocation();
         double lat = g.getLatitude();
         double lon = g.getLongitude();
         if (lat < minLat) minLat = lat;
         if (lat > maxLat) maxLat = lat;
         lons[i++] = lon;
      }
      // Find the bounding longitude range along the *shorter* arc around the
      // globe. Naive min/max would give a 358° span — centered at 0° — for
      // nodes at +179° and -179°, even though they're 2° apart across the
      // antimeridian. Sort the longitudes, find the largest gap between
      // consecutive samples (including the wrap-around gap), and let the
      // bbox span the complement of that gap.
      java.util.Arrays.sort(lons);
      double lonSpan;
      double centerLon;
      if (lons.length == 1)
      {
         lonSpan = 0;
         centerLon = lons[0];
      }
      else
      {
         double wrapGap = (lons[0] + 360.0) - lons[lons.length - 1];
         double maxGap = wrapGap;
         int gapAfter = lons.length - 1; // -1 means the wrap gap is largest
         for(int j = 0; j < lons.length - 1; j++)
         {
            double gap = lons[j + 1] - lons[j];
            if (gap > maxGap)
            {
               maxGap = gap;
               gapAfter = j;
            }
         }
         double startLon, endLon;
         if (gapAfter == lons.length - 1)
         {
            // Largest gap is the wrap; bbox is the simple east-going span.
            startLon = lons[0];
            endLon = lons[lons.length - 1];
         }
         else
         {
            // Bbox crosses the antimeridian: it runs east from the point right
            // after the gap, wraps through ±180°, and ends at the point right
            // before the gap (represented here as +360 for arithmetic).
            startLon = lons[gapAfter + 1];
            endLon = lons[gapAfter] + 360.0;
         }
         lonSpan = endLon - startLon;
         centerLon = (startLon + endLon) / 2.0;
         if (centerLon > 180.0) centerLon -= 360.0;
      }
      double centerLat = (minLat + maxLat) / 2.0;

      org.eclipse.swt.graphics.Rectangle client = getClientArea();
      int viewportW = Math.max(1, client.width - marginPx * 2);
      int viewportH = Math.max(1, client.height - marginPx * 2);

      // Single point or zero-extent bbox: pick a comfortable street-level zoom.
      if ((minLat == maxLat) && (lonSpan == 0))
         return new org.netxms.nxmc.modules.worldmap.tools.MapAccessor(centerLat, centerLon, 12);

      // Walk zoom levels top-down; the first zoom at which the bbox in pixel
      // space fits in the viewport wins. Width is computed from lonSpan
      // directly (linear in Mercator), height via the projection of the
      // lat range (non-linear, so we still go through coordinateToDisplay).
      for(int zoom = org.netxms.nxmc.modules.worldmap.tools.MapAccessor.MAX_MAP_ZOOM;
          zoom >= org.netxms.nxmc.modules.worldmap.tools.MapAccessor.MIN_MAP_ZOOM; zoom--)
      {
         double worldWidthPx = Math.pow(2, zoom) * 256.0;
         int dx = (int)Math.round(lonSpan / 360.0 * worldWidthPx);
         Point topLeft = GeoLocationCache.coordinateToDisplay(new GeoLocation(maxLat, centerLon), zoom);
         Point bottomRight = GeoLocationCache.coordinateToDisplay(new GeoLocation(minLat, centerLon), zoom);
         int dy = Math.abs(bottomRight.y - topLeft.y);
         if ((dx <= viewportW) && (dy <= viewportH))
            return new org.netxms.nxmc.modules.worldmap.tools.MapAccessor(centerLat, centerLon, zoom);
      }
      return new org.netxms.nxmc.modules.worldmap.tools.MapAccessor(centerLat, centerLon,
            org.netxms.nxmc.modules.worldmap.tools.MapAccessor.MIN_MAP_ZOOM);
   }

   /**
    * Frame all placed objects in the viewport. No-op until the geo viewer has
    * been laid out and content has been set (we need both a viewport size and
    * the bbox of the placed objects).
    */
   public void zoomFit()
   {
      org.eclipse.swt.graphics.Rectangle client = getClientArea();
      if ((client.width <= 0) || (client.height <= 0) || placedObjects.isEmpty())
         return;
      org.netxms.nxmc.modules.worldmap.tools.MapAccessor target = computeFitToScreen(40);
      if (target != null)
         showMap(target);
   }

   /**
    * Map-level defaults used for links whose color source is
    * {@code COLOR_SOURCE_DEFAULT} or whose own width/style is unset.
    *
    * @param defaultLinkColorInt encoded link color (RGB packed into int), or
    *        {@code -1} for "no map-level default". Resolved through the
    *        viewer's {@code colorCache} so the SWT {@link Color} resource is
    *        owned by the cache and disposed with the widget — passing a raw
    *        {@code new Color(...)} per call (as an older revision did) leaked
    *        a handle on every map-options update.
    */
   public void setLinkDefaults(int defaultLinkColorInt, int defaultLinkColorSource, int defaultLinkWidth, int defaultLinkStyle)
   {
      this.defaultLinkColor = (defaultLinkColorInt >= 0)
            ? colorCache.create(org.netxms.nxmc.tools.ColorConverter.rgbFromInt(defaultLinkColorInt))
            : null;
      this.defaultLinkColorSource = defaultLinkColorSource;
      this.defaultLinkWidth = defaultLinkWidth;
      this.defaultLinkStyle = (defaultLinkStyle >= 1 && defaultLinkStyle <= 5) ? defaultLinkStyle : 1;
   }

   /**
    * Map-level default routing used to resolve links whose own routing is
    * {@code ROUTING_DEFAULT}. Call this from the owning view whenever the
    * {@code NetworkMap} object's {@code defaultLinkRouting} changes.
    */
   public void setDefaultLinkRouting(int defaultLinkRouting)
   {
      this.defaultLinkRouting = defaultLinkRouting;
   }

   /**
    * Install the host's link-change callback. The viewer notifies it through
    * {@link FigureChangeCallback#onLinkChange(NetworkMapLink)} after every
    * persisted bendpoint operation (drag commit / add / delete) so the host
    * can save the map. {@code onMove} is unused on the geo canvas (object
    * positions are world-anchored, never widget-relative).
    */
   public void setLinkChangeCallback(org.netxms.nxmc.modules.networkmaps.widgets.helpers.FigureChangeCallback callback)
   {
      this.linkChangeCallback = callback;
   }

   /**
    * Begin or end interactive bendpoint editing. Pass the link to edit (it
    * must be present in the current map page) or {@code null} to leave edit
    * mode. Switching links cancels any in-progress drag. The viewer redraws
    * once to refresh the handle overlay.
    */
   public void setEditedLink(NetworkMapLink link)
   {
      if (this.editedLink == link)
         return;
      this.editedLink = link;
      this.draggingBendpointIndex = -1;
      redraw();
   }

   /**
    * @return the link currently in bendpoint-edit mode, or {@code null}
    */
   public NetworkMapLink getEditedLink()
   {
      return editedLink;
   }

   /**
    * Toggle rendering of the direction arrow at each link's target end.
    */
   public void setShowLinkDirection(boolean showLinkDirection)
   {
      this.showLinkDirection = showLinkDirection;
   }

   /**
    * Toggle whether links are drawn at all. Mirrors the
    * {@code Hide links} action on the Zest canvas.
    */
   public void setLinksVisible(boolean linksVisible)
   {
      this.linksVisible = linksVisible;
   }

   /**
    * Toggle whether link labels (connector names, link name, DCI values)
    * are drawn. Mirrors the {@code Hide link labels} action on the Zest canvas.
    */
   public void setLinkLabelsVisible(boolean linkLabelsVisible)
   {
      this.linkLabelsVisible = linkLabelsVisible;
   }

   /**
    * Count of NetworkMapObject elements whose referenced object has no
    * geolocation (and is therefore not drawn). Cached during
    * {@link #rebuildObjectList} so callers don't pay for a second walk of
    * {@code content.getElements()} (each iteration does {@code findObjectById}).
    */
   public int getUnplacedObjectCount()
   {
      return unplacedObjectCount;
   }

   private int unplacedObjectCount;
   // Resolved at rebuild time so drawLinks can look up pin positions by
   // element id (cheap) instead of resolving link.getElement() through the
   // page's getElement() + instanceof + getObjectId() chain on every paint.
   private final Map<Long, Long> objectIdByElementId = new HashMap<>();

   private void rebuildObjectList()
   {
      placedObjects.clear();
      objectIdByElementId.clear();
      unplacedObjectCount = 0;
      if (content == null)
      {
         notifyUnplacedCount(0);
         return;
      }
      NXCSession session = Registry.getSession();
      for(NetworkMapElement element : content.getElements())
      {
         if (!(element instanceof NetworkMapObject))
            continue;
         long objectId = ((NetworkMapObject)element).getObjectId();
         AbstractObject o = session.findObjectById(objectId);
         if (o == null)
            continue;
         if (hasUsableLocation(o))
         {
            placedObjects.add(o);
            objectIdByElementId.put(element.getId(), objectId);
         }
         else
         {
            unplacedObjectCount++;
         }
      }
      notifyUnplacedCount(unplacedObjectCount);
   }

   /**
    * Install a listener that receives the count of map elements whose
    * underlying object has no usable geolocation each time the placed-object
    * list is rebuilt. Used by the host view to surface the count as a
    * standard message-area entry instead of a translucent overlay.
    */
   public void setUnplacedCountListener(java.util.function.IntConsumer listener)
   {
      this.unplacedCountListener = listener;
      lastReportedUnplacedCount = -1; // force initial fire when content next rebuilt
      if (content != null)
         notifyUnplacedCount(unplacedObjectCount);
   }

   private void notifyUnplacedCount(int count)
   {
      if (count == lastReportedUnplacedCount)
         return;
      lastReportedUnplacedCount = count;
      if (unplacedCountListener != null)
         unplacedCountListener.accept(count);
   }

   private static boolean hasUsableLocation(AbstractObject o)
   {
      GeoLocation g = o.getGeolocation();
      return (g != null) && (g.getType() != GeoLocation.UNSET);
   }

   @Override
   protected void onMapLoad()
   {
      // Objects come from the map page, not from the geo cache.
   }

   @Override
   protected void onCacheChange(AbstractObject object, GeoLocation prevLocation)
   {
      // A node we're displaying may have gained or lost its geolocation,
      // or been added/removed. Rebuild the placed list when it touches us.
      if (content == null)
         return;
      if (content.findObjectElement(object.getObjectId()) != null)
      {
         rebuildObjectList();
         redraw();
      }
   }

   @Override
   protected void drawContent(GC gc, GeoLocation currentLocation, int imgW, int imgH, int verticalOffset)
   {
      objectIcons.clear();
      linkGeometries.clear();
      if (placedObjects.isEmpty())
         return;

      final Point centerXY = GeoLocationCache.coordinateToDisplay(currentLocation, accessor.getZoom());

      // Pass 1: resolve tip pixel positions for every placed object. We index
      // by both objectId (used by drawPin & buildEditedLinkGeometry) and
      // elementId (used by drawLinks) so the per-link content.getElement()
      // lookup goes away on the paint hot path.
      Map<Long, Point> objectPixels = new HashMap<>(placedObjects.size() * 2);
      Map<Long, Point> elementPixels = new HashMap<>(placedObjects.size() * 2);
      for(AbstractObject object : placedObjects)
      {
         Point xy = GeoLocationCache.coordinateToDisplay(object.getGeolocation(), accessor.getZoom());
         int px = imgW / 2 + (xy.x - centerXY.x);
         int py = imgH / 2 + (xy.y - centerXY.y) + verticalOffset;
         Point pt = new Point(px, py);
         objectPixels.put(object.getObjectId(), pt);
      }
      // objectIdByElementId is maintained by rebuildObjectList — invert into
      // elementPixels in a single pass through its entries.
      for(Map.Entry<Long, Long> e : objectIdByElementId.entrySet())
      {
         Point pt = objectPixels.get(e.getValue());
         if (pt != null)
            elementPixels.put(e.getKey(), pt);
      }

      // Pass 1b: project the edited link's geometry to pixels regardless of
      // whether the line will be drawn — handle drawing / hit-testing must
      // work even when "Hide links" is on or the endpoints are off-screen.
      editedLinkGeometry = (editedLink != null)
            ? buildEditedLinkGeometry(centerXY, imgW, imgH, verticalOffset, elementPixels)
            : null;

      // Pass 2: draw link polylines (lines + arrows only — labels deferred).
      if (linksVisible)
         drawLinks(gc, currentLocation, centerXY, imgW, imgH, verticalOffset, elementPixels);

      // Pass 3: draw pins on top of links.
      for(AbstractObject object : placedObjects)
      {
         Point p = objectPixels.get(object.getObjectId());
         drawPin(gc, p.x, p.y, object);
      }

      // Pass 4: link labels (connector names, link name, DCI values) on top of
      // pins. The Zest canvas naturally layers labels above pins via the
      // draw2d figure z-order; we recreate that here so short links don't
      // have their middle DCI / traffic labels hidden behind the pin pills.
      if (linksVisible && linkLabelsVisible)
         drawLinkLabelsPass(gc);

      // Pass 5: bendpoint handles for the link being edited (if any). Drawn
      // last so they sit above pins, links and labels.
      if (editedLink != null)
         drawBendpointHandles(gc, centerXY, imgW, imgH, verticalOffset);
   }

   /**
    * Render the labels for every link drawn in pass 2 — connector names,
    * link name, CENTER / OBJECT1 / OBJECT2 DCI values. Uses the same polyline
    * vertices captured in {@link #linkGeometries} during pass 2 so labels
    * land exactly where the line ran. Called after pins to ensure the labels
    * are not obscured by pin pills.
    */
   private void drawLinkLabelsPass(GC gc)
   {
      NXCSession session = Registry.getSession();
      for(LinkGeometry g : linkGeometries)
      {
         List<Point> bends = null;
         if (g.xs.length > 2)
         {
            bends = new ArrayList<>(g.xs.length - 2);
            for(int i = 1; i < g.xs.length - 1; i++)
               bends.add(new Point(g.xs[i], g.ys[i]));
         }
         Point src = new Point(g.xs[0], g.ys[0]);
         Point dst = new Point(g.xs[g.xs.length - 1], g.ys[g.ys.length - 1]);
         // Reuse the color resolved in drawLinks — second call would walk the
         // session's object table again under a global lock.
         GeoLinkRenderer.drawLinkLabels(gc, src, dst, g.link, session, dciValueProvider, colorCache,
               g.cachedColor, bends, objectLabelFont);
      }
   }

   /**
    * Draw a small rounded-rectangle handle at each projected geo bendpoint of
    * the currently-edited link.
    */
   private void drawBendpointHandles(GC gc, Point centerXY, int imgW, int imgH, int verticalOffset)
   {
      double[][] geo = editedLink.getGeoBendPoints();
      if ((geo == null) || (geo.length == 0))
         return;
      // editedLinkFanOffset is the same perpendicular shift applied to the
      // edited link's polyline in drawLinks; mirror it here so handles stay
      // anchored on the visible line for parallel-link groups.
      int fanX = (int)Math.round(editedLinkFanOffset[0]);
      int fanY = (int)Math.round(editedLinkFanOffset[1]);
      Color oldFg = gc.getForeground();
      Color oldBg = gc.getBackground();
      int oldLineWidth = gc.getLineWidth();
      gc.setLineWidth(1);
      for(double[] ll : geo)
      {
         if ((ll == null) || (ll.length < 2))
            continue;
         Point xy = GeoLocationCache.coordinateToDisplay(new GeoLocation(ll[0], ll[1]), accessor.getZoom());
         int px = imgW / 2 + (xy.x - centerXY.x) + fanX;
         int py = imgH / 2 + (xy.y - centerXY.y) + verticalOffset + fanY;
         gc.setBackground(HANDLE_FILL);
         gc.fillRoundRectangle(px - HANDLE_SIZE / 2, py - HANDLE_SIZE / 2, HANDLE_SIZE, HANDLE_SIZE, 4, 4);
         gc.setForeground(HANDLE_BORDER);
         gc.drawRoundRectangle(px - HANDLE_SIZE / 2, py - HANDLE_SIZE / 2, HANDLE_SIZE, HANDLE_SIZE, 4, 4);
      }
      gc.setForeground(oldFg);
      gc.setBackground(oldBg);
      gc.setLineWidth(oldLineWidth);
   }

   /**
    * Build a pixel-space {@link LinkGeometry} for the currently-edited link:
    * src pin → projected geo bendpoints → dst pin. Returns {@code null} when
    * either endpoint is missing (object removed, geolocation lost). The
    * geometry is independent of whether the link line is rendered, so handle
    * hit-testing keeps working with "Hide links" or off-screen endpoints.
    */
   private LinkGeometry buildEditedLinkGeometry(Point centerXY, int imgW, int imgH, int verticalOffset,
         Map<Long, Point> elementPixels)
   {
      editedLinkFanOffset = NO_FAN_OFFSET;
      if ((content == null) || (editedLink == null))
         return null;
      Point p1 = elementPixels.get(editedLink.getElement1());
      Point p2 = elementPixels.get(editedLink.getElement2());
      if ((p1 == null) || (p2 == null))
         return null;
      // Mirror the fan-out applied to the same link in drawLinks so the
      // bendpoint handles, segment hit-test, and the visible polyline all
      // share one polyline. The reverse offset is used by
      // updateBendpointDuringDrag and insertBendpoint to map shifted click
      // pixels back to the link's true lat/lon.
      editedLinkFanOffset = computeFanOffset(editedLink, p1, p2);
      Point p1f = shiftPoint(p1, editedLinkFanOffset);
      Point p2f = shiftPoint(p2, editedLinkFanOffset);
      List<Point> bendPixels = projectGeoBendPoints(editedLink, centerXY, imgW, imgH, verticalOffset);
      List<Point> bendsShifted = shiftBends(bendPixels, editedLinkFanOffset);
      return buildLinkGeometry(editedLink, p1f, p2f, bendsShifted);
   }

   /**
    * Hit-test the currently-edited link's bendpoint handles against
    * widget-relative point {@code p}. Walks in reverse order so the topmost
    * handle wins on overlap.
    *
    * @return geo-bendpoint index, or -1 when no handle is under {@code p} (or
    *         when no link is being edited)
    */
   private int hitBendpointHandle(Point p)
   {
      if ((editedLink == null) || (editedLinkGeometry == null))
         return -1;
      double[][] geo = editedLink.getGeoBendPoints();
      if ((geo == null) || (geo.length == 0))
         return -1;
      // xs[0]/ys[0] is src, xs[N+1]/ys[N+1] is dst, intermediate are bends
      LinkGeometry g = editedLinkGeometry;
      for(int i = g.xs.length - 2; i >= 1; i--)
      {
         int dx = g.xs[i] - p.x;
         int dy = g.ys[i] - p.y;
         if (dx * dx + dy * dy <= HANDLE_HIT_TOLERANCE * HANDLE_HIT_TOLERANCE)
            return i - 1;
      }
      return -1;
   }

   private void drawLinks(GC gc, GeoLocation currentLocation, Point centerXY, int imgW, int imgH, int verticalOffset,
         Map<Long, Point> elementPixels)
   {
      if (content == null)
         return;
      Rectangle client = getClientArea();
      NXCSession session = Registry.getSession();
      for(NetworkMapLink link : content.getLinks())
      {
         // elementPixels is keyed by NetworkMapElement.id, populated only for
         // elements whose object resolved + has a usable location. A missing
         // entry covers all the "skip this link" reasons in one map lookup
         // (non-object element, removed/un-located object).
         Point p1 = elementPixels.get(link.getElement1());
         Point p2 = elementPixels.get(link.getElement2());
         if ((p1 == null) || (p2 == null))
            continue;

         // Fan-out shift for parallel links: shift the whole polyline
         // (endpoints + bendpoints) by the same perpendicular offset so links
         // sharing an endpoint pair render as N visible parallel lines instead
         // of stacking. Pin pixel positions stay unchanged — only the line
         // geometry is offset.
         double[] fan = computeFanOffset(link, p1, p2);
         Point p1f = shiftPoint(p1, fan);
         Point p2f = shiftPoint(p2, fan);

         if (!GeoLinkRenderer.hasVisibleEndpoint(p1f, p2f, client.width, client.height))
            continue; // both endpoints off-screen — rule: skip

         List<Point> bendPixels = computeIntermediatePoints(link, p1, p2, centerXY, imgW, imgH, verticalOffset);
         List<Point> bendsShifted = shiftBends(bendPixels, fan);
         // Resolve the line color once per link per paint and stash it on the
         // geometry — drawLinkLabelsPass reuses it instead of running
         // LinkStylingHelper.resolveLinkColor a second time (each resolution
         // calls N session.findObjectById under a global synchronized lock).
         Color lineColor = GeoLinkRenderer.resolveLineColor(link, session, dciValueProvider, colorCache,
               defaultLinkColor, defaultLinkColorSource);
         // Labels are deferred to a post-pins pass via drawLinkLabelsPass —
         // tell drawLink to render only the line + arrow here.
         GeoLinkRenderer.drawLinkWithColor(gc, p1f, p2f, link, session, dciValueProvider, colorCache,
               lineColor, defaultLinkWidth, defaultLinkStyle, bendsShifted,
               showLinkDirection, false, objectLabelFont);
         LinkGeometry g = buildLinkGeometry(link, p1f, p2f, bendsShifted);
         g.cachedColor = lineColor;
         linkGeometries.add(g);
      }
   }

   /**
    * Perpendicular fan-out offset for a parallel link in pixel space. Returns
    * {@link #NO_FAN_OFFSET} when this link is the only one between its endpoint
    * pair (so the original pin-to-pin chord is preserved) or the chord
    * degenerates to a point.
    *
    * <p>{@link NetworkMapLink#getDuplicateCount()} is {@code N − 1} (siblings,
    * not including this link) and {@link NetworkMapLink#getPosition()} is the
    * 0-based index within the group — both populated by
    * {@link NetworkMapPage#addLink} on insertion. Spacing is a fixed pixel
    * constant so links stay readable at every zoom level.</p>
    */
   private static double[] computeFanOffset(NetworkMapLink link, Point p1, Point p2)
   {
      int dc = link.getDuplicateCount();
      if (dc <= 0)
         return NO_FAN_OFFSET;
      double vx = p2.x - p1.x;
      double vy = p2.y - p1.y;
      double len = Math.sqrt(vx * vx + vy * vy);
      if (len < 1e-9)
         return NO_FAN_OFFSET;
      // Counter-clockwise 90° rotation of the unit chord vector.
      double nx = -vy / len;
      double ny = vx / len;
      // dc + 1 is the group size N; centring on dc/2 gives a symmetric
      // [-(N-1)/2 .. +(N-1)/2] range of multipliers across positions [0..N-1].
      double shift = (link.getPosition() - dc / 2.0) * LINK_FAN_SPACING_PX;
      return new double[] { nx * shift, ny * shift };
   }

   private static Point shiftPoint(Point p, double[] offset)
   {
      if ((offset[0] == 0.0) && (offset[1] == 0.0))
         return p;
      return new Point((int)Math.round(p.x + offset[0]), (int)Math.round(p.y + offset[1]));
   }

   private static List<Point> shiftBends(List<Point> bends, double[] offset)
   {
      if ((bends == null) || ((offset[0] == 0.0) && (offset[1] == 0.0)))
         return bends;
      List<Point> result = new ArrayList<>(bends.size());
      for(Point b : bends)
         result.add(new Point((int)Math.round(b.x + offset[0]), (int)Math.round(b.y + offset[1])));
      return result;
   }

   /**
    * Resolve the link's effective routing (substituting the map-level default
    * for {@code ROUTING_DEFAULT}) and produce the intermediate pixel-space
    * points used by both the renderer and the hit-test geometry:
    * <ul>
    *   <li>{@code ROUTING_BENDPOINTS}: project user-authored geo bendpoints.</li>
    *   <li>{@code ROUTING_MANHATTAN}: synthesize an L-shape (two corners) so
    *       the link bends along the dominant axis first.</li>
    *   <li>{@code ROUTING_DIRECT} and unknown values: {@code null} (straight line).</li>
    * </ul>
    */
   private List<Point> computeIntermediatePoints(NetworkMapLink link, Point p1, Point p2,
         Point centerXY, int imgW, int imgH, int verticalOffset)
   {
      int routing = link.getRouting();
      if (routing == NetworkMapLink.ROUTING_DEFAULT)
         routing = defaultLinkRouting;
      switch(routing)
      {
         case NetworkMapLink.ROUTING_BENDPOINTS:
            return projectGeoBendPoints(link, centerXY, imgW, imgH, verticalOffset);
         case NetworkMapLink.ROUTING_MANHATTAN:
            return computeManhattanBends(p1, p2);
         default:
            return null;
      }
   }

   /**
    * Two-bend Manhattan polyline matching draw2d's
    * {@code ManhattanConnectionRouter}: route along the dominant axis first.
    * Returns {@code null} if {@code src} and {@code dst} coincide.
    */
   private static List<Point> computeManhattanBends(Point src, Point dst)
   {
      int dx = dst.x - src.x;
      int dy = dst.y - src.y;
      if ((dx == 0) && (dy == 0))
         return null;
      List<Point> bends = new ArrayList<>(2);
      if (Math.abs(dx) >= Math.abs(dy))
      {
         int midX = (src.x + dst.x) / 2;
         bends.add(new Point(midX, src.y));
         bends.add(new Point(midX, dst.y));
      }
      else
      {
         int midY = (src.y + dst.y) / 2;
         bends.add(new Point(src.x, midY));
         bends.add(new Point(dst.x, midY));
      }
      return bends;
   }

   private static LinkGeometry buildLinkGeometry(NetworkMapLink link, Point p1, Point p2, List<Point> bendPixels)
   {
      int n = (bendPixels != null) ? bendPixels.size() : 0;
      int[] xs = new int[n + 2];
      int[] ys = new int[n + 2];
      xs[0] = p1.x;
      ys[0] = p1.y;
      for(int i = 0; i < n; i++)
      {
         Point bp = bendPixels.get(i);
         xs[i + 1] = bp.x;
         ys[i + 1] = bp.y;
      }
      xs[n + 1] = p2.x;
      ys[n + 1] = p2.y;
      return new LinkGeometry(link, xs, ys);
   }

   private List<Point> projectGeoBendPoints(NetworkMapLink link, Point centerXY, int imgW, int imgH, int verticalOffset)
   {
      double[][] geo = link.getGeoBendPoints();
      if ((geo == null) || (geo.length == 0))
         return null;
      List<Point> out = new ArrayList<>(geo.length);
      for(double[] ll : geo)
      {
         if ((ll == null) || (ll.length < 2))
            continue;
         Point xy = GeoLocationCache.coordinateToDisplay(new GeoLocation(ll[0], ll[1]), accessor.getZoom());
         int px = imgW / 2 + (xy.x - centerXY.x);
         int py = imgH / 2 + (xy.y - centerXY.y) + verticalOffset;
         out.add(new Point(px, py));
      }
      return out;
   }

   /**
    * Draw a single pin at pixel (x, y). The pin's pixel size is constant
    * across zoom levels.
    */
   private void drawPin(GC gc, int x, int y, AbstractObject object)
   {
      boolean selected = isObjectSelected(object);

      Image image = labelProvider.getImage(object);
      if (image == null)
         image = SharedIcons.IMG_UNKNOWN_OBJECT;

      // getBounds() reads cached width/height; getImageData() copies the
      // entire pixel array out of the native handle into a fresh ImageData.
      // Repaint hot path — for 200 pins at 60 fps drag, that's 24k pointless
      // ImageData allocations per second.
      Rectangle imageBounds = image.getBounds();
      int w = imageBounds.width + LABEL_X_MARGIN * 2;
      int h = imageBounds.height + LABEL_Y_MARGIN * 2;
      Rectangle rect = new Rectangle(x - w / 2 - 1, y - LABEL_ARROW_HEIGHT - h, w, h);

      Color bgColor = selected ? PIN_SELECTION_COLOR
            : ColorConverter.adjustColor(StatusDisplayInfo.getStatusColor(object.getStatus()), new RGB(0, 0, 0), 0.2f, colorCache);

      gc.setBackground(bgColor);
      gc.fillArc(rect.x, rect.y, rect.width, rect.height, 0, 360);
      gc.setLineWidth(2);
      gc.setForeground(INNER_BORDER_COLOR);
      gc.drawArc(rect.x + 4, rect.y + 4, rect.width - 8, rect.height - 8, 0, 360);
      gc.setLineWidth(1);
      gc.setForeground(BORDER_COLOR);

      final int[] arrow = new int[] { rect.x + rect.width / 2 - 3, rect.y + rect.height - 1, x, y, rect.x + rect.width / 2 + 4, rect.y + rect.height - 1 };
      gc.fillPolygon(arrow);
      gc.setForeground(BORDER_COLOR);

      gc.drawImage(image, rect.x + LABEL_X_MARGIN, rect.y + LABEL_Y_MARGIN);

      if (showObjectNames)
      {
         String text = object.getObjectName();
         if (!object.getAlias().isEmpty())
            text += "\n" + object.getAlias();

         gc.setFont(objectLabelFont);
         Point textSize = gc.textExtent(text);
         gc.setAlpha(128);
         gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_BLACK));
         gc.fillRoundRectangle(rect.x + rect.width + 3, rect.y + rect.height / 2 - textSize.y / 2 - 2, textSize.x + 6, textSize.y + 4, 4, 4);
         gc.setAlpha(255);
         Color textColor = ColorConverter.adjustColor(selected ? PIN_SELECTION_COLOR : StatusDisplayInfo.getStatusColor(object.getStatus()),
               new RGB(255, 255, 255), 0.6f, colorCache);
         gc.setForeground(textColor);
         gc.drawText(text, rect.x + rect.width + 6, rect.y + rect.height / 2 - textSize.y / 2, true);
      }

      objectIcons.add(new ObjectIcon(object, rect, x, y));
   }

   /**
    * @return true when {@code object} is part of the current selection (single
    *         click, find-object navigation, or "Select all objects")
    */
   private boolean isObjectSelected(AbstractObject object)
   {
      if (object == null)
         return false;
      if ((currentObject != null) && (currentObject.getObjectId() == object.getObjectId()))
         return true;
      if (selection instanceof IStructuredSelection)
      {
         for(Object o : ((IStructuredSelection)selection).toList())
         {
            if ((o instanceof AbstractObject) && (((AbstractObject)o).getObjectId() == object.getObjectId()))
               return true;
         }
      }
      return false;
   }

   /**
    * Replace the selection with every placed object on the map and notify
    * listeners. Pins are repainted with the selection highlight on the next
    * redraw. Equivalent of {@code Select all objects} on the Zest canvas.
    */
   public void selectAllObjects()
   {
      List<AbstractObject> all = new ArrayList<>(placedObjects);
      currentObject = null;
      currentLink = null;
      selection = new StructuredSelection(all);
      fireSelectionChanged();
      redraw();
   }

   /**
    * Find a placed object whose visible label matches {@code text} and centre
    * the map on it. Walks {@link #placedObjects} in iteration order and
    * advances from the currently-selected match (if any) so successive calls
    * step through hits. Returns {@code true} when a match is found, {@code
    * false} otherwise — the caller is expected to surface a "no match" hint.
    *
    * @param text       case-insensitive substring to match against the
    *                   object's display name and alias
    * @param forward    {@code true} to walk forwards, {@code false} backwards
    */
   public boolean findObjectByText(String text, boolean forward)
   {
      if ((text == null) || text.isEmpty() || placedObjects.isEmpty())
         return false;
      String needle = text.toUpperCase();
      int n = placedObjects.size();

      int startIndex;
      if (currentObject != null)
      {
         int idx = placedObjects.indexOf(currentObject);
         startIndex = (idx < 0) ? (forward ? 0 : n - 1) : (forward ? idx + 1 : idx - 1);
      }
      else
      {
         startIndex = forward ? 0 : n - 1;
      }
      if (startIndex >= n) startIndex = 0;
      else if (startIndex < 0) startIndex = n - 1;

      int i = startIndex;
      do
      {
         AbstractObject candidate = placedObjects.get(i);
         String haystack = candidate.getObjectName();
         if (haystack == null) haystack = "";
         String alias = candidate.getAlias();
         if ((alias != null) && !alias.isEmpty())
            haystack = haystack + "\n" + alias;
         if (haystack.toUpperCase().contains(needle))
         {
            currentObject = candidate;
            currentLink = null;
            selection = new StructuredSelection(candidate);
            fireSelectionChanged();
            // Centre on the matched object so it's visible after the find.
            GeoLocation loc = candidate.getGeolocation();
            if ((loc != null) && (loc.getType() != GeoLocation.UNSET))
            {
               accessor.setLatitude(loc.getLatitude());
               accessor.setLongitude(loc.getLongitude());
               showMap(accessor);
            }
            else
            {
               redraw();
            }
            return true;
         }
         i = forward ? (i + 1) : (i - 1);
         if (i >= n) i = 0;
         else if (i < 0) i = n - 1;
      }
      while(i != startIndex);
      return false;
   }

   @Override
   public AbstractObject getObjectAtPoint(Point p)
   {
      for(int i = objectIcons.size() - 1; i >= 0; i--)
      {
         ObjectIcon icon = objectIcons.get(i);
         if (icon.contains(p))
            return icon.object;
      }
      return null;
   }

   @Override
   public void mouseDown(MouseEvent e)
   {
      Point pt = new Point(e.x, e.y);
      if (e.button == 3)
      {
         rightClickPoint = pt;
         rightClickLinkHit = getLinkHit(pt);
      }

      // Bendpoint editing intercepts left-button presses on a handle (drag or
      // Ctrl+delete). Suppress the parent's pan setup and the subclass's
      // selection logic in that case so the gesture isn't confused with a
      // map drag or a click on the underlying link.
      if ((e.button == 1) && (editedLink != null))
      {
         int idx = hitBendpointHandle(pt);
         if (idx >= 0)
         {
            if ((e.stateMask & SWT.CONTROL) != 0)
            {
               removeBendpoint(idx);
            }
            else
            {
               draggingBendpointIndex = idx;
            }
            return;
         }
      }

      super.mouseDown(e);

      AbstractObject object = getObjectAtPoint(pt);
      NetworkMapLink link = (object == null) ? getLinkAtPoint(pt) : null;

      ISelection newSelection;
      if (object != null)
         newSelection = new StructuredSelection(object);
      else if (link != null)
         newSelection = new StructuredSelection(link);
      else
         newSelection = new StructuredSelection();

      if ((object != currentObject) || (link != currentLink))
      {
         currentObject = object;
         currentLink = link;
         selection = newSelection;
         fireSelectionChanged();
         redraw();
      }
   }

   @Override
   public void mouseMove(MouseEvent e)
   {
      if (draggingBendpointIndex >= 0)
      {
         updateBendpointDuringDrag(e.x, e.y);
         return;
      }
      updateHoverTooltip(new Point(e.x, e.y));
      super.mouseMove(e);
   }

   /**
    * Show a native tooltip over the pin currently under the cursor, mirroring
    * what the Zest canvas shows via {@link ObjectTooltip} (name, status, IPs).
    * Clears the tooltip when the cursor is not over a pin. The canvas is also
    * a regular SWT control, so we set its {@code toolTipText} property — the
    * platform handles show/hide timing and positioning.
    */
   private void updateHoverTooltip(Point pt)
   {
      AbstractObject hover = getObjectAtPoint(pt);
      String text = (hover != null) ? buildHoverText(hover) : null;
      // Avoid pointless string churn that would flicker the tooltip.
      String current = getToolTipText();
      if ((current == null) ? (text != null) : !current.equals(text))
         setToolTipText(text);
   }

   /**
    * Compose the multi-line tooltip text for {@code object}, matching the
    * field set shown by the Zest canvas's {@link ObjectTooltip}: object name,
    * status, (for Nodes) hardware/vendor, primary IP + MAC, (for APs)
    * model and radios, and comments. The platform tooltip is plain text, so
    * we can't show the icons and headings the figure-based tooltip uses, but
    * the content is the same.
    */
   private static String buildHoverText(AbstractObject object)
   {
      if (object == null)
         return null;
      StringBuilder sb = new StringBuilder();
      sb.append(object.getObjectName());
      sb.append('\n').append(StatusDisplayInfo.getStatusText(object.getStatus()));

      if (object instanceof org.netxms.client.objects.Node)
      {
         org.netxms.client.objects.Node node = (org.netxms.client.objects.Node)object;
         if ((node.getCapabilities() & org.netxms.client.objects.AbstractNode.NC_IS_ETHERNET_IP) != 0)
         {
            String hw = node.getHardwareProductName();
            String cip = node.getCipDeviceTypeName();
            if ((hw != null) && !hw.isEmpty())
               sb.append('\n').append(hw).append(" (").append(cip).append(')');
            else if ((cip != null) && !cip.isEmpty())
               sb.append('\n').append(cip);
         }
         else
         {
            String hw = node.getHardwareProductName();
            if ((hw != null) && !hw.isEmpty())
               sb.append('\n').append(hw);
         }
         String vendor = node.getHardwareVendor();
         if ((vendor != null) && !vendor.isEmpty())
            sb.append("\nVendor: ").append(vendor);

         org.netxms.base.InetAddressEx ip = node.getPrimaryIP();
         if ((ip != null) && ip.isValidAddress() && !ip.getAddress().isAnyLocalAddress())
         {
            sb.append('\n').append(ip.getHostAddress());
            org.netxms.base.MacAddress mac = node.getPrimaryMAC();
            if (mac != null)
               sb.append(" (").append(mac.toString()).append(')');
         }
      }
      else if (object instanceof org.netxms.client.objects.AccessPoint)
      {
         org.netxms.client.objects.AccessPoint ap = (org.netxms.client.objects.AccessPoint)object;
         String model = ap.getModel();
         if ((model != null) && !model.isEmpty())
            sb.append('\n').append(model);
         org.netxms.base.MacAddress mac = ap.getMacAddress();
         if (mac != null)
            sb.append('\n').append(mac.toString());
         for(org.netxms.client.topology.RadioInterface rif : ap.getRadios())
         {
            sb.append("\nRadio").append(rif.getIndex())
                  .append(" (").append(rif.getBSSID().toString()).append(")")
                  .append("\n\tChannel: ").append(rif.getChannel())
                  .append("\n\tTX power: ").append(rif.getPowerMW()).append(" mW");
         }
      }

      String comments = object.getComments();
      if ((comments != null) && !comments.isEmpty())
         sb.append('\n').append(comments);

      return sb.toString();
   }

   @Override
   public void mouseUp(MouseEvent e)
   {
      if ((e.button == 1) && (draggingBendpointIndex >= 0))
      {
         updateBendpointDuringDrag(e.x, e.y);
         draggingBendpointIndex = -1;
         notifyLinkChange();
         return;
      }
      super.mouseUp(e);
   }

   @Override
   public void mouseDoubleClick(MouseEvent e)
   {
      // Double-click on the edited link's polyline inserts a new bendpoint at
      // the click location (matching the Zest editor's gesture). Falls
      // through to the parent's zoom-in behaviour otherwise.
      if ((e.button == 1) && (editedLink != null))
      {
         Point pt = new Point(e.x, e.y);
         int seg = hitEditedLinkSegment(pt);
         if (seg >= 0)
         {
            insertBendpoint(seg, pt);
            return;
         }
      }
      super.mouseDoubleClick(e);
   }

   /**
    * Move the geo bendpoint at {@link #draggingBendpointIndex} to the
    * geographic coordinate currently under {@code (x, y)}. Updates the link's
    * geo-bendpoints array in place; the host is notified once on mouseUp.
    */
   private void updateBendpointDuringDrag(int x, int y)
   {
      if (editedLink == null)
         return;
      // Mouse position sits on the fanned polyline; subtract the fan offset
      // so the resulting lat/lon corresponds to the link's true (unfanned)
      // path. Without this, dragging a handle on a parallel link would walk
      // it sideways by the fan spacing every drag start.
      GeoLocation loc = getLocationAtPoint(unshiftMouse(x, y));
      if (editedLink.updateGeoBendPoint(draggingBendpointIndex, loc.getLatitude(), loc.getLongitude()))
         redraw();
   }

   /**
    * Convert a widget-relative mouse point on the (possibly fanned) edited
    * link's polyline back to the equivalent point on the link's true path —
    * used as the input to {@link #getLocationAtPoint} so coordinate
    * conversions stay 1:1 with the user's drag.
    */
   private Point unshiftMouse(int x, int y)
   {
      return new Point(x - (int)Math.round(editedLinkFanOffset[0]),
            y - (int)Math.round(editedLinkFanOffset[1]));
   }

   /**
    * Hit-test the edited link's segments only.
    *
    * @return segment index 0..N where N is the number of bendpoints, or -1
    *         if {@code p} is not on a segment of the edited link
    */
   private int hitEditedLinkSegment(Point p)
   {
      if ((editedLink == null) || (editedLinkGeometry == null))
         return -1;
      return editedLinkGeometry.hitSegment(p.x, p.y, SEGMENT_HIT_TOLERANCE);
   }

   /**
    * Insert a new geo bendpoint at the geographic coordinate of {@code pt}
    * into the edited link's bendpoints, at position {@code segmentIndex} of
    * the current polyline. No-op when the bendpoint cap is reached.
    */
   private void insertBendpoint(int segmentIndex, Point pt)
   {
      if (editedLink == null)
         return;
      // Click landed on the fanned polyline — un-shift so the new bendpoint's
      // lat/lon sits on the link's true path. See updateBendpointDuringDrag.
      GeoLocation loc = getLocationAtPoint(unshiftMouse(pt.x, pt.y));
      if (!editedLink.insertGeoBendPoint(segmentIndex, loc.getLatitude(), loc.getLongitude()))
         return;
      // Editing is only entered for BENDPOINTS-routed links in the host, but
      // be defensive: if a future caller invokes us with a different routing,
      // pin it to BENDPOINTS so the polyline reflects the new vertex.
      if (editedLink.getRouting() != NetworkMapLink.ROUTING_BENDPOINTS)
         editedLink.setRouting(NetworkMapLink.ROUTING_BENDPOINTS);
      notifyLinkChange();
      redraw();
   }

   /**
    * Remove the bendpoint at {@code index} from the edited link's
    * geo-bendpoints array. No-op when the index is out of range.
    */
   private void removeBendpoint(int index)
   {
      if ((editedLink == null) || !editedLink.removeGeoBendPoint(index))
         return;
      notifyLinkChange();
      redraw();
   }

   private void notifyLinkChange()
   {
      if ((linkChangeCallback != null) && (editedLink != null))
         linkChangeCallback.onLinkChange(editedLink);
   }

   /**
    * Convenience overload returning only the link, or {@code null} if no
    * link is under {@code p}. Bendpoint hits also count as link hits.
    */
   public NetworkMapLink getLinkAtPoint(Point p)
   {
      LinkHitInfo info = getLinkHit(p);
      return (info != null) ? info.link : null;
   }

   /**
    * Hit-test all rendered links against widget-relative point {@code p},
    * preferring bendpoint vertices over plain segments. Walks links in
    * reverse render order so the topmost link wins.
    *
    * @return the {@link LinkHitInfo} for the topmost hit link, or
    *         {@code null} if the point isn't on any link or bendpoint
    */
   public LinkHitInfo getLinkHit(Point p)
   {
      for(int i = linkGeometries.size() - 1; i >= 0; i--)
      {
         LinkGeometry g = linkGeometries.get(i);
         int bp = g.hitBendpoint(p.x, p.y, BENDPOINT_HIT_TOLERANCE);
         if (bp >= 0)
            return new LinkHitInfo(g.link, -1, bp);
         int seg = g.hitSegment(p.x, p.y, SEGMENT_HIT_TOLERANCE);
         if (seg >= 0)
            return new LinkHitInfo(g.link, seg, -1);
      }
      return null;
   }

   /**
    * {@link LinkHitInfo} captured at the most recent right-click, or
    * {@code null} if the most recent right-click was not on a link.
    */
   public LinkHitInfo getRightClickLinkHit()
   {
      return rightClickLinkHit;
   }

   /**
    * Geographic coordinate at the point of the most recent right-click on this
    * viewer, or {@code null} if no right-click has been seen yet. The view's
    * context-menu actions consult this so "Place node here" can resolve to the
    * lat/lon under the cursor.
    */
   public GeoLocation getRightClickLocation()
   {
      if (rightClickPoint == null)
         return null;
      return getLocationAtPoint(rightClickPoint);
   }

   /**
    * Callback invoked when objects are dropped onto the geo viewer. Receives
    * the list of dropped objects (already filtered to those allowed on a map)
    * and the geographic coordinate of the drop point.
    */
   public interface DropHandler
   {
      void onDrop(List<AbstractObject> objects, GeoLocation location);
   }

   /**
    * Install a {@link DropHandler} for drag-and-drop of objects from the
    * object browser. Only one handler is supported; calling this again
    * replaces the previous handler. Drops of non-{@link AbstractObject}
    * payloads or of objects that report {@code !isAllowedOnMap()} are
    * rejected.
    */
   public void addDropSupport(DropHandler handler)
   {
      this.dropHandler = handler;
      DropTarget target = new DropTarget(this, DND.DROP_COPY | DND.DROP_MOVE);
      target.setTransfer(new Transfer[] { LocalSelectionTransfer.getTransfer() });
      target.addDropListener(new DropTargetAdapter() {
         @Override
         public void dragEnter(DropTargetEvent event)
         {
            event.detail = validateTransfer() ? DND.DROP_COPY : DND.DROP_NONE;
         }

         @Override
         public void dragOver(DropTargetEvent event)
         {
            event.detail = validateTransfer() ? DND.DROP_COPY : DND.DROP_NONE;
         }

         @Override
         public void drop(DropTargetEvent event)
         {
            if (dropHandler == null)
               return;
            ISelection sel = LocalSelectionTransfer.getTransfer().getSelection();
            if (!(sel instanceof IStructuredSelection))
               return;
            List<AbstractObject> objs = new ArrayList<>();
            for(Object o : ((IStructuredSelection)sel).toList())
            {
               if ((o instanceof AbstractObject) && ((AbstractObject)o).isAllowedOnMap())
                  objs.add((AbstractObject)o);
            }
            if (objs.isEmpty())
               return;
            // event.x/event.y are display-relative; convert to widget coords.
            Point widgetPt = toControl(event.x, event.y);
            GeoLocation loc = getLocationAtPoint(widgetPt);
            dropHandler.onDrop(objs, loc);
         }
      });
   }

   private boolean validateTransfer()
   {
      ISelection sel = LocalSelectionTransfer.getTransfer().getSelection();
      if (!(sel instanceof IStructuredSelection))
         return false;
      for(Object o : ((IStructuredSelection)sel).toList())
      {
         if (!((o instanceof AbstractObject) && ((AbstractObject)o).isAllowedOnMap()))
            return false;
      }
      return true;
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
    * @see org.eclipse.jface.viewers.ISelectionProvider#removeSelectionChangedListener(org.eclipse.jface.viewers.ISelectionChangedListener)
    */
   @Override
   public void removeSelectionChangedListener(ISelectionChangedListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#getSelection()
    */
   @Override
   public ISelection getSelection()
   {
      return selection;
   }

   /**
    * @see org.eclipse.jface.viewers.ISelectionProvider#setSelection(org.eclipse.jface.viewers.ISelection)
    */
   @Override
   public void setSelection(ISelection selection)
   {
      this.selection = (selection != null) ? selection : new StructuredSelection();
   }

   /**
    * Programmatically select the given link. Mirrors what a left-click on
    * the link does: updates the current selection, fires
    * {@link ISelectionChangedListener}s, and triggers a redraw. The host
    * view's selection listener then enters bendpoint-edit mode.
    */
   public void selectLink(NetworkMapLink link)
   {
      currentObject = null;
      currentLink = link;
      selection = (link != null) ? new StructuredSelection(link) : new StructuredSelection();
      fireSelectionChanged();
      redraw();
   }

   /**
    * Programmatically select the given object's pin, or clear selection if
    * {@code object} is {@code null} or has no geolocation (not in
    * {@link #placedObjects}). Used when swapping canvas modes so an object
    * selected on the graph canvas carries over to the same pin here.
    *
    * @return true if the object was selected, false if the selection was
    *         cleared (caller can use this to decide whether to clear the
    *         counterpart selection on the graph canvas)
    */
   public boolean selectObject(AbstractObject object)
   {
      if ((object == null) || !placedObjects.contains(object))
      {
         currentObject = null;
         currentLink = null;
         selection = new StructuredSelection();
         fireSelectionChanged();
         redraw();
         return false;
      }
      currentObject = object;
      currentLink = null;
      selection = new StructuredSelection(object);
      fireSelectionChanged();
      redraw();
      return true;
   }

   private void fireSelectionChanged()
   {
      SelectionChangedEvent event = new SelectionChangedEvent(this, selection);
      for(ISelectionChangedListener l : selectionListeners)
         l.selectionChanged(event);
   }

   /**
    * Hit-testable pin icon.
    */
   private static final class ObjectIcon
   {
      final AbstractObject object;
      final Rectangle rect;
      final int pointX;
      final int pointY;

      ObjectIcon(AbstractObject object, Rectangle rect, int pointX, int pointY)
      {
         this.object = object;
         this.rect = rect;
         this.pointX = pointX;
         this.pointY = pointY;
      }

      boolean contains(Point p)
      {
         return rect.contains(p) || ((Math.abs(p.x - pointX) < 4) && (p.y >= rect.y + rect.height) && (p.y <= pointY));
      }
   }

   /**
    * Pixel-space polyline of a rendered link (endpoints + projected
    * bendpoints) used for hit-testing. Vertex 0 is the source endpoint,
    * vertex {@code xs.length - 1} is the destination, and intermediate
    * vertices are bendpoints in order.
    */
   private static final class LinkGeometry
   {
      final NetworkMapLink link;
      final int[] xs;
      final int[] ys;
      /**
       * Resolved line color stashed during the draw-lines pass and reused
       * by the post-pins label pass — without this, {@link LinkStylingHelper}
       * runs twice per paint (it does N {@code findObjectById} calls under
       * the session's global lock for every link). Lazily assigned by
       * {@link #drawLinks}; {@code null} before the first pass.
       */
      Color cachedColor;

      LinkGeometry(NetworkMapLink link, int[] xs, int[] ys)
      {
         this.link = link;
         this.xs = xs;
         this.ys = ys;
      }

      /**
       * @return logical bendpoint index (0..N-1, where N is the number of
       *         bendpoints) if {@code (x, y)} is within {@code tolerance}
       *         pixels of an existing bendpoint vertex, otherwise -1
       */
      int hitBendpoint(int x, int y, int tolerance)
      {
         int t2 = tolerance * tolerance;
         for(int i = 1; i < xs.length - 1; i++)
         {
            int dx = xs[i] - x;
            int dy = ys[i] - y;
            if (dx * dx + dy * dy <= t2)
               return i - 1;
         }
         return -1;
      }

      /**
       * @return segment index (0..N, where N is the number of bendpoints)
       *         of the first segment whose perpendicular distance to
       *         {@code (x, y)} is at most {@code tolerance} pixels, or -1
       *         if none. Segment {@code k} is the gap before bendpoint
       *         {@code k} (or before the destination endpoint when
       *         {@code k == N}).
       */
      int hitSegment(int x, int y, int tolerance)
      {
         double t = tolerance;
         for(int i = 1; i < xs.length; i++)
         {
            if (pointToSegmentDistance(x, y, xs[i - 1], ys[i - 1], xs[i], ys[i]) <= t)
               return i - 1;
         }
         return -1;
      }

      private static double pointToSegmentDistance(int px, int py, int x1, int y1, int x2, int y2)
      {
         double dx = x2 - x1;
         double dy = y2 - y1;
         double len2 = dx * dx + dy * dy;
         if (len2 == 0.0)
         {
            int ddx = px - x1;
            int ddy = py - y1;
            return Math.sqrt(ddx * ddx + ddy * ddy);
         }
         double t = ((px - x1) * dx + (py - y1) * dy) / len2;
         if (t < 0.0) t = 0.0;
         else if (t > 1.0) t = 1.0;
         double cx = x1 + t * dx;
         double cy = y1 + t * dy;
         double ex = px - cx;
         double ey = py - cy;
         return Math.sqrt(ex * ex + ey * ey);
      }
   }

   /**
    * Result of a link hit-test. Either {@link #segmentIndex} or
    * {@link #bendpointIndex} is non-negative (the other is {@code -1}):
    * a bendpoint hit takes precedence over a segment hit.
    */
   public static final class LinkHitInfo
   {
      public final NetworkMapLink link;
      /** Segment index 0..N (N = #bendpoints), or -1 if hit was on a bendpoint vertex. */
      public final int segmentIndex;
      /** Bendpoint index 0..N-1, or -1 if hit was on a plain segment. */
      public final int bendpointIndex;

      LinkHitInfo(NetworkMapLink link, int segmentIndex, int bendpointIndex)
      {
         this.link = link;
         this.segmentIndex = segmentIndex;
         this.bendpointIndex = bendpointIndex;
      }
   }
}
