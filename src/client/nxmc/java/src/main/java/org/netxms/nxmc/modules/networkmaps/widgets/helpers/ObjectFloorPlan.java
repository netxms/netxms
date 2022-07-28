/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import org.eclipse.draw2d.BorderLayout;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.MouseListener;
import org.eclipse.draw2d.MouseMotionListener;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.nxmc.resources.StatusDisplayInfo;

public class ObjectFloorPlan extends ObjectFigure
{
   private static final int MARGIN_X = 3;
   private static final int MARGIN_Y = 3;      
   private static final int TOP_LEFT = 0;
   private static final int TOP_RIGHT = 1;
   private static final int BOTTOM_LEFT = 2;
   private static final int BOTTOM_RIGHT = 3;
   
   private NetworkMapObject element;
   private boolean resize = true;
   private Label label;
   private int lastX;
   private int lastY;

   /**
    * Constructor
    * 
    * @param element Network map object
    * @param labelProvider map label provider
    */
   public ObjectFloorPlan(NetworkMapObject element, MapLabelProvider labelProvider)
   {
      super(element, labelProvider);
      this.element = element;
      setLayoutManager(new BorderLayout());      
      setSize(element.getWidth(), element.getHeight());
      
      label = new Label(object.getNameOnMap());
      label.setFont(labelProvider.getTitleFont());
      label.setLabelAlignment(PositionConstants.CENTER);
      add(label);
      
      Dimension d = label.getPreferredSize();
      label.setSize(d);
      label.setLocation(new Point(element.getWidth() / 2 - d.width / 2, element.getHeight() / 2 - d.height / 2));
      setBackgroundColor(StatusDisplayInfo.getStatusColor(object.getStatus()));
      
      createResizeHandle(BOTTOM_RIGHT);
   }
  
   /**
    * Create resize handle
    */
   private void createResizeHandle(int pos)
   {
      final Figure handle = new Figure();
      add(handle);
      Dimension size = getSize();
      handle.setSize(8, 8);
      
      switch(pos)
      {
         case TOP_LEFT:
            handle.setLocation(new Point(-1, -1));
            handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENWSE));
            break;
         case TOP_RIGHT:
            handle.setLocation(new Point(size.width - 7, -1));
            handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENESW));
            break;
         case BOTTOM_LEFT:
            handle.setLocation(new Point(-1, size.height - 7));
            handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENESW));
            break;
         case BOTTOM_RIGHT:
            handle.setLocation(new Point(size.width - 7, size.height - 7));
            handle.setCursor(Display.getCurrent().getSystemCursor(SWT.CURSOR_SIZENWSE));
            break;
      }
      
      handle.addMouseListener(new MouseListener() {
         @Override
         public void mouseReleased(MouseEvent me)
         {
            if (resize)
            {
               resize = false;
               Dimension size = getSize();
               element.setSize(size.width, size.height);
            }
         }
         
         @Override
         public void mousePressed(MouseEvent me)
         {
            if (me.button == 1)
            {
               resize = true;
               lastX = me.x;
               lastY = me.y;
               me.consume();
            }
         }
         
         @Override
         public void mouseDoubleClicked(MouseEvent me)
         {
         }
      });
      handle.addMouseMotionListener(new MouseMotionListener() {
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
            if (resize)
            {
               Dimension size = getSize();
               
               int dx = me.x - lastX;
               int dy = me.y - lastY;
               
               if ((dx < 0) && (size.width <= 40))
                  dx = 0;
               if ((dy < 0) && (size.height <= 20))
                  dy = 0;
               
               size.width += dx;
               size.height += dy;
               setSize(size);
               
               Point p = handle.getLocation();
               p.performTranslate(dx, dy);
               handle.setLocation(p);
               
               lastX = me.x;
               lastY = me.y;
               me.consume();

               Dimension d = label.getPreferredSize();
               p = getLocation();
               label.setLocation(new Point(p.x + size.width / 2 - d.width/2, p.y + size.height / 2 - d.height / 2));
            }
         }
      });
   }

   /* (non-Javadoc)
    * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
    */
   @Override
   protected void paintFigure(Graphics gc)
   {
      gc.setAntialias(SWT.ON);
      
      Rectangle rect = new Rectangle(getBounds());
      
      // Adjust border to fit inside the rectangle
      rect.x++;
      rect.y++;
      rect.width -= MARGIN_X;
      rect.height -= MARGIN_Y;
      
      gc.setAlpha(255);      
      gc.setForegroundColor(isElementSelected() ? SELECTION_COLOR : StatusDisplayInfo.getStatusColor(object.getStatus()));
      gc.setLineWidth(3);
      gc.setLineStyle(isElementSelected() ? SWT.LINE_DOT : SWT.LINE_SOLID);
      gc.drawRoundRectangle(rect, 8, 8);

      if (!labelProvider.isTranslucentLabelBackground())
      {
         gc.setBackgroundColor(SOLID_WHITE);
         gc.fillRoundRectangle(getBounds(), 8, 8);
      }
      gc.setBackgroundColor(isElementSelected() ? SELECTION_COLOR : StatusDisplayInfo.getStatusColor(object.getStatus()));
      gc.setAlpha(32);
      gc.fillRoundRectangle(getBounds(), 8, 8);
   }

   
}
