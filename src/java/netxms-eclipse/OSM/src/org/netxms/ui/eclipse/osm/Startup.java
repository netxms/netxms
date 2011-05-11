/**
 * 
 */
package org.netxms.ui.eclipse.osm;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
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
		Job job = new Job("Initialize geolocation cache") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
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
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
}
