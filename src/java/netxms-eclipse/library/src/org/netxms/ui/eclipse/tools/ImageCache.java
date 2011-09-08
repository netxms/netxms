/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
import java.util.Map;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Control;

/**
 * Simple image cache which takes care of image destruction when associated control disposed
 */
public class ImageCache implements DisposeListener
{
	private Map<ImageDescriptor, Image> cache = new HashMap<ImageDescriptor, Image>();

	/**
	 * Create unassociated image cache. It must be disposed explicitly by calling dispose().
	 */
	public ImageCache()
	{
	}
	
	/**
	 * Create image cache associated with given control. All images in a cache will be disposed when
	 * associatet control gets disposed.
	 * 
	 * @param control control
	 */
	public ImageCache(Control control)
	{
		control.addDisposeListener(this);
	}
	
	/**
	 * Add image to cache. For convenience, return created Image object.
	 * 
	 * @param descriptor image descriptor
	 */
	public Image add(ImageDescriptor descriptor)
	{
		Image image = cache.get(descriptor);
		if (image == null)
		{
			image = descriptor.createImage();
			cache.put(descriptor, image);
		}
		return image;
	}
	
	/**
	 * Get image from cache.
	 * 
	 * @param descriptor
	 * @return
	 */
	public Image get(ImageDescriptor descriptor)
	{
		return cache.get(descriptor);
	}
	
	/**
	 * Dispose all images in a cache
	 */
	public void dispose()
	{
		for(Image image : cache.values())
			image.dispose();
		cache.clear();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		dispose();
	}
}
