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

import java.io.File;
import java.io.FilenameFilter;
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
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.tools.ColorCache;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.FontCache;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Theme manager
 */
public class ThemeEngine
{
   private static final Logger logger = LoggerFactory.getLogger(ThemeEngine.class);

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
   private ThemeEngine(final Display display)
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
      reload(display);
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
    * Get storage directory for custom themes.
    *
    * @return storage directory for custom themes
    */
   public static File getThemeStorageDirectory()
   {
      return new File(Registry.getStateDir(), "themes");
   }

   /**
    * Save theme to storage location.
    *
    * @param theme theme to save
    * @throws Exception
    */
   public static void saveTheme(Theme theme) throws Exception
   {
      File storage = getThemeStorageDirectory();
      if (!storage.exists())
         storage.mkdirs();
      File file = new File(storage, theme.getName() + ".xml");
      theme.save(file);
   }

   /**
    * Delete theme file from storage.
    *
    * @param name theme name
    * @return true if file successfully deleted
    */
   public static boolean deleteTheme(String name)
   {
      return new File(getThemeStorageDirectory(), name + ".xml").delete();
   }

   /**
    * Reload current instance of theme manager
    */
   public static void reload()
   {
      Display display = Display.getCurrent();
      getInstance(display).reload(display);
   }

   /**
    * Reload this instance of theme manager
    *
    * @param display associated display
    */
   private void reload(Display display)
   {
      PreferenceStore ps = PreferenceStore.getInstance(display);
      String currentTheme = ps.getString("CurrentTheme");
      if ((currentTheme == null) || currentTheme.isEmpty() || currentTheme.equalsIgnoreCase("[automatic]"))
      {
         loadDefaultTheme(display);
      }
      else if (currentTheme.equalsIgnoreCase("Light [built-in]"))
      {
         loadTheme(new DefaultLightTheme());
      }
      else if (currentTheme.equalsIgnoreCase("Dark [built-in]"))
      {
         loadTheme(new DefaultDarkTheme());
      }
      else
      {
         boolean loaded = false;

         File base = getThemeStorageDirectory();
         if (base.isDirectory())
         {
            for(File f : base.listFiles(new FilenameFilter() {
               @Override
               public boolean accept(File dir, String name)
               {
                  return name.endsWith(".xml");
               }
            }))
            {
               logger.info("Loading theme file " + f.getAbsolutePath());
               try
               {
                  Theme t = Theme.load(f);
                  if (t.getName().equalsIgnoreCase(currentTheme))
                  {
                     logger.info("Applying theme " + t.getName());
                     boolean darkOSTheme = ColorConverter.isDarkColor(display.getSystemColor(SWT.COLOR_WIDGET_BACKGROUND).getRGB());
                     t.setMissingElements(darkOSTheme ? new DefaultDarkTheme() : new DefaultLightTheme());
                     loadTheme(t);
                     loaded = true;
                     break;
                  }
               }
               catch(Exception e)
               {
                  logger.error("Error loading theme file " + f.getAbsolutePath(), e);
               }
            }
         }

         if (!loaded)
            loadDefaultTheme(display);
      }
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
    * Load theme
    *
    * @param theme theme to load
    */
   private void loadTheme(Theme theme)
   {
      elements.clear();
      elements.putAll(theme.elements);
   }

   /**
    * Load default theme
    */
   private void loadDefaultTheme(Display display)
   {
      loadTheme(WidgetHelper.isSystemDarkTheme(display) ? new DefaultDarkTheme() : new DefaultLightTheme());
   }
}
