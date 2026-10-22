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
package org.netxms.nxmc.modules.objects.ssh;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.nio.channels.ClosedChannelException;
import org.netxms.client.NXCSession;
import org.netxms.client.TcpProxy;
import org.netxms.client.objects.AbstractNode;
import org.netxms.nxmc.Registry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.jcraft.jsch.ChannelShell;
import com.jcraft.jsch.JSch;
import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.Session;
import com.jcraft.jsch.SocketFactory;
import com.jcraft.jsch.UIKeyboardInteractive;
import com.jcraft.jsch.UserInfo;

/**
 * Terminal transport with SSH client running in the console, connected to the node over TCP proxy channel. SSH session is
 * end-to-end encrypted between console and node; the proxy agent only forwards TCP stream. Host keys are kept in file
 * "known_hosts" in the console state directory: unknown keys are confirmed via host key verifier, changed keys are
 * rejected. Location of "known_hosts" is resolved in constructor, so transport must be created on UI thread (web client
 * resolves state directory through UI session); connect() itself may run on any thread. Private key file is accepted only
 * in desktop client - in web client path would be resolved on web server host. Transport is single-use: close() called
 * while connect() is still in progress is remembered, and connect() then releases everything it has established and
 * throws ClosedChannelException instead of handing the session over.
 */
public class DirectSshTransport implements TerminalTransport
{
   private static final Logger logger = LoggerFactory.getLogger(DirectSshTransport.class);

   private static final int CONNECT_TIMEOUT = 30000;

   private final NXCSession session;
   private final AbstractNode node;
   private final int port;
   private final String login;
   private final String password;
   private final String keyFile;
   private final File knownHostsFile;
   private final HostKeyVerifier hostKeyVerifier;
   private TcpProxy proxy = null; // guarded by this
   private Session sshSession = null; // guarded by this
   private ChannelShell channel = null; // guarded by this
   private boolean closed = false; // guarded by this
   private InputStream inputStream = null;
   private OutputStream outputStream = null;
   private String hostKeyError = null;

