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
package org.netxms.api.client.users;

/**
 * Abstract access list element.
 *
 */
public abstract class AbstractAccessListElement
{
	protected long userId;
	protected int accessRights;

	/**
	 * Create new ACL element with given user ID and rights
	 * 
	 * @param userId
	 * @param accessRights
	 */
	public AbstractAccessListElement(long userId, int accessRights)
	{
		this.userId = userId;
		this.accessRights = accessRights;
	}
	
	/**
	 * Copy constructor
	 * 
	 * @param src Source ACL element
	 */
	public AbstractAccessListElement(AbstractAccessListElement src)
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
	
	

	/* (non-Javadoc)
	 * @see java.lang.Object#equals(java.lang.Object)
	 */
	@Override
	public boolean equals(Object aThat)
	{
		if (this == aThat)
			return true;
		
		if (!(aThat instanceof AbstractAccessListElement))
			return false;
		
		return (this.userId == ((AbstractAccessListElement)aThat).userId) &&
		       (this.accessRights == ((AbstractAccessListElement)aThat).accessRights);
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#hashCode()
	 */
	@Override
	public int hashCode()
	{
		return (int)((accessRights << 16) & userId);
	}
}
