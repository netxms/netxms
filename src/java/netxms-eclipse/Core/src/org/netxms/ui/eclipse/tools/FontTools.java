/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.tools;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.widgets.Display;

/**
 * Font tools
 */
public class FontTools
{
   /**
    * Find first available font from given list
    * 
    * @param names
    * @return
    */
   public static String findFirstAvailableFont(String[] names)
   {
      FontData[] fonts = Display.getCurrent().getFontList(null, true);
      for(String name : names)
      {
         for(FontData fd : fonts)
         {
            if (fd.getName().equalsIgnoreCase(name))
               return name;
         }
      }
      return null;
   }
   
   /**
    * Create first available font from given list with adjusted height
    * 
    * @param names
    * @param height
    * @param style
    * @return
    */
   public static Font createFont(String[] names, int heightAdjustment, int style)
   {
      String name = findFirstAvailableFont(names);
      if (name == null)
         return null;
      return new Font(Display.getCurrent(), name, JFaceResources.getDefaultFont().getFontData()[0].getHeight() + heightAdjustment, style);
   }

   /**
    * Create first available font from given list with same height as default system font
    * 
    * @param names
    * @param style
    * @return
    */
   public static Font createFont(String[] names, int style)
   {
      return createFont(names, 0, style);
   }
}
