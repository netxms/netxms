package org.netxms.ui.eclipse.imagelibrary;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;

/**
 * Early startup handler
 */
public class LoginListener implements ConsoleLoginListener
{
	@Override
	public void afterLogin(final NXCSession session, final Display display)
	{
		Job job = new Job("Initialize image library") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				try
				{
					ImageProvider.getInstance().syncMetaData(session, display);
				}
				catch(Exception e)
				{
					// FIXME
					e.printStackTrace();
				}
				return Status.OK_STATUS;
			}
		};
		job.setSystem(true);
		job.schedule();
	}
}
