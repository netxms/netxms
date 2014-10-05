/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.BorderLayout;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.maps.elements.NetworkMapObject;

/**
 * Figure representing NetXMS object as icon with object name below
 */
public class ObjectStatusIcon extends ObjectFigure
{
   private static final int IMAGE_MARGIN_Y = 4;
   private static final int BACKGROUND_MARGIN_X = 4;
   private static final int BACKGROUND_MARGIN_Y = 4;
   private static final int FRAME_LINE_WIDTH = 3;
   
   private int imageWidth;
   private int imageHeight;
   
   /**
    * Constructor
    */
   public ObjectStatusIcon(NetworkMapObject element, MapLabelProvider labelProvider)
   {
      super(element, labelProvider);      
      setLayoutManager(new BorderLayout());      
      setOpaque(false);
      updateSize(labelProvider);
   }
   
   /**
    * Update figure's size
    */
   private void updateSize(MapLabelProvider labelProvider)
   {
      Image image = labelProvider.getStatusImage(object); 
      
      if (image != null)
      {
         imageWidth = image.getImageData().width;
         imageHeight = image.getImageData().height;
      }
      else
      {
         imageWidth = 32;
         imageHeight = 32;
      }
      
      setSize(imageWidth + BACKGROUND_MARGIN_X * 2 + FRAME_LINE_WIDTH, imageHeight + IMAGE_MARGIN_Y * 2 + FRAME_LINE_WIDTH);
   }

   /* (non-Javadoc)
    * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
    */
   @Override
   protected void paintFigure(Graphics gc)
   {
      gc.setAntialias(SWT.ON);
      Rectangle rect = new Rectangle(getBounds());
      
      final int imageOffset = (rect.width - imageWidth) / 2;
         
      // Selection frame
      if (isElementSelected())
      {
         rect.x += imageOffset - BACKGROUND_MARGIN_X;
         rect.y += IMAGE_MARGIN_Y - BACKGROUND_MARGIN_Y + 1;
         rect.width = imageWidth + BACKGROUND_MARGIN_X * 2;
         rect.height = imageWidth + BACKGROUND_MARGIN_Y * 2 - 1;
         
         gc.setForegroundColor(SELECTION_COLOR);
         gc.setLineWidth(FRAME_LINE_WIDTH);
         gc.setLineStyle(labelProvider.isElementSelected(element) ? SWT.LINE_DOT : SWT.LINE_SOLID);
         gc.drawRoundRectangle(rect, 16, 16);

         rect = new Rectangle(getBounds());
      }
         
      // Object image
      Image image = labelProvider.getStatusImage(object);  
      if (image != null)
      {
         gc.drawImage(image, rect.x + imageOffset, rect.y + IMAGE_MARGIN_Y);
      } 
   }
}
