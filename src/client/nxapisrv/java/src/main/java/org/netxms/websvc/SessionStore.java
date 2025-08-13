/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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

import jakarta.servlet.ServletContext;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Session store
 */
public class SessionStore
{
   private static ApiProperties properties = new ApiProperties();

   private Map<UUID, SessionToken> sessions = new HashMap<UUID, SessionToken>();
   private Logger log = LoggerFactory.getLogger(SessionStore.class);
   private Thread sessionManager = null;

   /**
    * Get session store instance for servlet
    * 
    * @param context servlet context
    * @return session store instance
    */
   public static synchronized SessionStore getInstance(ServletContext context)
   {
      SessionStore s = (SessionStore)context.getAttribute("org.netxms.webui.sessionStore");
      if (s == null)
      {
         s = new SessionStore();
         context.setAttribute("org.netxms.webui.sessionStore", s);
      }
      return s;
   }
   
   /**
    * Get session token with given UUID.
    *
    * @param guid token UUID
    * @return session token or null
    */
   public synchronized SessionToken getSessionToken(UUID guid)
   {
      SessionToken s = sessions.get(guid);
      if (s != null)
         s.updateActivityTimestamp();
      return s;
   }

   /**
    * Register session.
    *
    * @param session session to register
    * @return token assigned to provided session
    */
   public synchronized SessionToken registerSession(final NXCSession session)
   {
      if (sessionManager == null)
      {
         sessionManager = new Thread(new Runnable() {
            @Override
            public void run()
            {
               sessionManagerThread();
            }
         }, "Session Manager");
         sessionManager.setDaemon(true);
         sessionManager.start();
      }

      final SessionToken token = new SessionToken(session);
      sessions.put(token.getSessionHandle(), token);
      session.addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.CONNECTION_BROKEN) ||
                (n.getCode() == SessionNotification.SERVER_SHUTDOWN) ||
                (n.getCode() == SessionNotification.SESSION_KILLED))
            {
               log.info("Received disconnect notification for session " + token.getSessionHandle());
               session.disconnect();
               unregisterSession(token.getSessionHandle());
            }
         }
      });
      log.info("Session " + token.getSessionHandle() + " registered");
      return token;
   }

   /**
    * Unregister session.
    *
    * @param guid session token UUID
    */
   public synchronized void unregisterSession(UUID guid)
   {
      sessions.remove(guid);
      log.info("Session " + guid + " unregistered");
   }

   /**
    * Session manager background thread
    */
   private void sessionManagerThread()
   {
      while(true)
      {
         try
         {
            Thread.sleep(60000);
         }
         catch(InterruptedException e)
         {
         }
         checkSessions();
      }
   }

   /**
    * Check active sessions
    */
   private synchronized void checkSessions()
   {
      long now = System.currentTimeMillis();
      List<UUID> disconnectedSessions = new ArrayList<UUID>();
      for(SessionToken s : sessions.values())
      {
         if (now - s.getActivityTimestamp() > properties.getSessionTimeout())
         {
            log.info("Session " + s.getSessionHandle() + " disconnected by inactivity timeout");
            s.getSession().disconnect();
            disconnectedSessions.add(s.getSessionHandle());
         }
         else if (!s.getSession().checkConnection())
         {
            log.info("Session " + s.getSessionHandle() + " removed due to communication failure");
            s.getSession().disconnect();
            disconnectedSessions.add(s.getSessionHandle());
         }
      }
      
      for(UUID u : disconnectedSessions)
         unregisterSession(u);
   }
}
