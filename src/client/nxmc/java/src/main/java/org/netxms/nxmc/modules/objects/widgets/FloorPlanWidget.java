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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.constants.RoomElementType;
import org.netxms.client.constants.RoomGridLabels;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.Room;
import org.netxms.client.objects.configs.RoomPassiveElement;
import org.netxms.client.objects.configs.RoomPoint;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener;
import org.netxms.nxmc.modules.objects.views.AdHocRackView;
import org.netxms.nxmc.modules.objects.widgets.helpers.ElementSelectionListener;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanEditListener;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanGeometry;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanModel;
import org.netxms.nxmc.modules.objects.widgets.helpers.FloorPlanModel.RackPlacement;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.DragTrackingHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Room floor plan: outline, floor tile grid, optional backdrop image, passive elements and racks drawn to scale. Supports
 * viewing (zoom, pan, selection, navigation to rack view) and editing (drag / rotate / snap for racks and passive elements,
 * outline vertex editing, backdrop placement and calibration). Room coordinates are millimetres with Y axis pointing down;
 * rack and element rotation is clockwise around the reference corner, front face of a rack at zero rotation is its +Y edge.
 */
public class FloorPlanWidget extends Canvas implements PaintListener, MouseListener, MouseMoveListener, ImageUpdateListener
{
   /**
    * Outline vertex handle (selection target in edit mode)
    */
   public static class OutlineVertex
   {
      public final int index;

      OutlineVertex(int index)
      {
         this.index = index;
      }
   }

   private static final double MIN_SCALE = 0.002;   // pixels per mm
   private static final double MAX_SCALE = 0.5;
   private static final double ZOOM_STEP = 1.25;
   private static final int FIT_MARGIN = 24;
   private static final int HANDLE_SIZE = 7;
   private static final int EDGE_HIT_DISTANCE = 6;
   private static final int DEFAULT_ELEMENT_SIZE = 500;

   private final I18n i18n = LocalizationHelper.getI18n(FloorPlanWidget.class);

   private final View view;
   private FloorPlanModel model;
   private double scale = 0.05;
   private double originX = 0;   // screen X of room point (0, 0)
   private double originY = 0;
   private boolean fitPending = true;

   private boolean editMode = false;
   private boolean snapToGrid = true;
   private boolean moveBackdropMode = false;
   private boolean calibrationMode = false;
   private Point calibrationPoint = null;

   private Object selection = null;
   private Point lastMousePoint = null;
   private boolean dragging = false;
   private boolean dragMoved = false;
   private int dragOffsetX = 0;   // offset from mouse position to dragged item reference point, in mm
   private int dragOffsetY = 0;

   private List<String> warnings = new ArrayList<String>();
   private Set<Long> rackWarnings = new HashSet<Long>();
   private Set<Long> elementWarnings = new HashSet<Long>();

   private Set<ElementSelectionListener> selectionListeners = new HashSet<ElementSelectionListener>();
   private FloorPlanEditListener editListener = null;

   private final Color colorBackground;
   private final Color colorText;
   private final Color colorFloor;
   private final Color colorWall;
   private final Color colorGrid;
   private final Color colorElement;
   private final Color colorElementBorder;
   private final Color colorRackBorder;
   private final Color colorWarning;
   private final Color colorSelection;

   /**
    * Create floor plan widget.
    *
    * @param parent parent composite
    * @param style style bits
    * @param room room object
    * @param view owning view (used for navigation to rack views)
    */
   public FloorPlanWidget(Composite parent, int style, Room room, View view)
   {
      super(parent, style | SWT.DOUBLE_BUFFERED);
      this.view = view;
      model = new FloorPlanModel(room);

      colorBackground = ThemeEngine.getBackgroundColor("FloorPlan");
      colorText = ThemeEngine.getForegroundColor("FloorPlan");
      colorFloor = ThemeEngine.getBackgroundColor("FloorPlan.Floor");
      colorWall = ThemeEngine.getForegroundColor("FloorPlan.Floor");
      colorGrid = ThemeEngine.getForegroundColor("FloorPlan.Grid");
      colorElement = ThemeEngine.getBackgroundColor("FloorPlan.Element");
      colorElementBorder = ThemeEngine.getForegroundColor("FloorPlan.Element");
      colorRackBorder = ThemeEngine.getForegroundColor("FloorPlan.Rack");
      colorWarning = ThemeEngine.getForegroundColor("FloorPlan.Warning");
      colorSelection = getDisplay().getSystemColor(SWT.COLOR_LIST_SELECTION);
      setBackground(colorBackground);

      addPaintListener(this);
      DragTrackingHelper.attach(this, this, this);
      ImageProvider.getInstance().addUpdateListener(this);
      addDisposeListener((e) -> ImageProvider.getInstance().removeUpdateListener(FloorPlanWidget.this));

      validate();
   }

