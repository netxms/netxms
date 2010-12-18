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

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * Map decoration figure
 *
 */
public class DecorationFigure extends Figure
{
	private static final int MARGIN_X = 4;
	private static final int MARGIN_Y = 4;
	private static final int LABEL_MARGIN = 5;
	private static final int TITLE_OFFSET = MARGIN_X + 10;

	private NetworkMapDecoration decoration;
	private MapLabelProvider labelProvider;
	private Label label;
	
	public DecorationFigure(NetworkMapDecoration decoration, MapLabelProvider labelProvider)
	{
		this.decoration = decoration;
		this.labelProvider = labelProvider;
		
		setSize(decoration.getWidth(), decoration.getHeight());

		label = new Label(decoration.getTitle());
		label.setFont(labelProvider.getFont());
		add(label);
		
		Dimension d = label.getPreferredSize();
		label.setSize(d.width + LABEL_MARGIN * 2, d.height + 2);
		label.setLocation(new Point(TITLE_OFFSET, 0));
		label.setBackgroundColor(ColorConverter.colorFromInt(decoration.getColor()));
		label.setForegroundColor(new Color(Display.getDefault(), 255, 255, 255));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		switch(decoration.getDecorationType())
		{
			case NetworkMapDecoration.GROUP_BOX:
				drawGroupBox(gc);
				break;
			case NetworkMapDecoration.IMAGE:
				drawImage(gc);
				break;
			default:
				break;
		}
	}
	
	/**
	 * Draw "group box" decoration
	 */
	private void drawGroupBox(Graphics gc)
	{
		Rectangle rect = new Rectangle(getBounds());
		
		rect.x += MARGIN_X;
		rect.y += label.getSize().height / 2;
		rect.width -= MARGIN_X * 2;
		rect.height -= MARGIN_Y * 2 + 1;
		
		gc.setForegroundColor(ColorConverter.colorFromInt(decoration.getColor()));
		gc.setAntialias(SWT.ON);
		gc.setLineWidth(3);
		gc.drawRoundRectangle(rect, 8, 8);
		
		gc.setBackgroundColor(ColorConverter.colorFromInt(decoration.getColor()));
		gc.fillRoundRectangle(label.getBounds(), 8, 8);
	}
	
	/**
	 * Draw "image" decoration
	 */
	private void drawImage(Graphics gc)
	{
		
	}
}
