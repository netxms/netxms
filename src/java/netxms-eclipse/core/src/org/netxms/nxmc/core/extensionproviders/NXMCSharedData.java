/**
 * 
 */
package org.netxms.nxmc.core.extensionproviders;

import org.netxms.nxmc.core.Activator;
import org.netxms.client.NXCSession;

/**
 * Shared data for NXMC extensions
 * 
 * @author Victor
 *
 */
public class NXMCSharedData
{
	/**
	 * Get NetXMS client session
	 * 
	 * @return NetXMS client session
	 */
	public static NXCSession getSession()
	{
		return Activator.getDefault().getSession();
	}
}
