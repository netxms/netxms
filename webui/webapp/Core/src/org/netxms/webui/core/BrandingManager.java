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
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.RGB;
import org.netxms.ui.eclipse.console.api.BrandingProvider;

/**
 * Branding manager. There should be only one instance of branding manager,
 * created early during console startup.
 */
public class BrandingManager
{
	private static BrandingManager instance = null;
	
	/**
	 * Get branding manager instance.
	 * 
	 * @return
	 */
	public static BrandingManager getInstance()
	{
		return instance;
	}
	
	/**
	 * Create branding manager instance
	 */
	protected static void create()
	{
		if (instance == null)
			instance = new BrandingManager();
	}
	
	private Map<Integer, BrandingProvider> providers = new TreeMap<Integer, BrandingProvider>();
	
	/**
	 * Constructor
	 */
	private BrandingManager()
	{
		// Read all registered extensions and create branding providers
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.branding"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final BrandingProvider p = (BrandingProvider)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
				int priority = 65535;
				String value = elements[i].getAttribute("priority");
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
	 * Get default perspective ID. 
	 * 
	 * @return default perspective ID or null if no branding provider defines one.
	 */
	public String getDefaultPerspective()
	{
		for(BrandingProvider p : providers.values())
		{
			String pid = p.getDefaultPerspective();
			if (pid != null)
				return pid;
		}
		return null;
	}
	
	/**
	 * Get image descriptor for login dialog title image.
	 * 
	 * @return image descriptor for login dialog title image or null if no branding provider defines one.
	 */
	public ImageDescriptor getLoginTitleImage()
	{
		for(BrandingProvider p : providers.values())
		{
			ImageDescriptor d = p.getLoginTitleImage();
			if (d != null)
				return d;
		}
		return null;
	}
	
	/**
	 * Get filler color for login dialog title image.
	 * 
	 * @return image descriptor for login dialog title image or null if no branding provider defines one.
	 */
	public RGB getLoginTitleColor()
	{
		for(BrandingProvider p : providers.values())
		{
			RGB rgb = p.getLoginTitleColor();
			if (rgb != null)
				return rgb;
		}
		return null;
	}

	/**
	 * Get default perspective ID. 
	 * 
	 * @return default perspective ID or null if no branding provider defines one.
	 */
	public String getLoginTitle()
	{
		for(BrandingProvider p : providers.values())
		{
			String t = p.getLoginTitle();
			if (t != null)
				return t;
		}
		return null;
	}
}
