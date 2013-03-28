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
package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ILightweightLabelDecorator;
import org.netxms.client.objects.AbstractObject;

/**
 * Label decorator for NetXMS objects
 */
public class ObjectDecorator implements ILightweightLabelDecorator
{
	// Status images
	private ImageDescriptor[] statusImages;
	
	/**
	 * Default constructor
	 */
	public ObjectDecorator()
	{
		statusImages = new ImageDescriptor[9];
		statusImages[1] = Activator.getImageDescriptor("icons/status/warning.png"); //$NON-NLS-1$
		statusImages[2] = Activator.getImageDescriptor("icons/status/minor.png"); //$NON-NLS-1$
		statusImages[3] = Activator.getImageDescriptor("icons/status/major.png"); //$NON-NLS-1$
		statusImages[4] = Activator.getImageDescriptor("icons/status/critical.png"); //$NON-NLS-1$
		statusImages[5] = Activator.getImageDescriptor("icons/status/unknown.gif"); //$NON-NLS-1$
		statusImages[6] = Activator.getImageDescriptor("icons/status/unmanaged.gif"); //$NON-NLS-1$
		statusImages[7] = Activator.getImageDescriptor("icons/status/disabled.gif"); //$NON-NLS-1$
		statusImages[8] = Activator.getImageDescriptor("icons/status/testing.png"); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void addListener(ILabelProviderListener listener)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
	 */
	@Override
	public void dispose()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
	 */
	@Override
	public boolean isLabelProperty(Object element, String property)
	{
		return false;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
	 */
	@Override
	public void removeListener(ILabelProviderListener listener)
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILightweightLabelDecorator#decorate(java.lang.Object, org.eclipse.jface.viewers.IDecoration)
	 */
	@Override
	public void decorate(Object element, IDecoration decoration)
	{
		int status = ((AbstractObject)element).getStatus();
		decoration.addOverlay(statusImages[status], IDecoration.BOTTOM_RIGHT);
	}
}
