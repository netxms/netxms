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
package org.netxms.client;

import org.netxms.api.client.constants.UserAccessRights;
import org.netxms.api.client.users.AbstractAccessListElement;

/**
 * Access list element for NetXMS objects
 *
 */
public class AccessListElement extends AbstractAccessListElement
{
	/**
	 * Create new ACL element with given user ID and rights
	 * 
	 * @param userId
	 * @param accessRights
	 */
	public AccessListElement(long userId, int accessRights)
	{
		super(userId, accessRights);
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src Source ACL element
	 */
	public AccessListElement(AccessListElement src)
	{
		super(src);
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
}
