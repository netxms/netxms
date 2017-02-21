/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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

import java.util.UUID;
import org.netxms.client.NXCSession;

/**
 * Session token
 */
public class SessionToken
{
   private UUID guid;
   private NXCSession session;
   private long activityTimestamp;
   
   /**
    * Create new session object
    * 
    * @param session
    */
   public SessionToken(NXCSession session)
   {
      guid = UUID.randomUUID();
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
   public UUID getGuid()
   {
      return guid;
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
}