   /**
    * Create transport.
    *
    * @param session client session
    * @param node target node
    * @param port SSH port on target node
    * @param login login name
    * @param password password, or passphrase of private key if key file is given
    * @param keyFile private key file or null/empty for password authentication (ignored in web client)
    * @param hostKeyVerifier verifier for unknown host keys
    */
   public DirectSshTransport(NXCSession session, AbstractNode node, int port, String login, String password, String keyFile, HostKeyVerifier hostKeyVerifier)
   {
      this.session = session;
      this.node = node;
      this.port = port;
      this.login = (login != null) ? login : "";
      this.password = (password != null) ? password : "";
      this.keyFile = (!Registry.IS_WEB_CLIENT && (keyFile != null) && !keyFile.trim().isEmpty()) ? keyFile.trim() : null;
      this.knownHostsFile = new File(Registry.getStateDir(), "known_hosts");
      this.hostKeyVerifier = hostKeyVerifier;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#connect(int, int)
    */
   @Override
   public void connect(int columns, int rows) throws Exception
   {
      InetAddress address = node.getPrimaryIP().getAddress();
      if ((address == null) || address.isAnyLocalAddress())
         throw new IOException("Node does not have valid primary IP address");
      String host = address.getHostAddress();

      try
      {
         JSch jsch = new JSch();
         if (!knownHostsFile.exists())
            knownHostsFile.createNewFile();
         jsch.setKnownHosts(knownHostsFile.getAbsolutePath());
         if (keyFile != null)
            jsch.addIdentity(keyFile, password);

         TcpProxy p = session.setupTcpProxy(node.getObjectId(), port);
         p.setBufferingThresholds(1, 0); // SSH layer does its own packetization
         Session s = jsch.getSession(login, host, port);
         s.setSocketFactory(new ProxySocketFactory(p));
         s.setConfig("StrictHostKeyChecking", "ask");
         s.setConfig("PreferredAuthentications", (keyFile != null) ? "publickey,password,keyboard-interactive" : "password,keyboard-interactive,publickey");
         if (keyFile == null)
            s.setPassword(password);
         s.setUserInfo(new ConsoleUserInfo());
         synchronized(this)
         {
            proxy = p;
            sshSession = s;
            if (closed)
               throw new ClosedChannelException(); // released by catch block below
         }

         // close() from another thread past this point closes the proxy, which fails connect() below with JSch exception
         s.connect(CONNECT_TIMEOUT);

         ChannelShell c = (ChannelShell)s.openChannel("shell");
         c.setPtyType("xterm", columns, rows, 0, 0);
         synchronized(this)
         {
            channel = c;
            if (closed)
               throw new ClosedChannelException();
         }
         inputStream = c.getInputStream();
         outputStream = c.getOutputStream();
         c.connect(CONNECT_TIMEOUT);
         logger.debug("Direct SSH session established with {}:{} as {}", host, port, login);
      }
      catch(Exception e)
      {
         releaseResources();
         synchronized(this)
         {
            if (closed)
               throw new ClosedChannelException();
         }
         if (hostKeyError != null)
            throw new IOException(hostKeyError);
         if (e instanceof JSchException)
            throw new IOException(e.getMessage(), e);
         throw e;
      }
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#getInputStream()
    */
   @Override
   public InputStream getInputStream()
   {
      return inputStream;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#getOutputStream()
    */
   @Override
   public OutputStream getOutputStream()
   {
      return outputStream;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#resize(int, int)
    */
   @Override
   public void resize(int columns, int rows)
   {
      ChannelShell c;
      synchronized(this)
      {
         c = channel;
      }
      if ((c != null) && c.isConnected())
         c.setPtySize(columns, rows, 0, 0);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.ssh.TerminalTransport#close()
    */
   @Override
   public void close()
   {
      synchronized(this)
      {
         closed = true;
      }
      releaseResources();
   }

   /**
    * Disconnect SSH channel and session and close TCP proxy (whatever of them is established). Does not mark transport as
    * closed, so connect() failure path can use it without masking the failure.
    */
   private void releaseResources()
   {
      ChannelShell c;
      Session s;
      TcpProxy p;
      synchronized(this)
      {
         c = channel;
         s = sshSession;
         p = proxy;
         channel = null;
         sshSession = null;
         proxy = null;
      }
      if (c != null)
         c.disconnect();
      if (s != null)
         s.disconnect();
      if (p != null)
         p.close();
   }

   /**
    * Socket factory supplying TCP proxy streams instead of a real socket. The socket object handed to JSch is a
    * placeholder that never touches the network; all stream and option calls are intercepted.
    */
   private static class ProxySocketFactory implements SocketFactory
   {
      private final TcpProxy proxy;

      ProxySocketFactory(TcpProxy proxy)
      {
         this.proxy = proxy;
      }

      @Override
      public Socket createSocket(String host, int port) throws IOException
      {
         return new ProxySocket(proxy);
      }

      @Override
      public InputStream getInputStream(Socket socket) throws IOException
      {
         return proxy.getInputStream();
      }

      @Override
      public OutputStream getOutputStream(Socket socket) throws IOException
      {
         return proxy.getOutputStream();
      }
   }

   /**
    * Placeholder socket backed by TCP proxy channel
    */
   private static class ProxySocket extends Socket
   {
      private final TcpProxy proxy;

      ProxySocket(TcpProxy proxy)
      {
         this.proxy = proxy;
      }

      @Override
      public InputStream getInputStream() throws IOException
      {
         return proxy.getInputStream();
      }

      @Override
      public OutputStream getOutputStream() throws IOException
      {
         return proxy.getOutputStream();
      }

      @Override
      public synchronized void setSoTimeout(int timeout)
      {
      }

      @Override
      public void setTcpNoDelay(boolean on)
      {
      }

      @Override
      public void setKeepAlive(boolean on)
      {
      }

      @Override
      public boolean isConnected()
      {
         return !proxy.isClosed();
      }

      @Override
      public boolean isClosed()
      {
         return proxy.isClosed();
      }

      @Override
      public synchronized void close()
      {
         proxy.close();
      }
   }

   /**
    * User interaction for JSch: answers password prompts from settings and routes host key questions to the verifier.
    */
   private class ConsoleUserInfo implements UserInfo, UIKeyboardInteractive
   {
      @Override
      public String getPassphrase()
      {
         return password;
      }

      @Override
      public String getPassword()
      {
         return password;
      }

      @Override
      public boolean promptPassword(String message)
      {
         return true;
      }

      @Override
      public boolean promptPassphrase(String message)
      {
         return true;
      }

      @Override
      public boolean promptYesNo(String message)
      {
         if (message.contains("HAS CHANGED"))
         {
            hostKeyError = "Host key of " + node.getPrimaryIP().getHostAddress() + " has changed; " +
                  (Registry.IS_WEB_CLIENT ? "ask web console administrator to remove the old key from known_hosts file if the change is expected" :
                        "remove the old key from " + knownHostsFile.getAbsolutePath() + " if the change is expected");
            logger.warn(hostKeyError);
            return false;
         }
         if (!hostKeyVerifier.confirmUnknownHostKey(message))
         {
            hostKeyError = "Host key rejected by user";
            return false;
         }
         return true;
      }

      @Override
      public void showMessage(String message)
      {
         logger.info("SSH server message: {}", message);
      }

      @Override
      public String[] promptKeyboardInteractive(String destination, String name, String instruction, String[] prompt, boolean[] echo)
      {
         if (prompt.length != 1)
            return null;
         return new String[] { password };
      }
   }
}
