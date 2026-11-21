/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import jakarta.websocket.CloseReason;
import jakarta.websocket.CloseReason.CloseCodes;
import jakarta.websocket.Session;

/**
 * Keepalive for long-lived websocket sessions. Sends websocket ping frames periodically so that neither the servlet
 * container's idle timeout nor an intermediate reverse proxy closes a session that carries no data (idle terminal, static
 * VNC screen). Browsers answer pings automatically; endpoint must pass received pong messages to
 * {@link #pongReceived()}. Session is closed if browser stops answering.
 */
public class WebSocketKeepalive
{
   private static final Logger logger = LoggerFactory.getLogger(WebSocketKeepalive.class);

   private static final long PING_INTERVAL = 15000;
   private static final long PONG_TIMEOUT = 60000;
   private static final long IDLE_TIMEOUT = 60000;

   private static final ScheduledExecutorService executor = Executors.newScheduledThreadPool(2, (r) -> {
      Thread t = new Thread(r, "WebSocketKeepalive");
      t.setDaemon(true);
      return t;
   });

   private final Session session;
   private final ScheduledFuture<?> task;
   private volatile long lastPongTime;

   /**
    * Start keepalive for given websocket session.
    *
    * @param session websocket session
    */
   public WebSocketKeepalive(Session session)
   {
      this.session = session;
      lastPongTime = System.currentTimeMillis();
      session.setMaxIdleTimeout(IDLE_TIMEOUT);
      task = executor.scheduleWithFixedDelay(() -> ping(), PING_INTERVAL, PING_INTERVAL, TimeUnit.MILLISECONDS);
   }

   /**
    * Register pong received from browser.
    */
   public void pongReceived()
   {
      lastPongTime = System.currentTimeMillis();
   }

   /**
    * Stop keepalive.
    */
   public void stop()
   {
      task.cancel(false);
   }

   /**
    * Send ping or close session if browser stopped responding. Ping is a control frame and may be interleaved with data
    * messages sent by other threads.
    */
   private void ping()
   {
      if (!session.isOpen())
      {
         stop();
         return;
      }

      if (System.currentTimeMillis() - lastPongTime > PONG_TIMEOUT)
      {
         logger.debug("No pong from websocket session {} within {} ms, closing", session.getId(), PONG_TIMEOUT);
         stop();
         try
         {
            session.close(new CloseReason(CloseCodes.GOING_AWAY, "Keepalive timeout"));
         }
         catch(IOException e)
         {
            logger.debug("Error closing websocket session {}", session.getId(), e);
         }
         return;
      }

      try
      {
         session.getBasicRemote().sendPing(ByteBuffer.allocate(0));
      }
      catch(Exception e)
      {
         logger.debug("Cannot send ping to websocket session {}", session.getId(), e);
      }
   }
}
