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

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Group of NetXMS users
 *
 */
public class UserGroup extends AbstractUserObject
{
	private long[] members;
	
	/**
	 * Default constructor
	 */
	public UserGroup(final String name)
	{
		super(name);
		members = new long[0];
	}
	
	/**
	 * Copy constructor
	 */
	public UserGroup(final UserGroup src)
	{
		super(src);
		this.members = new long[src.members.length];
		System.arraycopy(src.members, 0, this.members, 0, src.members.length);
	}
	
	/**
	 * Create group from NXCP message
	 */
	public UserGroup(final NXCPMessage msg)
	{
		super(msg);

		int count = msg.getVariableAsInteger(NXCPCodes.VID_NUM_MEMBERS);
		members = new long[count];
		for(int i = 0; i < count; i++)
		{
			members[i] = msg.getVariableAsInt64(NXCPCodes.VID_GROUP_MEMBER_BASE + i);
		}
	}
	
	/**
	 * Fill NXCP message with group data
	 */
	public void fillMessage(final NXCPMessage msg)
	{
		super.fillMessage(msg);
		msg.setVariableInt32(NXCPCodes.VID_NUM_MEMBERS, members.length);
		for(int i = 0; i < members.length; i++)
			msg.setVariableInt32(NXCPCodes.VID_GROUP_MEMBER_BASE + i, (int)members[i]);
	}

	/**
	 * @return the members
	 */
	public long[] getMembers()
	{
		return members;
	}

	/**
	 * @param members the members to set
	 */
	public void setMembers(long[] members)
	{
		this.members = members;
	}

	/* (non-Javadoc)
	 * @see java.lang.Object#clone()
	 */
	@Override
	public Object clone() throws CloneNotSupportedException
	{
		return new UserGroup(this);
	}
}
