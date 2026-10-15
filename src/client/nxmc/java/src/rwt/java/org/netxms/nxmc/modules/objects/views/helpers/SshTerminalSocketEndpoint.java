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
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;
import java.util.UUID;
import org.netxms.nxmc.modules.objects.ssh.TerminalTransport;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import jakarta.servlet.http.HttpSession;
import jakarta.websocket.CloseReason;
import jakarta.websocket.CloseReason.CloseCodes;
import jakarta.websocket.EndpointConfig;
import jakarta.websocket.HandshakeResponse;
import jakarta.websocket.OnClose;
import jakarta.websocket.OnMessage;
import jakarta.websocket.OnOpen;
import jakarta.websocket.Session;
import jakarta.websocket.server.HandshakeRequest;
import jakarta.websocket.server.PathParam;
import jakarta.websocket.server.ServerEndpoint;
import jakarta.websocket.server.ServerEndpointConfig;

/**
 * Websocket endpoint connecting browser-side terminal emulator (xterm.js) with terminal transport running in the web
 * application. Protocol: binary frames carry raw terminal data in both directions; text frames carry JSON control messages
 * ({"type":"init","cols":N,"rows":N} and {"type":"resize","cols":N,"rows":N} from browser, {"type":"error","message":S}
 * and {"type":"closed"} from server). A transport is registered by the view with the ID of the HTTP session that created
 * it; only a websocket from the same HTTP session may attach to it.
 */
@ServerEndpoint(value = "/sshterminal/{tid}", configurator = SshTerminalSocketEndpoint.HttpSessionConfigurator.class)
public class SshTerminalSocketEndpoint
{
   private static final Logger logger = LoggerFactory.getLogger(SshTerminalSocketEndpoint.class);

   private static final String HTTP_SESSION_ID = "httpSessionId";
   private static final long ATTACH_TIMEOUT = 60000; // transport closed if browser does not attach within this time

   private static Map<UUID, TerminalSession> transports = Collections.synchronizedMap(new HashMap<UUID, TerminalSession>());
   private static Map<String, TerminalSession> sessions = Collections.synchronizedMap(new HashMap<String, TerminalSession>());
   private static Timer cleanupTimer = new Timer("SshTerminalCleanup", true);

   /**
    * Handshake configurator capturing HTTP session ID of the websocket request
    */
   public static class HttpSessionConfigurator extends ServerEndpointConfig.Configurator
   {
      @Override
      public void modifyHandshake(ServerEndpointConfig config, HandshakeRequest request, HandshakeResponse response)
      {
         Object httpSession = request.getHttpSession();
         if (httpSession instanceof HttpSession)
            config.getUserProperties().put(HTTP_SESSION_ID, ((HttpSession)httpSession).getId());
      }
   }

   /**
    * Register terminal transport.
    *
    * @param transport terminal transport (not connected yet)
    * @param httpSessionId ID of HTTP session that owns the transport
    * @return terminal ID to be used in websocket URL
    */
   public static UUID registerTransport(TerminalTransport transport, String httpSessionId)
   {
      final TerminalSession ts = new TerminalSession(transport, httpSessionId);
      transports.put(ts.id, ts);
      cleanupTimer.schedule(new TimerTask() {
         @Override
         public void run()
         {
            if (!ts.attached)
            {
               logger.debug("SSH terminal {} was not attached within timeout, closing", ts.id);
               transports.remove(ts.id);
               ts.close();
            }
         }
      }, ATTACH_TIMEOUT);
      return ts.id;
   }

   /**
    * Close transport registered under given ID (called when view is disposed).
    *
    * @param id terminal ID
    */
   public static void closeTransport(UUID id)
   {
      TerminalSession ts = transports.remove(id);
      if (ts != null)
         ts.close();
   }

   /**
    * Websocket session open handler.
    *
    * @param session websocket session
    * @param config endpoint configuration
    * @param terminalId terminal ID
    */
   @OnOpen
   public void onOpen(Session session, EndpointConfig config, @PathParam("tid") String terminalId)
   {
      logger.debug("SSH terminal websocket session open request (terminal ID {})", terminalId);
      TerminalSession ts = null;
      try
      {
         ts = transports.get(UUID.fromString(terminalId));
      }
      catch(IllegalArgumentException e)
      {
      }
      if (ts == null)
      {
         logger.debug("Invalid terminal ID {} in websocket session open request", terminalId);
         closeSession(session, CloseCodes.CANNOT_ACCEPT, "Invalid terminal ID");
         return;
      }

      Object httpSessionId = config.getUserProperties().get(HTTP_SESSION_ID);
      if (!ts.httpSessionId.equals(httpSessionId))
      {
         logger.warn("Attempt to attach to SSH terminal {} from different HTTP session", terminalId);
         closeSession(session, CloseCodes.CANNOT_ACCEPT, "Invalid terminal ID");
         return;
      }

      if (!ts.attach(session))
      {
         logger.debug("SSH terminal {} is already attached", terminalId);
         closeSession(session, CloseCodes.CANNOT_ACCEPT, "Terminal already attached");
         return;
      }
      sessions.put(session.getId(), ts);
   }

