package org.netxms.ui.eclipse.imagelibrary;

import java.io.IOException;
import java.util.UUID;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.widgets.Display;
import org.netxms.api.client.SessionListener;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;

/**
 * Early startup handler
 */
public class LoginListener implements ConsoleLoginListener
{
	private final class ImageLibraryListener implements SessionListener
	{
		private final Display display;
		private final NXCSession session;

		private ImageLibraryListener(Display display, NXCSession session)
		{
			this.display = display;
			this.session = session;
		}

		@Override
		public void notificationHandler(SessionNotification n)
		{
			if (n.getCode() == NXCNotification.IMAGE_LIBRARY_UPDATED)
			{
				final UUID guid = (UUID)n.getObject();
				System.out.println("Updated image: " + guid);
				// final ImageProvider imageProvider = ImageProvider.getInstance();
				// imageProvider.invalidateImage(session, guid);

				new Thread()
				{
					@Override
					public void run()
					{
						try
						{
							ImageProvider.getInstance().syncMetaData(session, display);
						}
						catch(NXCException e)
						{
							// FIXME
							e.printStackTrace();
						}
						catch(IOException e)
						{
							// FIXME
							e.printStackTrace();
						}
					}
				}.start();
			}
		}
	}

	@Override
	public void afterLogin(final NXCSession session, final Display display)
	{
		ImageProvider.createInstance(display);
		Job job = new Job("Initialize image library")
		{
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				try
				{
					ImageProvider.getInstance().syncMetaData(session, display);
					session.addListener(new ImageLibraryListener(display, session));
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
