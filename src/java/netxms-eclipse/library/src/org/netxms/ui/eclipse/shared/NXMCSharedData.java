/**
 * 
 */
package org.netxms.ui.eclipse.shared;

import org.netxms.client.NXCSession;

/**
 * Shared data for NXMC extensions
 * 
 * @author Victor
 *
 */
public class NXMCSharedData
{
	private static NXMCSharedData instance = new NXMCSharedData();
	private NXCSession session = null;
	
	/**
	 * Private constructor
	 */
	private NXMCSharedData()
	{
	}
	
	
	/**
	 * Get instance of shared data object
	 * 
	 * @return Shared data instance
	 */
	public static NXMCSharedData getInstance()
	{
		return instance;
	}
	
	/**
	 * Get current NetXMS client library session
	 * 
	 * @return Current session
	 */
	public NXCSession getSession()
	{
		return session;
	}

	/**
	 * Set current NetXMS client library session
	 * 
	 * @param session Current session
	 */
	public void setSession(NXCSession session)
	{
		this.session = session;
	}
}
