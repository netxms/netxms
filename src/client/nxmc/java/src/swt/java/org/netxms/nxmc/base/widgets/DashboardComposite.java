/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.base.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Composite with lightweight border (Windows 7 style)
 */
public class DashboardComposite extends Canvas implements PaintListener
{
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
      if (ColorConverter.isDarkColor(getDisplay().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND).getRGB()))
      {
         borderOuterColor = colors.create(110, 111, 115);
         borderInnerColor = colors.create(53, 53, 53);
      }
      else
      {
         borderOuterColor = colors.create(171, 173, 179);
         borderInnerColor = colors.create(255, 255, 255);
      }
      backgroundColor = getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND);

		hasBorder = ((style & SWT.BORDER) != 0);
		setBackground(backgroundColor);

      addPaintListener(this);
      addDisposeListener((e) -> removePaintListener(this));
	}

   /**
    * @see org.eclipse.swt.widgets.Scrollable#computeTrim(int, int, int, int)
    */
	@Override
	public Rectangle computeTrim(int x, int y, int width, int height)
	{
		Rectangle trim = super.computeTrim(x, y, width, height);
		if (hasBorder)
		{
			trim.x -= 2;
			trim.y -= 2;
			trim.width += 4;
			trim.height += 4;
		}
		return trim;
	}

   /**
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
			area.width -= 4;
			area.height -= 4;
		}
		return area;
	}

   /**
    * @see org.eclipse.swt.widgets.Control#getBorderWidth()
    */
	@Override
	public int getBorderWidth()
	{
		return hasBorder ? 2 : 0;
	}

   /**
    * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
    */
	@Override
	public void paintControl(PaintEvent e)
	{
      if (!hasBorder)
         return;

      Point size = getSize();
      Rectangle rect = new Rectangle(0, 0, size.x, size.y);

      rect.width--;
      rect.height--;
      e.gc.setForeground(borderOuterColor);
      e.gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 2, 2);

      rect.x++;
      rect.y++;
      rect.width -= 2;
      rect.height -= 2;
      e.gc.setForeground(borderInnerColor);
      e.gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 2, 2);
	}

   /**
    * Get full client area (including border)
    * 
    * @return full client area
    */
   protected Rectangle getFullClientArea()
   {
      return super.getClientArea();
   }

   /**
    * @return the borderOuterColor
    */
   protected Color getBorderOuterColor()
   {
      return borderOuterColor;
   }

   /**
    * @return the borderInnerColor
    */
   protected Color getBorderInnerColor()
   {
      return borderInnerColor;
   }
}
