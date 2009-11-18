/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * @author Victor
 *
 */
public class AgentPolicy extends GenericObject
{
	private int version;
	private int policyType;
	private String description;
	
	/**
	 * @param msg
	 * @param session
	 */
	public AgentPolicy(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		policyType = msg.getVariableAsInteger(NXCPCodes.VID_POLICY_TYPE);
		version = msg.getVariableAsInteger(NXCPCodes.VID_VERSION);
		description = msg.getVariableAsString(NXCPCodes.VID_DESCRIPTION);
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "AgentPolicy";
	}

	/**
	 * @return the version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @return the policyType
	 */
	public int getPolicyType()
	{
		return policyType;
	}

	/**
	 * @return the description
	 */
	public String getDescription()
	{
		return description;
	}
}
