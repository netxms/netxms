/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
import org.eclipse.swt.graphics.Color;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Map DCI container
 */
public class DCIContainerFigure extends DecorationLayerAbstractFigure
{
   private static final int BORDER_SIZE = 7;
   private static final int MARGIN = 7;
   
	private LinkDciValueProvider dciValueProvider;
	private NetworkMapDCIContainer container;
	private Label label;
	Color borderColor;
	Color backgroundColor;
	
	/**
	 * @param decoration
	 * @param labelProvider
	 */
	public DCIContainerFigure(NetworkMapDCIContainer container, MapLabelProvider labelProvider, ExtendedGraphViewer viewer)
	{
	   super(container, viewer);
	   
		this.container = container;
		dciValueProvider = LinkDciValueProvider.getInstance();		
		String text = dciValueProvider.getDciDataAsString(container.getDciAsList());
      
      backgroundColor = labelProvider.getColors().create(ColorConverter.rgbFromInt(container.getBackgroundColor()));
      final Color textColor = labelProvider.getColors().create(ColorConverter.rgbFromInt(container.getTextColor()));
      borderColor = labelProvider.getColors().create(ColorConverter.rgbFromInt(container.getBorderColor()));
	
		label = new Label(text.isEmpty() ? "No value" : text);
		label.setFont(labelProvider.getTitleFont());
		add(label);
			
		Dimension d = label.getPreferredSize();
		label.setSize(d.width+MARGIN, d.height+MARGIN);
		int border = 0;
		if(container.isBorderRequired())
		{
		   label.setLocation(new Point(BORDER_SIZE, BORDER_SIZE));
		   border = BORDER_SIZE;
		}
		setSize(d.width + (border *2) + MARGIN, d.height + (border *2) + MARGIN);
      
		label.setForegroundColor(textColor);		
		label.setBackgroundColor(backgroundColor);
	}


   protected void paintFigure(Graphics gc)
   {      
      gc.setAntialias(SWT.ON);
      
      if(container.isBorderRequired())
      {
         Rectangle rect = new Rectangle(label.getClientArea());
         
         gc.setForegroundColor(borderColor);
         gc.setLineWidth(BORDER_SIZE);
         gc.setLineStyle(Graphics.LINE_SOLID);
         gc.drawRoundRectangle(rect, 8, 8);         
      }

      gc.setBackgroundColor(backgroundColor);
      gc.fillRoundRectangle(label.getBounds(), 8, 8);
   }

   
   public void refresh()
   {
      //Set new label text   
      String text = dciValueProvider.getDciDataAsString(container.getDciAsList());      
      label.setText(text.isEmpty() ? "No value" : text);
      
      //Recalculate figure size
      Dimension d = label.getPreferredSize();
      label.setSize(d.width+MARGIN, d.height+MARGIN);
      int border = container.isBorderRequired() ? BORDER_SIZE : 0;
      setSize(d.width + (border *2) + MARGIN, d.height + (border *2) + MARGIN);    
      
      repaint();
   }
}
