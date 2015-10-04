/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.osm.widgets;

import java.awt.Polygon;
import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
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
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.MobileDevice;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.osm.GeoLocationCache;
import org.netxms.ui.eclipse.tools.ColorConverter;
import org.netxms.ui.eclipse.tools.FontTools;

/**
 * Geo location viewer for objects
 */
public class ObjectGeoLocationViewer extends AbstractGeoMapViewer
{
   private static final int OBJECT_TOOLTIP_X_MARGIN = 6;
   private static final int OBJECT_TOOLTIP_Y_MARGIN = 6;
   private static final int OBJECT_TOOLTIP_SPACING = 6;

   private static final Color ACTIVE_BORDER_COLOR = new Color(Display.getCurrent(), 255, 255, 255);
   
   private List<AbstractObject> objects = new ArrayList<AbstractObject>();
   private AbstractObject currentObject = null;
   private List<ObjectIcon> objectIcons = new ArrayList<ObjectIcon>();
   private Point objectToolTipLocation = null;
   private Rectangle objectTooltipRectangle = null;
   private Font objectToolTipHeaderFont;
   
   /**
    * @param parent
    * @param style
    */
   public ObjectGeoLocationViewer(Composite parent, int style)
   {
      super(parent, style);

      objectToolTipHeaderFont = FontTools.createFont(TITLE_FONTS, 1, SWT.BOLD);
      
      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            objectToolTipHeaderFont.dispose();
         }
      });
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#onMapLoad()
    */
   @Override
   protected void onMapLoad()
   {
      objects = GeoLocationCache.getInstance().getObjectsInArea(coverage);
      redraw();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#onCacheChange(org.netxms.client.objects.AbstractObject, org.netxms.base.GeoLocation)
    */
   @Override
   protected void onCacheChange(AbstractObject object, GeoLocation prevLocation)
   {
      GeoLocation currLocation = object.getGeolocation();
      if (((currLocation.getType() != GeoLocation.UNSET) && 
            coverage.contains(currLocation.getLatitude(), currLocation.getLongitude()))
            || ((prevLocation != null) && (prevLocation.getType() != GeoLocation.UNSET) && 
                  coverage.contains(prevLocation.getLatitude(), prevLocation.getLongitude())))
      {
         objects = GeoLocationCache.getInstance().getObjectsInArea(coverage);
         redraw();
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#mouseMove(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseMove(MouseEvent e)
   {
      super.mouseMove(e);
      if ((objectTooltipRectangle != null) && !objectTooltipRectangle.contains(e.x, e.y))
      {
         objectTooltipRectangle = null;
         objectToolTipLocation = null;
         currentObject = null;
         redraw();
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#mouseDown(org.eclipse.swt.events.MouseEvent)
    */
   @Override
   public void mouseDown(MouseEvent e)
   {
      super.mouseDown(e);
      if (e.button != 1)
      {
         AbstractObject object = getObjectAtPoint(currentPoint);
         if (object != currentObject)
            setCurrentObject(object);
      }
   }

   /**
    * Set current object
    * 
    * @param object
    */
   private void setCurrentObject(AbstractObject object)
   {
      currentObject = object;
      if (currentObject != null)
      {
         int idx = objects.indexOf(currentObject);
         objects.remove(idx);
         objects.add(currentObject);
      }
      else
      {
         objectToolTipLocation = null;
         objectTooltipRectangle = null;
      }
      redraw();
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#drawContent(org.eclipse.swt.graphics.GC, org.netxms.base.GeoLocation, int, int)
    */
   @Override
   protected void drawContent(GC gc, GeoLocation currentLocation, int imgW, int imgH)
   {
      objectIcons.clear();
      
      final Point centerXY = GeoLocationCache.coordinateToDisplay(currentLocation, accessor.getZoom());
      for(AbstractObject object : objects)
      {
         final Point virtualXY = GeoLocationCache.coordinateToDisplay(object.getGeolocation(), accessor.getZoom());
         final int dx = virtualXY.x - centerXY.x;
         final int dy = virtualXY.y - centerXY.y;
         drawObject(gc, imgW / 2 + dx, imgH / 2 + dy, object);
      }
      if (objectToolTipLocation != null)
         drawObjectToolTip(gc);
   }

   /**
    * Draw object on map
    * 
    * @param gc
    * @param x
    * @param y
    * @param object
    */
   private void drawObject(GC gc, int x, int y, AbstractObject object) 
   {
      boolean selected = (currentObject != null) && (currentObject.getObjectId() == object.getObjectId());
      
      Image image = labelProvider.getImage(object);
      if (image == null)
         image = SharedIcons.IMG_UNKNOWN_OBJECT;
      
      int w = image.getImageData().width + LABEL_X_MARGIN * 2;
      int h = image.getImageData().height+ LABEL_Y_MARGIN * 2;
      Rectangle rect = new Rectangle(x - w / 2 - 1, y - LABEL_ARROW_HEIGHT - h, w, h);
      
      Color bgColor = ColorConverter.adjustColor(StatusDisplayInfo.getStatusColor(object.getStatus()), new RGB(255, 255, 255),  0.5f, colorCache);
      
      gc.setBackground(bgColor);
      gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 4, 4);
      if (selected)
      {
         gc.setLineWidth(3);
         gc.setForeground(ACTIVE_BORDER_COLOR);
         gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 4, 4);
      }
      gc.setLineWidth(1);
      gc.setForeground(BORDER_COLOR);
      gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 4, 4);
      
      final int[] arrow = new int[] { x - 4, rect.y + rect.height, x, y, x + 4, rect.y + rect.height };

      gc.fillPolygon(arrow);
      gc.setForeground(bgColor);
      gc.drawPolygon(arrow);
      if (selected)
      {
         gc.drawLine(arrow[0], arrow[1] - 1, arrow[4], arrow[5] - 1);
         gc.setLineWidth(3);
         gc.setForeground(ACTIVE_BORDER_COLOR);
         gc.drawPolyline(arrow);
      }
      gc.setLineWidth(1);
      gc.setForeground(BORDER_COLOR);
      gc.drawPolyline(arrow);

      gc.drawImage(image, rect.x + LABEL_X_MARGIN, rect.y + LABEL_Y_MARGIN);
      
      objectIcons.add(new ObjectIcon(object, rect, x, y));
   }
   
   /**
    * Draw tooltip for current object
    * 
    * @param gc
    */
   private void drawObjectToolTip(GC gc)
   {
      gc.setFont(objectToolTipHeaderFont);
      Point titleSize = gc.textExtent(currentObject.getObjectName());
      gc.setFont(JFaceResources.getDefaultFont());
      
      // Calculate width and height
      int width = Math.max(titleSize.x + 12, 128);
      int height = OBJECT_TOOLTIP_Y_MARGIN * 2 + titleSize.y + 2 + OBJECT_TOOLTIP_SPACING;
      
      final String location = currentObject.getGeolocation().toString();
      Point pt = gc.textExtent(location);
      if (width < pt.x)
         width = pt.x;
      height += pt.y;
      
      String locationDetails;
      if (currentObject.getGeolocation().getTimestamp().getTime() > 0)
      {
         locationDetails = String.format("Obtained at %s from %s", 
               RegionalSettings.getDateTimeFormat().format(currentObject.getGeolocation().getTimestamp()),
               (currentObject.getGeolocation().getType() == GeoLocation.GPS) ? "GPS" : "network");
         pt = gc.textExtent(locationDetails);
         if (width < pt.x)
            width = pt.x;
         height += pt.y;
      }
      else
      {
         locationDetails = null;
      }
      
      final String postalAddress = currentObject.getPostalAddress().getAddressLine();
      if (!postalAddress.isEmpty())
      {
         pt = gc.textExtent(postalAddress);
         if (width < pt.x)
            width = pt.x;
         height += pt.y + OBJECT_TOOLTIP_SPACING;
      }
      
      String lastReport, batteryLevel;
      if (currentObject instanceof MobileDevice)
      {
         lastReport = String.format("Last report: %s",
               ((MobileDevice)currentObject).getLastReportTime().getTime() > 0 ?
                     RegionalSettings.getDateTimeFormat().format(((MobileDevice)currentObject).getLastReportTime()) :
                     "never");
         pt = gc.textExtent(lastReport);
         if (width < pt.x)
            width = pt.x;
         height += pt.y + OBJECT_TOOLTIP_SPACING * 2 + 1;
         
         if (((MobileDevice)currentObject).getBatteryLevel() >= 0)
         {
            batteryLevel = String.format("Battery level: %d%%", ((MobileDevice)currentObject).getBatteryLevel());
            pt = gc.textExtent(batteryLevel);
            if (width < pt.x)
               width = pt.x;
            height += pt.y;
         }
         else
         {
            batteryLevel = null;
         }
      }
      else
      {
         lastReport = null;
         batteryLevel = null;
      }
      
      if ((currentObject.getComments() != null) && !currentObject.getComments().isEmpty())
      {
         pt = gc.textExtent(currentObject.getComments());
         if (width < pt.x)
            width = pt.x;
         height += pt.y + OBJECT_TOOLTIP_SPACING * 2 + 1;
      }
      
      width += OBJECT_TOOLTIP_X_MARGIN * 2;
      
      Rectangle ca = getClientArea();
      Rectangle rect = new Rectangle(objectToolTipLocation.x - width / 2, objectToolTipLocation.y - height / 2, width, height);
      if (rect.x < 0)
         rect.x = 0;
      else if (rect.x + rect.width >= ca.width)
         rect.x = ca.width - rect.width - 1;
      if (rect.y < 0)
         rect.y = 0;
      else if (rect.y + rect.height  >= ca.height)
         rect.y = ca.height - rect.height - 1;
      
      gc.setBackground(colorCache.create(224, 224, 224));
      gc.setAlpha(192);
      gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 3, 3);
      
      gc.setForeground(colorCache.create(92, 92, 92));
      gc.setAlpha(255);
      gc.setLineWidth(3);
      gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 3, 3);
      gc.setLineWidth(1);
      int y = rect.y + OBJECT_TOOLTIP_Y_MARGIN + titleSize.y + 2;
      gc.drawLine(rect.x + 1, y, rect.x + rect.width - 1, y);
      
      gc.setBackground(StatusDisplayInfo.getStatusColor(currentObject.getStatus()));
      gc.fillOval(rect.x + OBJECT_TOOLTIP_X_MARGIN, rect.y + OBJECT_TOOLTIP_Y_MARGIN + titleSize.y / 2 - 4, 8, 8);
      
      gc.setForeground(colorCache.create(0, 0, 0));
      gc.setFont(objectToolTipHeaderFont);
      gc.drawText(currentObject.getObjectName(), rect.x + OBJECT_TOOLTIP_X_MARGIN + 12, rect.y + OBJECT_TOOLTIP_Y_MARGIN, true);
      
      gc.setFont(JFaceResources.getDefaultFont());
      int textLineHeight = gc.textExtent("M").y;
      y = rect.y + OBJECT_TOOLTIP_Y_MARGIN + titleSize.y + OBJECT_TOOLTIP_SPACING + 2;
      gc.drawText(location, rect.x + OBJECT_TOOLTIP_X_MARGIN, y, true);
      if (locationDetails != null)
      {
         y += textLineHeight;
         gc.drawText(locationDetails, rect.x + OBJECT_TOOLTIP_X_MARGIN, y, true);
      }
      if (!postalAddress.isEmpty())
      {
         y += textLineHeight;
         gc.drawText(postalAddress, rect.x + OBJECT_TOOLTIP_X_MARGIN, y, true);
      }
      
      if (lastReport != null)
      {
         y += textLineHeight + OBJECT_TOOLTIP_SPACING;
         gc.setForeground(colorCache.create(92, 92, 92));
         gc.drawLine(rect.x + 1, y, rect.x + rect.width - 1, y);
         y += OBJECT_TOOLTIP_SPACING;
         gc.setForeground(colorCache.create(0, 0, 0));
         gc.drawText(lastReport, rect.x + OBJECT_TOOLTIP_X_MARGIN, y, true);
         if (batteryLevel != null)
         {
            y += textLineHeight;
            gc.drawText(batteryLevel, rect.x + OBJECT_TOOLTIP_X_MARGIN, y, true);
         }
      }
      
      if ((currentObject.getComments() != null) && !currentObject.getComments().isEmpty())
      {
         y += textLineHeight + OBJECT_TOOLTIP_SPACING;
         gc.setForeground(colorCache.create(92, 92, 92));
         gc.drawLine(rect.x + 1, y, rect.x + rect.width - 1, y);
         y += OBJECT_TOOLTIP_SPACING;
         gc.setForeground(colorCache.create(0, 0, 0));
         gc.drawText(currentObject.getComments(), rect.x + OBJECT_TOOLTIP_X_MARGIN, y, true);
      }
      
      objectTooltipRectangle = rect;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.osm.widgets.AbstractGeoMapViewer#getObjectAtPoint(org.eclipse.swt.graphics.Point)
    */
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

   /**
    * Object icon on map
    */
   private class ObjectIcon
   {
      public Rectangle rect;
      public Polygon arrow;
      public AbstractObject object;

      public ObjectIcon(AbstractObject object, Rectangle rect, int x, int y)
      {
         this.rect = rect;
         this.object = object;
         arrow = new Polygon();
         arrow.addPoint(x, y);
         arrow.addPoint(x - 4, rect.y + rect.height);
         arrow.addPoint(x + 4, rect.y + rect.height);
      }
      
      public boolean contains(Point p)
      {
         return rect.contains(p) || arrow.contains(p.x, p.y); 
      }
   }
}
