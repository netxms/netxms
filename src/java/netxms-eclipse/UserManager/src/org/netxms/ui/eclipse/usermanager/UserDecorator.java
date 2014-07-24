/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.usermanager;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.BaseLabelProvider;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILightweightLabelDecorator;
import org.netxms.api.client.users.AbstractUserObject;

/**
 * Label decorator for NetXMS objects
 */
public class UserDecorator extends BaseLabelProvider implements ILightweightLabelDecorator
{
	private ImageDescriptor imageDisabled;
   private ImageDescriptor imageError;
	
	/**
	 * Default constructor
	 */
	public UserDecorator()
	{
		imageDisabled = Activator.getImageDescriptor("icons/overlay_disabled.gif"); //$NON-NLS-1$		
      imageError = Activator.getImageDescriptor("icons/overlay_error.png"); //$NON-NLS-1$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILightweightLabelDecorator#decorate(java.lang.Object, org.eclipse.jface.viewers.IDecoration)
	 */
	@Override
	public void decorate(Object element, IDecoration decoration)
	{
	   if ((((AbstractUserObject)element).getFlags() & AbstractUserObject.DISABLED) != 0)
	   {
	      decoration.addOverlay(imageDisabled, IDecoration.BOTTOM_RIGHT);
	   }
	   else if ((((AbstractUserObject)element).getFlags() & AbstractUserObject.SYNC_EXCEPTION) != 0)
	   {
         decoration.addOverlay(imageError, IDecoration.BOTTOM_RIGHT);
	   }
	}
}
