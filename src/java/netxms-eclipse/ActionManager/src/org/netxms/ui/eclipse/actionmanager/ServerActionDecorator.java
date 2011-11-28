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
package org.netxms.ui.eclipse.actionmanager;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.DecorationOverlayIcon;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;

/**
 * Label decorator for server actions
 *
 */
public class ServerActionDecorator implements ILabelDecorator
{
	private ImageDescriptor disabledMark;
	
	/**
	 * The constructor
	 */
	public ServerActionDecorator()
	{
		disabledMark = Activator.getImageDescriptor("icons/disabled_overlay.png"); //$NON-NLS-1$
	}

	@Override
	public Image decorateImage(Image image, Object element)
	{
		if (!((ServerAction)element).isDisabled() || (image == null))
			return null;
		
		DecorationOverlayIcon overlay = new DecorationOverlayIcon(image, disabledMark, IDecoration.TOP_RIGHT);
		return overlay.createImage();
	}

	@Override
	public String decorateText(String text, Object element)
	{
		return null;
	}

	@Override
	public void addListener(ILabelProviderListener listener)
	{
	}

	@Override
	public void dispose()
	{
	}

	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		return false;
	}

	@Override
	public void removeListener(ILabelProviderListener listener)
	{
	}
}
