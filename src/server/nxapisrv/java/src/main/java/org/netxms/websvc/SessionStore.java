/**
 * Web services API for NetXMS
 * Copyright (c) 2017 Raden Solutions
 */
package org.netxms.websvc;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import javax.servlet.ServletContext;
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
   private Map<UUID, SessionToken> sessions = new HashMap<UUID, SessionToken>();
   private Logger log = LoggerFactory.getLogger(SessionStore.class);
   
   /**
    * Get session store instance for servlet
    * 
    * @param context
    * @return
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
    * @param guid
    * @return
    */
   public synchronized SessionToken getSessionToken(UUID guid)
   {
      return sessions.get(guid);
   }
   
   /**
    * @param s
    */
   public synchronized SessionToken registerSession(final NXCSession session)
   {
      final SessionToken token = new SessionToken(session);
      sessions.put(token.getGuid(), token);
      session.addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if ((n.getCode() == SessionNotification.CONNECTION_BROKEN) ||
                (n.getCode() == SessionNotification.SERVER_SHUTDOWN) ||
                (n.getCode() == SessionNotification.SESSION_KILLED))
            {
               log.info("Received disconnect notification for session " + token.getGuid());
               session.disconnect();
               unregisterSession(token.getGuid());
            }
         }
      });
      log.info("Session " + token.getGuid() + " registered");
      return token;
   }
   
   /**
    * @param guid
    */
   public synchronized void unregisterSession(UUID guid)
   {
      sessions.remove(guid);
      log.info("Session " + guid + " unregistered");
   }
}
