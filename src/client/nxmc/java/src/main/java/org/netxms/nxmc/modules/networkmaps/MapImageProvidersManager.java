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
package org.netxms.nxmc.modules.networkmaps;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.ServiceLoader;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.services.NetworkMapImageProvider;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manager of network map image providers. There should be only one instance of it,
 * created early during console startup.
 */
public class MapImageProvidersManager
{
   private static final Logger logger = LoggerFactory.getLogger(MapImageProvidersManager.class);

   private static MapImageProvidersManager instance = new MapImageProvidersManager();

	/**
    * Get map image providers manager instance.
    * 
    * @return
    */
	public static MapImageProvidersManager getInstance()
	{
		return instance;
	}

   private List<NetworkMapImageProvider> providers = new ArrayList<NetworkMapImageProvider>(0);

	/**
	 * Constructor
	 */
	private MapImageProvidersManager()
	{
		// Read all registered extensions and create image providers
      ServiceLoader<NetworkMapImageProvider> loader = ServiceLoader.load(NetworkMapImageProvider.class, getClass().getClassLoader());
      for(NetworkMapImageProvider p : loader)
      {
         logger.debug("Adding network map image provider " + p.getDescription());
         providers.add(p);
      }
      providers.sort(new Comparator<NetworkMapImageProvider>() {
         @Override
         public int compare(NetworkMapImageProvider p1, NetworkMapImageProvider p2)
         {
            return p1.getPriority() - p2.getPriority();
         }
      });
	}

	/**
	 * Get image for object.
	 * 
	 * @return image for given object or null.
	 */
	public Image getMapImage(AbstractObject object)
	{
      for(NetworkMapImageProvider p : providers)
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
      for(NetworkMapImageProvider p : providers)
		{
			Image i = p.getStatusIcon(status);
			if (i != null)
				return i;
		}
		return null;
	}
}
