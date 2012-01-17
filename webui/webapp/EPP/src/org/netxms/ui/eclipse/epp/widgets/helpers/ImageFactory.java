/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.epp.widgets.helpers;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.ui.eclipse.epp.Activator;

/**
 * Image factory for policy editor. Create images from descriptors and caches them for future use.
 *
 */
public class ImageFactory
{
	private static Map<String, Image> cache = new HashMap<String, Image>();
	private static WorkbenchLabelProvider wbLabelProvider = null;

	/**
	 * Get image by descriptor
	 * @param name Image name
	 * @return Image object
	 */
	public static Image getImage(ImageDescriptor d)
	{
		Image image = cache.get(d.toString());
		if (image == null)
		{
			image = d.createImage(true);
			cache.put(d.toString(), image);
		}
		return image;
	}
	
	/**
	 * Get image by name
	 * @param name Image name
	 * @return Image object
	 */
	public static Image getImage(final String name)
	{
		Image image = cache.get(name);
		if (image == null)
		{
			ImageDescriptor d = Activator.getImageDescriptor(name);
			image = d.createImage(true);
			cache.put(name, image);
		}
		return image;
	}
	
	/**
	 * Get image from workbench label provider
	 * 
	 * @param object Object to get image for
	 * @return Image
	 */
	public static Image getWorkbenchImage(Object object)
	{
		if (wbLabelProvider == null)
			wbLabelProvider = new WorkbenchLabelProvider();
		return wbLabelProvider.getImage(object);
	}
	
	/**
	 * Clear image cache
	 */
	public static void clearCache()
	{
		for(Image image : cache.values())
		{
			image.dispose();
		}
		cache.clear();
		
		if (wbLabelProvider != null)
		{
			wbLabelProvider.dispose();
			wbLabelProvider = null;
		}
	}
}
