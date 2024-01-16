/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.nxmc.tools.ColorConverter;

public class TextBoxFigure extends DecorationLayerAbstractFigure
{
   private static final int MARGIN_X = 3;
   private static final int MARGIN_Y = 3;
   private static final int BORDER_SIZE = 3;
   
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

      setLayoutManager(new BorderLayout());

      text = new Label(textBoxElement.getText());
      text.setFont(new Font(Display.getCurrent(), "Verdana", textBoxElement.getFontSize(), SWT.NONE));
      text.setLabelAlignment(PositionConstants.CENTER);
      
      Dimension d = text.getPreferredSize();
      text.setSize(d);

      Point p = text.getLocation();
      p.translate(MARGIN_X*4, MARGIN_Y);
      text.setLocation(p);
      add(text);
      setSize(d.width + MARGIN_X*8, d.height + MARGIN_Y*2);

      text.setForegroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getTextColor())));  
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.views.helpers.DecorationLayerAbstractFigure#paintFigure(org.eclipse.draw2d.Graphics)
    */
   @Override
   protected void paintFigure(Graphics gc)
   {
      gc.setAntialias(SWT.ON);
      
      Rectangle rect = new Rectangle(getBounds());
      rect.x+=MARGIN_X;
      rect.y+=MARGIN_Y;
      rect.width -= MARGIN_X*2;
      rect.height -= MARGIN_Y*2;
      gc.setBackgroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getBackgroundColor())));
      gc.fillRoundRectangle(rect, 3, 3);
      
      if(textBoxElement.isBorderRequired())
      {
         rect = new Rectangle(getBounds());
         rect.x++;
         rect.y++;
         rect.width -= MARGIN_X;
         rect.height -= MARGIN_Y;
         
         gc.setForegroundColor(labelProvider.getColors().create(ColorConverter.rgbFromInt(textBoxElement.getBorderColor())));
         gc.setLineWidth(BORDER_SIZE);
         gc.setLineStyle(SWT.LINE_SOLID);
         gc.drawRoundRectangle(rect, 8, 8);         
      }
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.networkmaps.views.helpers.DecorationLayerAbstractFigure#refresh()
    */
   @Override
   public void refresh()
   {
      text.setText(textBoxElement.getText());
      
      Dimension d = text.getPreferredSize();
      text.setSize(d);
      
      Point p = text.getLocation();
      p.translate(MARGIN_X*4, MARGIN_Y);
      text.setLocation(p);
      setSize(d.width + MARGIN_X*8, d.height + MARGIN_Y*2);  
      
      repaint();
   }

}
