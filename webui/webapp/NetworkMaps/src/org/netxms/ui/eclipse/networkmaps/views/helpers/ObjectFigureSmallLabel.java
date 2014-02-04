/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Figure which represents NetXMS object as small icon with label on the right
 */
public class ObjectFigureSmallLabel extends ObjectFigure
{
	private static final int BORDER_WIDTH = 2;
	private static final int MARGIN_WIDTH = 4;
	private static final int MARGIN_HEIGHT = 2;
	
	private Label label;
	
	/**
	 * @param element
	 * @param labelProvider
	 */
	public ObjectFigureSmallLabel(NetworkMapObject element, MapLabelProvider labelProvider)
	{
		super(element, labelProvider);

		setLayoutManager(new BorderLayout());
		
		label = new Label(object.getObjectName());
		label.setFont(labelProvider.getLabelFont());
		label.setLabelAlignment(PositionConstants.CENTER);
		label.setIcon(labelProvider.getWorkbenchIcon(object));
		add(label, BorderLayout.CENTER);
		
		setOpaque(true);
		setBackgroundColor(SharedColors.getColor(SharedColors.MAP_SMALL_LABEL_BACKGROUND, Display.getCurrent()));
		
		updateSize();
	}

	/**
	 * Update figure's size
	 */
	private void updateSize()
	{
		Dimension ls = label.getPreferredSize(-1, -1);
		setSize(ls.width + MARGIN_WIDTH * 2 + BORDER_WIDTH * 2, ls.height + MARGIN_HEIGHT * 2 + BORDER_WIDTH * 2);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		gc.setAntialias(SWT.ON);
		gc.setBackgroundColor(SharedColors.getColor(SharedColors.MAP_SMALL_LABEL_BACKGROUND, Display.getCurrent()));
		gc.setForegroundColor(StatusDisplayInfo.getStatusColor(object.getStatus()));
		Rectangle rect = new Rectangle(getBounds());
		rect.x += BORDER_WIDTH / 2;
		rect.y += BORDER_WIDTH / 2;
		rect.width -= BORDER_WIDTH;
		rect.height -= BORDER_WIDTH;
		gc.setLineWidth(BORDER_WIDTH);
		gc.fillRoundRectangle(rect, 8, 8);
		gc.drawRoundRectangle(rect, 8, 8);
	}
}
