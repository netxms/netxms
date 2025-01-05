/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.resources;

import org.eclipse.swt.graphics.RGB;
import org.netxms.nxmc.tools.FontTools;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Root;

/**
 * Theme element
 */
@Root(name = "element")
public class ThemeElement
{
   @Element(required = false)
   public RGB background;

   @Element(required = false)
   public RGB foreground;

   @Element(required = false)
   public String fontName;

   @Element(required = false)
   public int fontHeight;

   public ThemeElement()
   {
   }

   public ThemeElement(ThemeElement src)
   {
      this.background = src.background;
      this.foreground = src.foreground;
      this.fontName = src.fontName;
      this.fontHeight = src.fontHeight;
   }

   public ThemeElement(RGB background, RGB foreground, String fontName, int fontHeight)
   {
      this.background = background;
      this.foreground = foreground;
      this.fontName = (fontName != null) ? FontTools.findFirstAvailableFont(fontName.split(",")) : null;
      this.fontHeight = fontHeight;
   }

   public ThemeElement(RGB background, RGB foreground)
   {
      this.background = background;
      this.foreground = foreground;
      this.fontName = null;
      this.fontHeight = 0;
   }
}
