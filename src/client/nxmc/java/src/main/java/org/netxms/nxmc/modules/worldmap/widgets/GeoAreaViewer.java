/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.worldmap.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.GeoLocation;
import org.netxms.client.GeoArea;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.worldmap.GeoLocationCache;
import org.netxms.nxmc.modules.worldmap.tools.Area;
import org.netxms.nxmc.modules.worldmap.tools.MapAccessor;

/**
 * Show area on map
 */
public class GeoAreaViewer extends AbstractGeoMapViewer
{
   private GeoArea area = null;

   /**
    * Create geo area viewer.
    *
    * @param parent parent control
    * @param style control style
    * @param view owning view
    */
   public GeoAreaViewer(Composite parent, int style, View view)
   {
      super(parent, style, view);
   }

   /**
    * Set area to display
    *
    * @param area area to display
    */
   public void setArea(GeoArea area)
   {
      this.area = area;
      if (area != null)
      {
         GeoLocation[] boundingBox = area.getBoundingBox();
         GeoLocation center = new GeoLocation(
               boundingBox[0].getLatitude() + (boundingBox[1].getLatitude() - boundingBox[0].getLatitude()) / 2,
               boundingBox[0].getLongitude() + (boundingBox[1].getLongitude() - boundingBox[0].getLongitude()) / 2);

         int zoom = 0;
         while(zoom < MapAccessor.MAX_MAP_ZOOM)
         {
            zoom++;
            final Area coverage = GeoLocationCache.calculateCoverage(getSize(), center, GeoLocationCache.CENTER, zoom);
            if (!coverage.contains(boundingBox[0]) || !coverage.contains(boundingBox[1]))
            {
               zoom--;
               break;
            }
         }

         showMap(center.getLatitude(), center.getLongitude(), zoom);
      }
   }

   /**
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#onMapLoad()
    */
   @Override
   protected void onMapLoad()
   {
   }

   /**
    * @see org.netxms.nxmc.modules.worldmap.widgets.AbstractGeoMapViewer#drawContent(org.eclipse.swt.graphics.GC,
    *      org.netxms.base.GeoLocation, int, int, int)
    */
   @Override
   protected void drawContent(GC gc, GeoLocation currentLocation, int imgW, int imgH, int verticalOffset)
   {
      if (area == null)
         return;

      final Point centerXY = GeoLocationCache.coordinateToDisplay(currentLocation, accessor.getZoom());

      gc.setForeground(getDisplay().getSystemColor(SWT.COLOR_DARK_RED));
      gc.setBackground(getDisplay().getSystemColor(SWT.COLOR_DARK_RED));
      int[] points = new int[area.getBorder().size() * 2];
      int i = 0;
      for(GeoLocation p : area.getBorder())
      {
         final Point virtualXY = GeoLocationCache.coordinateToDisplay(p, accessor.getZoom());
         final int dx = virtualXY.x - centerXY.x;
         final int dy = virtualXY.y - centerXY.y;
         points[i++] = imgW / 2 + dx;
         points[i++] = imgH / 2 + dy + verticalOffset;
      }
      gc.setAlpha(40);
      gc.fillPolygon(points);
      gc.setAlpha(255);
      gc.drawPolygon(points);
   }

   /**
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#onCacheChange(org.netxms.client.objects.AbstractObject, org.netxms.base.GeoLocation)
    */
   @Override
   protected void onCacheChange(AbstractObject object, GeoLocation prevLocation)
   {
   }

   /**
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#getObjectAtPoint(org.eclipse.swt.graphics.Point)
    */
   @Override
   public AbstractObject getObjectAtPoint(Point p)
   {
      return null;
   }
}
