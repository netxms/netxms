/**
 * Web services API for NetXMS
 * Copyright (c) 2017 Raden Solutions
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
