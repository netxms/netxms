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

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.widgets.Display;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Font tools
 */
public class FontTools
{
   private static final String[] TITLE_FONTS = { "Segoe UI", "Liberation Sans", "DejaVu Sans", "Verdana", "Arial" };
   
   //private static Set<String> availableFonts = null;
   //private static Map<String, Font> fontCache = new HashMap<String, Font>();  
   
   /**
    * Find first available font from given list
    * 
    * @param names
    * @return
    */
   @SuppressWarnings("unchecked")
   public static String findFirstAvailableFont(String[] names)
   {
      Set<String> availableFonts = (Set<String>)ConsoleSharedData.getProperty("FontTools.availableFonts");
      if (availableFonts == null)
      {
         availableFonts = new HashSet<String>();
         FontData[] fonts = Display.getCurrent().getFontList(null, true);
         for(FontData fd : fonts)
         {
            availableFonts.add(fd.getName().toUpperCase());
         }
         ConsoleSharedData.setProperty("FontTools.availableFonts", availableFonts);
      }

      for(String name : names)
      {
         if (availableFonts.contains(name.toUpperCase()))
            return name;
      }
      return null;
   }
   
   /**
    * Get font cache for current session
    * 
    * @return font cache
    */
   @SuppressWarnings("unchecked")
   private static Map<String, Font> getFontCache()
   {
      Map<String, Font> fontCache = (Map<String, Font>)ConsoleSharedData.getProperty("FontTools.fontCache");
      if (fontCache == null)
      {
         fontCache = new HashMap<String, Font>();
         ConsoleSharedData.setProperty("FontTools.fontCache", fontCache);
      }
      return fontCache;
   }

   /**
    * Get first available font from given list with adjusted height. Fonts will be created as needed
    * and cached within font tools class.
    * 
    * @param names possible font names
    * @param heightAdjustment height adjustment
    * @param style font style
    * @return font object
    */
   public static Font getFont(String[] names, int heightAdjustment, int style)
   {
      String name = findFirstAvailableFont(names);
      if (name == null)
         return null;
      
      Map<String, Font> fontCache = getFontCache();
      
      String key = name + "/A=" + heightAdjustment + "/" + style;
      Font f = fontCache.get(key);
      if (f != null)
         return f;

      f = new Font(Display.getCurrent(), name, JFaceResources.getDefaultFont().getFontData()[0].getHeight() + heightAdjustment, style);
      fontCache.put(key, f);
      return f;
   }
   
   /**
    * Get array of font objects with first available font name from given list with increasing height. 
    * Fonts will be created as needed and cached within font tools class.
    * 
    * @param names possible font names
    * @param baseHeight base font height
    * @param style font style
    * @param count number of fonts to be created
    * @return array of font objects
    */
   public static Font[] getFonts(String[] names, int baseHeight, int style, int count)
   {
      String name = findFirstAvailableFont(names);
      if (name == null)
         return null;
   
      Map<String, Font> fontCache = getFontCache();
      
      Font[] fonts = new Font[count];
      for(int i = 0; i < count; i++)
      {
         int height = baseHeight + i;
         String key = name + "/H=" + height + "/" + style;
         Font f = fontCache.get(key);
         if (f == null)
         {
            f = new Font(Display.getCurrent(), name, height, style);
            fontCache.put(key, f);
         }
         fonts[i] = f;
      }
      return fonts;
   }
   
   /**
    * Create first available font from given list with adjusted height
    * 
    * @param names possible font names
    * @param heightAdjustment height adjustment
    * @param style font style
    * @return font object
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
    * @param names possible font names
    * @param style font style
    * @return font object
    */
   public static Font createFont(String[] names, int style)
   {
      return createFont(names, 0, style);
   }
   
   /**
    * Create standard title font
    * 
    * @return title font
    */
   public static Font createTitleFont()
   {
      return createFont(TITLE_FONTS, 2, SWT.BOLD);
   }
}