   /**
    * @return floor plan model
    */
   public FloorPlanModel getModel()
   {
      return model;
   }

   /**
    * Reload floor plan from room object, discarding local changes.
    *
    * @param room room object
    */
   public void reload(Room room)
   {
      model.reset(room);
      if (!isValidSelection())
         setSelection(null);
      validate();
      redraw();
   }

   /**
    * Update from room object keeping local changes (rack list, names and statuses are refreshed).
    *
    * @param room room object
    */
   public void update(Room room)
   {
      model.updateRacks(room);
      if (!isValidSelection())
         setSelection(null);
      validate();
      redraw();
   }

   /**
    * Check that current selection still refers to an existing item
    */
   private boolean isValidSelection()
   {
      if (selection instanceof Rack)
         return model.getPlacement(((Rack)selection).getObjectId()) != null;
      if (selection instanceof RoomPassiveElement)
         return model.getPassiveElements().contains(selection);
      if (selection instanceof OutlineVertex)
         return editMode && (((OutlineVertex)selection).index < model.getOutline().size());
      return true;
   }

   /**
    * Enable or disable edit mode.
    *
    * @param editMode true to enable editing
    */
   public void setEditMode(boolean editMode)
   {
      this.editMode = editMode;
      moveBackdropMode = false;
      cancelCalibration();
      if (!editMode && (selection instanceof OutlineVertex))
         setSelection(null);
      redraw();
   }

   /**
    * @return true if edit mode is enabled
    */
   public boolean isEditMode()
   {
      return editMode;
   }

   /**
    * @param snapToGrid true to snap moved items to floor tile grid
    */
   public void setSnapToGrid(boolean snapToGrid)
   {
      this.snapToGrid = snapToGrid;
   }

   /**
    * @param moveBackdropMode true if dragging on empty area should move the backdrop instead of panning the view
    */
   public void setMoveBackdropMode(boolean moveBackdropMode)
   {
      this.moveBackdropMode = moveBackdropMode;
   }

