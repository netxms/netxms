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
package org.netxms.client.objects;

import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectStatus;

/**
 * Pseudo-object placeholder for objects being loaded from server
 */
public class LoadingObject extends GenericObject
{
	/**
	 * Crate new placeholder.
	 * 
	 * @param id object ID
	 * @param session owning client session
	 */
	public LoadingObject(final long id, final NXCSession session)
	{
		super(id, session);
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectName()
	 */
	@Override
	public String getObjectName()
	{
		return "Loading...";
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getStatus()
	 */
	@Override
	public ObjectStatus getStatus()
	{
		return ObjectStatus.UNKNOWN;
	}

	/**
	 * @see org.netxms.client.objects.AbstractObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "UNKNOWN";
	}
}
