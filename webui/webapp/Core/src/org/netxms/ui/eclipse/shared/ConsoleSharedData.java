/**
 * 
 */
package org.netxms.ui.eclipse.shared;

import org.eclipse.rwt.RWT;
import org.netxms.api.client.Session;

/**
 * Compatibility class for RCP plugins
 */
public class ConsoleSharedData
{
	/**
	 * Get NetXMS session
	 * 
	 * @return
	 */
	public static Session getSession()
	{
		return (Session)RWT.getSessionStore().getAttribute("netxms.session");
	}
}
