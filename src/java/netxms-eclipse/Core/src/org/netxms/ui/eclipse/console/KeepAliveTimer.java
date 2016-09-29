/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.console;

import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;

/**
 * Console keepalive timer
 */
public class KeepAliveTimer extends Thread implements SessionListener
{
   private NXCSession session;
   
   /**
    * @param session
    */
   public KeepAliveTimer(NXCSession session)
   {
      super("Session Keepalive Timer");
      setDaemon(true);
      this.session = session;
      session.addListener(this);
   }
   
   /* (non-Javadoc)
    * @see java.lang.Thread#run()
    */
   @Override
   public void run()
   {
      while(session.isConnected())
      {
         try
         {
            sleep(1000 * 60); // send keep-alive every 60 seconds
            if (!session.checkConnection())
               break;   // session broken, application will exit (handled by workbench advisor)
         }
         catch(InterruptedException e)
         {
         }
         catch(Exception e)
         {
            Activator.logError("Exception in keep-alive thread", e);
         }
      }
      session = null;
      Activator.logInfo("Session keepalive timer stopped");
   }

   /* (non-Javadoc)
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(SessionNotification n)
   {
      if ((n.getCode() == SessionNotification.CONNECTION_BROKEN) || 
          (n.getCode() == SessionNotification.SESSION_KILLED) ||
          (n.getCode() == SessionNotification.SERVER_SHUTDOWN))
      {
         interrupt();
      }
   }
}
