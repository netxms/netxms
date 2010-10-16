/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2009 Victor Kirhenshtein
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
package org.netxms.client;

import org.netxms.client.constants.UserAccessRights;

public final class AccessListElement
{
	private long userId;
	private int accessRights;

	/**
	 * Create new ACL element with given user ID and rights
	 * 
	 * @param userId
	 * @param accessRights
	 */
	public AccessListElement(long userId, int accessRights)
	{
		this.userId = userId;
		this.accessRights = accessRights;
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src Source ACL element
	 */
	public AccessListElement(AccessListElement src)
	{
		this.userId = src.userId;
		this.accessRights = src.accessRights;
	}

	/**
	 * @return the userId
	 */
	public long getUserId()
	{
		return userId;
	}

	/**
	 * @param accessRights the accessRights to set
	 */
	public void setAccessRights(int accessRights)
	{
		this.accessRights = accessRights;
	}

	/**
	 * @return the accessRights
	 */
	public int getAccessRights()
	{
		return accessRights;
	}
	
	/**
	 * @return true if READ access granted
	 */
	public boolean hasRead()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_READ) != 0;
	}
	
	/**
	 * @return true if MODIFY access granted
	 */
	public boolean hasModify()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_MODIFY) != 0;
	}
	
	/**
	 * @return true if DELETE access granted
	 */
	public boolean hasDelete()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_DELETE) != 0;
	}
	
	/**
	 * @return true if CREATE access granted
	 */
	public boolean hasCreate()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_CREATE) != 0;
	}
	
	/**
	 * @return true if READ ALARMS access granted
	 */
	public boolean hasReadAlarms()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_READ_ALARMS) != 0;
	}
	
	/**
	 * @return true if ACK ALARMS access granted
	 */
	public boolean hasAckAlarms()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_ACK_ALARMS) != 0;
	}
	
	/**
	 * @return true if TERMINATE ALARMS access granted
	 */
	public boolean hasTerminateAlarms()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_TERM_ALARMS) != 0;
	}
	
	/**
	 * @return true if CONTROL access granted
	 */
	public boolean hasControl()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_CONTROL) != 0;
	}
	
	/**
	 * @return true if SEND EVENTS access granted
	 */
	public boolean hasSendEvents()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_SEND_EVENTS) != 0;
	}
	
	/**
	 * @return true if ACCESS CONTROL access granted
	 */
	public boolean hasAccessControl()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_ACL) != 0;
	}
	
	/**
	 * @return true if PUSH DATA access granted
	 */
	public boolean hasPushData()
	{
		return (accessRights & UserAccessRights.OBJECT_ACCESS_PUSH_DATA) != 0;
	}
	
	@Override
	public boolean equals(Object aThat)
	{
		if (this == aThat)
			return true;
		
		if (!(aThat instanceof AccessListElement))
			return false;
		
		return (this.userId == ((AccessListElement)aThat).userId) &&
		       (this.accessRights == ((AccessListElement)aThat).accessRights);
	}
	
	@Override
	public int hashCode()
	{
		return (int)((accessRights << 16) & userId);
	}
}