   /**
    * Websocket session closure handler.
    *
    * @param session websocket session being closed
    */
   @OnClose
   public void onClose(Session session)
   {
      logger.debug("SSH terminal websocket session {} closed", session.getId());
      TerminalSession ts = sessions.remove(session.getId());
      if (ts != null)
      {
         transports.remove(ts.id);
         ts.close();
      }
   }

   /**
    * Handler for control messages.
    *
    * @param session websocket session
    * @param text JSON control message
    */
   @OnMessage
   public void onMessage(Session session, String text)
   {
      TerminalSession ts = sessions.get(session.getId());
      if (ts == null)
      {
         closeSession(session, CloseCodes.CANNOT_ACCEPT, "Invalid session");
         return;
      }
      try
      {
         JsonObject msg = JsonParser.parseString(text).getAsJsonObject();
         String type = msg.get("type").getAsString();
         int cols = msg.has("cols") ? msg.get("cols").getAsInt() : 80;
         int rows = msg.has("rows") ? msg.get("rows").getAsInt() : 24;
         if ((cols < 1) || (cols > 1024) || (rows < 1) || (rows > 1024))
            return;
         if (type.equals("init"))
            ts.start(cols, rows);
         else if (type.equals("resize"))
            ts.transport.resize(cols, rows);
      }
      catch(Exception e)
      {
         logger.debug("Invalid control message from SSH terminal websocket session {}", session.getId(), e);
      }
   }

   /**
    * Handler for terminal input.
    *
    * @param session websocket session
    * @param data received data
    */
   @OnMessage(maxMessageSize = 1024000)
   public void onMessage(Session session, byte[] data)
   {
      TerminalSession ts = sessions.get(session.getId());
      if (ts == null)
      {
         closeSession(session, CloseCodes.CANNOT_ACCEPT, "Invalid session");
         return;
      }
      try
      {
         ts.write(data);
      }
      catch(IOException e)
      {
         logger.debug("Write error on SSH terminal {}", ts.id, e);
      }
   }

   /**
    * Safely close websocket session.
    *
    * @param session websocket session to close
    * @param code closure code
    * @param message closure message
    */
   private static void closeSession(Session session, CloseReason.CloseCode code, String message)
   {
      try
      {
         session.close(new CloseReason(code, message));
      }
      catch(IOException e)
      {
         logger.error("Error closing websocket session", e);
      }
   }

   /**
    * Terminal session: transport plus attached websocket
    */
   private static class TerminalSession
   {
      final UUID id = UUID.randomUUID();
      final TerminalTransport transport;
      final String httpSessionId;
      volatile boolean attached = false;
      volatile boolean connected = false;
      private Session wsSession = null;
      private Thread readerThread = null;

      TerminalSession(TerminalTransport transport, String httpSessionId)
      {
         this.transport = transport;
         this.httpSessionId = httpSessionId;
      }

      /**
       * Attach websocket session
       *
       * @param session websocket session
       * @return false if another session is already attached
       */
      synchronized boolean attach(Session session)
      {
         if (attached)
            return false;
         attached = true;
         wsSession = session;
         return true;
      }

      /**
       * Start transport connection and reader thread
       *
       * @param cols initial width
       * @param rows initial height
       */
      synchronized void start(int cols, int rows)
      {
         if (readerThread != null)
            return;
         readerThread = new Thread(() -> reader(cols, rows), "SshTerminal-" + id);
         readerThread.setDaemon(true);
         readerThread.start();
      }

      /**
       * Reader thread: connect, then forward terminal output to websocket
       */
      private void reader(int cols, int rows)
      {
         try
         {
            transport.connect(cols, rows);
            connected = true;
            InputStream in = transport.getInputStream();
            byte[] buffer = new byte[16384];
            int bytes;
            while((bytes = in.read(buffer)) > 0)
            {
               sendBinary(buffer, bytes);
            }
            sendControl("{\"type\":\"closed\"}");
         }
         catch(Exception e)
         {
            logger.debug("SSH terminal {} error", id, e);
            String message = (e.getLocalizedMessage() != null) ? e.getLocalizedMessage() : e.getClass().getName();
            JsonObject json = new JsonObject();
            json.addProperty("type", "error");
            json.addProperty("message", message);
            sendControl(json.toString());
         }
         finally
         {
            connected = false;
            transport.close();
            Session s = wsSession;
            if (s != null)
            {
               try
               {
                  s.close();
               }
               catch(IOException e)
               {
               }
            }
         }
      }

      /**
       * Write terminal input to transport
       */
      void write(byte[] data) throws IOException
      {
         if (connected)
         {
            transport.getOutputStream().write(data);
            transport.getOutputStream().flush();
         }
      }

      /**
       * Send binary frame to browser
       */
      private void sendBinary(byte[] data, int length) throws IOException
      {
         Session s = wsSession;
         if (s == null)
            return;
         synchronized(s)
         {
            s.getBasicRemote().sendBinary(ByteBuffer.wrap(data, 0, length));
         }
      }

      /**
       * Send control message to browser
       */
      private void sendControl(String json)
      {
         Session s = wsSession;
         if (s == null)
            return;
         try
         {
            synchronized(s)
            {
               s.getBasicRemote().sendText(json);
            }
         }
         catch(IOException e)
         {
            logger.debug("Cannot send control message to SSH terminal {}", id, e);
         }
      }

      /**
       * Close transport (and reader thread will finish)
       */
      void close()
      {
         transport.close();
      }
   }
}
