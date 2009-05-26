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
public class NXCAgentPolicy extends NXCObject
{
	private int version;
	private int policyType;
	private String description;
	
	/**
	 * @param msg
	 * @param session
	 */
	public NXCAgentPolicy(NXCPMessage msg, NXCSession session)
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
