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
package org.netxms.ui.eclipse.osm.widgets.helpers;

import org.netxms.base.GeoLocation;

/**
 * Listener which will be notified when geo map widget panned or zoomed
 * by it's internal controls (mouse wheel, for example)
 */
public interface GeoMapListener
{
	/**
	 * Called when zoom level changed 
	 * @param zoomLevel new zoom level
	 */
	public abstract void onZoom(int zoomLevel);

	/**
	 * Called when center point changed 
	 * @param centerPoint new center point
	 */
	public abstract void onPan(GeoLocation centerPoint);
}
