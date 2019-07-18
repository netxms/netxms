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
package org.netxms.nxmc.tools;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;

/**
 * Simple font cache which takes care of font destruction when associated control disposed
 */
public class FontCache implements DisposeListener
{
   private Map<String, Font> cache = new HashMap<String, Font>();

	/**
    * Create unassociated font cache. It must be disposed explicitly by calling dispose().
    */
	public FontCache()
	{
	}

	/**
    * Create font cache associated with given control. All fonts in a cache will be disposed when associated control gets disposed.
    * 
    * @param control control
    */
	public FontCache(Control control)
	{
		control.addDisposeListener(this);
	}

	/**
    * Create font. If already presented in cache, return cached font.
    * 
    * @param name font name
    * @param height font height in points
    * @param style font style (one of SWT.NORMAL, SWT.BOLD, SWT.ITALIC)
    * @return font object
    */
   public Font create(String name, int height, int style)
	{
      String key = name + "@" + Integer.toString(height) + "-" + Integer.toString(style);
      Font font = cache.get(key);
      if (font == null)
		{
         font = new Font(Display.getCurrent(), name, height, style);
         cache.put(key, font);
		}
      return font;
	}

   /**
    * Create normal font. If already presented in cache, return cached font.
    * 
    * @param name font name
    * @param height font height in points
    * @return font object
    */
   public Font create(String name, int height)
   {
      return create(name, height, SWT.NORMAL);
   }

	/**
    * Dispose all fonts in a cache
    */
	public void dispose()
	{
      for(Font f : cache.values())
         f.dispose();
		cache.clear();
	}

   /**
    * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
    */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		dispose();
	}
}
