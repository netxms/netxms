/**
 * 
 */
package org.netxms.client.objects;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;

/**
 * This class represents NetXMS TEMPLATE objects.
 * 
 * @author Victor
 *
 */
public class Template extends GenericObject
{
	private int version;
	private boolean autoApplyEnabled;
	private String autoApplyFilter;

	/**
	 * @param msg
	 * @param session
	 */
	public Template(NXCPMessage msg, NXCSession session)
	{
		super(msg, session);
		
		version = msg.getVariableAsInteger(NXCPCodes.VID_VERSION);
		autoApplyEnabled = msg.getVariableAsBoolean(NXCPCodes.VID_AUTO_APPLY);
		autoApplyFilter = msg.getVariableAsString(NXCPCodes.VID_APPLY_FILTER);
	}

	/**
	 * @return template version
	 */
	public int getVersion()
	{
		return version;
	}

	/**
	 * @return template version as string in form major.minor
	 */
	public String getVersionAsString()
	{
		return Integer.toString(version >> 16) + "." + Integer.toString(version & 0xFFFF);
	}

	/**
	 * @return true if automatic apply is enabled
	 */
	public boolean isAutoApplyEnabled()
	{
		return autoApplyEnabled;
	}

	/**
	 * @return Filter script for automatic apply
	 */
	public String getAutoApplyFilter()
	{
		return autoApplyFilter;
	}

	/* (non-Javadoc)
	 * @see org.netxms.client.NXCObject#getObjectClassName()
	 */
	@Override
	public String getObjectClassName()
	{
		return "Template";
	}
}
