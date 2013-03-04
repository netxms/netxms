/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Composite with lightweight border (Windows 7 style)
 *
 */
public class DashboardComposite extends Canvas implements PaintListener
{
	private static final long serialVersionUID = 1L;

	protected ColorCache colors;
	
	private Color borderOuterColor;
	private Color borderInnerColor;
	private Color backgroundColor;
	private boolean hasBorder = true;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DashboardComposite(Composite parent, int style)
	{
		super(parent, style & ~SWT.BORDER);
		
		colors = new ColorCache(this);
		borderOuterColor = colors.create(171, 173, 179);
		borderInnerColor = colors.create(255, 255, 255);
		backgroundColor = colors.create(255, 255, 255);
		
		hasBorder = ((style & SWT.BORDER) != 0);
		addPaintListener(this);
		setBackground(backgroundColor);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Scrollable#getClientArea()
	 */
	@Override
	public Rectangle getClientArea()
	{
		Rectangle area = super.getClientArea();
		if (hasBorder)
		{
			area.x += 2;
			area.y += 2;
		}
		return area;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#getBorderWidth()
	 */
	@Override
	public int getBorderWidth()
	{
		return 2;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		if (hasBorder)
		{
			Point size = getSize();
			Rectangle rect = new Rectangle(0, 0, size.x, size.y);
			
			rect.width--;
			rect.height--;
			e.gc.setForeground(borderOuterColor);
			e.gc.drawRectangle(rect);
			
			rect.x++;
			rect.y++;
			rect.width -= 2;
			rect.height -= 2;
			e.gc.setForeground(borderInnerColor);
			e.gc.drawRectangle(rect);
		}
	}
}
