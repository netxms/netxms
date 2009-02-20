/**
 * 
 */
package org.netxms.ui.eclipse.console.extensionproviders;

import org.netxms.ui.eclipse.console.Activator;
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
