/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps;

import java.util.Map;
import java.util.TreeMap;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IConfigurationElement;
import org.eclipse.core.runtime.IExtensionRegistry;
import org.eclipse.core.runtime.Platform;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.networkmaps.api.NetworkMapImageProvider;

/**
 * Manager of network map image providers. There should be only one instance of it,
 * created early during console startup.
 */
public class MapImageProvidersManager
{
	private static MapImageProvidersManager instance = null;
	
	/**
	 * Get color manager instance.
	 * 
	 * @return
	 */
	public static MapImageProvidersManager getInstance()
	{
		return instance;
	}
	
	/**
	 * Create color manager instance
	 */
	protected static void create()
	{
		if (instance == null)
			instance = new MapImageProvidersManager();
	}
	
	private Map<Integer, NetworkMapImageProvider> providers = new TreeMap<Integer, NetworkMapImageProvider>();
	
	/**
	 * Constructor
	 */
	private MapImageProvidersManager()
	{
		// Read all registered extensions and create image providers
		final IExtensionRegistry reg = Platform.getExtensionRegistry();
		IConfigurationElement[] elements = reg.getConfigurationElementsFor("org.netxms.ui.eclipse.networkmaps.imageproviders"); //$NON-NLS-1$
		for(int i = 0; i < elements.length; i++)
		{
			try
			{
				final NetworkMapImageProvider p = (NetworkMapImageProvider)elements[i].createExecutableExtension("class"); //$NON-NLS-1$
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
	 * Get image for object.
	 * 
	 * @return image for given object or null.
	 */
	public Image getMapImage(AbstractObject object)
	{
		for(NetworkMapImageProvider p : providers.values())
		{
			Image i = p.getMapImage(object);
			if (i != null)
				return i;
		}
		return null;
	}
	
	/**
	 * Get status icon for given status code.
	 * 
	 * @return icon for given status code or null.
	 */
	public Image getStatusIcon(ObjectStatus status)
	{
		for(NetworkMapImageProvider p : providers.values())
		{
			Image i = p.getStatusIcon(status);
			if (i != null)
				return i;
		}
		return null;
	}
}
