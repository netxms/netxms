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
package org.netxms.ui.eclipse.osm;

import java.util.HashMap;
import java.util.Map;

import org.eclipse.ui.IStartup;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.GeoLocation;
import org.netxms.client.NXCNotification;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.osm.tools.QuadTree;

/**
 * Cache for objects' geolocation information
 */
public class GeoLocationCache implements IStartup, SessionListener
{
	private Map<Long, GenericObject> objects = new HashMap<Long, GenericObject>();
	private QuadTree<Long> locationTree = new QuadTree<Long>();
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	@Override
	public void earlyStartup()
	{
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.SessionListener#notificationHandler(org.netxms.api.client.SessionNotification)
	 */
	@Override
	public void notificationHandler(SessionNotification n)
	{
		if (n.getCode() == NXCNotification.OBJECT_CHANGED)
			onObjectChange((GenericObject)n.getObject());
	}
	
	/**
	 * Handle object change
	 * 
	 * @param object
	 */
	private void onObjectChange(GenericObject object)
	{
		if ((object.getObjectClass() != GenericObject.OBJECT_NODE) ||
		    (object.getObjectClass() != GenericObject.OBJECT_CLUSTER) ||
		    (object.getObjectClass() != GenericObject.OBJECT_CONTAINER))
			return;
		
		GeoLocation gl = object.getGeolocation();
		if (gl.getType() == GeoLocation.UNSET)
		{
			objects.remove(object.getObjectId());
		}
		else
		{
			if (!objects.containsKey(object.getObjectId()))
			{
				locationTree.insert(gl.getLatitude(), gl.getLongitude(), object.getObjectId());
			}
			else
			{
				if (!gl.equals(objects.get(object.getObjectId()).getGeolocation()))
				{
					
				}
			}
			objects.put(object.getObjectId(), object);
		}
	}
}
