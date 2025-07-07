/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.LineBorder;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Insets;
import org.eclipse.draw2d.geometry.Point;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Connector label
 */
public class ConnectorLabel extends Label
{
	private static final Color DEFAULT_FOREGROUND_COLOR = new Color(Display.getCurrent(), 0, 0, 0);
   private static final Color DEFAULT_BACKGROUND_COLOR = new Color(Display.getCurrent(), 240, 240, 240);
   private static final Color DEFAULT_BORDER_COLOR = new Color(Display.getCurrent(), 64, 64, 64);

	private Color backgroundColor = null;
   private ColorCache cCache = new ColorCache();
   private MapLabelProvider labelProvider;

	/**
	 * Create connector label with text
	 * 
	 * @param s label's text
	 */
	public ConnectorLabel(String s, MapLabelProvider labelProvider)
	{
		super(s);
		initLabel();
		this.labelProvider = labelProvider;
      setFont(labelProvider.getLabelFont());
      setBorder(new LineBorder(DEFAULT_BORDER_COLOR, 1, SWT.LINE_DOT));
	}

	/**
    * Create connector label with text and custom
    * background color
    *
    * @param s label`s text
    * @param backgroundColor label`s background color
    */
   public ConnectorLabel(String s, MapLabelProvider labelProvider, Color backgroundColor)
   {
      super(s);
      this.backgroundColor = backgroundColor;
      this.labelProvider = labelProvider;
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
		setForegroundColor(backgroundColor == null ? DEFAULT_FOREGROUND_COLOR : 
		   ColorConverter.selectTextColorByBackgroundColor(backgroundColor, cCache));
		setBackgroundColor(backgroundColor == null ? DEFAULT_BACKGROUND_COLOR : backgroundColor);
	}

   /**
    * @see org.eclipse.draw2d.Label#paintFigure(org.eclipse.draw2d.Graphics)
    */
	@Override
	protected void paintFigure(Graphics gc)
	{
      setFont(labelProvider.getLabelFont());
      
		Rectangle bounds = getBounds();

		gc.setBackgroundColor(backgroundColor == null ? DEFAULT_BACKGROUND_COLOR : backgroundColor);
		gc.setAntialias(SWT.ON);
		gc.fillRoundRectangle(bounds, 8, 8);

		gc.translate(bounds.x, bounds.y);
		final Point pos = getTextLocation();
		gc.drawText(getSubStringText(), pos.x + 3, pos.y);
		gc.translate(-bounds.x, -bounds.y);
	}

   /**
    * @see org.eclipse.draw2d.Label#getPreferredSize(int, int)
    */
	@Override
	public Dimension getPreferredSize(int wHint, int hHint)
	{
		Dimension d = calculateLabelSize(getTextSize());;
		Insets insets = getInsets();
		d.expand(insets.getWidth(), insets.getHeight());
		if (getLayoutManager() != null)
			d.union(getLayoutManager().getPreferredSize(this, wHint,
					hHint));
		d.height += 4;
		d.width += 6;
		return d;
	}
}
