package org.netxms.ui.eclipse.imagelibrary;

import java.util.UUID;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.ui.eclipse.console.api.ConsoleLoginListener;
import org.netxms.ui.eclipse.imagelibrary.shared.ImageProvider;

/**
 * Early startup handler
 */
public class LoginListener implements ConsoleLoginListener
{
	private final class ImageLibraryListener implements SessionListener
	{
		private Display display;
		
		private ImageLibraryListener(Display display, NXCSession session)
		{
			this.display = display;
		}

		@Override
		public void notificationHandler(SessionNotification n)
		{
			if (n.getCode() == SessionNotification.IMAGE_LIBRARY_CHANGED)
			{
				final UUID guid = (UUID)n.getObject();
				final ImageProvider imageProvider = ImageProvider.getInstance(display);
            if (n.getSubCode() == SessionNotification.IMAGE_DELETED)
               imageProvider.deleteImage(guid);
            else
               imageProvider.updateImage(guid);
			}
		}
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.console.api.ConsoleLoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
	 */
	@Override
	public void afterLogin(final NXCSession session, final Display display)
	{
		ImageProvider.createInstance(display, session);
		Thread worker = new Thread(new Runnable() {
         @Override
         public void run()
         {
            try
            {
               ImageProvider.getInstance(display).syncMetaData();
               session.addListener(new ImageLibraryListener(display, session));
            }
            catch(Exception e)
            {
               Activator.logError("Exception in login listener worker thread", e);
            }
         }
      }, "");
		worker.setDaemon(true);
		worker.start();
	}
}
