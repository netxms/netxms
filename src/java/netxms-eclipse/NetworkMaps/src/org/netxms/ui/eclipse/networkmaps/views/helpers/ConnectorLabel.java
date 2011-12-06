/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;

/**
 * Connector label
 *
 */
public class ConnectorLabel extends Label
{
	private static final Color FOREGROUND_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
	private static final Color BACKGROUND_COLOR = new Color(Display.getCurrent(), 166, 205, 139);
		
	/**
	 * Create connector label with text
	 * 
	 * @param s label's text
	 */
	public ConnectorLabel(String s)
	{
		super(s);
		initLabel();
	}

	/**
	 * Create connector label with image and text
	 * 
	 * @param s label's text
	 * @param i label's image
	 */
	public ConnectorLabel(String s, Image i)
	{
		super(s, i);
		initLabel();
	}

	/**
	 * Common initialization
	 */
	private void initLabel()
	{
		setForegroundColor(FOREGROUND_COLOR);
		setBackgroundColor(BACKGROUND_COLOR);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Label#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		Rectangle rect = new Rectangle(getBounds());

		gc.setBackgroundColor(BACKGROUND_COLOR);
		gc.setAntialias(SWT.ON);
		gc.fillRoundRectangle(rect, 8, 8);
		
		super.paintFigure(gc);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Label#getPreferredSize(int, int)
	 */
	@Override
	public Dimension getPreferredSize(int wHint, int hHint)
	{
		Dimension d = super.getPreferredSize(wHint, hHint);
		d.height += 2;
		d.width += 4;
		return d;
	}
}
