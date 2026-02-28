package org.netxms.tcpproxy;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import org.netxms.client.NXCSession;
import org.netxms.client.TcpProxy;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Listens on a local port and forwards each accepted connection through a NetXMS TCP proxy.
 */
public class PortForwarder
{
   private static final Logger logger = LoggerFactory.getLogger(PortForwarder.class);

   private final NXCSession session;
   private final long proxyObjectId;
   private final ForwardingRule rule;
   private final ServerSocket serverSocket;
   private final Map<Integer, ProxySession> sessions = new ConcurrentHashMap<>();
   private final AtomicInteger sessionIdCounter = new AtomicInteger(0);

   public PortForwarder(NXCSession session, long proxyObjectId, ForwardingRule rule) throws IOException
   {
      this.session = session;
      this.proxyObjectId = proxyObjectId;
      this.rule = rule;
      this.serverSocket = new ServerSocket(rule.getLocalPort());
   }

   /**
    * Start accept loop in a daemon thread.
    */
   public void start()
   {
      Thread thread = new Thread(() -> acceptLoop(), "Forwarder-" + rule.getLocalPort());
      thread.setDaemon(true);
      thread.start();
      logger.info("Listening on port {} (forwarding to {}:{} via {})", rule.getLocalPort(),
            rule.getRemoteAddress().getHostAddress(), rule.getRemotePort(), rule.getProxyNode());
   }

   private void acceptLoop()
   {
      while(!serverSocket.isClosed())
      {
         try
         {
            Socket socket = serverSocket.accept();
            try
            {
               int sessionId = sessionIdCounter.incrementAndGet();
               logger.info("[Port {}] Session {} accepted from {}", rule.getLocalPort(), sessionId,
                     socket.getRemoteSocketAddress());
               TcpProxy proxy = session.setupTcpProxy(proxyObjectId, rule.getRemoteAddress(), rule.getRemotePort());
               ProxySession proxySession = new ProxySession(sessionId, socket, proxy);
               sessions.put(sessionId, proxySession);
               proxySession.start();
            }
            catch(Exception e)
            {
               logger.error("[Port {}] Failed to setup proxy session", rule.getLocalPort(), e);
               socket.close();
            }
         }
         catch(IOException e)
         {
            if (!serverSocket.isClosed())
               logger.error("[Port {}] Accept failed", rule.getLocalPort(), e);
         }
      }
      logger.debug("[Port {}] Accept loop terminated", rule.getLocalPort());
   }

   /**
    * Close this forwarder and all active sessions.
    */
   public void close()
   {
      try
      {
         serverSocket.close();
      }
      catch(IOException e)
      {
         logger.debug("[Port {}] Error closing server socket", rule.getLocalPort(), e);
      }
      for(ProxySession s : sessions.values())
      {
         s.close();
      }
      sessions.clear();
   }

   /**
    * Bidirectional forwarding between a local socket and a NetXMS TCP proxy for one connection.
    */
   private class ProxySession
   {
      private final int id;
      private final Socket socket;
      private final TcpProxy proxy;

      ProxySession(int id, Socket socket, TcpProxy proxy)
      {
         this.id = id;
         this.socket = socket;
         this.proxy = proxy;
      }

      void start()
      {
         Thread socketThread = new Thread(() -> socketReader(),
               "Fwd-" + rule.getLocalPort() + "-S" + id + "-Socket");
         socketThread.setDaemon(true);
         socketThread.start();

         Thread proxyThread = new Thread(() -> proxyReader(),
               "Fwd-" + rule.getLocalPort() + "-S" + id + "-Proxy");
         proxyThread.setDaemon(true);
         proxyThread.start();
      }

      private void socketReader()
      {
         try
         {
            InputStream in = socket.getInputStream();
            OutputStream out = proxy.getOutputStream();
            byte[] buffer = new byte[32768];
            while(true)
            {
               int bytes = in.read(buffer);
               if (bytes <= 0)
               {
                  logger.debug("[Port {}] Session {} socket read returned {}", rule.getLocalPort(), id, bytes);
                  break;
               }
               out.write(buffer, 0, bytes);
            }
         }
         catch(Exception e)
         {
            logger.debug("[Port {}] Session {} socket reader exception", rule.getLocalPort(), id, e);
         }

         proxy.close();

         try
         {
            socket.close();
         }
         catch(IOException e)
         {
         }

         sessions.remove(id);
         logger.info("[Port {}] Session {} closed", rule.getLocalPort(), id);
      }

      private void proxyReader()
      {
         try
         {
            InputStream in = proxy.getInputStream();
            OutputStream out = socket.getOutputStream();
            byte[] buffer = new byte[32768];
            while(true)
            {
               int bytes = in.read(buffer);
               if (bytes <= 0)
               {
                  logger.debug("[Port {}] Session {} proxy read returned {}", rule.getLocalPort(), id, bytes);
                  break;
               }
               out.write(buffer, 0, bytes);
            }
         }
         catch(Exception e)
         {
            logger.debug("[Port {}] Session {} proxy reader exception", rule.getLocalPort(), id, e);
         }

         try
         {
            socket.close();
         }
         catch(IOException e)
         {
         }
      }

      void close()
      {
         try
         {
            socket.shutdownInput();
         }
         catch(IOException e)
         {
         }
         try
         {
            socket.shutdownOutput();
         }
         catch(IOException e)
         {
         }
         proxy.close();
      }
   }
}
