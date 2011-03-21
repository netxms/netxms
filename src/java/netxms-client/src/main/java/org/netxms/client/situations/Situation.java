/**
 * 
 */
package org.netxms.client.situations;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Situation object
 *
 */
public class Situation
{
	private long id;
	private String name;
	private String comments;
	private List<SituationInstance> instances;
	
	/**
	 * Create situation object from NXCP message
	 * 
	 * @param msg
	 */
	public Situation(NXCPMessage msg)
	{
		id = msg.getVariableAsInt64(NXCPCodes.VID_SITUATION_ID);
		name = msg.getVariableAsString(NXCPCodes.VID_NAME);
		comments = msg.getVariableAsString(NXCPCodes.VID_COMMENTS);
		
		int count = msg.getVariableAsInteger(NXCPCodes.VID_INSTANCE_COUNT);
		instances = new ArrayList<SituationInstance>(count);
		long varId = NXCPCodes.VID_INSTANCE_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			final SituationInstance si = new SituationInstance(this, msg, varId);
			instances.add(si);
			varId += si.getAttributeCount() * 2 + 2; 
		}
	}

	/**
	 * @return the id
	 */
	public long getId()
	{
		return id;
	}

	/**
	 * @return the name
	 */
	public String getName()
	{
		return name;
	}

	/**
	 * @return the comments
	 */
	public String getComments()
	{
		return comments;
	}

	/**
	 * @return the instances
	 */
	public List<SituationInstance> getInstances()
	{
		return instances;
	}
}
