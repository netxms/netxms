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
package org.netxms.ui.eclipse.objectview.api;

import org.eclipse.ui.IViewPart;
import org.netxms.client.objects.AbstractObject;

/**
 * Interface for object details providers
 */
public interface ObjectDetailsProvider
{
	/**
	 * Called by framework to check if provider can provide detailed information for given object.
	 * 
	 * @param object object to evaluate
	 * @return true if provider can provide information
	 */
	public abstract boolean canProvideDetails(AbstractObject object);
	
	/**
	 * Called by framework to provide additional information for object.
	 * 
	 * @param object object to provide information for
	 * @param viewPart active view, can be null
	 */
	public abstract void provideDetails(AbstractObject object, IViewPart viewPart);
}
