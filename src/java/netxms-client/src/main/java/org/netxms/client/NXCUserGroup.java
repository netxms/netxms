/**
 * 
 */
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * @author Victor
 *
 */
public class NXCUserGroup extends NXCUserDBObject
{
	private long[] members;
	
	/**
	 * Default constructor
	 */
	public NXCUserGroup(final String name)
	{
		super(name);
		members = new long[0];
	}
	
	/**
	 * Copy constructor
	 */
	public NXCUserGroup(final NXCUserGroup src)
	{
		super(src);
		this.members = new long[src.members.length];
		System.arraycopy(src.members, 0, this.members, 0, src.members.length);
	}
	
	/**
	 * Create group from NXCP message
	 */
	protected NXCUserGroup(final NXCPMessage msg)
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
}
