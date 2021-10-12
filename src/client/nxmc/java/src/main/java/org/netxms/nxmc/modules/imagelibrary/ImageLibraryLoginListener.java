/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.imagelibrary;

import java.util.UUID;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.nxmc.services.LoginListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Early startup handler
 */
public class ImageLibraryLoginListener implements LoginListener
{
   private Logger logger = LoggerFactory.getLogger(ImageLibraryLoginListener.class);

	private final class ImageLibraryListener implements SessionListener
	{
		private ImageLibraryListener(Display display, NXCSession session)
		{
		}

		@Override
		public void notificationHandler(SessionNotification n)
		{
			if (n.getCode() == SessionNotification.IMAGE_LIBRARY_CHANGED)
			{
				final UUID guid = (UUID)n.getObject();
				final ImageProvider imageProvider = ImageProvider.getInstance();
            if (n.getSubCode() == SessionNotification.IMAGE_DELETED)
               imageProvider.deleteImage(guid);
            else
               imageProvider.updateImage(guid);
			}
		}
	}

   /**
    * @see org.netxms.nxmc.services.LoginListener#afterLogin(org.netxms.client.NXCSession, org.eclipse.swt.widgets.Display)
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
               ImageProvider.getInstance().syncMetaData();
               session.addListener(new ImageLibraryListener(display, session));
            }
            catch(Exception e)
            {
               logger.error("Exception in login listener worker thread", e);
            }
         }
      }, "");
		worker.setDaemon(true);
		worker.start();
	}
}
