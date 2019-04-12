/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2019 Raden Solutions
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
package org.netxms.websvc;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import org.netxms.base.annotations.Internal;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionNotification;

/**
 * Session token
 */
public class SessionToken
{
   private UUID sessionHandle;
   private NXCSession session;
   @Internal private long activityTimestamp;
   @Internal private LinkedBlockingQueue<SessionNotification> notificationQueue = new LinkedBlockingQueue<SessionNotification>(8192);
   
   /**
    * Create new session object
    * 
    * @param session
    */
   public SessionToken(NXCSession session)
   {
      sessionHandle = UUID.randomUUID();
      activityTimestamp = System.currentTimeMillis();
      this.session = session;
   }
   
   /**
    * Update last activity timestamp
    */
   public void updateActivityTimestamp()
   {
      activityTimestamp = System.currentTimeMillis();
   }

   /**
    * @return
    */
   public UUID getSessionHandle()
   {
      return sessionHandle;
   }

   /**
    * @return
    */
   public NXCSession getSession()
   {
      return session;
   }

   /**
    * @return
    */
   public long getActivityTimestamp()
   {
      return activityTimestamp;
   }
   
   /**
    * Add notification to queue
    * 
    * @param notification to add
    */
   public void addNotificationToQueue(SessionNotification notification)
   {
      notificationQueue.offer(notification);
   }
   
   /**
    * Take notification from queue
    * 
    * @param timeout timeout to wait in seconds
    * @return notification
    * @throws InterruptedException 
    */
   public List<SessionNotification> pollNotificationQueue(long timeout) throws InterruptedException
   {
      List<SessionNotification> notifications = new ArrayList<SessionNotification>();
      notificationQueue.drainTo(notifications);
      if (notifications.isEmpty())
      {
         SessionNotification n = notificationQueue.poll(timeout, TimeUnit.SECONDS);
         if (n != null)
            notifications.add(n);
      }
      updateActivityTimestamp();
      return notifications;
   }
}
