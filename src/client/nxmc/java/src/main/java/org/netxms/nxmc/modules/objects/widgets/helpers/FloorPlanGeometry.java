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
package org.netxms.nxmc.modules.objects.widgets.helpers;

import java.util.List;
import org.netxms.client.objects.configs.RoomPoint;

/**
 * Geometry helpers for room floor plan. All coordinates are in room-local millimetres with Y axis pointing down (screen
 * orientation). Polygons are represented as flat arrays of interleaved x,y values.
 */
public final class FloorPlanGeometry
{
   /**
    * Build footprint polygon of a rectangle placed with its reference (top-left at zero rotation) corner at (x, y) and rotated
    * clockwise by given angle around that corner.
    *
    * @param x X coordinate of reference corner
    * @param y Y coordinate of reference corner
    * @param width footprint width
    * @param depth footprint depth
    * @param rotation rotation in degrees
    * @return polygon vertices in order: reference corner, right corner, far right corner, far left corner
    */
   public static double[] footprint(int x, int y, int width, int depth, int rotation)
   {
      double a = Math.toRadians(rotation);
      double cos = Math.cos(a);
      double sin = Math.sin(a);
      return new double[] {
         x, y,
         x + width * cos, y + width * sin,
         x + width * cos - depth * sin, y + width * sin + depth * cos,
         x - depth * sin, y + depth * cos
      };
   }

   /**
    * Convert outline point list to polygon array.
    *
    * @param points outline points
    * @return polygon array
    */
   public static double[] toPolygon(List<RoomPoint> points)
   {
      double[] polygon = new double[points.size() * 2];
      int i = 0;
      for(RoomPoint p : points)
      {
         polygon[i++] = p.x;
         polygon[i++] = p.y;
      }
      return polygon;
   }

   /**
    * Check if point is inside polygon (ray casting; points on the boundary may be reported either way).
    *
    * @param polygon polygon
    * @param px point X
    * @param py point Y
    * @return true if point is inside polygon
    */
   public static boolean contains(double[] polygon, double px, double py)
   {
      int n = polygon.length / 2;
      boolean inside = false;
      for(int i = 0, j = n - 1; i < n; j = i++)
      {
         double xi = polygon[i * 2], yi = polygon[i * 2 + 1];
         double xj = polygon[j * 2], yj = polygon[j * 2 + 1];
         if (((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
            inside = !inside;
      }
      return inside;
   }

   /**
    * Check if all vertices of the first polygon are inside the second polygon.
    *
    * @param inner polygon to test
    * @param outer containing polygon
    * @return true if every vertex of <code>inner</code> is inside <code>outer</code>
    */
   public static boolean containsAll(double[] inner, double[] outer)
   {
      for(int i = 0; i < inner.length; i += 2)
         if (!contains(outer, inner[i], inner[i + 1]))
            return false;
      return true;
   }

   /**
    * Check if two convex polygons overlap (separating axis theorem). Polygons that only touch along an edge are not
    * considered overlapping.
    *
    * @param a first convex polygon
    * @param b second convex polygon
    * @return true if polygons overlap
    */
   public static boolean overlaps(double[] a, double[] b)
   {
      return !hasSeparatingAxis(a, b) && !hasSeparatingAxis(b, a);
   }

   /**
    * Check if any edge normal of polygon <code>a</code> is a separating axis for polygons <code>a</code> and <code>b</code>.
    */
   private static boolean hasSeparatingAxis(double[] a, double[] b)
   {
      int n = a.length / 2;
      for(int i = 0; i < n; i++)
      {
         int j = (i + 1) % n;
         double nx = a[j * 2 + 1] - a[i * 2 + 1];
         double ny = a[i * 2] - a[j * 2];
         double[] pa = project(a, nx, ny);
         double[] pb = project(b, nx, ny);
         if ((pa[1] <= pb[0]) || (pb[1] <= pa[0]))
            return true;
      }
      return false;
   }

   /**
    * Project polygon onto axis, returning minimum and maximum.
    */
   private static double[] project(double[] polygon, double nx, double ny)
   {
      double min = Double.MAX_VALUE, max = -Double.MAX_VALUE;
      for(int i = 0; i < polygon.length; i += 2)
      {
         double d = polygon[i] * nx + polygon[i + 1] * ny;
         if (d < min)
            min = d;
         if (d > max)
            max = d;
      }
      return new double[] { min, max };
   }

   /**
    * Get bounding box of polygon.
    *
    * @param polygon polygon
    * @return array of minX, minY, maxX, maxY (all zeros for empty polygon)
    */
   public static double[] bounds(double[] polygon)
   {
      if (polygon.length < 2)
         return new double[] { 0, 0, 0, 0 };
      double[] r = new double[] { Double.MAX_VALUE, Double.MAX_VALUE, -Double.MAX_VALUE, -Double.MAX_VALUE };
      for(int i = 0; i < polygon.length; i += 2)
      {
         r[0] = Math.min(r[0], polygon[i]);
         r[1] = Math.min(r[1], polygon[i + 1]);
         r[2] = Math.max(r[2], polygon[i]);
         r[3] = Math.max(r[3], polygon[i + 1]);
      }
      return r;
   }

   /**
    * Get squared distance from point to line segment.
    *
    * @param px point X
    * @param py point Y
    * @param x1 segment start X
    * @param y1 segment start Y
    * @param x2 segment end X
    * @param y2 segment end Y
    * @return squared distance
    */
   public static double distanceToSegmentSquared(double px, double py, double x1, double y1, double x2, double y2)
   {
      double dx = x2 - x1, dy = y2 - y1;
      double l2 = dx * dx + dy * dy;
      double t = (l2 == 0) ? 0 : Math.max(0, Math.min(1, ((px - x1) * dx + (py - y1) * dy) / l2));
      double cx = x1 + t * dx - px, cy = y1 + t * dy - py;
      return cx * cx + cy * cy;
   }

   /**
    * Snap value to grid.
    *
    * @param value value to snap
    * @param origin grid origin
    * @param step grid step (no snapping if 0)
    * @return snapped value
    */
   public static int snap(int value, int origin, int step)
   {
      if (step <= 0)
         return value;
      return origin + (int)Math.round((double)(value - origin) / step) * step;
   }

   /**
    * Normalize rotation angle to range 0..359.
    *
    * @param degrees angle in degrees
    * @return normalized angle
    */
   public static int normalizeRotation(int degrees)
   {
      return ((degrees % 360) + 360) % 360;
   }
}
