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
package org.netxms.ui.eclipse.networkmaps.api;

import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objects.AbstractObject;

/**
 * Interface for network map custom image providers
 */
public interface NetworkMapImageProvider
{
	/**
	 * Get map image for given NetXMS object.
	 *  
	 * @param object NetXMS object
	 * @return image for given object or null to continue search
	 */
	public Image getMapImage(AbstractObject object);
	
	/**
	 * Get status icon (shown on top of map image) for given status.
	 * 
	 * @param status status code
	 * @return icon for given status or null to continue search
	 */
	public Image getStatusIcon(ObjectStatus status);
}
