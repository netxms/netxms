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

import java.util.UUID;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.configs.DCIImageConfiguration;
import org.netxms.client.maps.elements.NetworkMapDCIImage;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;

/**
 * Map decoration figure
 */
public class DCIImageFigure extends DecorationLayerAbstractFigure
{

   private static final int DEFAULT_SIZE = 30;
	
	private LinkDciValueProvider dciValueProvider;
	private DCIImageConfiguration dciImageConfiguration;
	
	/**
	 * @param element
	 * @param labelProvider
	 */
	public DCIImageFigure(NetworkMapDCIImage element, MapLabelProvider labelProvider, ExtendedGraphViewer viewer)
	{
	   super(element, viewer);
	   
		dciImageConfiguration = element.getImageOptions();
		dciValueProvider = LinkDciValueProvider.getinstance();  
		
		//get last DCI value
      DciValue latDCIValue = dciValueProvider.getLastDciData(dciImageConfiguration.getDci());

		try
		{
			UUID guid = UUID.fromString(dciImageConfiguration.getCorrectImage(latDCIValue));
         org.eclipse.swt.graphics.Rectangle bounds = ImageProvider.getInstance(Display.getCurrent()).getImage(guid).getBounds();
         setSize(bounds.width, bounds.height);
		}
		catch(IllegalArgumentException e)
		{			
			setSize(DEFAULT_SIZE, DEFAULT_SIZE);
		}
		setToolTip(new Label(dciImageConfiguration.getDci().getName()));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
	   DciValue latDCIValue = dciValueProvider.getLastDciData(dciImageConfiguration.getDci());
	   try
      {
	      UUID guid = UUID.fromString(dciImageConfiguration.getCorrectImage(latDCIValue));
         Image image = ImageProvider.getInstance(Display.getCurrent()).getImage(guid);
         Rectangle rect = new Rectangle(getBounds());
         gc.drawImage(image, rect.x, rect.y);
      }
      catch(IllegalArgumentException e)
      {        
      }
	}
	
	/**
	 * Refresh figure
	 */
	public void refresh()
	{
		repaint();
	}
}