   /**
    * Add selection listener.
    *
    * @param listener listener
    */
   public void addSelectionListener(ElementSelectionListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * Set edit listener.
    *
    * @param listener listener or null
    */
   public void setEditListener(FloorPlanEditListener listener)
   {
      editListener = listener;
   }

   /**
    * @return current selection (Rack, RoomPassiveElement, OutlineVertex, or null)
    */
   public Object getSelection()
   {
      return selection;
   }

   /**
    * Set current selection and notify listeners.
    *
    * @param object new selection
    */
   public void setSelection(Object object)
   {
      selection = object;
      for(ElementSelectionListener l : selectionListeners)
         l.objectSelected(object);
      redraw();
   }

   /**
    * @return placement warnings for current plan (empty list if none)
    */
   public List<String> getWarnings()
   {
      return warnings;
   }

   /*
    * Coordinate transformation
    */

   private int toScreenX(double x)
   {
      return (int)Math.round(originX + x * scale);
   }

   private int toScreenY(double y)
   {
      return (int)Math.round(originY + y * scale);
   }

   private int toRoomX(int sx)
   {
      return (int)Math.round((sx - originX) / scale);
   }

   private int toRoomY(int sy)
   {
      return (int)Math.round((sy - originY) / scale);
   }

   private int[] toScreen(double[] polygon)
   {
      int[] p = new int[polygon.length];
      for(int i = 0; i < polygon.length; i += 2)
      {
         p[i] = toScreenX(polygon[i]);
         p[i + 1] = toScreenY(polygon[i + 1]);
      }
      return p;
   }

   /**
    * Get room outline as polygon
    */
   private double[] getOutlinePolygon()
   {
      return FloorPlanGeometry.toPolygon(model.getOutline());
   }

   /**
    * Get rack footprint polygon in room coordinates
    */
   private double[] getRackPolygon(Rack rack, RackPlacement p)
   {
      return FloorPlanGeometry.footprint(p.x, p.y, rack.getWidth(), rack.getDepth(), p.rotation);
   }

   /**
    * Get passive element footprint polygon in room coordinates
    */
   private double[] getElementPolygon(RoomPassiveElement e)
   {
      return FloorPlanGeometry.footprint(e.x, e.y, e.width, e.depth, e.rotation);
   }

   /*
    * Zoom and pan
    */

   /**
    * Fit whole floor plan into widget area.
    */
   public void zoomToFit()
   {
      Rectangle area = getClientArea();
      if ((area.width <= 0) || (area.height <= 0))
      {
         fitPending = true;
         return;
      }
      fitPending = false;

      double[] b = FloorPlanGeometry.bounds(getOutlinePolygon());
      for(Rack rack : model.getRacks())
      {
         RackPlacement p = model.getPlacement(rack.getObjectId());
         if ((p != null) && p.placed)
            b = union(b, FloorPlanGeometry.bounds(getRackPolygon(rack, p)));
      }
      for(RoomPassiveElement e : model.getPassiveElements())
         b = union(b, FloorPlanGeometry.bounds(getElementPolygon(e)));

      double width = Math.max(b[2] - b[0], 1000);
      double height = Math.max(b[3] - b[1], 1000);
      scale = Math.min((area.width - FIT_MARGIN * 2) / width, (area.height - FIT_MARGIN * 2) / height);
      scale = Math.max(MIN_SCALE, Math.min(MAX_SCALE, scale));
      originX = (area.width - width * scale) / 2 - b[0] * scale;
      originY = (area.height - height * scale) / 2 - b[1] * scale;
      redraw();
   }

   /**
    * Union of two bounding boxes
    */
   private static double[] union(double[] a, double[] b)
   {
      return new double[] { Math.min(a[0], b[0]), Math.min(a[1], b[1]), Math.max(a[2], b[2]), Math.max(a[3], b[3]) };
   }

   /**
    * Zoom in around the centre of the widget.
    */
   public void zoomIn()
   {
      Rectangle area = getClientArea();
      zoomAround(area.width / 2, area.height / 2, ZOOM_STEP);
   }

   /**
    * Zoom out around the centre of the widget.
    */
   public void zoomOut()
   {
      Rectangle area = getClientArea();
      zoomAround(area.width / 2, area.height / 2, 1 / ZOOM_STEP);
   }

   /**
    * Change scale keeping the room point under given screen position in place.
    */
   private void zoomAround(int sx, int sy, double factor)
   {
      double newScale = Math.max(MIN_SCALE, Math.min(MAX_SCALE, scale * factor));
      double rx = (sx - originX) / scale;
      double ry = (sy - originY) / scale;
      scale = newScale;
      originX = sx - rx * scale;
      originY = sy - ry * scale;
      redraw();
   }

   /*
    * Painting
    */

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
   @Override
   public void paintControl(PaintEvent e)
   {
      if (fitPending)
         zoomToFit();

      GC gc = e.gc;
      gc.setAntialias(SWT.ON);
      gc.setTextAntialias(SWT.ON);

      double[] outline = getOutlinePolygon();
      int[] screenOutline = toScreen(outline);
      boolean hasBackdrop = drawBackdrop(gc, outline);

      // Floor
      if (outline.length >= 6)
      {
         if (!hasBackdrop)
         {
            gc.setBackground(colorFloor);
            gc.fillPolygon(screenOutline);
         }
         drawGrid(gc, outline, screenOutline);
         gc.setForeground(colorWall);
         gc.setLineWidth(3);
         gc.drawPolygon(screenOutline);
         gc.setLineWidth(1);
      }

      // Passive elements
      gc.setFont(getFont());
      for(RoomPassiveElement element : model.getPassiveElements())
      {
         int[] p = toScreen(getElementPolygon(element));
         gc.setBackground(colorElement);
         gc.setForeground(colorElementBorder);
         gc.setAlpha(element.type == RoomElementType.DOOR ? 96 : 255);
         gc.fillPolygon(p);
         gc.setAlpha(255);
         gc.setLineStyle((element.type == RoomElementType.DOOR) || (element.type == RoomElementType.RAMP) ? SWT.LINE_DASH : SWT.LINE_SOLID);
         gc.drawPolygon(p);
         gc.setLineStyle(SWT.LINE_SOLID);
         if (element.type == RoomElementType.STAIRS)
         {
            gc.drawLine(p[0], p[1], p[4], p[5]);
            gc.drawLine(p[2], p[3], p[6], p[7]);
         }
         drawLabel(gc, p, element.name, colorText);
         if (editMode && elementWarnings.contains(element.id))
            drawWarningOutline(gc, p);
         if (element == selection)
            drawSelectionOutline(gc, p);
      }

      // Racks
      for(Rack rack : model.getRacks())
      {
         RackPlacement placement = model.getPlacement(rack.getObjectId());
         if ((placement == null) || !placement.placed)
            continue;

         int[] p = toScreen(getRackPolygon(rack, placement));
         gc.setBackground(StatusDisplayInfo.getStatusColor(rack.getStatus()));
         gc.setAlpha(160);
         gc.fillPolygon(p);
         gc.setAlpha(255);
         gc.setForeground(colorRackBorder);
         gc.drawPolygon(p);
         gc.setLineWidth(3);
         gc.drawLine(p[6], p[7], p[4], p[5]);   // front face
         gc.setLineWidth(1);
         drawLabel(gc, p, rack.getObjectName(), colorText);
         if (editMode && rackWarnings.contains(rack.getObjectId()))
            drawWarningOutline(gc, p);
         if (rack == selection)
            drawSelectionOutline(gc, p);
      }

      // Outline vertex handles
      if (editMode)
      {
         int selectedVertex = (selection instanceof OutlineVertex) ? ((OutlineVertex)selection).index : -1;
         for(int i = 0; i < screenOutline.length; i += 2)
         {
            gc.setBackground((i / 2 == selectedVertex) ? colorSelection : colorWall);
            gc.fillRectangle(screenOutline[i] - HANDLE_SIZE / 2, screenOutline[i + 1] - HANDLE_SIZE / 2, HANDLE_SIZE, HANDLE_SIZE);
         }
      }

      // Calibration marker
      if (calibrationMode && (calibrationPoint != null))
      {
         gc.setForeground(colorWarning);
         gc.setLineWidth(2);
         gc.drawLine(calibrationPoint.x - 8, calibrationPoint.y, calibrationPoint.x + 8, calibrationPoint.y);
         gc.drawLine(calibrationPoint.x, calibrationPoint.y - 8, calibrationPoint.x, calibrationPoint.y + 8);
         gc.setLineWidth(1);
      }
   }

   /**
    * Draw backdrop image. Returns true if backdrop was drawn.
    */
   private boolean drawBackdrop(GC gc, double[] outline)
   {
      UUID guid = model.getRoom().getBackgroundImage();
      if (guid == null)
         return false;

      Image image = ImageProvider.getInstance().getImage(guid, () -> {
         if (!isDisposed())
            redraw();
      });
      if (image == null)
         return false;

      Rectangle r = image.getBounds();
      double mmPerPixel = getBackdropMmPerPixel(r.width, outline);
      int dx = toScreenX(model.getBackgroundX());
      int dy = toScreenY(model.getBackgroundY());
      int dw = Math.max(1, (int)Math.round(r.width * mmPerPixel * scale));
      int dh = Math.max(1, (int)Math.round(r.height * mmPerPixel * scale));
      gc.drawImage(image, 0, 0, r.width, r.height, dx, dy, dw, dh);
      return true;
   }

   /**
    * Get backdrop scale in millimetres per image pixel. Uncalibrated backdrop is stretched to the width of the room outline.
    */
   private double getBackdropMmPerPixel(int imageWidth, double[] outline)
   {
      if (model.getBackgroundScale() > 0)
         return model.getBackgroundScale() / 1000.0;
      double[] b = FloorPlanGeometry.bounds(outline);
      double width = b[2] - b[0];
      return (width > 0) ? width / imageWidth : 10;
   }

   /**
    * Draw floor tile grid with labels
    */
   private void drawGrid(GC gc, double[] outline, int[] screenOutline)
   {
      Room room = model.getRoom();
      int tile = room.getGridTileSize();
      if (tile <= 0)
         return;

      double tilePixels = tile * scale;
      if (tilePixels < 3)
         return;

      double[] b = FloorPlanGeometry.bounds(outline);
      int firstColumn = (int)Math.floor((b[0] - room.getGridOriginX()) / tile);
      int lastColumn = (int)Math.ceil((b[2] - room.getGridOriginX()) / tile);
      int firstRow = (int)Math.floor((b[1] - room.getGridOriginY()) / tile);
      int lastRow = (int)Math.ceil((b[3] - room.getGridOriginY()) / tile);

      gc.setForeground(colorGrid);
      gc.setLineStyle(SWT.LINE_DOT);
      int top = toScreenY(b[1]);
      int left = toScreenX(b[0]);
      for(int c = firstColumn; c <= lastColumn; c++)
         drawClippedGridLine(gc, outline, room.getGridOriginX() + (double)c * tile, true);
      for(int r = firstRow; r <= lastRow; r++)
         drawClippedGridLine(gc, outline, room.getGridOriginY() + (double)r * tile, false);
      gc.setLineStyle(SWT.LINE_SOLID);

      // Labels outside the room, first tile touching the outline is column A / row 1
      if ((room.getGridLabels() == RoomGridLabels.NONE) || (tilePixels < 14))
         return;

      gc.setForeground(colorText);
      Point extent = gc.textExtent("W");
      boolean letters = (room.getGridLabels() == RoomGridLabels.LETTERS_NUMBERS);
      for(int c = firstColumn; c < lastColumn; c++)
      {
         String label = letters ? columnLetters(c - firstColumn) : Integer.toString(c - firstColumn + 1);
         int x = toScreenX(room.getGridOriginX() + (c + 0.5) * tile);
         Point te = gc.textExtent(label);
         gc.drawText(label, x - te.x / 2, top - extent.y - 2, SWT.DRAW_TRANSPARENT);
      }
      for(int r = firstRow; r < lastRow; r++)
      {
         String label = Integer.toString(r - firstRow + 1);
         int y = toScreenY(room.getGridOriginY() + (r + 0.5) * tile);
         Point te = gc.textExtent(label);
         gc.drawText(label, left - te.x - 4, y - te.y / 2, SWT.DRAW_TRANSPARENT);
      }
   }

   /**
    * Draw grid line (vertical at room X = position, or horizontal at room Y = position) clipped to the room outline: the line
    * is drawn only between pairs of intersections with outline edges.
    */
   private void drawClippedGridLine(GC gc, double[] outline, double position, boolean vertical)
   {
      int n = outline.length / 2;
      List<Double> crossings = new ArrayList<Double>();
      for(int i = 0; i < n; i++)
      {
         int j = (i + 1) % n;
         double a1 = vertical ? outline[i * 2] : outline[i * 2 + 1];   // coordinate along the axis perpendicular to the line
         double a2 = vertical ? outline[j * 2] : outline[j * 2 + 1];
         double b1 = vertical ? outline[i * 2 + 1] : outline[i * 2];   // coordinate along the line
         double b2 = vertical ? outline[j * 2 + 1] : outline[j * 2];
         if ((a1 <= position) != (a2 <= position))
            crossings.add(b1 + (position - a1) * (b2 - b1) / (a2 - a1));
      }
      if (crossings.size() < 2)
         return;
      crossings.sort(null);
      for(int i = 0; i + 1 < crossings.size(); i += 2)
      {
         if (vertical)
            gc.drawLine(toScreenX(position), toScreenY(crossings.get(i)), toScreenX(position), toScreenY(crossings.get(i + 1)));
         else
            gc.drawLine(toScreenX(crossings.get(i)), toScreenY(position), toScreenX(crossings.get(i + 1)), toScreenY(position));
      }
   }

   /**
    * Convert zero-based column index to spreadsheet style letters (A, B, ..., Z, AA, AB, ...)
    */
   private static String columnLetters(int index)
   {
      StringBuilder sb = new StringBuilder();
      do
      {
         sb.insert(0, (char)('A' + index % 26));
         index = index / 26 - 1;
      } while(index >= 0);
      return sb.toString();
   }

   /**
    * Draw text label centred in polygon if it fits
    */
   private void drawLabel(GC gc, int[] polygon, String text, Color color)
   {
      if ((text == null) || text.isEmpty())
         return;

      int minX = Integer.MAX_VALUE, minY = Integer.MAX_VALUE, maxX = Integer.MIN_VALUE, maxY = Integer.MIN_VALUE;
      for(int i = 0; i < polygon.length; i += 2)
      {
         minX = Math.min(minX, polygon[i]);
         maxX = Math.max(maxX, polygon[i]);
         minY = Math.min(minY, polygon[i + 1]);
         maxY = Math.max(maxY, polygon[i + 1]);
      }
      Point te = gc.textExtent(text);
      if ((te.x + 4 > maxX - minX) || (te.y + 2 > maxY - minY))
         return;
      gc.setForeground(color);
      gc.drawText(text, (minX + maxX - te.x) / 2, (minY + maxY - te.y) / 2, SWT.DRAW_TRANSPARENT);
   }

   /**
    * Draw selection highlight around polygon
    */
   private void drawSelectionOutline(GC gc, int[] polygon)
   {
      gc.setForeground(colorSelection);
      gc.setLineWidth(2);
      gc.drawPolygon(polygon);
      gc.setLineWidth(1);
   }

   /**
    * Draw warning highlight around polygon
    */
   private void drawWarningOutline(GC gc, int[] polygon)
   {
      gc.setForeground(colorWarning);
      gc.setLineWidth(2);
      gc.setLineStyle(SWT.LINE_DASH);
      gc.drawPolygon(polygon);
      gc.setLineStyle(SWT.LINE_SOLID);
      gc.setLineWidth(1);
   }

   /**
    * @see org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener#imageUpdated(java.util.UUID)
    */
   @Override
   public void imageUpdated(UUID guid)
   {
      if (guid.equals(model.getRoom().getBackgroundImage()))
         redraw();
   }

   /*
    * Hit testing
    */

   /**
    * Find item at given screen position. Vertex handles (edit mode only) take precedence over racks, racks over passive
    * elements.
    */
   private Object getItemAt(int sx, int sy)
   {
      if (editMode)
      {
         List<RoomPoint> outline = model.getOutline();
         for(int i = 0; i < outline.size(); i++)
         {
            if ((Math.abs(toScreenX(outline.get(i).x) - sx) <= HANDLE_SIZE) && (Math.abs(toScreenY(outline.get(i).y) - sy) <= HANDLE_SIZE))
               return new OutlineVertex(i);
         }
      }

      double rx = (sx - originX) / scale;
      double ry = (sy - originY) / scale;
      List<Rack> racks = model.getRacks();
      for(int i = racks.size() - 1; i >= 0; i--)
      {
         Rack rack = racks.get(i);
         RackPlacement p = model.getPlacement(rack.getObjectId());
         if ((p != null) && p.placed && FloorPlanGeometry.contains(getRackPolygon(rack, p), rx, ry))
            return rack;
      }
      List<RoomPassiveElement> elements = model.getPassiveElements();
      for(int i = elements.size() - 1; i >= 0; i--)
      {
         if (FloorPlanGeometry.contains(getElementPolygon(elements.get(i)), rx, ry))
            return elements.get(i);
      }
      return null;
   }

   /**
    * Find outline edge near given screen position. Returns index of edge start vertex or -1.
    */
   private int getOutlineEdgeAt(int sx, int sy)
   {
      List<RoomPoint> outline = model.getOutline();
      int n = outline.size();
      double limit = EDGE_HIT_DISTANCE * EDGE_HIT_DISTANCE;
      for(int i = 0; i < n; i++)
      {
         RoomPoint p1 = outline.get(i);
         RoomPoint p2 = outline.get((i + 1) % n);
         if (FloorPlanGeometry.distanceToSegmentSquared(sx, sy, toScreenX(p1.x), toScreenY(p1.y), toScreenX(p2.x), toScreenY(p2.y)) <= limit)
            return i;
      }
      return -1;
   }

   /*
    * Mouse handling
    */

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseDown(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseDown(MouseEvent e)
   {
      lastMousePoint = new Point(e.x, e.y);
      dragMoved = false;

      if (e.button != 1)
      {
         Object item = getItemAt(e.x, e.y);
         if ((item != null) && (item != selection))
            setSelection(item);
         return;
      }

      if (calibrationMode)
      {
         if (calibrationPoint == null)
         {
            calibrationPoint = new Point(e.x, e.y);
            redraw();
         }
         else
         {
            double distance = Math.hypot(e.x - calibrationPoint.x, e.y - calibrationPoint.y);
            Image image = getBackdropImage();
            double mmPerPixel = (image != null) ? getBackdropMmPerPixel(image.getBounds().width, getOutlinePolygon()) : 1;
            double imagePixelDistance = distance / (scale * mmPerPixel);
            cancelCalibration();
            if ((editListener != null) && (imagePixelDistance > 0))
               editListener.calibrationPointsSelected(imagePixelDistance);
         }
         return;
      }

      Object item = getItemAt(e.x, e.y);
      if (item != selection)
         setSelection(item);

      dragging = true;
      if (editMode && (item != null))
      {
         int rx = toRoomX(e.x), ry = toRoomY(e.y);
         if (item instanceof Rack)
         {
            RackPlacement p = model.getPlacement(((Rack)item).getObjectId());
            dragOffsetX = rx - p.x;
            dragOffsetY = ry - p.y;
         }
         else if (item instanceof RoomPassiveElement)
         {
            dragOffsetX = rx - ((RoomPassiveElement)item).x;
            dragOffsetY = ry - ((RoomPassiveElement)item).y;
         }
         else
         {
            dragOffsetX = 0;
            dragOffsetY = 0;
         }
      }
      setCursor(getDisplay().getSystemCursor(SWT.CURSOR_SIZEALL));
   }

   /**
    * @see org.eclipse.swt.events.MouseMoveListener#mouseMove(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseMove(MouseEvent e)
   {
      if (!dragging || (lastMousePoint == null))
         return;

      int dx = e.x - lastMousePoint.x;
      int dy = e.y - lastMousePoint.y;
      if ((dx == 0) && (dy == 0))
         return;
      lastMousePoint = new Point(e.x, e.y);
      dragMoved = true;

      if (editMode && (selection != null))
      {
         int rx = snapX(toRoomX(e.x) - dragOffsetX);
         int ry = snapY(toRoomY(e.y) - dragOffsetY);
         if (selection instanceof Rack)
         {
            RackPlacement p = model.getPlacement(((Rack)selection).getObjectId());
            p.x = rx;
            p.y = ry;
         }
         else if (selection instanceof RoomPassiveElement)
         {
            ((RoomPassiveElement)selection).x = rx;
            ((RoomPassiveElement)selection).y = ry;
         }
         else if (selection instanceof OutlineVertex)
         {
            RoomPoint p = model.getOutline().get(((OutlineVertex)selection).index);
            p.x = rx;
            p.y = ry;
         }
      }
      else if (editMode && moveBackdropMode)
      {
         model.setBackground(model.getBackgroundScale(), model.getBackgroundX() + (int)Math.round(dx / scale), model.getBackgroundY() + (int)Math.round(dy / scale));
      }
      else
      {
         originX += dx;
         originY += dy;
      }
      redraw();
   }

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseUp(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseUp(MouseEvent e)
   {
      if (!dragging)
         return;
      dragging = false;
      setCursor(null);

      if (editMode && dragMoved)
      {
         if (selection != null)
         {
            if (!(selection instanceof Rack))
               model.markRoomModified();
            modelChanged();
         }
         else if (moveBackdropMode)
         {
            modelChanged();
         }
      }
   }

   /**
    * @see org.eclipse.swt.events.MouseListener#mouseDoubleClick(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseDoubleClick(MouseEvent e)
   {
      if (e.button != 1)
         return;

      Object item = getItemAt(e.x, e.y);
      if (item instanceof Rack)
      {
         view.openView(new AdHocRackView(model.getRoom().getObjectId(), (Rack)item));
      }
      else if (editMode && (item instanceof RoomPassiveElement))
      {
         if (editListener != null)
            editListener.editElementRequested((RoomPassiveElement)item);
      }
      else if (editMode && (item == null))
      {
         int edge = getOutlineEdgeAt(e.x, e.y);
         if (edge != -1)
         {
            model.getOutline().add(edge + 1, new RoomPoint(snapX(toRoomX(e.x)), snapY(toRoomY(e.y))));
            model.markRoomModified();
            setSelection(new OutlineVertex(edge + 1));
            modelChanged();
         }
      }
   }

   private int snapX(int x)
   {
      return snapToGrid ? FloorPlanGeometry.snap(x, model.getRoom().getGridOriginX(), model.getRoom().getGridTileSize()) : x;
   }

   private int snapY(int y)
   {
      return snapToGrid ? FloorPlanGeometry.snap(y, model.getRoom().getGridOriginY(), model.getRoom().getGridTileSize()) : y;
   }

   /**
    * Handle model change: revalidate, redraw, notify listener
    */
   private void modelChanged()
   {
      validate();
      redraw();
      if (editListener != null)
         editListener.modelChanged();
   }

   /*
    * Editing operations
    */

   /**
    * Rotate selected rack or passive element by given angle keeping its footprint centre in place.
    *
    * @param degrees rotation angle (positive is clockwise)
    */
   public void rotateSelection(int degrees)
   {
      if (selection instanceof Rack)
      {
         Rack rack = (Rack)selection;
         RackPlacement p = model.getPlacement(rack.getObjectId());
         double[] before = centre(getRackPolygon(rack, p));
         p.rotation = FloorPlanGeometry.normalizeRotation(p.rotation + degrees);
         double[] after = centre(getRackPolygon(rack, p));
         p.x += (int)Math.round(before[0] - after[0]);
         p.y += (int)Math.round(before[1] - after[1]);
      }
      else if (selection instanceof RoomPassiveElement)
      {
         RoomPassiveElement e = (RoomPassiveElement)selection;
         double[] before = centre(getElementPolygon(e));
         e.rotation = FloorPlanGeometry.normalizeRotation(e.rotation + degrees);
         double[] after = centre(getElementPolygon(e));
         e.x += (int)Math.round(before[0] - after[0]);
         e.y += (int)Math.round(before[1] - after[1]);
         model.markRoomModified();
      }
      else
      {
         return;
      }
      modelChanged();
   }

   /**
    * Centre of polygon bounding box
    */
   private static double[] centre(double[] polygon)
   {
      double[] b = FloorPlanGeometry.bounds(polygon);
      return new double[] { (b[0] + b[2]) / 2, (b[1] + b[3]) / 2 };
   }

   /**
    * Delete selected item: passive element is removed, outline vertex is removed (outline keeps at least 3 vertices), rack is
    * moved back to the list of unplaced racks.
    */
   public void deleteSelection()
   {
      if (selection instanceof Rack)
      {
         model.getPlacement(((Rack)selection).getObjectId()).placed = false;
      }
      else if (selection instanceof RoomPassiveElement)
      {
         model.getPassiveElements().remove(selection);
         model.markRoomModified();
      }
      else if (selection instanceof OutlineVertex)
      {
         if (model.getOutline().size() <= 3)
            return;
         model.getOutline().remove(((OutlineVertex)selection).index);
         model.markRoomModified();
      }
      else
      {
         return;
      }
      setSelection(null);
      modelChanged();
   }

   /**
    * Place rack on the floor plan at the centre of the visible area and select it.
    *
    * @param rack rack to place
    */
   public void placeRack(Rack rack)
   {
      RackPlacement p = model.getPlacement(rack.getObjectId());
      if (p == null)
         return;

      Rectangle area = getClientArea();
      p.x = snapX(toRoomX(area.width / 2) - rack.getWidth() / 2);
      p.y = snapY(toRoomY(area.height / 2) - rack.getDepth() / 2);
      p.placed = true;
      setSelection(rack);
      modelChanged();
   }

   /**
    * Create new passive element at the centre of the visible area.
    *
    * @return new element (already added to the model)
    */
   public RoomPassiveElement createPassiveElement()
   {
      RoomPassiveElement e = new RoomPassiveElement();
      Rectangle area = getClientArea();
      e.x = snapX(toRoomX(area.width / 2) - DEFAULT_ELEMENT_SIZE / 2);
      e.y = snapY(toRoomY(area.height / 2) - DEFAULT_ELEMENT_SIZE / 2);
      e.width = DEFAULT_ELEMENT_SIZE;
      e.depth = DEFAULT_ELEMENT_SIZE;
      model.getPassiveElements().add(e);
      model.markRoomModified();
      setSelection(e);
      modelChanged();
      return e;
   }

   /**
    * Notify widget that passive element was changed externally (in edit dialog).
    */
   public void passiveElementChanged()
   {
      model.markRoomModified();
      modelChanged();
   }

   /**
    * Start backdrop calibration: next two clicks select two points with known real-world distance.
    */
   public void startCalibration()
   {
      calibrationMode = true;
      calibrationPoint = null;
      setCursor(getDisplay().getSystemCursor(SWT.CURSOR_CROSS));
   }

   /**
    * Cancel backdrop calibration.
    */
   public void cancelCalibration()
   {
      if (!calibrationMode)
         return;
      calibrationMode = false;
      calibrationPoint = null;
      setCursor(null);
      redraw();
   }

   /**
    * @return true if backdrop calibration is in progress
    */
   public boolean isCalibrating()
   {
      return calibrationMode;
   }

   /**
    * Apply backdrop calibration.
    *
    * @param imagePixelDistance measured distance in backdrop image pixels
    * @param realDistance real distance in millimetres
    */
   public void applyCalibration(double imagePixelDistance, int realDistance)
   {
      int umPerPixel = (int)Math.round(realDistance * 1000.0 / imagePixelDistance);
      model.setBackground(Math.max(1, umPerPixel), model.getBackgroundX(), model.getBackgroundY());
      modelChanged();
   }

   /**
    * Get backdrop image if set and available
    */
   private Image getBackdropImage()
   {
      UUID guid = model.getRoom().getBackgroundImage();
      return (guid != null) ? ImageProvider.getInstance().getImage(guid) : null;
   }

   /*
    * Validation
    */

   /**
    * Check placement of racks and passive elements: anything outside the outline or overlapping something else is reported
    * as a warning.
    */
   private void validate()
   {
      warnings = new ArrayList<String>();
      rackWarnings = new HashSet<Long>();
      elementWarnings = new HashSet<Long>();

      double[] outline = getOutlinePolygon();
      boolean checkOutline = outline.length >= 6;

      List<Object> items = new ArrayList<Object>();
      List<double[]> polygons = new ArrayList<double[]>();
      for(Rack rack : model.getRacks())
      {
         RackPlacement p = model.getPlacement(rack.getObjectId());
         if ((p != null) && p.placed)
         {
            items.add(rack);
            polygons.add(getRackPolygon(rack, p));
         }
      }
      for(RoomPassiveElement e : model.getPassiveElements())
      {
         items.add(e);
         polygons.add(getElementPolygon(e));
      }

      for(int i = 0; i < items.size(); i++)
      {
         if (checkOutline && !FloorPlanGeometry.containsAll(polygons.get(i), outline))
         {
            warnings.add(i18n.tr("{0} is outside the room outline", itemName(items.get(i))));
            flag(items.get(i));
         }
         for(int j = i + 1; j < items.size(); j++)
         {
            if (FloorPlanGeometry.overlaps(polygons.get(i), polygons.get(j)))
            {
               warnings.add(i18n.tr("{0} overlaps {1}", itemName(items.get(i)), itemName(items.get(j))));
               flag(items.get(i));
               flag(items.get(j));
            }
         }
      }
   }

   private void flag(Object item)
   {
      if (item instanceof Rack)
         rackWarnings.add(((Rack)item).getObjectId());
      else
         elementWarnings.add(((RoomPassiveElement)item).id);
   }

   private String itemName(Object item)
   {
      if (item instanceof Rack)
         return i18n.tr("Rack \"{0}\"", ((Rack)item).getObjectName());
      RoomPassiveElement e = (RoomPassiveElement)item;
      return ((e.name != null) && !e.name.isEmpty()) ? i18n.tr("Element \"{0}\"", e.name) : i18n.tr("Unnamed element");
   }
}
