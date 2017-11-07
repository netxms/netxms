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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.ui.eclipse.tools.ColorConverter;

public class TextBoxFigure extends DecorationLayerAbstractFigure
{
   private static final int BORDER_SIZE = 7;
   private static final int MARGIN = 7;
   
   private Label text;
   private NetworkMapTextBox textBoxElement;
   private MapLabelProvider labelProvider;

   /**
    * Create a text box figure
    * 
    * @param decoration
    * @param viewer
    */
   public TextBoxFigure(NetworkMapTextBox decoration, MapLabelProvider labelProvider, ExtendedGraphViewer viewer)
   {
      super(decoration, viewer);
      this.textBoxElement = decoration;
      this.labelProvider = labelProvider;

      text = new Label(textBoxElement.getText());
      text.setFont(new Font(Display.getCurrent(), "Verdana", textBoxElement.getFontSize(), SWT.NONE));
      add(text);
      
      Dimension d = text.getPreferredSize();
      text.setSize(d.width+MARGIN, d.height+MARGIN);
      int border = 0;
      if(textBoxElement.isBorderRequired())
      {
         text.setLocation(new Point(BORDER_SIZE, BORDER_SIZE));
         border = BORDER_SIZE;
      }
      setSize(d.width + (border *2) + MARGIN, d.height + (border *2) + MARGIN);
      
      text.setBackgroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getBackgroundColor())));
      text.setForegroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getTextColor())));  
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.views.helpers.DecorationLayerAbstractFigure#paintFigure(org.eclipse.draw2d.Graphics)
    */
   @Override
   protected void paintFigure(Graphics gc)
   {
      gc.setAntialias(SWT.ON);
      
      if(textBoxElement.isBorderRequired())
      {
         Rectangle rect = new Rectangle(text.getClientArea());
         
         gc.setForegroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getTextColor())));
         gc.setLineWidth(BORDER_SIZE);
         gc.setLineStyle(SWT.LINE_SOLID);
         gc.drawRoundRectangle(rect, 8, 8);         
      }

      gc.setBackgroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getBackgroundColor())));
      gc.fillRoundRectangle(text.getBounds(), 8, 8);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.views.helpers.DecorationLayerAbstractFigure#refresh()
    */
   @Override
   public void refresh()
   {
      //Set new label text
      text.setText(textBoxElement.getText());
      
      //Recalculate figure size
      Dimension d = text.getPreferredSize();
      text.setSize(d.width+MARGIN, d.height+MARGIN);
      int border = textBoxElement.isBorderRequired() ? BORDER_SIZE : 0;
      setSize(d.width + (border *2) + MARGIN, d.height + (border *2) + MARGIN);    
      
      repaint();
   }

}
