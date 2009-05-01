/**
 * 
 */
package org.netxms.client;

import org.netxms.base.*;

/**
 * @author Victor
 *
 */
public class NXCContainer extends NXCObject
{
	private int category;
	private boolean autoBindEnabled;
	private String autoBindFilter;
	
	/**
	 * @param msg
	 */
	public NXCContainer(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		category = msg.getVariableAsInteger(NXCPCodes.VID_CATEGORY);
		autoBindEnabled = msg.getVariableAsBoolean(NXCPCodes.VID_ENABLE_AUTO_BIND);
		autoBindFilter = msg.getVariableAsString(NXCPCodes.VID_AUTO_BIND_FILTER);
	}

	/**
	 * @return the category
	 */
	public int getCategory()
	{
		return category;
	}

	/**
	 * @return true if automatic bind is enabled
	 */
	public boolean isAutoBindEnabled()
	{
		return autoBindEnabled;
	}

	/**
	 * @return Filter script for automatic bind
	 */
	public String getAutoBindFilter()
	{
		return autoBindFilter;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Container";
	}
}
