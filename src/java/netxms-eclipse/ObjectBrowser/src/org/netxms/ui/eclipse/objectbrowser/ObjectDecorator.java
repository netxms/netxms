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
package org.netxms.ui.eclipse.objectbrowser;

import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.BaseLabelProvider;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILightweightLabelDecorator;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;

/**
 * Label decorator for NetXMS objects
 */
public class ObjectDecorator extends BaseLabelProvider implements ILightweightLabelDecorator
{
	// Status images
	private ImageDescriptor[] statusImages;
	private ImageDescriptor maintModeImage;
	private Color maintColor;
	
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
		maintModeImage = Activator.getImageDescriptor("icons/maint_mode.png"); //$NON-NLS-1$
		maintColor = new Color(Display.getDefault(), 96, 96, 144);
	}

	/* (non-Javadoc)
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      maintColor.dispose();
      super.dispose();
   }

   /* (non-Javadoc)
	 * @see org.eclipse.jface.viewers.ILightweightLabelDecorator#decorate(java.lang.Object, org.eclipse.jface.viewers.IDecoration)
	 */
	@Override
	public void decorate(Object element, IDecoration decoration)
	{
		ObjectStatus status = ((AbstractObject)element).getStatus();
		decoration.addOverlay(statusImages[status.getValue()], IDecoration.BOTTOM_RIGHT);
		if (((AbstractObject)element).isInMaintenanceMode())
		{
	      decoration.addOverlay(maintModeImage, IDecoration.TOP_RIGHT);
	      decoration.addSuffix(" [Maintenance]");
	      decoration.setForegroundColor(maintColor);
		}
		if (element instanceof Interface)
		{
		   if ((((Interface)element).getOperState() == Interface.OPER_STATE_DOWN) &&
		       (((Interface)element).getAdminState() == Interface.ADMIN_STATE_UP) &&
		       (((Interface)element).getExpectedState() == Interface.EXPECTED_STATE_IGNORE))
		      decoration.addOverlay(statusImages[ObjectStatus.CRITICAL.getValue()], IDecoration.TOP_LEFT);
		}
	}
}
