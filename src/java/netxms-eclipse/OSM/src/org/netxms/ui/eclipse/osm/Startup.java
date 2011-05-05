/**
 * 
 */
package org.netxms.ui.eclipse.osm;

import org.eclipse.ui.IStartup;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Early startup handler
 */
public class Startup implements IStartup
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	@Override
	public void earlyStartup()
	{
		while(ConsoleSharedData.getSession() == null)
		{
			try
			{
				Thread.sleep(1000);
			}
			catch(InterruptedException e)
			{
			}
		}
		GeoLocationCache.getInstance().initialize();
	}
}
