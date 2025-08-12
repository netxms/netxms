/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import org.netxms.nxmc.modules.objecttools.TcpPortForwarder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import jakarta.websocket.CloseReason;
import jakarta.websocket.CloseReason.CloseCode;
import jakarta.websocket.CloseReason.CloseCodes;
import jakarta.websocket.EndpointConfig;
import jakarta.websocket.OnClose;
import jakarta.websocket.OnMessage;
import jakarta.websocket.OnOpen;
import jakarta.websocket.Session;
import jakarta.websocket.server.PathParam;
import jakarta.websocket.server.ServerEndpoint;

/**
 * Endpoint for VNC proxy websockets
 */
@ServerEndpoint("/vncviewer/{fwid}")
public class VncSocketEndpoint
{
   private static final Logger logger = LoggerFactory.getLogger(VncSocketEndpoint.class);

   private static Map<UUID, TcpPortForwarder> forwarders = Collections.synchronizedMap(new HashMap<UUID, TcpPortForwarder>());
   private static Map<String, VncSession> sessions = Collections.synchronizedMap(new HashMap<String, VncSession>());

   /**
    * Register TCP port forwarder.
    *
    * @param forwarder TCP port forwarder
    */
   public static void registerForwarder(TcpPortForwarder forwarder)
   {
      forwarders.put(forwarder.getId(), forwarder);
   }

   /**
    * Websocket session open handler.
    *
    * @param session websocket session
    * @param config endpoint configuration
    * @param forwarderId TCP port forwarder ID
    */
   @OnOpen
   public void onOpen(Session session, EndpointConfig config, @PathParam("fwid") String forwarderId)
   {
      logger.debug("VNC websocket session open request (session ID {})", forwarderId);
      try
      {
         TcpPortForwarder forwarder = forwarders.get(UUID.fromString(forwarderId));
         if (forwarder != null)
         {
            VncSession vncSession = new VncSession(session, forwarder);
            sessions.put(session.getId(), vncSession);
            logger.debug("Proxy ID {} assigned to VNC websocket session {}", forwarderId, session.getId());
         }
         else
         {
            logger.debug("Invalid proxy ID {} in VNC websocket session open request", forwarderId);
            closeSession(session, CloseCodes.CANNOT_ACCEPT, "Invalid proxy ID");
         }
      }
      catch(Exception e)
      {

      }
   }

   /**
    * Websocket session closure handler.
    *
    * @param session websocket session being closed
    */
   @OnClose
   public void onClose(Session session)
   {
      logger.debug("VNC websocket session {} closed", session.getId());
      VncSession vncSession = sessions.remove(session.getId());
      if (vncSession != null)
      {
         vncSession.close();
      }
   }

   /**
    * Handler for received binary messages.
    *
    * @param session websocket session
    * @param data received data
    */
   @OnMessage(maxMessageSize = 1024000)
   public void onMessage(Session session, byte[] data)
   {
      VncSession vncSession = sessions.get(session.getId());
      if (vncSession != null)
      {
         try
         {
            vncSession.send(data);
         }
         catch(IOException e)
         {
            closeSession(session, CloseCodes.PROTOCOL_ERROR, "Socket I/O error");
         }
      }
      else
      {
         logger.debug("VNC websocket request with invalid session ID {}", session.getId());
         closeSession(session, CloseCodes.CANNOT_ACCEPT, "Invalid proxy ID");
      }
   }

   /**
    * Safely close websocket session.
    *
    * @param session websocket session to close
    * @param code closure code
    * @param message closure message
    */
   private static void closeSession(Session session, CloseCode code, String message)
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
    * VNC proxy session
    */
   private static class VncSession
   {
      private String id;
      private Session wsSession;
      private TcpPortForwarder forwarder;
      private Socket socket;
      private Thread socketReaderThread;

      /**
       * Create new VNC proxy session.
       *
       * @param wsSession websocket session
       * @param forwarder TCP port forwarder
       * @throws UnknownHostException if host name could not be resolved
       * @throws IOException if TCP connection cannot be established
       */
      VncSession(Session wsSession, TcpPortForwarder forwarder) throws UnknownHostException, IOException
      {
         this.id = wsSession.getId();
         this.wsSession = wsSession;
         this.forwarder = forwarder;
         socket = new Socket("127.0.0.1", forwarder.getLocalPort());
         socketReaderThread = new Thread(() -> {
            logger.debug("Socket reader started");
            socketReader();
         }, "VncSession-" + wsSession.getId() + "-Socket");
         socketReaderThread.setDaemon(true);
         socketReaderThread.start();
      }

      /**
       * Send data to remote system.
       *
       * @param data data to send
       * @throws IOException on socket error
       */
      public void send(byte[] data) throws IOException
      {
         socket.getOutputStream().write(data);
      }

      /**
       * Close this VNC proxy session
       */
      public void close()
      {
         try
         {
            socket.close();
         }
         catch(IOException e)
         {
            logger.error("VNC socket close exception", e);
         }
      }

      /**
       * TCP socket reader
       */
      private void socketReader()
      {
         try
         {
            InputStream in = socket.getInputStream();
            byte[] buffer = new byte[32768];
            while(true)
            {
               int bytes = in.read(buffer);
               if (bytes <= 0)
               {
                  logger.debug("Exit code " + bytes + " while reading socket input stream");
                  break;
               }
               wsSession.getBasicRemote().sendBinary(ByteBuffer.wrap(buffer, 0, bytes));
            }
         }
         catch(Exception e)
         {
            logger.error("VNC socket reader exception", e);
         }

         try
         {
            wsSession.close();
         }
         catch(IOException e)
         {
         }
         
         sessions.remove(id);
         forwarder.close();
         forwarders.remove(forwarder.getId());
      }
   }
}
