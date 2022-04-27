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

import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;

/**
 * Composite with lightweight border (Windows 7 style)
 */
public class DashboardComposite extends Canvas
{
	protected ColorCache colors;

	private Color borderOuterColor;
	private Color borderInnerColor;

	/**
	 * @param parent
	 * @param style
	 */
	public DashboardComposite(Composite parent, int style)
	{
		super(parent, style);
		setData(RWT.CUSTOM_VARIANT, "DashboardComposite");
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

      setLayout(new FillLayout());
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
