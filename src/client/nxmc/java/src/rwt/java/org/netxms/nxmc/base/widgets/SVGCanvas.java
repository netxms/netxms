/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.svg.SVGImage;

/**
 * Canvas that displays SVG image.
 */
public class SVGCanvas extends Canvas
{
   private SVGImage image = null;
   private Color defaultColor = null;

   /**
    * @param parent
    * @param style
    */
   public SVGCanvas(Composite parent, int style)
   {
      super(parent, style);
      addPaintListener((e) -> {
         if (image != null)
         {
            Point size = getSize();
            image.render(e.gc, 0, 0, size.x, size.y, defaultColor);
         }
      });
   }

   /**
    * @return the image
    */
   public SVGImage getImage()
   {
      return image;
   }

   /**
    * @param image the image to set
    */
   public void setImage(SVGImage image)
   {
      this.image = image;
   }

   /**
    * @return the defaultColor
    */
   public Color getDefaultColor()
   {
      return defaultColor;
   }

   /**
    * @param defaultColor the defaultColor to set
    */
   public void setDefaultColor(Color defaultColor)
   {
      this.defaultColor = defaultColor;
   }
}
