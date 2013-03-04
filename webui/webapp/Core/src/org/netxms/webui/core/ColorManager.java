/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.webui.core;

import java.util.Map;
import java.util.TreeMap;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.graphics.RGB;
import org.netxms.ui.eclipse.console.api.ColorProvider;

/**
 * Color manager. There should be only one instance of color manager,
 * created early during console startup.
 */
public class ColorManager
{
	private static ColorManager instance = null;
	
	/**
	 * Get color manager instance.
	 * 
	 * @return
	 */
	public static ColorManager getInstance()
	{
		return instance;
	}
	
	/**
	 * Create color manager instance
	 */
	protected static void create()
	{
		if (instance == null)
			instance = new ColorManager();
	}
	
	private Map<Integer, ColorProvider> providers = new TreeMap<Integer, ColorProvider>();
	
	/**
	 * Constructor
	 */
	private ColorManager()
	{
		// Read all registered extensions and create branding providers
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.colorproviders"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final ColorProvider p = (ColorProvider)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				int priority = 65535;
				String value = elements[i].getAttribute("priority"); //$NON-NLS-1$
				if (value != null)
				{
					try
					{
						priority = Integer.parseInt(value);
						if (priority < 0)
							priority = 65535;
					}
					catch(NumberFormatException e)
					{
					}
				}
				providers.put(priority, p);
			}
			catch(CoreException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	/**
	 * Get color by name.
	 * 
	 * @return image descriptor for login dialog title image or null if no branding provider defines one.
	 */
	public RGB getColor(String name)
	{
		for(ColorProvider p : providers.values())
		{
			RGB rgb = p.getColor(name);
			if (rgb != null)
				return rgb;
		}
		return null;
	}
}
