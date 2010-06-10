/**
 * 
 */
package org.netxms.ui.eclipse.alarmviewer;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.ui.IStartup;
import org.netxms.ui.eclipse.shared.NXMCSharedData;

/**
 * Early startup class
 *
 */
public class Startup implements IStartup
{
	/* (non-Javadoc)
	 * @see org.eclipse.ui.IStartup#earlyStartup()
	 */
	@Override
	public void earlyStartup()
	{
		// wait for connect
		Job job = new Job("Set alarm listener for tray popups") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				while(NXMCSharedData.getInstance().getSession() == null)
				{
					try
					{
						Thread.sleep(1000);
					}
					catch(InterruptedException e)
					{
					}
				}
				AlarmNotifier.init();
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
}
