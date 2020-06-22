/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console.resources;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.netxms.ui.eclipse.tools.ColorCache;
import org.netxms.ui.eclipse.tools.FontCache;

/**
 * Theme manager
 */
public class ThemeEngine
{
   /**
    * Theme engine instances
    */
	private static final Map<Display, ThemeEngine> instances = new HashMap<Display, ThemeEngine>();
	
	/**
	 * Get shared colors instance for given display
	 * 
	 * @param display
	 * @return
	 */
	private static ThemeEngine getInstance(Display display)
	{
		ThemeEngine instance;
		synchronized(instances)
		{
			instance = instances.get(display);
			if (instance == null)
			{
				instance = new ThemeEngine(display);
				instances.put(display, instance);
			}
		}
		return instance;
	}
	
	private ColorCache colors;
   private FontCache fonts;
   private Map<String, ThemeElement> elements;
	
	/**
	 * @param display
	 */
	public ThemeEngine(final Display display)
	{
		colors = new ColorCache();
      fonts = new FontCache();
      elements = new HashMap<String, ThemeElement>();
		display.addListener(SWT.Dispose, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				synchronized(instances)
				{
					instances.remove(display);
				}
				colors.dispose();
            fonts.dispose();
			}
		});
      loadDefaultTheme();
	}
	
   /**
    * Get background color definition
    * 
    * @param path theme element path
    * @return color definition
    */
   public static RGB getBackgroundColorDefinition(String path)
   {
      ThemeEngine instance = getInstance(Display.getCurrent());
      for(ThemeElement e : instance.getElementChain(path))
         if (e.background != null)
            return e.background;
      return Display.getCurrent().getSystemColor(SWT.COLOR_WIDGET_BACKGROUND).getRGB();
   }
   
   /**
    * Get background color definition in text form (as string in format "red,green,blue")
    * 
    * @param path theme element path
    * @return color definition
    */
   public static String getBackgroundColorDefinitionAsText(String path)
   {
      RGB rgb = getBackgroundColorDefinition(path);
      return rgb.red + "," + rgb.green + "," + rgb.blue;
   }
   
	/**
    * Get background color.
    * 
    * @param path theme element path
    * @return color object
    */
   public static Color getBackgroundColor(String path)
	{
      return getInstance(Display.getCurrent()).colors.create(getBackgroundColorDefinition(path));
	}

   /**
    * Get foreground color definition
    * 
    * @param path theme element path
    * @return color definition
    */
   public static RGB getForegroundColorDefinition(String path)
   {
      ThemeEngine instance = getInstance(Display.getCurrent());
      for(ThemeElement e : instance.getElementChain(path))
         if (e.foreground != null)
            return e.foreground;
      return Display.getCurrent().getSystemColor(SWT.COLOR_WIDGET_FOREGROUND).getRGB();
   }

   /**
    * Get foreground color definition in text form (as string in format "red,green,blue")
    * 
    * @param path theme element path
    * @return color definition
    */
   public static String getForegroundColorDefinitionAsText(String path)
   {
      RGB rgb = getForegroundColorDefinition(path);
      return rgb.red + "," + rgb.green + "," + rgb.blue;
   }

   /**
    * Get foreground color.
    * 
    * @param path theme element path
    * @return color object
    */
   public static Color getForegroundColor(String path)
   {
      return getInstance(Display.getCurrent()).colors.create(getForegroundColorDefinition(path));
   }

   /**
    * Get foreground color definition
    * 
    * @param path theme element path
    * @return color definition
    */
   public static Font getFont(String path)
   {
      ThemeEngine instance = getInstance(Display.getCurrent());
      for(ThemeElement e : instance.getElementChain(path))
         if (e.fontName != null)
            return instance.fonts.create(e.fontName, e.fontHeight);
      return Display.getCurrent().getSystemFont();
   }

   /**
    * Get theme element chain from selected element up to the root
    *
    * @param path element path (separated with dots)
    * @return element chain
    */
   private List<ThemeElement> getElementChain(String path)
   {
      List<ThemeElement> chain = new ArrayList<ThemeElement>(16);
      while(!path.isEmpty())
      {
         ThemeElement e = elements.get(path);
         if (e != null)
            chain.add(e);
         int index = path.lastIndexOf('.');
         path = (index != -1) ? path.substring(0, index) : "";
      }
      chain.add(elements.get(".")); // Add root element
      return chain;
   }

   /**
    * Load default theme
    */
   private void loadDefaultTheme()
   {
      elements.clear();
      elements.put(".", new ThemeElement(null, null, null, 0));
      elements.put("Card.Title", new ThemeElement(new RGB(153, 180, 209), new RGB(0, 0, 0)));
      elements.put("Dashboard", new ThemeElement(new RGB(240, 240, 240), null));
      elements.put("GeoMap.Title", new ThemeElement(null, new RGB(0, 0, 0)));
      elements.put("List.Error", new ThemeElement(null, new RGB(255, 0, 0)));
      elements.put("Map.GroupBox", new ThemeElement(null, new RGB(255, 255, 255)));
      elements.put("Map.LastValues", new ThemeElement(null, new RGB(0, 64, 0)));
      elements.put("MessageBar", new ThemeElement(new RGB(255, 252, 192), new RGB(0, 0, 0)));
      elements.put("MibExplorer.Header", new ThemeElement(new RGB(153, 180, 209), new RGB(255, 255, 255)));
      elements.put("ObjectTab.Header", new ThemeElement(new RGB(153, 180, 209), new RGB(255, 255, 255)));
      elements.put("Rack", new ThemeElement(new RGB(255, 255, 255), new RGB(0, 0, 0)));
      elements.put("Rack.Border", new ThemeElement(new RGB(92, 92, 92), new RGB(92, 92, 92)));
      elements.put("Rack.EmptySpace", new ThemeElement(new RGB(224, 224, 224), null));
      elements.put("RuleEditor", new ThemeElement(new RGB(255, 255, 255), new RGB(0, 0, 0)));
      elements.put("RuleEditor.Title.Disabled", new ThemeElement(new RGB(202, 227, 206), null));
      elements.put("RuleEditor.Title.Normal", new ThemeElement(new RGB(225, 233, 241), null));
      elements.put("RuleEditor.Title.Selected", new ThemeElement(new RGB(245, 249, 104), null));
      elements.put("RuleEditor.Border.Action", new ThemeElement(new RGB(186, 176, 201), null));
      elements.put("RuleEditor.Border.Condition", new ThemeElement(new RGB(198, 214, 172), null));
      elements.put("RuleEditor.Border.Rule", new ThemeElement(new RGB(153, 180, 209), null));
      elements.put("ServiceAvailability.Legend", new ThemeElement(null, new RGB(0, 0, 0)));
      elements.put("Status.Normal", new ThemeElement(null, new RGB(0, 192, 0)));
      elements.put("Status.Warning", new ThemeElement(null, new RGB(0, 255, 255)));
      elements.put("Status.Minor", new ThemeElement(null, new RGB(231, 226, 0)));
      elements.put("Status.Major", new ThemeElement(null, new RGB(255, 128, 0)));
      elements.put("Status.Critical", new ThemeElement(null, new RGB(192, 0, 0)));
      elements.put("Status.Unknown", new ThemeElement(null, new RGB(0, 0, 128)));
      elements.put("Status.Unmanaged", new ThemeElement(null, new RGB(192, 192, 192)));
      elements.put("Status.Disabled", new ThemeElement(null, new RGB(128, 64, 0)));
      elements.put("Status.Testing", new ThemeElement(null, new RGB(255, 128, 255)));
      elements.put("StatusMap.Text", new ThemeElement(null, new RGB(0, 0, 0)));
      elements.put("TextInput.Error", new ThemeElement(new RGB(255, 0, 0), null));
   }

   /**
    * Theme element
    */
   private class ThemeElement
   {
      RGB background;
      RGB foreground;
      String fontName;
      int fontHeight;

      public ThemeElement(RGB background, RGB foreground, String fontName, int fontHeight)
      {
         this.background = background;
         this.foreground = foreground;
         this.fontName = fontName;
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
}
